#include "ambient_effects.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.14159265358979323846f

static int16_t to_i16(float x)
{
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return (int16_t)lrintf(x * 32767.0f);
}

static void write_u16_le(FILE *file, uint16_t value)
{
    fputc((int)(value & 255u), file);
    fputc((int)((value >> 8) & 255u), file);
}

static void write_u32_le(FILE *file, uint32_t value)
{
    fputc((int)(value & 255u), file);
    fputc((int)((value >> 8) & 255u), file);
    fputc((int)((value >> 16) & 255u), file);
    fputc((int)((value >> 24) & 255u), file);
}

static int write_wav(const char *path, const float *audio, size_t frames)
{
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    uint64_t data_size_64 = frames * 4u;
    if (data_size_64 > UINT32_MAX - 36u) {
        fclose(file);
        return 0;
    }
    uint32_t data_size = (uint32_t)data_size_64;
    fwrite("RIFF", 1u, 4u, file);
    write_u32_le(file, 36u + data_size);
    fwrite("WAVEfmt ", 1u, 8u, file);
    write_u32_le(file, 16u);
    write_u16_le(file, 1u);
    write_u16_le(file, 2u);
    write_u32_le(file, AMBIENT_FX_SAMPLE_RATE);
    write_u32_le(file, AMBIENT_FX_SAMPLE_RATE * 4u);
    write_u16_le(file, 4u);
    write_u16_le(file, 16u);
    fwrite("data", 1u, 4u, file);
    write_u32_le(file, data_size);
    for (size_t i = 0u; i < frames * 2u; ++i) {
        write_u16_le(file, (uint16_t)to_i16(audio[i]));
    }
    int ok = ferror(file) == 0;
    fclose(file);
    return ok;
}

static void add_note(float *audio, size_t total_frames,
                     float start_seconds, float duration_seconds,
                     float frequency, float gain, float pan)
{
    size_t start = (size_t)(start_seconds * (float)AMBIENT_FX_SAMPLE_RATE);
    size_t count = (size_t)(duration_seconds * (float)AMBIENT_FX_SAMPLE_RATE);
    if (start >= total_frames) return;
    if (count > total_frames - start) count = total_frames - start;
    float phase = 0.0f;
    float phase_b = 0.0f;
    float attack = 0.36f * (float)AMBIENT_FX_SAMPLE_RATE;
    float release = 1.15f * (float)AMBIENT_FX_SAMPLE_RATE;
    float left_gain = sqrtf(0.5f * (1.0f - pan));
    float right_gain = sqrtf(0.5f * (1.0f + pan));
    for (size_t i = 0u; i < count; ++i) {
        float envelope = 1.0f;
        if ((float)i < attack) envelope *= (float)i / attack;
        size_t remaining = count - 1u - i;
        if ((float)remaining < release) envelope *= (float)remaining / release;
        envelope = envelope * envelope * (3.0f - 2.0f * envelope);
        float body = sinf(phase) * 0.72f + sinf(phase_b) * 0.28f;
        float sample = body * envelope * gain;
        audio[(start + i) * 2u] += sample * left_gain;
        audio[(start + i) * 2u + 1u] += sample * right_gain;
        phase += 2.0f * PI_F * frequency / (float)AMBIENT_FX_SAMPLE_RATE;
        phase_b += 2.0f * PI_F * frequency * 2.003f / (float)AMBIENT_FX_SAMPLE_RATE;
        if (phase >= 2.0f * PI_F) phase -= 2.0f * PI_F;
        if (phase_b >= 2.0f * PI_F) phase_b -= 2.0f * PI_F;
    }
}

static void make_phrase(float *audio, size_t frames)
{
    memset(audio, 0, frames * 2u * sizeof(float));
    static const float chord[4][4] = {
        {73.42f, 110.00f, 146.83f, 164.81f},
        {49.00f, 73.42f, 82.41f, 123.47f},
        {65.41f, 98.00f, 123.47f, 146.83f},
        {55.00f, 82.41f, 98.00f, 146.83f}
    };
    static const float pans[4] = {-0.55f, -0.18f, 0.22f, 0.58f};
    for (unsigned c = 0u; c < 4u; ++c) {
        float start = 1.20f + (float)c * 3.70f;
        for (unsigned n = 0u; n < 4u; ++n) {
            add_note(audio, frames, start, 4.35f, chord[c][n], 0.105f, pans[n]);
        }
    }
    static const float melody[][3] = {
        {1.55f, 440.00f, 1.45f}, {3.35f, 523.25f, 1.18f},
        {5.22f, 493.88f, 1.42f}, {7.08f, 440.00f, 1.20f},
        {8.92f, 659.25f, 1.40f}, {10.75f, 587.33f, 1.30f},
        {12.62f, 440.00f, 1.48f}, {14.35f, 329.63f, 1.18f}
    };
    for (unsigned i = 0u; i < sizeof(melody) / sizeof(melody[0]); ++i) {
        add_note(audio, frames, melody[i][0], melody[i][2], melody[i][1], 0.16f, 0.0f);
    }
}

static void finish_mode_transition(AmbientFx *fx)
{
    float silence[512u * 2u];
    memset(silence, 0, sizeof(silence));
    ambient_fx_process_f32(fx, silence, 512u);
}

