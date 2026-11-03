#include "otokix.h"
#include "music.h"
#include <ctype.h>

static int last_min = -1, last_hour = -1, last_charging = -1, last_percent = -1;

struct tm *get_local_time() { time_t t; time(&t); return localtime(&t); }

void update_battery() {
  uint32_t now = SDL_GetTicks();
  if (now - _G.battery.lastUpdate < BATTERY_UPDATE_MS) return;
  _G.battery.lastUpdate = now;
  FILE *f = fopen("/sys/class/power_supply/battery/capacity", "r");
  if (f) { fscanf(f, "%d", &_G.battery.percent); fclose(f); }
  f = fopen("/sys/class/power_supply/battery/status", "r");
  if (f) { char s[16]; fscanf(f, "%15s", s); _G.battery.charging = (strcmp(s, "Charging") == 0); fclose(f); }
}

void check_dirty() {
    struct tm *time = get_local_time();
    int h = _G.manualTimeEnabled ? _G.manualHour : time->tm_hour;
    int m = _G.manualTimeEnabled ? _G.manualMinute : time->tm_min;

    // Redraw on time/battery change
    if (m != last_min || h != last_hour || _G.battery.charging != last_charging ||
        _G.battery.percent != last_percent) {
        _G.dirty = 1;
    }

    // STANDBY/Clock animation: redraw every 1 second
    if (_G.screen == SCREEN_CLOCK) {
        static int last_sec = -1;
        int sec = time->tm_sec;
        if (sec != last_sec) {
            _G.dirty = 1;
            last_sec = sec;
        }
    }

    last_min = m; last_hour = h; last_charging = _G.battery.charging; last_percent = _G.battery.percent;
}

void draw_text(TTF_Font *f, const char *t, int x, int y, Uint32 color, int center_x) {
    SDL_Color c = uint32ToSDLColor(color);
    SDL_Surface *s = TTF_RenderText_Blended(f, t, c);
    if (!s) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(_G.renderer, s);
    SDL_Rect dst = {center_x ? x - s->w/2 : x, y, s->w, s->h};
    SDL_RenderCopy(_G.renderer, tex, NULL, &dst);
    SDL_FreeSurface(s); SDL_DestroyTexture(tex);
}

void toggle_ampm_mode() {
    _G.flags ^= (1 << FLAG_AMPM);
    _G.dirty = 1;
}

