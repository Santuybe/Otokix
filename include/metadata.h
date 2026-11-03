#ifndef METADATA_H__
#define METADATA_H__

#include <stdint.h>

// Extract title/artist from audio file headers only (no full decode)
// Fills fixed buffers. Safe for ARM cross-compile, no malloc.
void extract_metadata(const char *path, char *title, int title_len, char *artist, int artist_len);

#endif
