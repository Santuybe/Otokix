#include "otokix.h"
#include "music.h"
#include <ctype.h>
#include <string.h>

static int playlist_scroll = 0;
static int playlist_selected = 0;

void draw_tab(const char *label, int active, int x, int y) {
    SDL_Color c = active ? uint32ToSDLColor(ACCENT_COLOR) : uint32ToSDLColor(TEXT_COLOR);
    SDL_Surface *s = TTF_RenderText_Blended(_G.smallFont, label, c);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(_G.renderer, s);
    if (!t) {
        SDL_FreeSurface(s);
        return;
    }
    SDL_Rect r = {x, y, s->w, s->h};
    SDL_RenderCopy(_G.renderer, t, NULL, &r);
    if (active) {
        SDL_SetRenderDrawColor(_G.renderer, 232, 85, 43, 255);
        SDL_RenderDrawLine(_G.renderer, x, y + s->h + 2, x + s->w, y + s->h + 2);
    }
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}

void draw_equalizer(int cx, int cy) {
    SDL_SetRenderDrawColor(_G.renderer, 232, 85, 43, 255);
    for (int i = 0; i < 7; i++) {
        int x = cx - 45 + i * 15;
        int h = (g_audio.playing && !g_audio.paused) ? (int)(g_audio.eq[i] * 15) + 5 : 5;
        SDL_RenderDrawLine(_G.renderer, x, cy + h, x + 6, cy + h);
    }
}

