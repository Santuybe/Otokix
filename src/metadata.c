#include "metadata.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int read_file_head(const char *path, uint8_t *buf, int max_size) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int n = fread(buf, 1, max_size, f);
    fclose(f);
    return n;
}

static const uint8_t *id3_find_frame(const uint8_t *data, int size, const char *frame_id) {
    if (size < 10) return NULL;
    if (data[0] != 'I' || data[1] != 'D' || data[2] != '3') return NULL;

    int pos = 10;

    while (pos + 10 <= size) {
        if (data[pos] == 0) return NULL;
        if (memcmp(&data[pos], frame_id, 4) == 0) {
            return &data[pos + 10];
        }
        uint32_t frame_size = ((data[pos+4] & 0x7f) << 21) | ((data[pos+5] & 0x7f) << 14)
                            | ((data[pos+6] & 0x7f) << 7) | (data[pos+7] & 0x7f);
        pos += 10 + frame_size;
    }
    return NULL;
}

static void id3_extract_text(const uint8_t *frame_data, int frame_size, char *out, int out_len) {
    if (!frame_data || frame_size < 2) { out[0] = '\0'; return; }

    int src = 1;
    int dst = 0;

    while (src < frame_size && dst < out_len - 1) {
        uint8_t c = frame_data[src];
        if (c == 0) break;
        if (isprint(c)) out[dst++] = c;
        src++;
    }
    out[dst] = '\0';
}

static void extract_mp3_metadata(const char *path, char *title, int title_len, char *artist, int artist_len) {
    uint8_t buf[10240];
    int size = read_file_head(path, buf, sizeof(buf));

    title[0] = artist[0] = '\0';

    const uint8_t *frame = id3_find_frame(buf, size, "TIT2");
    if (frame) id3_extract_text(frame, 256, title, title_len);

    frame = id3_find_frame(buf, size, "TPE1");
    if (frame) id3_extract_text(frame, 256, artist, artist_len);
}

static void extract_flac_metadata(const char *path, char *title, int title_len, char *artist, int artist_len) {
    uint8_t buf[8192];
    int size = read_file_head(path, buf, sizeof(buf));

    title[0] = artist[0] = '\0';

    if (size < 4 || buf[0] != 'f' || buf[1] != 'L' || buf[2] != 'a' || buf[3] != 'C') return;

    int pos = 4;
    while (pos + 7 < size) {
        uint8_t block_type = buf[pos] & 0x7f;
        uint32_t block_len = ((buf[pos+1] & 0xff) << 16) | ((buf[pos+2] & 0xff) << 8) | (buf[pos+3] & 0xff);

        if (block_type == 4) {
            pos += 4;
            if (pos + block_len > size) break;

            uint32_t vendor_len = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
            pos += 4 + vendor_len;

            if (pos + 4 > size) break;
            uint32_t num_comments = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
            pos += 4;

            for (uint32_t i = 0; i < num_comments && pos + 4 < size; i++) {
                uint32_t comment_len = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
                pos += 4;

                if (pos + comment_len > size) break;

                char *comment = (char *)&buf[pos];
                const char *eq = strchr(comment, '=');
                if (eq && (eq - comment) < comment_len) {
                    int key_len = eq - comment;
                    int val_len = comment_len - key_len - 1;
                    const char *val = eq + 1;

                    if (strncasecmp(comment, "TITLE", key_len) == 0 && title[0] == '\0') {
                        int copy = val_len < title_len - 1 ? val_len : title_len - 1;
                        strncpy(title, val, copy);
                        title[copy] = '\0';
                    }
                    if (strncasecmp(comment, "ARTIST", key_len) == 0 && artist[0] == '\0') {
                        int copy = val_len < artist_len - 1 ? val_len : artist_len - 1;
                        strncpy(artist, val, copy);
                        artist[copy] = '\0';
                    }
                }
                pos += comment_len;
            }
            break;
        }
        pos += 4 + block_len;
    }
}

static void extract_ogg_metadata(const char *path, char *title, int title_len, char *artist, int artist_len) {
    uint8_t buf[8192];
    int size = read_file_head(path, buf, sizeof(buf));

    title[0] = artist[0] = '\0';

    for (int pos = 0; pos + 7 < size; pos++) {
        if (buf[pos] == 0x03 && memcmp(&buf[pos+1], "vorbis", 6) == 0) {
            pos += 7;
            if (pos + 4 > size) break;

            uint32_t vendor_len = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
            pos += 4 + vendor_len;

            if (pos + 4 > size) break;
            uint32_t num_comments = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
            pos += 4;

            for (uint32_t i = 0; i < num_comments && pos + 4 < size; i++) {
                uint32_t comment_len = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
                pos += 4;

                if (pos + comment_len > size) break;

                char *comment = (char *)&buf[pos];
                const char *eq = strchr(comment, '=');
                if (eq && (eq - comment) < comment_len) {
                    int key_len = eq - comment;
                    int val_len = comment_len - key_len - 1;
                    const char *val = eq + 1;

                    if (strncasecmp(comment, "TITLE", key_len) == 0 && title[0] == '\0') {
                        int copy = val_len < title_len - 1 ? val_len : title_len - 1;
                        strncpy(title, val, copy);
                        title[copy] = '\0';
                    }
                    if (strncasecmp(comment, "ARTIST", key_len) == 0 && artist[0] == '\0') {
                        int copy = val_len < artist_len - 1 ? val_len : artist_len - 1;
                        strncpy(artist, val, copy);
                        artist[copy] = '\0';
                    }
                }
                pos += comment_len;
            }
            break;
        }
    }
}

void extract_metadata(const char *path, char *title, int title_len, char *artist, int artist_len) {
    if (!path || !title || !artist) return;

    const char *ext = strrchr(path, '.');
    if (!ext) { title[0] = artist[0] = '\0'; return; }

    if (!strcasecmp(ext, ".mp3")) extract_mp3_metadata(path, title, title_len, artist, artist_len);
    else if (!strcasecmp(ext, ".flac")) extract_flac_metadata(path, title, title_len, artist, artist_len);
    else if (!strcasecmp(ext, ".ogg")) extract_ogg_metadata(path, title, title_len, artist, artist_len);
    else { title[0] = artist[0] = '\0'; }
}
