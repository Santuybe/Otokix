#include "otokix.h"

static int init_display_Resolution() {
  SDL_DisplayMode mode;
  SDL_GetCurrentDisplayMode(0, &mode);
  DEBUG_LOG("Native display mode: %dx%d", mode.w, mode.h);

  if (mode.h > mode.w) { // Portrait native, keep native size for window
    _G.rotate = 3;
    _G.WINDOW_WIDTH = mode.w;    // Keep native width (480)
    _G.WINDOW_HEIGHT = mode.h;   // Keep native height (640)
    _G.DISPLAY_WIDTH = 640;      // Logical display is landscape
    _G.DISPLAY_HEIGHT = 480;
    DEBUG_LOG("Portrait detected: window=%dx%d, display=%dx%d, rotate=270°",
              _G.WINDOW_WIDTH, _G.WINDOW_HEIGHT, _G.DISPLAY_WIDTH, _G.DISPLAY_HEIGHT);
  } else {
    _G.rotate = 0;
    _G.WINDOW_WIDTH = mode.w;
    _G.WINDOW_HEIGHT = mode.h;
    _G.DISPLAY_WIDTH = mode.w;
    _G.DISPLAY_HEIGHT = mode.h;
    DEBUG_LOG("Landscape: window=%dx%d, display=%dx%d, rotate=0",
              _G.WINDOW_WIDTH, _G.WINDOW_HEIGHT, _G.DISPLAY_WIDTH, _G.DISPLAY_HEIGHT);
  }
  return 0;
}

static void create_pill_texture() {
    _G.pillW = 340; _G.pillH = 34;
    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, _G.pillW, _G.pillH, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surf);
    int rad = _G.pillH / 2;
    SDL_SetRenderDrawColor(r, 0xCF, 0xCF, 0xCF, 255);
    SDL_RenderDrawLine(r, rad, 0, _G.pillW - rad, 0);
    SDL_RenderDrawLine(r, rad, _G.pillH - 1, _G.pillW - rad, _G.pillH - 1);
    for (int i = 0; i <= 90; i++) {
        float rd = i * M_PI / 180.0f;
        int dx = rad * cos(rd); int dy = rad * sin(rd);
        SDL_RenderDrawPoint(r, rad - dx, rad - dy); SDL_RenderDrawPoint(r, rad - dx, rad + dy - 1);
        SDL_RenderDrawPoint(r, _G.pillW - rad + dx - 1, rad - dy); SDL_RenderDrawPoint(r, _G.pillW - rad + dx - 1, rad + dy - 1);
    }
    _G.pillTexture = SDL_CreateTextureFromSurface(_G.renderer, surf);
    SDL_DestroyRenderer(r); SDL_FreeSurface(surf);
}

int init() {
  // Timezone init
  const char *tz_env = getenv("TZ");
  if (tz_env) {
    DEBUG_LOG("TZ env: %s", tz_env);
  } else {
    _G.tzOffset = 7;
    char tz_str[32];
    snprintf(tz_str, sizeof(tz_str), "UTC%+d", -_G.tzOffset);
    setenv("TZ", tz_str, 1);
    DEBUG_LOG("TZ set to: %s", tz_str);
  }
  tzset();

  DEBUG_LOG("SDL_Init VIDEO + AUDIO...");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    ERROR_LOG("SDL_Init failed: %s", SDL_GetError());
    return -1;
  }
  DEBUG_LOG("SDL_Init OK");

  DEBUG_LOG("TTF_Init...");
  if (TTF_Init() < 0) {
    ERROR_LOG("TTF_Init failed: %s", TTF_GetError());
    return -1;
  }
  DEBUG_LOG("TTF_Init OK");

  DEBUG_LOG("IMG_Init PNG...");
  if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
    ERROR_LOG("IMG_Init PNG failed");
    return -1;
  }
  DEBUG_LOG("IMG_Init OK");

  init_display_Resolution();

  DEBUG_LOG("SDL_CreateWindow %dx%d...", _G.WINDOW_WIDTH, _G.WINDOW_HEIGHT);
  _G.window = SDL_CreateWindow("otokix", 0, 0, _G.WINDOW_WIDTH, _G.WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  if (!_G.window) {
    ERROR_LOG("SDL_CreateWindow failed: %s", SDL_GetError());
    return -1;
  }
  DEBUG_LOG("Window created");

  DEBUG_LOG("SDL_CreateRenderer...");
  _G.renderer = SDL_CreateRenderer(_G.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!_G.renderer) {
    ERROR_LOG("SDL_CreateRenderer failed: %s", SDL_GetError());
    return -1;
  }
  DEBUG_LOG("Renderer created");

  // Create render target for rotation
  if (_G.rotate != 0) {
    DEBUG_LOG("Creating renderTarget 640x480 for rotation...");
    _G.renderTarget = SDL_CreateTexture(_G.renderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_TARGET, _G.DISPLAY_WIDTH, _G.DISPLAY_HEIGHT);
    if (!_G.renderTarget) {
      ERROR_LOG("renderTarget creation failed: %s", SDL_GetError());
      return -1;
    }
    DEBUG_LOG("renderTarget created");
  }

  DEBUG_LOG("Loading fonts...");
  char fontpath[256];
  snprintf(fontpath, sizeof(fontpath), "./fonts/Doto.ttf");
  DEBUG_LOG("Font path: %s", fontpath);

  _G.bigFont = TTF_OpenFont(fontpath, 96);
  if (!_G.bigFont) {
    ERROR_LOG("bigFont load failed: %s", TTF_GetError());
    return -1;
  }

  _G.labelFont = TTF_OpenFont(fontpath, 24);
  if (!_G.labelFont) {
    ERROR_LOG("labelFont load failed: %s", TTF_GetError());
    return -1;
  }

  _G.smallFont = TTF_OpenFont(fontpath, 18);
  if (!_G.smallFont) {
    ERROR_LOG("smallFont load failed: %s", TTF_GetError());
    return -1;
  }
  DEBUG_LOG("Fonts loaded OK");

  DEBUG_LOG("Creating pill texture...");
  create_pill_texture();
  DEBUG_LOG("Pill texture created");

  _G.flags = 1 << FLAG_AMPM; _G.screen = SCREEN_CLOCK; _G.dirty = 1; _G.quit = 0;
  DEBUG_LOG("Init complete, starting main loop");

  // Audio init
  if (audio_init() < 0) {
    DEBUG_LOG("Audio init warning - playback disabled");
  }

  return 0;
}
