#include "otokix.h"
#include "music.h"
#include <errno.h>
#include <dirent.h>

Otokix _G = { .DISPLAY_WIDTH = 640, .DISPLAY_HEIGHT = 480, .dirty = 1 };

void quit(int exitcode) { destroy(); exit(exitcode); }

int main(int argc, char *argv[]) {
  log_init();

  DEBUG_LOG("Args: argc=%d", argc);
  for (int i = 0; i < argc; i++) {
    DEBUG_LOG("  argv[%d]=%s", i, argv[i]);
    if (!strcmp(argv[i], "--keytest")) _G.keytest = 1;
    if (!strcmp(argv[i], "--set-time-hook") && i + 1 < argc) {
      _G.timeSetHook = argv[++i];
      DEBUG_LOG("Time hook: %s", _G.timeSetHook);
    }
  }

  signal(SIGTERM, quit); signal(SIGINT, quit);
  if (init() < 0) {
    ERROR_LOG("init() failed");
    log_close();
    return EXIT_FAILURE;
  }

  music_init();
  DEBUG_LOG("Music initialized: %d items", g_audio.playlist.count);

  while(!_G.quit) {
    pollevent();
    handle_key();

    // Update music state and sync UI
    music_update();
    // Sync elapsed time from audio
    g_audio.elapsed = audio_get_elapsed_ms() / 1000;
    // Detect song end
    if (g_audio.playing && !g_audio.paused && !audio_is_playing()) {
      if (g_audio.repeatMode == 1) {
        // Once mode: stop playing
        g_audio.playing = 0;
        g_audio.paused = 0;
        audio_stop();
      } else {
        // Repeat or shuffle: next track
        music_next();
      }
    }

    if (_G.dirty) {
        draw_loop();
    }
    SDL_Delay(33);
  }
  destroy();
  log_close();
  return EXIT_SUCCESS;
}
