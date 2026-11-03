#include "otokix.h"
#include "music.h"
#include "music_scanner.h"
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// SDL2_mixer placeholder (full impl fase 2)
// #include <SDL2/SDL_mixer.h>

AudioState g_audio = {0};

// Navigation breadcrumb stack (max depth 8, no malloc in hot path)
#define MAX_BREADCRUMB_DEPTH 8
static struct {
    char paths[MAX_BREADCRUMB_DEPTH][256];
    int depth;
} breadcrumb = {0};

// Seek feedback timing (visual highlight on seek)
static uint32_t last_seek_click_time = 0;
#define SEEK_FEEDBACK_MS 200

static const char *get_config_dir() {
    static char config_dir[256] = {0};
    if (!config_dir[0]) {
        // Config in app folder, not home
        snprintf(config_dir, sizeof(config_dir), ".");
    }
    return config_dir;
}

static char *get_config_path() {
    static char config_path[256] = {0};
    if (!config_path[0]) {
        snprintf(config_path, sizeof(config_path), "./otokix.cfg");
    }
    return config_path;
}

#define CONFIG_PATH get_config_path()

static void save_music_config() {
    FILE *f = fopen(CONFIG_PATH, "w");
    if (!f) return;
    fprintf(f, "track=%d\npos=%d\nvol=%d\n", g_audio.playlist.currentIdx, g_audio.elapsed, g_audio.volume);
    fclose(f);
}

static void load_music_config() {
    FILE *f = fopen(CONFIG_PATH, "r");
    if (!f) {
        g_audio.volume = audio_get_volume();
        audio_set_volume(g_audio.volume);
        return;
    }
    int track = -1, pos = 0, vol = -1;
    fscanf(f, "track=%d\npos=%d\nvol=%d\n", &track, &pos, &vol);
    fclose(f);
    if (track >= 0 && track < g_audio.playlist.count) {
        // Only restore track index and elapsed, don't auto-play
        g_audio.playlist.currentIdx = track;
        g_audio.elapsed = pos;
    }
    if (vol >= 0) {
        g_audio.volume = (vol > 128) ? 128 : vol;
    } else {
        g_audio.volume = audio_get_volume();
    }
    audio_set_volume(g_audio.volume);
}

static int is_audio_file(const char *name) {
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;
    return !strcasecmp(ext, ".mp3") || !strcasecmp(ext, ".ogg") || !strcasecmp(ext, ".flac");
}

static void extract_title(const char *path, char *out, int len) {
    const char *base = strrchr(path, '/');
    if (!base) base = path;
    else base++;
    char *dot = strchr(base, '.');
    if (dot) snprintf(out, len, "%.*s", (int)(dot - base), base);
    else snprintf(out, len, "%s", base);
}

void music_init_scan(const char *root_path) {
    if (!root_path) return;
    breadcrumb.depth = 0;
    strncpy(g_audio.playlist.currentPath, root_path, sizeof(g_audio.playlist.currentPath) - 1);
    g_audio.playlist.currentPath[sizeof(g_audio.playlist.currentPath) - 1] = '\0';
    g_audio.playlist.currentIdx = 0;
    g_audio.playlist.count = 0;
    scan_directory(root_path, &g_audio.playlist);
}

void music_init() {
    g_audio.playing = 0;
    g_audio.paused = 0;
    g_audio.repeatMode = 0;  // 0=repeat all, 1=once, 2=shuffle
    g_audio.playlist.capacity = 256;
    g_audio.playlist.items = malloc(sizeof(Track) * 256);
    g_audio.playlist.count = 0;
    g_audio.playlist.currentIdx = -1;
    strcpy(g_audio.playlist.currentPath, "/mnt/SDCARD/Music");
    music_init_scan("/mnt/SDCARD/Music");
    load_music_config();
}

void music_scan_folder(const char *path) {
    if (!path) return;
    g_audio.playlist.count = 0;
    g_audio.playlist.currentIdx = -1;
    strncpy(g_audio.playlist.currentPath, path, sizeof(g_audio.playlist.currentPath) - 1);
    g_audio.playlist.currentPath[sizeof(g_audio.playlist.currentPath) - 1] = '\0';
    scan_directory(path, &g_audio.playlist);
}

void music_enter_folder(int idx) {
    if (idx < 0 || idx >= g_audio.playlist.count) return;
    Track *t = &g_audio.playlist.items[idx];
    if (!t->isDir) return;

    // Push current path to breadcrumb
    if (breadcrumb.depth < MAX_BREADCRUMB_DEPTH) {
        strncpy(breadcrumb.paths[breadcrumb.depth], g_audio.playlist.currentPath, 255);
        breadcrumb.paths[breadcrumb.depth][255] = '\0';
        breadcrumb.depth++;
    }

    // Enter folder
    strncpy(g_audio.playlist.currentPath, t->path, sizeof(g_audio.playlist.currentPath) - 1);
    g_audio.playlist.currentPath[sizeof(g_audio.playlist.currentPath) - 1] = '\0';
    g_audio.playlist.count = 0;
    g_audio.playlist.currentIdx = 0;
    scan_directory(t->path, &g_audio.playlist);
    _G.dirty = 1;
}

void music_up_folder() {
    if (breadcrumb.depth == 0) return;  // at root

    breadcrumb.depth--;
    strncpy(g_audio.playlist.currentPath, breadcrumb.paths[breadcrumb.depth], sizeof(g_audio.playlist.currentPath) - 1);
    g_audio.playlist.currentPath[sizeof(g_audio.playlist.currentPath) - 1] = '\0';
    g_audio.playlist.count = 0;
    g_audio.playlist.currentIdx = 0;
    scan_directory(g_audio.playlist.currentPath, &g_audio.playlist);
    _G.dirty = 1;
}

