#include "music_scanner.h"
#include "decode.h"
#include "metadata.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
    char path[256];
    char title[256];
    int isDir;
    uint32_t duration;
} ScanItem;

static const char *get_extension(const char *name) {
    const char *ext = strrchr(name, '.');
    return ext ? ext : "";
}

static int is_audio_file(const char *name) {
    const char *ext = get_extension(name);
    return !strcasecmp(ext, ".mp3") || !strcasecmp(ext, ".flac") || !strcasecmp(ext, ".ogg");
}

static void extract_title(const char *name, char *out, int len) {
    const char *base = strrchr(name, '/');
    if (!base) base = name;
    else base++;

    const char *dot = strchr(base, '.');
    if (dot) {
        int title_len = dot - base;
        if (title_len >= len) title_len = len - 1;
        strncpy(out, base, title_len);
        out[title_len] = '\0';
    } else {
        strncpy(out, base, len - 1);
        out[len - 1] = '\0';
    }
}

static uint32_t get_file_duration(const char *path) {
    AudioStream *s = stream_open(path);
    if (!s) return 0;

    uint32_t duration_sec = s->duration_ms / 1000;
    stream_close(s);
    return duration_sec;
}

static int compare_items(const void *a, const void *b) {
    const ScanItem *ia = (const ScanItem *)a;
    const ScanItem *ib = (const ScanItem *)b;

    // Directories first
    if (ia->isDir != ib->isDir) {
        return ib->isDir - ia->isDir;
    }

    // Then alphabetically
    return strcasecmp(ia->title, ib->title);
}

static int scan_recursive(const char *path, ScanItem *items, int *count, int max_count, int depth) {
    if (*count >= max_count) return 0;
    if (depth > 16) return 0;  // Prevent infinite recursion

    DIR *d = opendir(path);
    if (!d) return -1;

    struct dirent *ent;
    while ((ent = readdir(d)) && *count < max_count) {
        if (ent->d_name[0] == '.') continue;

        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, ent->d_name);

        if (ent->d_type == DT_DIR) {
            ScanItem *item = &items[*count];
            snprintf(item->path, sizeof(item->path), "%s", fullpath);
            extract_title(ent->d_name, item->title, sizeof(item->title));
            item->isDir = 1;
            item->duration = 0;
            (*count)++;
        } else if (ent->d_type == DT_REG && is_audio_file(ent->d_name)) {
            ScanItem *item = &items[*count];
            snprintf(item->path, sizeof(item->path), "%s", fullpath);
            extract_title(ent->d_name, item->title, sizeof(item->title));
            item->isDir = 0;
            item->duration = get_file_duration(fullpath);
            (*count)++;
        }
    }

    closedir(d);
    return 0;
}

int scan_directory(const char *path, Playlist *out) {
    if (!path || !out) return -1;

    ScanItem *items = malloc(out->capacity * sizeof(ScanItem));
    if (!items) return -1;

    int count = 0;
    int result = scan_recursive(path, items, &count, out->capacity, 0);

    if (result == 0 && count > 0) {
        qsort(items, count, sizeof(ScanItem), compare_items);
    }

    // Copy to playlist
    out->count = 0;
    for (int i = 0; i < count && i < out->capacity; i++) {
        Track *t = &out->items[i];
        snprintf(t->path, sizeof(t->path), "%s", items[i].path);
        snprintf(t->title, sizeof(t->title), "%s", items[i].title);
        strcpy(t->artist, "");
        t->duration = items[i].duration;
        t->isDir = items[i].isDir;
        out->count++;
    }

    free(items);
    return out->count;
}