void draw_battery(int x, int y) {
    int iconW = 30, iconH = 16, capW = 2, capH = 8;
    SDL_Rect body = {x, y, iconW, iconH}, cap = {x + iconW, y + (iconH - capH)/2, capW, capH};
    SDL_SetRenderDrawColor(_G.renderer, 207, 207, 207, 255);
    SDL_RenderDrawRect(_G.renderer, &body); SDL_RenderFillRect(_G.renderer, &cap);
    int segs = (_G.battery.percent > 66) ? 3 : (_G.battery.percent > 33) ? 2 : (_G.battery.percent > 5) ? 1 : 0;
    SDL_SetRenderDrawColor(_G.renderer, (_G.battery.percent < 15) ? 217 : 232, (_G.battery.percent < 15) ? 58 : 85, 43, 255);
    for (int i = 0; i < segs; i++) { SDL_Rect s = {x + 3 + i * 9, y + 3, 7, 10}; SDL_RenderFillRect(_G.renderer, &s); }
    if (_G.battery.charging) {
        SDL_SetRenderDrawColor(_G.renderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(_G.renderer, x+15, y+2, x+10, y+8); SDL_RenderDrawLine(_G.renderer, x+10, y+8, x+20, y+8);
        SDL_RenderDrawLine(_G.renderer, x+20, y+8, x+15, y+14);
    }
}

static void draw_menu() {
    // Menu overlay: dark panel 200x220 centered
    int mx = _G.DISPLAY_WIDTH/2 - 100, my = _G.DISPLAY_HEIGHT/2 - 110;
    SDL_Rect bg = {mx, my, 200, 220};

    // Dark background
    RenderFillRect(_G.renderer, &bg, 0x40000000);

    // Menu items
    const char *items[] = {
        "Set Time",
        is_active_flag(FLAG_AMPM) ? "24-Hour" : "12-Hour",
        "Exit"
    };
    int item_h = 40;

    for (int i = 0; i < 3; i++) {
        int y = my + 20 + i * item_h;

        // Highlight selected item with orange pill
        if (i == _G.menuIndex) {
            SDL_Rect pill = {mx + 10, y, 180, 30};
            RenderFillRect(_G.renderer, &pill, ACCENT_COLOR);
            draw_text(_G.smallFont, items[i], mx + 100, y + 5, BG_COLOR, 1);
        } else {
            draw_text(_G.smallFont, items[i], mx + 100, y + 5, TEXT_COLOR, 1);
        }
    }

    // Instructions
    draw_text(_G.smallFont, "A select  B back", _G.DISPLAY_WIDTH/2, my + 200, TEXT_BRIGHT, 1);
}

void execute_menu_item(int idx) {
    switch (idx) {
        case 0: // Set Time
            _G.menuEditMode = 1;
            _G.menuEditField = 0;
            DEBUG_LOG("Menu: Set Time mode");
            break;
        case 1: // Toggle 12/24
            toggle_ampm_mode();
            DEBUG_LOG("Menu: Toggled 12/24 mode");
            break;
        case 2: // Exit
            _G.quit = 1;
            DEBUG_LOG("Menu: Exit");
            break;
    }
}

static void draw_big_time(int h, int m, int cy) {
    char ts[8], ampm[4] = ""; int display_h = h;
    if (is_active_flag(FLAG_AMPM)) { strcpy(ampm, (h >= 12) ? "PM" : "AM"); display_h = h % 12; if (display_h == 0) display_h = 12; }
    snprintf(ts, sizeof(ts), "%02d:%02d", display_h, m);
    int w, fh; TTF_SizeText(_G.bigFont, ts, &w, &fh);
    draw_text(_G.bigFont, ts, _G.DISPLAY_WIDTH/2, cy, TEXT_BRIGHT, 1);
    if (is_active_flag(FLAG_AMPM)) {
        int ampm_y = cy + fh - 18;
        draw_text(_G.smallFont, ampm, _G.DISPLAY_WIDTH/2 + w/2 + 8, ampm_y, TEXT_COLOR, 0);
    }
}

int draw_screen(float fps) {
    if (_G.screen == SCREEN_MUSIC) {
        return music_screen();
    }

    struct tm *t = get_local_time();
    int h = _G.manualTimeEnabled ? _G.manualHour : t->tm_hour, m = _G.manualTimeEnabled ? _G.manualMinute : t->tm_min;

    draw_big_time(h, m, UI_TIME_Y);

    // Date format: TUE. 27 OCT with proper spacing
    char day_str[8], date_str[4], month_str[8];
    strftime(day_str, sizeof(day_str), "%a", t);
    strftime(date_str, sizeof(date_str), "%d", t);
    strftime(month_str, sizeof(month_str), "%b", t);
    for (int i = 0; i < 3 && day_str[i]; i++) day_str[i] = toupper(day_str[i]);
    for (int i = 0; month_str[i]; i++) month_str[i] = toupper(month_str[i]);

    int d_w, dot_w, date_w, m_w, fh;
    TTF_SizeText(_G.smallFont, day_str, &d_w, &fh);
    TTF_SizeText(_G.smallFont, ".", &dot_w, NULL);
    TTF_SizeText(_G.smallFont, date_str, &date_w, NULL);
    TTF_SizeText(_G.smallFont, month_str, &m_w, NULL);

    int gap = 8;
    int total_w = d_w + dot_w + gap + date_w + gap + m_w;
    int start_x = _G.DISPLAY_WIDTH/2 - total_w/2;

    draw_text(_G.smallFont, day_str, start_x, UI_DATE_Y, TEXT_COLOR, 0);
    int x_pos = start_x + d_w;
    draw_text(_G.smallFont, ".", x_pos, UI_DATE_Y, TEXT_COLOR, 0);
    x_pos += dot_w + gap;
    draw_text(_G.smallFont, date_str, x_pos, UI_DATE_Y, ACCENT_COLOR, 0);
    x_pos += date_w + gap;
    draw_text(_G.smallFont, month_str, x_pos, UI_DATE_Y, TEXT_COLOR, 0);

    // Pill bar with current track title
    SDL_Rect pDst = {(_G.DISPLAY_WIDTH - _G.pillW)/2, UI_BUTTON_Y, _G.pillW, _G.pillH};
    SDL_RenderCopy(_G.renderer, _G.pillTexture, NULL, &pDst);

    // Get current title
    char pill_text[128] = "No Music";
    if (_G.now_playing.playing && _G.now_playing.title[0] != '\0') {
        snprintf(pill_text, sizeof(pill_text), "%s", _G.now_playing.title);
    } else if (g_audio.playlist.count > 0 && g_audio.playlist.currentIdx >= 0 && g_audio.playlist.currentIdx < g_audio.playlist.count) {
        snprintf(pill_text, sizeof(pill_text), "%s", g_audio.playlist.items[g_audio.playlist.currentIdx].title);
    }

    // Draw '>' indicator on the left side with accent color (like music player)
    int icon_x = pDst.x + 16;
    int icon_w = 0, icon_h = 0;
    TTF_SizeText(_G.smallFont, ">", &icon_w, &icon_h);
    int icon_y = UI_BUTTON_Y + (_G.pillH - icon_h) / 2;
    draw_text(_G.smallFont, ">", icon_x, icon_y, ACCENT_COLOR, 0);

    // Scrolling logic
    int tw = 0, th = 0;
    TTF_SizeText(_G.smallFont, pill_text, &tw, &th);
    int text_area_x = icon_x + icon_w + 10;
    int available_w = (pDst.x + _G.pillW - 16) - text_area_x;
    if (available_w < 10) available_w = 10;
    static int scroll_x = 0;
    static int last_sec = -1;
    static char last_title[128] = "";

    if (strcmp(last_title, pill_text) != 0) {
        strncpy(last_title, pill_text, sizeof(last_title) - 1);
        last_title[sizeof(last_title) - 1] = '\0';
        scroll_x = 0;
    }

    struct tm *t_scroll = get_local_time();
    int current_3sec_block = t_scroll->tm_sec / 3;
    static int last_3sec_block = -1;
    if (current_3sec_block != last_3sec_block) {
        last_3sec_block = current_3sec_block;
        if (tw > available_w) {
            scroll_x += 6;
            if (scroll_x > tw + 30) {
                scroll_x = 0;
            }
        } else {
            scroll_x = 0;
        }
    }

    // Clip rendering to text area inside pill box
    SDL_Rect clip = { text_area_x, pDst.y + 2, available_w, _G.pillH - 4 };
    SDL_RenderSetClipRect(_G.renderer, &clip);

    // Draw text (left-aligned with offset when scrolling)
    int draw_x = (tw <= available_w) ? text_area_x : (text_area_x - scroll_x);
    int draw_y = UI_BUTTON_Y + (_G.pillH - th) / 2;
    draw_text(_G.smallFont, pill_text, draw_x, draw_y, TEXT_BRIGHT, 0);

    SDL_RenderSetClipRect(_G.renderer, NULL);

    draw_battery(_G.DISPLAY_WIDTH - 42, 12);
    return 0;
}

int draw_loop() {
    static uint32_t last_fail_log = 0;

    update_battery(); check_dirty();
    if (!_G.dirty) return 0;

    // Set render target if rotation needed
    if (_G.rotate != 0) {
        if (!_G.renderTarget) {
            uint32_t now = SDL_GetTicks();
            if (now - last_fail_log > 5000) {
                ERROR_LOG("renderTarget is NULL!");
                last_fail_log = now;
            }
            // Fail-safe: fill screen with solid color
            SDL_SetRenderDrawColor(_G.renderer, 32, 32, 32, 255);
            SDL_RenderClear(_G.renderer);
            SDL_RenderPresent(_G.renderer);
            return -1;
        }
        SDL_SetRenderTarget(_G.renderer, _G.renderTarget);
    }

    RenderClear(_G.renderer, BG_COLOR);
    draw_screen(0);

    // Draw menu overlay if open
    if (_G.menuOpen) {
        draw_menu();
    }

    // Reset render target and present
    if (_G.rotate != 0) {
        SDL_SetRenderTarget(_G.renderer, NULL);
    }

    RenderPresent(_G.renderer);
    _G.dirty = 0; return 0;
}
