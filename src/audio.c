#include "otokix.h"
#include "decode.h"
#include <string.h>
#include <msettings.h>

static struct {
    SDL_AudioDeviceID device;
    int playing, paused;
    uint32_t elapsed_ms, duration_ms;
    uint8_t volume;
    AudioStream *stream;
} audio_state = {0};

static void audio_callback(void *ud, Uint8 *s, int len) {
    (void)ud;
    memset(s, 0, len);
    if (!audio_state.playing || audio_state.paused || !audio_state.stream) return;

    int16_t *dst = (int16_t *)s;
    size_t samples_needed = len / sizeof(int16_t);
    size_t read_samples = stream_read_pcm_s16(audio_state.stream, dst, samples_needed);

    if (read_samples > 0) {
        for (size_t i = 0; i < read_samples; i++) {
            int32_t sample = (int32_t)dst[i] * audio_state.volume / 128;
            if (sample > 32767) sample = 32767;
            if (sample < -32768) sample = -32768;
            dst[i] = (int16_t)sample;
        }
        if (audio_state.stream->sample_rate > 0) {
            audio_state.elapsed_ms = (uint32_t)(audio_state.stream->current_frame * 1000 / audio_state.stream->sample_rate);
        }
    }

    if (read_samples == 0) {
        audio_state.playing = 0;
        DEBUG_LOG("Audio: finished streaming");
    }
}

int audio_init() {
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 44100;
    desired.format = AUDIO_S16;
    desired.channels = 2;
    desired.samples = 4096;
    desired.callback = audio_callback;

    audio_state.device = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (!audio_state.device) {
        ERROR_LOG("Audio: init failed: %s", SDL_GetError());
        return -1;
    }

    InitSettings();
    int v = GetVolume();
    audio_state.volume = (v * 128) / 20;
    if (audio_state.volume > 128) audio_state.volume = 128;

    SDL_PauseAudioDevice(audio_state.device, 1);
    return 0;
}

void audio_close() {
    if (audio_state.device) {
        SDL_LockAudioDevice(audio_state.device);
    }
    if (audio_state.stream) {
        stream_close(audio_state.stream);
        audio_state.stream = NULL;
    }
    if (audio_state.device) {
        SDL_UnlockAudioDevice(audio_state.device);
        SDL_CloseAudioDevice(audio_state.device);
        audio_state.device = 0;
    }
    QuitSettings();
}

int audio_load_file(const char *p) {
    DEBUG_LOG("Audio: load stream %s", p);

    if (audio_state.device) SDL_LockAudioDevice(audio_state.device);

    if (audio_state.stream) {
        stream_close(audio_state.stream);
        audio_state.stream = NULL;
    }

    AudioStream *st = stream_open(p);
    if (!st) {
        ERROR_LOG("Audio: open stream failed: %s", p);
        if (audio_state.device) SDL_UnlockAudioDevice(audio_state.device);
        return -1;
    }

    audio_state.stream = st;
    audio_state.duration_ms = st->duration_ms;
    audio_state.elapsed_ms = 0;

    if (audio_state.device) SDL_UnlockAudioDevice(audio_state.device);

    DEBUG_LOG("Audio: stream loaded %ums, %dHz %dch", st->duration_ms, st->sample_rate, st->channels);
    return 0;
}

void audio_play() {
    if (!audio_state.device || !audio_state.stream) return;
    audio_state.playing = 1;
    audio_state.paused = 0;
    SDL_PauseAudioDevice(audio_state.device, 0);
}

void audio_pause() {
    audio_state.paused = !audio_state.paused;
    SDL_PauseAudioDevice(audio_state.device, audio_state.paused);
}

void audio_stop() {
    audio_state.playing = 0;
    SDL_PauseAudioDevice(audio_state.device, 1);
}

void audio_set_volume(int v) {
    audio_state.volume = (v > 128) ? 128 : (v < 0) ? 0 : v;
}

int audio_get_volume() {
    return audio_state.volume;
}

void audio_seek_ms(uint32_t ms) {
    if (audio_state.device) SDL_LockAudioDevice(audio_state.device);
    if (audio_state.stream) {
        if (ms > audio_state.duration_ms) ms = audio_state.duration_ms;
        stream_seek_ms(audio_state.stream, ms);
        audio_state.elapsed_ms = ms;
    }
    if (audio_state.device) SDL_UnlockAudioDevice(audio_state.device);
}

uint32_t audio_get_elapsed_ms() {
    return audio_state.elapsed_ms;
}

uint32_t audio_get_duration_ms() {
    return audio_state.duration_ms;
}

int audio_is_playing() {
    return audio_state.playing && !audio_state.paused;
}
