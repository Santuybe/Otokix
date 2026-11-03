#ifndef MUSIC_H__
#define MUSIC_H__

#include <stdint.h>
#include <time.h>
#include "KeyMap.h"

typedef struct {
    char path[256];
    char title[256];
    char artist[64];
    uint32_t duration;
    int isDir;
} Track;

typedef struct {
    Track *items;
    int count;
    int capacity;
    int currentIdx;
    char currentPath[256];
} Playlist;

typedef struct {
    int playing;
    int paused;
    uint32_t elapsed;
    int volume;
    int repeatMode;  // 0=repeat all, 1=once, 2=shuffle
    Playlist playlist;
    float eq[7];
    uint32_t lastEqUpdate;
    uint8_t *buffer;
    size_t buffer_size;
    size_t buffer_pos;
    uint32_t duration_ms;
} AudioState;

extern AudioState g_audio;

void music_init();
void music_init_scan(const char *root_path);
void music_enter_folder(int idx);
void music_up_folder();
void music_play(int idx);
void music_pause_toggle();
void music_next();
void music_prev();
void music_seek(int sec);
void music_seek_click(int x, int y);
uint32_t music_get_seek_feedback_time();
void music_set_volume(int vol);
void music_update();
void music_destroy();
const char *music_get_breadcrumb();

#endif
