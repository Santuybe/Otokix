#include "otokix.h"

void destroy() {
  audio_close();
  if (_G.renderer) SDL_DestroyRenderer(_G.renderer);
  if (_G.window) SDL_DestroyWindow(_G.window);
  if (_G.bigFont) TTF_CloseFont(_G.bigFont);
  if (_G.labelFont) TTF_CloseFont(_G.labelFont);
  if (_G.smallFont) TTF_CloseFont(_G.smallFont);
  if (_G.pillTexture) SDL_DestroyTexture(_G.pillTexture);
  if (_G.renderTarget) SDL_DestroyTexture(_G.renderTarget);
  music_destroy();
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();
}