void draw_playlist_screen() {
    // Header tabs and battery
    draw_tab("CLOCK", 0, 10, 10);
    draw_tab("MUSIC", 1, 80, 10);
    draw_battery(_G.DISPLAY_WIDTH - 42, 12);

    // Sub-header: Current Folder / Track Count
    char header[128];
    const char *bc = music_get_breadcrumb();
    if (bc && strlen(bc) > 0) {
        snprintf(header, sizeof(header), "%s (%d)", bc, g_audio.playlist.count);
    } else {
        snprintf(header, sizeof(header), "ALL TRACKS %d", g_audio.playlist.count);
    }
    draw_text(_G.smallFont, header, 10, 36, TEXT_COLOR, 0);

    if (g_audio.playlist.count == 0) {
        draw_text(_G.labelFont, "NO MUSIC / EMPTY FOLDER", 40, 180, ACCENT_COLOR, 0);
    } else {
        // 6-item viewport
        int start_y = 58;
        int item_h = 52;
        int visible = 6;

        for (int i = 0; i < visible && (playlist_scroll + i) < g_audio.playlist.count; i++) {
            int idx = playlist_scroll + i;
            Track *t = &g_audio.playlist.items[idx];
            int y = start_y + i * item_h;

            // Selected item outline
            if (idx == playlist_selected) {
                SDL_Rect sel = {10, y, _G.DISPLAY_WIDTH - 20, item_h - 4};
                SDL_SetRenderDrawColor(_G.renderer, 232, 85, 43, 255);
                SDL_RenderDrawRect(_G.renderer, &sel);
            }

            // Item number
            char num_str[8];
            snprintf(num_str, sizeof(num_str), "%2d", idx + 1);
            Uint32 num_color = (idx == playlist_selected) ? ACCENT_COLOR : 0xFF606060;
            draw_text(_G.smallFont, num_str, 20, y + 8, num_color, 0);

            // Item title formatting
            char title_upper[256];
            if (t->isDir) {
                snprintf(title_upper, sizeof(title_upper), "[DIR] %s", t->title);
            } else {
                snprintf(title_upper, sizeof(title_upper), "%s", t->title);
            }
            for (int j = 0; title_upper[j]; j++) {
                title_upper[j] = toupper((unsigned char)title_upper[j]);
            }

            // Truncate if long
            int tw = 0;
            TTF_SizeText(_G.smallFont, title_upper, &tw, NULL);
            if (tw > 340) {
                for (int j = strlen(title_upper) - 1; tw > 320 && j > 0; j--) {
                    title_upper[j] = '\0';
                    TTF_SizeText(_G.smallFont, title_upper, &tw, NULL);
                }
                if (strlen(title_upper) < sizeof(title_upper) - 4) {
                    strcat(title_upper, "...");
                }
            }

            // Color highlight for playing item
            Uint32 title_color = TEXT_COLOR;
            if (idx == g_audio.playlist.currentIdx && g_audio.playing) {
                title_color = ACCENT_COLOR;
            } else if (idx == playlist_selected) {
                title_color = ACCENT_COLOR;
            }

            draw_text(_G.smallFont, title_upper, 55, y + 8, title_color, 0);

            // Subtitle / Duration
            char info_str[32];
            if (t->isDir) {
                strcpy(info_str, "FOLDER");
            } else if (t->duration > 0) {
                snprintf(info_str, sizeof(info_str), "%02d:%02d", t->duration / 60, t->duration % 60);
            } else {
                strcpy(info_str, "--:--");
            }
            draw_text(_G.smallFont, info_str, 55, y + 28, 0xFF707070, 0);

            // Playing indicator icon
            if (idx == g_audio.playlist.currentIdx && g_audio.playing) {
                draw_text(_G.smallFont, g_audio.paused ? "||" : ">", _G.DISPLAY_WIDTH - 35, y + 16, ACCENT_COLOR, 0);
            }
        }

        // Scrollbar dot indicator
        if (g_audio.playlist.count > visible) {
            float scroll_ratio = (float)playlist_scroll / (g_audio.playlist.count - visible);
            int scroll_pos = (int)(scroll_ratio * (visible - 1));
            SDL_Rect dot = {_G.DISPLAY_WIDTH - 12, start_y + scroll_pos * item_h + 10, 4, 12};
            SDL_SetRenderDrawColor(_G.renderer, 232, 85, 43, 255);
            SDL_RenderFillRect(_G.renderer, &dot);
        }
    }

    // Mini Playback Bar (bottom section y: 380)
    SDL_SetRenderDrawColor(_G.renderer, 40, 40, 40, 255);
    SDL_Rect bg_bar = {10, 380, _G.DISPLAY_WIDTH - 20, 44};
    SDL_RenderFillRect(_G.renderer, &bg_bar);

    if (g_audio.playlist.currentIdx >= 0 && g_audio.playlist.currentIdx < g_audio.playlist.count) {
        Track *curr = &g_audio.playlist.items[g_audio.playlist.currentIdx];

        // Progress line
        uint32_t dur = curr->duration;
        uint32_t elapsed = g_audio.elapsed;
        int bar_w = _G.DISPLAY_WIDTH - 140;
        SDL_Rect bar_bg = {20, 400, bar_w, 4};
        SDL_SetRenderDrawColor(_G.renderer, 80, 80, 80, 255);
        SDL_RenderFillRect(_G.renderer, &bar_bg);

        if (dur > 0) {
            int fill_w = (int)(((float)elapsed / dur) * bar_w);
            if (fill_w > bar_w) fill_w = bar_w;
            SDL_Rect fill = {20, 400, fill_w, 4};
            SDL_SetRenderDrawColor(_G.renderer, 232, 85, 43, 255);
            SDL_RenderFillRect(_G.renderer, &fill);
        }

        // Elapsed / Duration text
        char time_str[32];
        snprintf(time_str, sizeof(time_str), "%02d:%02d/%02d:%02d", elapsed / 60, elapsed % 60, dur / 60, dur % 60);
        draw_text(_G.smallFont, time_str, 20 + bar_w + 10, 392, TEXT_COLOR, 0);
    } else {
        draw_text(_G.smallFont, "NO TRACK PLAYING", 20, 392, 0xFF808080, 0);
    }

    // Help Footer (y: 445)
    draw_text(_G.smallFont, "A", 15, 445, ACCENT_COLOR, 0);
    draw_text(_G.smallFont, "Play", 33, 445, TEXT_COLOR, 0);

    draw_text(_G.smallFont, "B", 85, 445, ACCENT_COLOR, 0);
    draw_text(_G.smallFont, "Pause", 103, 445, TEXT_COLOR, 0);

    draw_text(_G.smallFont, "X", 160, 445, ACCENT_COLOR, 0);
    char rep_str[16];
    switch (g_audio.repeatMode) {
        case 1: strcpy(rep_str, "Once"); break;
        case 2: strcpy(rep_str, "Shuffle"); break;
        default: strcpy(rep_str, "Repeat"); break;
    }
    draw_text(_G.smallFont, rep_str, 178, 445, TEXT_COLOR, 0);

    draw_text(_G.smallFont, "L/R", 245, 445, ACCENT_COLOR, 0);
    draw_text(_G.smallFont, "Seek", 273, 445, TEXT_COLOR, 0);

    draw_text(_G.smallFont, "R1/R2", 335, 445, ACCENT_COLOR, 0);
    draw_text(_G.smallFont, "Skip", 378, 445, TEXT_COLOR, 0);

    draw_text(_G.smallFont, "SELECT", 440, 445, ACCENT_COLOR, 0);
    draw_text(_G.smallFont, "Clock", 495, 445, TEXT_COLOR, 0);
}

