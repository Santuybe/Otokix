#ifndef DECODE_H__
#define DECODE_H__

#include <stdint.h>
#include <stddef.h>

typedef enum {
    AUDIO_FORMAT_NONE,
    AUDIO_FORMAT_MP3,
    AUDIO_FORMAT_FLAC,
    AUDIO_FORMAT_VORBIS
} AudioFormat;

typedef struct {
    AudioFormat format;
    void *decoder;
    uint32_t duration_ms;
    int channels;
    int sample_rate;
    uint64_t total_pcm_frames;
    uint64_t current_frame;
} AudioStream;

AudioStream *stream_open(const char *path);
size_t stream_read_pcm_s16(AudioStream *stream, int16_t *buffer, size_t max_samples);
void stream_seek_ms(AudioStream *stream, uint32_t ms);
void stream_close(AudioStream *stream);

#endif
