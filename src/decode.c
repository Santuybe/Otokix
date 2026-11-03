#include "decode.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.c"

AudioStream *stream_open(const char *path) {
    AudioStream *s = calloc(1, sizeof(AudioStream));
    const char *ext = strrchr(path, '.');
    if (!ext) { free(s); return NULL; }

    if (!strcasecmp(ext, ".mp3")) {
        drmp3 *mp3 = malloc(sizeof(drmp3));
        if (!drmp3_init_file(mp3, path, NULL)) { free(mp3); free(s); return NULL; }
        s->format = AUDIO_FORMAT_MP3;
        s->decoder = mp3;
        s->channels = mp3->channels;
        s->sample_rate = mp3->sampleRate;
        s->total_pcm_frames = drmp3_get_pcm_frame_count(mp3);
        s->duration_ms = (uint32_t)(s->sample_rate ? (s->total_pcm_frames * 1000 / s->sample_rate) : 0);
    } else if (!strcasecmp(ext, ".flac")) {
        drflac *flac = drflac_open_file(path, NULL);
        if (!flac) { free(s); return NULL; }
        s->format = AUDIO_FORMAT_FLAC;
        s->decoder = flac;
        s->channels = flac->channels;
        s->sample_rate = flac->sampleRate;
        s->total_pcm_frames = flac->totalPCMFrameCount;
        s->duration_ms = (uint32_t)(s->sample_rate ? (s->total_pcm_frames * 1000 / s->sample_rate) : 0);
    } else if (!strcasecmp(ext, ".ogg")) {
        int error;
        stb_vorbis *v = stb_vorbis_open_filename(path, &error, NULL);
        if (!v) { free(s); return NULL; }
        s->format = AUDIO_FORMAT_VORBIS;
        s->decoder = v;
        stb_vorbis_info info = stb_vorbis_get_info(v);
        s->channels = info.channels;
        s->sample_rate = info.sample_rate;
        s->total_pcm_frames = stb_vorbis_stream_length_in_samples(v);
        s->duration_ms = (uint32_t)(s->sample_rate ? (s->total_pcm_frames * 1000 / s->sample_rate) : 0);
    } else {
        free(s);
        return NULL;
    }
    return s;
}

size_t stream_read_pcm_s16(AudioStream *stream, int16_t *buffer, size_t max_samples) {
    if (!stream || !stream->decoder) return 0;

    size_t target_frames = max_samples / 2;
    size_t frames_read = 0;

    if (stream->channels == 1) {
        int16_t *mono_buf = buffer + target_frames;
        if (stream->format == AUDIO_FORMAT_MP3) {
            frames_read = drmp3_read_pcm_frames_s16((drmp3 *)stream->decoder, target_frames, mono_buf);
        } else if (stream->format == AUDIO_FORMAT_FLAC) {
            frames_read = drflac_read_pcm_frames_s16((drflac *)stream->decoder, target_frames, mono_buf);
        } else if (stream->format == AUDIO_FORMAT_VORBIS) {
            frames_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis *)stream->decoder, 1, mono_buf, (int)target_frames);
        }
        for (ssize_t i = (ssize_t)frames_read - 1; i >= 0; i--) {
            buffer[i * 2]     = mono_buf[i];
            buffer[i * 2 + 1] = mono_buf[i];
        }
    } else {
        if (stream->format == AUDIO_FORMAT_MP3) {
            frames_read = drmp3_read_pcm_frames_s16((drmp3 *)stream->decoder, target_frames, buffer);
        } else if (stream->format == AUDIO_FORMAT_FLAC) {
            frames_read = drflac_read_pcm_frames_s16((drflac *)stream->decoder, target_frames, buffer);
        } else if (stream->format == AUDIO_FORMAT_VORBIS) {
            int shorts = stb_vorbis_get_samples_short_interleaved((stb_vorbis *)stream->decoder, stream->channels, buffer, (int)(target_frames * stream->channels));
            frames_read = (stream->channels > 0) ? (shorts / stream->channels) : 0;
        }
    }

    stream->current_frame += frames_read;
    return frames_read * 2;
}

void stream_seek_ms(AudioStream *stream, uint32_t ms) {
    if (!stream || !stream->decoder) return;
    uint64_t frame = (uint64_t)ms * stream->sample_rate / 1000;
    if (stream->format == AUDIO_FORMAT_MP3) drmp3_seek_to_pcm_frame((drmp3 *)stream->decoder, frame);
    else if (stream->format == AUDIO_FORMAT_FLAC) drflac_seek_to_pcm_frame((drflac *)stream->decoder, frame);
    else if (stream->format == AUDIO_FORMAT_VORBIS) stb_vorbis_seek_frame((stb_vorbis *)stream->decoder, frame);
    stream->current_frame = frame;
}

void stream_close(AudioStream *stream) {
    if (!stream) return;
    if (stream->decoder) {
        if (stream->format == AUDIO_FORMAT_MP3) drmp3_uninit((drmp3 *)stream->decoder);
        else if (stream->format == AUDIO_FORMAT_FLAC) drflac_close((drflac *)stream->decoder);
        else if (stream->format == AUDIO_FORMAT_VORBIS) stb_vorbis_close((stb_vorbis *)stream->decoder);
        free(stream->decoder);
    }
    free(stream);
}