int music_screen() {
    update_battery();
    draw_playlist_screen();
    return 0;
}

void music_input(int key) {
    if (g_audio.playlist.count > 0) {
        if (key == KEYCODE_UP) {
            if (playlist_selected > 0) {
                playlist_selected--;
                if (playlist_selected < playlist_scroll) {
                    playlist_scroll = playlist_selected;
                }
                _G.dirty = 1;
            }
        } else if (key == KEYCODE_DOWN) {
            if (playlist_selected < g_audio.playlist.count - 1) {
                playlist_selected++;
                if (playlist_selected >= playlist_scroll + 6) {
                    playlist_scroll = playlist_selected - 5;
                }
                _G.dirty = 1;
            }
        } else if (key == KEYCODE_A) {
            Track *t = &g_audio.playlist.items[playlist_selected];
            if (t->isDir) {
                music_enter_folder(playlist_selected);
                playlist_selected = 0;
                playlist_scroll = 0;
            } else {
                music_play(playlist_selected);
            }
            _G.dirty = 1;
        } else if (key == KEYCODE_B) {
            if (g_audio.playing) {
                music_pause_toggle();
                _G.dirty = 1;
            }
        } else if (key == KEYCODE_X) {
            g_audio.repeatMode = (g_audio.repeatMode + 1) % 3;
            _G.dirty = 1;
        } else if (key == KEYCODE_L1) {
            int max_scroll = (g_audio.playlist.count > 6) ? g_audio.playlist.count - 6 : 0;
            if (playlist_scroll < max_scroll) {
                playlist_scroll += 6;
                if (playlist_scroll > max_scroll) playlist_scroll = max_scroll;
                if (playlist_selected < playlist_scroll) {
                    playlist_selected = playlist_scroll;
                }
                _G.dirty = 1;
            }
        } else if (key == KEYCODE_L2) {
            if (playlist_scroll > 0) {
                playlist_scroll -= 6;
                if (playlist_scroll < 0) playlist_scroll = 0;
                if (playlist_selected > playlist_scroll + 5) {
                    playlist_selected = playlist_scroll + 5;
                }
                _G.dirty = 1;
            }
        } else if (key == KEYCODE_R1) {
            music_next();
            _G.dirty = 1;
        } else if (key == KEYCODE_R2) {
            music_prev();
            _G.dirty = 1;
        } else if (key == KEYCODE_LEFT) {
            if (g_audio.playing) {
                int target = (int)g_audio.elapsed - 5;
                if (target < 0) target = 0;
                music_seek(target);
                _G.dirty = 1;
            }
        } else if (key == KEYCODE_RIGHT) {
            if (g_audio.playing) {
                int target = (int)g_audio.elapsed + 5;
                music_seek(target);
                _G.dirty = 1;
            }
        }
    }

    if (key == KEYCODE_SELECT) {
        _G.screen = SCREEN_CLOCK;
        _G.dirty = 1;
    }
}
