#ifndef MUSIC_SCANNER_H__
#define MUSIC_SCANNER_H__

#include "music.h"

// Recursively scan directory and populate playlist
// Returns count of items added, or -1 on error
int scan_directory(const char *path, Playlist *out);

#endif