const char *music_get_breadcrumb() {
    static char breadcrumb[256];
    snprintf(breadcrumb, sizeof(breadcrumb), "MUSIC / %s", g_audio.playlist.currentPath + 1);
    return breadcrumb;
}

void music_play(int idx) {
    if (idx < 0 || idx >= g_audio.playlist.count) return;
    Track *t = &g_audio.playlist.items[idx];
    if (t->isDir) return;

    if (audio_load_file(t->path) != 0) {
        ERROR_LOG("Music: play failed %s", t->path);
        return;
    }

    g_audio.playlist.currentIdx = idx;
    g_audio.playing = 1;
    g_audio.paused = 0;
    g_audio.elapsed = 0;
    t->duration = audio_get_duration_ms() / 1000;

    strcpy(_G.now_playing.title, t->title);
    strcpy(_G.now_playing.artist, t->artist);
    _G.now_playing.playing = 1;
    audio_play();
    _G.dirty = 1;
}

void music_pause_toggle() {
    if (!g_audio.playing) return;
    audio_pause();
    g_audio.paused = !g_audio.paused;
    _G.dirty = 1;
}

void music_next() {
    if (g_audio.playlist.count == 0) return;

    int next;
    if (g_audio.repeatMode == 2) {
        // Shuffle: random non-folder track
        int attempts = 0;
        do {
            next = rand() % g_audio.playlist.count;
            attempts++;
        } while (g_audio.playlist.items[next].isDir && attempts < 100);

        if (g_audio.playlist.items[next].isDir) {
            // All items folders, fallback to first non-folder
            for (int i = 0; i < g_audio.playlist.count; i++) {
                if (!g_audio.playlist.items[i].isDir) {
                    next = i;
                    break;
                }
            }
        }
    } else {
        // Normal or once mode: linear next
        next = (g_audio.playlist.currentIdx + 1) % g_audio.playlist.count;
    }

    music_play(next);
}

void music_prev() {
    if (g_audio.playlist.count == 0) return;
    int prev = g_audio.playlist.currentIdx - 1;
    if (prev < 0) prev = g_audio.playlist.count - 1;
    music_play(prev);
}

void music_seek(int sec) {
    int dur = audio_get_duration_ms() / 1000;
    if (sec < 0) sec = 0;
    if (sec > dur) sec = dur;

    audio_seek_ms(sec * 1000);
    g_audio.elapsed = sec;
    last_seek_click_time = SDL_GetTicks();
    _G.dirty = 1;
}

void music_set_volume(int vol) {
    g_audio.volume = (vol < 0) ? 0 : (vol > 128) ? 128 : vol;
    audio_set_volume(g_audio.volume);
}

void music_update() {
    uint32_t now = SDL_GetTicks();

    // Decay when stopped/paused (always, no throttle)
    if (!g_audio.playing || g_audio.paused) {
        int active = 0;
        for (int i = 0; i < 7; i++) {
            g_audio.eq[i] *= 0.65f;
            if (g_audio.eq[i] < 0.01f) g_audio.eq[i] = 0;
            if (g_audio.eq[i] > 0) active = 1;
        }
        if (active) _G.dirty = 1;
        return;
    }

    // Playing: throttle animation to ~50ms (20fps)
    uint32_t delta = now - g_audio.lastEqUpdate;
    if (delta < 50) return;
    g_audio.lastEqUpdate = now;

    // Animate bars: energy from playback position + sine oscillation
    float energy = 0.0f;
    if (g_audio.buffer_size > 0) {
        energy = (float)g_audio.buffer_pos / (float)g_audio.buffer_size;
    }
    energy = energy * 0.6f + 0.2f;  // scale to 20-80% range

    // Oscillation phase (shared across bars, different offsets)
    int phase_ms = now % 2000;  // 2-second cycle

    for (int i = 0; i < 7; i++) {
        // Each bar: sine wave at different phase offset
        float bar_phase = (phase_ms + i * 285) / 2000.0f * 2.0f * M_PI;
        float oscillation = (sinf(bar_phase) + 1.0f) * 0.5f;  // 0-1

        // Combine: energy sets base height, oscillation adds bounce
        float height = energy * 0.7f + oscillation * 0.3f;

        g_audio.eq[i] = (height > 1.0f) ? 1.0f : height;
    }

    _G.dirty = 1;
}

// Visual feedback: highlight bar when seeking
void music_seek_click(int x, int y) {
    // Hit zone: y±10px around bar (y=250), x within bar (20-320)
    if (y < 240 || y > 260) return;
    if (x < 20 || x > 320) return;

    if (!g_audio.playing || g_audio.playlist.count == 0) return;
    if (g_audio.playlist.currentIdx < 0 || g_audio.playlist.currentIdx >= g_audio.playlist.count) return;

    int dur = g_audio.playlist.items[g_audio.playlist.currentIdx].duration;
    if (dur <= 0) return;

    // Map click x (20-320) to seek position (0-dur seconds)
    int bar_width = 300;  // 320 - 20
    int click_rel = x - 20;
    int seek_sec = (click_rel * dur) / bar_width;

    // Clamp
    if (seek_sec < 0) seek_sec = 0;
    if (seek_sec > dur) seek_sec = dur;

    music_seek(seek_sec);
    last_seek_click_time = SDL_GetTicks();
    DEBUG_LOG("Seek click: x=%d -> %d sec", x, seek_sec);
}

uint32_t music_get_seek_feedback_time() {
    uint32_t now = SDL_GetTicks();
    if (now - last_seek_click_time < SEEK_FEEDBACK_MS) {
        return now - last_seek_click_time;
    }
    return 0;  // No active feedback
}

void music_destroy() {
    save_music_config();
    if (g_audio.playlist.items) free(g_audio.playlist.items);
}