static int render_mode(const char *output_dir,
                       AmbientFxMode mode,
                       const char *slug,
                       const float *source,
                       size_t frames,
                       void *arena,
                       size_t arena_bytes,
                       const AmbientFxConfig *config)
{
    float *audio = (float *)malloc(frames * 2u * sizeof(float));
    if (!audio) return 0;
    memcpy(audio, source, frames * 2u * sizeof(float));
    AmbientFxStorage storage;
    AmbientFx *fx = ambient_fx_init(&storage, arena, arena_bytes, config);
    if (!fx) {
        free(audio);
        return 0;
    }
    AmbientFxParameters p = ambient_fx_world_parameters(AMBIENT_FX_TOKYO_CITY);
    if (mode == AMBIENT_FX_DARK_REVERB) {
        p.space = 0.90f; p.atmosphere = 0.82f; p.tone = 0.28f;
    } else if (mode == AMBIENT_FX_PING_PONG_DELAY) {
        p.echo = 0.82f; p.delay_seconds = 0.430f;
    } else if (mode == AMBIENT_FX_CHORUS_DETUNE) {
        p.motion = 0.88f; p.width = 0.96f;
    } else if (mode == AMBIENT_FX_TAPE_AGE) {
        p.age = 0.84f;
    } else if (mode == AMBIENT_FX_REVERSE_SWELL) {
        p.space = 0.72f; p.atmosphere = 0.70f;
    } else if (mode == AMBIENT_FX_SHIMMER_REVERB) {
        p.space = 0.88f; p.atmosphere = 0.82f; p.shimmer = 0.42f;
    } else if (mode == AMBIENT_FX_BLUR) {
        p.blur = 0.78f; p.motion = 0.48f;
    } else if (mode == AMBIENT_FX_DREAM_CHAIN) {
        p.space = 0.82f; p.atmosphere = 0.68f; p.echo = 0.42f;
        p.motion = 0.48f; p.age = 0.48f; p.shimmer = 0.09f; p.blur = 0.12f;
    }
    ambient_fx_set_parameters(fx, p);
    ambient_fx_set_mode(fx, mode);
    finish_mode_transition(fx);
    if (mode == AMBIENT_FX_REVERSE_SWELL || mode == AMBIENT_FX_DREAM_CHAIN) {
        ambient_fx_trigger_reverse_swell(fx, 220.0f, 0.58f, 1.20f);
    }
    ambient_fx_process_f32(fx, audio, frames);

    char path[512];
    int written = snprintf(path, sizeof(path), "%s/%s.wav", output_dir, slug);
    int ok = written > 0 && (size_t)written < sizeof(path) && write_wav(path, audio, frames);
    if (ok) printf("rendered %s\n", path);
    free(audio);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    const char *output_dir = argv[1];
    const size_t source_frames = AMBIENT_FX_SAMPLE_RATE * 16u;
    const size_t tail_frames = AMBIENT_FX_SAMPLE_RATE * 5u;
    const size_t render_frames = source_frames + tail_frames;
    float *source = (float *)calloc(render_frames * 2u, sizeof(float));
    if (!source) return 1;
    make_phrase(source, source_frames);

    AmbientFxConfig config = ambient_fx_default_config();
    size_t arena_bytes = ambient_fx_memory_required(&config);
    void *arena = malloc(arena_bytes);
    if (!arena) {
        free(source);
        return 1;
    }

    char dry_path[512];
    int dry_written = snprintf(dry_path, sizeof(dry_path), "%s/00-dry.wav", output_dir);
    int ok = dry_written > 0 && (size_t)dry_written < sizeof(dry_path) &&
             write_wav(dry_path, source, source_frames);
    ok &= render_mode(output_dir, AMBIENT_FX_DARK_REVERB, "01-dark-reverb",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_PING_PONG_DELAY, "02-ping-pong-delay",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_CHORUS_DETUNE, "03-chorus-detune",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_TAPE_AGE, "04-tape-age",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_REVERSE_SWELL, "05-live-reverse-swell",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_SHIMMER_REVERB, "06-shimmer-reverb",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_BLUR, "07-blur",
                      source, render_frames, arena, arena_bytes, &config);
    ok &= render_mode(output_dir, AMBIENT_FX_DREAM_CHAIN, "08-dream-chain",
                      source, render_frames, arena, arena_bytes, &config);

    AmbientFxStorage reverse_storage;
    AmbientFx *reverse_fx = ambient_fx_init(&reverse_storage, arena, arena_bytes, &config);
    size_t reverse_tail = AMBIENT_FX_SAMPLE_RATE * 3u;
    size_t reverse_frames = source_frames + reverse_tail;
    float *reverse_audio = (float *)calloc(reverse_frames * 2u, sizeof(float));
    if (!reverse_fx || !reverse_audio) {
        ok = 0;
    } else {
        AmbientFxParameters p = ambient_fx_world_parameters(AMBIENT_FX_AFTER_HOURS);
        p.space = 0.86f;
        ambient_fx_set_parameters(reverse_fx, p);
        size_t count = ambient_fx_reverse_reverb_offline_f32(
            reverse_fx, source, source_frames, reverse_audio, reverse_frames,
            reverse_tail, 0.70f, 0.72f);
        char path[512];
        int length = snprintf(path, sizeof(path), "%s/09-true-reverse-reverb.wav", output_dir);
        ok &= count == reverse_frames && length > 0 && (size_t)length < sizeof(path) &&
              write_wav(path, reverse_audio, reverse_frames);
        if (count == reverse_frames) printf("rendered %s\n", path);
    }

    free(reverse_audio);
    free(arena);
    free(source);
    return ok ? 0 : 1;
}
