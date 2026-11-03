#ifndef OTOKIX_H__
#define OTOKIX_H__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "defines.h"
#include "api.h"
#include "fcntl.h"
#include "math.h"
#include "signal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/wait.h"
#include "time.h"
#include "unistd.h"
#include "KeyMap.h"

#define FRAME_RATE 30.0f

// Screen modes
#define SCREEN_CLOCK 0
#define SCREEN_MUSIC 1

// Flags
#define FLAG_AMPM 0

// Menu items count
#define MENU_ITEMS 4

#define BG_COLOR 0xFF000000
#define TEXT_COLOR 0xFFCFCFCF
#define TEXT_BRIGHT 0xFFECECEC
#define ACCENT_COLOR 0xFFE8552B
#define BATTERY_RED 0xFFD93A2B

#define UI_TIME_Y 130
#define UI_DATE_Y 272
#define UI_BUTTON_Y 330

#define BATTERY_UPDATE_MS 2000

// Forward declarations
FILE *log_file(void);
int log_init(void);
void log_close(void);

// Debug logging
#define DEBUG_LOG(fmt, ...) do { \
  fprintf(log_file() ? log_file() : stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__); \
  if (log_file()) fflush(log_file()); \
} while(0)
#define ERROR_LOG(fmt, ...) do { \
  fprintf(log_file() ? log_file() : stdout, "[ERROR] " fmt "\n", ##__VA_ARGS__); \
  if (log_file()) fflush(log_file()); \
} while(0)

typedef struct BatteryState {
  int percent;
  int charging;
  uint32_t lastUpdate;
} BatteryState;

typedef struct {
  int playing;
  char title[64];
  char artist[64];
} NowPlaying;

struct Otokix {
  int32_t DISPLAY_WIDTH;
  int32_t DISPLAY_HEIGHT;
  int32_t WINDOW_WIDTH;
  int32_t WINDOW_HEIGHT;
  int rotate;
  int keytest;
  int dirty;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *renderTarget;
  TTF_Font *bigFont;
  TTF_Font *labelFont;
  TTF_Font *smallFont;
  int KEY_STATUS[KEY_STATUS_COUNT];
  int quit;
  int screen;
  int flags;
  int tzOffset;
  int manualTimeEnabled;
  int manualHour;
  int manualMinute;
  int menuOpen;
  int menuIndex;
  int menuEditMode;
  int menuEditField;
  char *timeSetHook;
  BatteryState battery;
  NowPlaying now_playing;
  SDL_Texture *pillTexture;
  int pillW, pillH;
} typedef Otokix;

extern Otokix _G;

#define MODE_AMPM 0
#define is_active_flag(FLAG) ((_G.flags) & (1 << (FLAG)))

// Declarations
int init();
void destroy();
int draw_loop();
void pollevent();
int handle_key();
int draw_screen(float fps);
struct tm *get_local_time();
void toggle_ampm_mode();

// Audio playback
int audio_init();
void audio_close();
int audio_load_file(const char *path);
void audio_play();
void audio_pause();
void audio_stop();
void audio_set_volume(int vol);
int audio_get_volume();
void audio_seek_ms(uint32_t ms);
uint32_t audio_get_elapsed_ms();
uint32_t audio_get_duration_ms();
int audio_is_playing();

// Draw functions
void update_battery();
void check_dirty();
void draw_battery(int x, int y);
void draw_text(TTF_Font *f, const char *t, int x, int y, Uint32 color, int center_x);
void draw_tab(const char *label, int active, int x, int y);
void draw_equalizer(int cx, int cy);
void draw_now_playing_screen();
void draw_playlist_screen();
int music_screen();
void execute_menu_item(int idx);

// Music functions
void music_init();
void music_scan_folder(const char *path);
void music_enter_folder(int idx);
void music_up_folder();
void music_play(int idx);
void music_pause_toggle();
void music_next();
void music_prev();
void music_seek(int sec);
void music_set_volume(int vol);
void music_input(int key);
void music_destroy();

// Input handling
void handle_keydown(SDL_Keycode sym);
void handle_keyup(SDL_Keycode sym);

// Render helpers
int RenderClear(SDL_Renderer *renderer, Uint32 color);
int RenderFillRect(SDL_Renderer *renderer, SDL_Rect *rect, Uint32 color);
int RenderPresent(SDL_Renderer *renderer);
void RenderFillCircle(SDL_Renderer *renderer, int cx, int cy, int radius, Uint32 color);
int BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Renderer *dst, SDL_Rect *dstrect);
SDL_Color uint32ToSDLColor(Uint32 color);

#endif
