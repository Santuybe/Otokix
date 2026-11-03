#include "otokix.h"

void handle_keydown(SDL_Keycode sym);
void handle_keyup(SDL_Keycode sym);

static int key_index_for(SDL_Keycode sym) {
  switch (sym) {
  case BTN_UP:
    return BTN_UP_INDEX;
  case BTN_DOWN:
    return BTN_DOWN_INDEX;
  case BTN_LEFT:
    return BTN_LEFT_INDEX;
  case BTN_RIGHT:
    return BTN_RIGHT_INDEX;
  case BTN_A:
    return BTN_A_INDEX;
  case BTN_B:
    return BTN_B_INDEX;
  case BTN_X:
    return BTN_X_INDEX;
  case BTN_Y:
    return BTN_Y_INDEX;
  case BTN_L1:
    return BTN_L1_INDEX;
  case BTN_R1:
    return BTN_R1_INDEX;
  case BTN_L2:
    return BTN_L2_INDEX;
  case BTN_R2:
    return BTN_R2_INDEX;
  case BTN_SELECT:
    return BTN_SELECT_INDEX;
  case BTN_START:
    return BTN_START_INDEX;
  case BTN_MENU:
    return BTN_MENU_INDEX;
  case BTN_POWER:
    return BTN_POWER_INDEX;
  default:
    return -1;
  }
}

void pollevent() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      DEBUG_LOG("SDL_QUIT event received");
      _G.quit = 1;
      break;
    case SDL_KEYDOWN:
      handle_keydown(event.key.keysym.sym);
      break;
    case SDL_KEYUP:
      handle_keyup(event.key.keysym.sym);
      break;
    default:
      break;
    }
  }
}

void handle_keydown(SDL_Keycode sym) {
  int keyIndex = key_index_for(sym);
  if (_G.keytest) {
    DEBUG_LOG("Key: %d", sym);
  }
  if (keyIndex >= 0) {
    _G.KEY_STATUS[keyIndex] = 1;
  }
}

void handle_keyup(SDL_Keycode sym) {
  int keyIndex = key_index_for(sym);
  if (keyIndex >= 0) {
    _G.KEY_STATUS[keyIndex] = 0;
  }
}

int handle_key() {
  if (_G.KEY_STATUS[BTN_MENU_INDEX]) {
    _G.KEY_STATUS[BTN_MENU_INDEX] = 0;
    _G.menuOpen = !_G.menuOpen;
    _G.dirty = 1;
  }
  if (_G.menuOpen) {
    if (_G.KEY_STATUS[BTN_UP_INDEX]) {
      _G.KEY_STATUS[BTN_UP_INDEX] = 0;
      _G.menuIndex = (_G.menuIndex - 1 + MENU_ITEMS) % MENU_ITEMS;
      _G.dirty = 1;
    }
    if (_G.KEY_STATUS[BTN_DOWN_INDEX]) {
      _G.KEY_STATUS[BTN_DOWN_INDEX] = 0;
      _G.menuIndex = (_G.menuIndex + 1) % MENU_ITEMS;
      _G.dirty = 1;
    }
    if (_G.KEY_STATUS[BTN_A_INDEX]) {
      _G.KEY_STATUS[BTN_A_INDEX] = 0;
      execute_menu_item(_G.menuIndex);
      _G.dirty = 1;
    }
    if (_G.KEY_STATUS[BTN_B_INDEX]) {
      _G.KEY_STATUS[BTN_B_INDEX] = 0;
      _G.menuOpen = 0;
      _G.dirty = 1;
    }
  } else {
    if (_G.KEY_STATUS[BTN_SELECT_INDEX]) {
      _G.KEY_STATUS[BTN_SELECT_INDEX] = 0;
      _G.screen = (_G.screen == SCREEN_CLOCK) ? SCREEN_MUSIC : SCREEN_CLOCK;
      _G.dirty = 1;
    }

    // Hotkeys for clock screen
    if (_G.screen == SCREEN_CLOCK) {
      if (_G.KEY_STATUS[BTN_X_INDEX]) {
        _G.KEY_STATUS[BTN_X_INDEX] = 0;
        toggle_ampm_mode();
        _G.dirty = 1;
      }
    }

    // Music input - edge-triggered per button
    if (_G.screen == SCREEN_MUSIC) {
      if (_G.KEY_STATUS[BTN_START_INDEX]) { _G.KEY_STATUS[BTN_START_INDEX] = 0; music_input(KEYCODE_START); }
      if (_G.KEY_STATUS[BTN_A_INDEX]) { _G.KEY_STATUS[BTN_A_INDEX] = 0; music_input(KEYCODE_A); }
      if (_G.KEY_STATUS[BTN_B_INDEX]) { _G.KEY_STATUS[BTN_B_INDEX] = 0; music_input(KEYCODE_B); }
      if (_G.KEY_STATUS[BTN_X_INDEX]) { _G.KEY_STATUS[BTN_X_INDEX] = 0; music_input(KEYCODE_X); }
      if (_G.KEY_STATUS[BTN_UP_INDEX]) { _G.KEY_STATUS[BTN_UP_INDEX] = 0; music_input(KEYCODE_UP); }
      if (_G.KEY_STATUS[BTN_DOWN_INDEX]) { _G.KEY_STATUS[BTN_DOWN_INDEX] = 0; music_input(KEYCODE_DOWN); }
      if (_G.KEY_STATUS[BTN_LEFT_INDEX]) { _G.KEY_STATUS[BTN_LEFT_INDEX] = 0; music_input(KEYCODE_LEFT); }
      if (_G.KEY_STATUS[BTN_RIGHT_INDEX]) { _G.KEY_STATUS[BTN_RIGHT_INDEX] = 0; music_input(KEYCODE_RIGHT); }
      if (_G.KEY_STATUS[BTN_L1_INDEX]) { _G.KEY_STATUS[BTN_L1_INDEX] = 0; music_input(KEYCODE_L1); }
      if (_G.KEY_STATUS[BTN_R1_INDEX]) { _G.KEY_STATUS[BTN_R1_INDEX] = 0; music_input(KEYCODE_R1); }
      if (_G.KEY_STATUS[BTN_L2_INDEX]) { _G.KEY_STATUS[BTN_L2_INDEX] = 0; music_input(KEYCODE_L2); }
      if (_G.KEY_STATUS[BTN_R2_INDEX]) { _G.KEY_STATUS[BTN_R2_INDEX] = 0; music_input(KEYCODE_R2); }
    }
  }
  return 0;
}
