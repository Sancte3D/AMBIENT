#include "ambient_models.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct HeldNote {
    uint16_t id;
    uint32_t off_frame;
} HeldNote;

static void write_u16(FILE *f, uint16_t x)
{
    fputc((int)(x & 255u), f);
    fputc((int)(x >> 8), f);
}

static void write_u32(FILE *f, uint32_t x)
{
    fputc((int)(x & 255u), f);
    fputc((int)((x >> 8) & 255u), f);
    fputc((int)((x >> 16) & 255u), f);
    fputc((int)(x >> 24), f);
}

static void write_wav_header(FILE *f, uint32_t frames)
{
    uint32_t data_bytes = frames * 4u;
    fwrite("RIFF", 1u, 4u, f);
    write_u32(f, 36u + data_bytes);
    fwrite("WAVEfmt ", 1u, 8u, f);
    write_u32(f, 16u);
    write_u16(f, 1u);
    write_u16(f, 2u);
    write_u32(f, AMBIENT_SAMPLE_RATE);
    write_u32(f, AMBIENT_SAMPLE_RATE * 4u);
    write_u16(f, 4u);
    write_u16(f, 16u);
    fwrite("data", 1u, 4u, f);
    write_u32(f, data_bytes);
}

static float midi_hz(int midi)
{
    return 440.0f * powf(2.0f, (float)(midi - 69) / 12.0f);
}

static int is_sustained(AmbientModel model)
{
    return model == AMBIENT_CHORUS_MIST || model == AMBIENT_ION_STORM ||
           model == AMBIENT_NACRE_HORIZON || model == AMBIENT_TIDEGLASS ||
           model == AMBIENT_HOLLOW_CHOIR;
}

static void note_on(AmbientSynth *s, HeldNote *held, unsigned *held_count,
                    int midi, float velocity, float pan, float duration_seconds,
                    uint32_t now)
{
    if (*held_count >= 32u) return;
    held[*held_count].id = ambient_synth_note_on(s, midi_hz(midi), velocity, pan);
    held[*held_count].off_frame = now + (uint32_t)(duration_seconds * AMBIENT_SAMPLE_RATE);
    ++*held_count;
}

static void release_due(AmbientSynth *s, HeldNote *held, unsigned *held_count, uint32_t now)
{
    unsigned write = 0u;
    for (unsigned i = 0; i < *held_count; ++i) {
        if (now >= held[i].off_frame) {
            ambient_synth_note_off(s, held[i].id);
        } else {
            held[write++] = held[i];
        }
    }
    *held_count = write;
}

static void schedule_sustained(AmbientSynth *s, HeldNote *held, unsigned *count,
                               unsigned scene, uint32_t now)
{
    /* Original interval fields: open, non-functional harmony with no quoted melody. */
    static const int roots[4] = {50, 46, 53, 48};
    static const int intervals[4][4] = {
        {0, 7, 14, 21}, {0, 5, 12, 19}, {0, 7, 10, 17}, {0, 4, 11, 18}
    };
    int root = roots[scene & 3u];
    for (unsigned i = 0; i < 4u; ++i) {
        float pan = -0.72f + (float)i * 0.48f;
        note_on(s, held, count, root + intervals[scene & 3u][i],
                0.52f - (float)i * 0.055f, pan, 4.05f, now);
    }
}

static void schedule_percussive(AmbientSynth *s, HeldNote *held, unsigned *count,
                                unsigned step, uint32_t now)
{
    static const int path[18] = {0, 7, 14, 3, 10, 19, 5, 12, 17,
                                 2, 9, 16, 7, 21, 12, 4, 18, 11};
    int base = s->model == AMBIENT_ACID_RAIN ? 67 : 55;
    if (s->model == AMBIENT_BAMBOO_CIRCUIT) base = 50;
    float pan = (float)((int)(step % 7u) - 3) * 0.24f;
    float velocity = 0.46f + (float)((step * 5u) % 9u) * 0.045f;
    note_on(s, held, count, base + path[step % 18u], velocity, pan,
            0.32f + (float)(step % 4u) * 0.18f, now);
    if ((step % 5u) == 0u) {
        note_on(s, held, count, base - 12 + path[(step + 7u) % 18u],
                velocity * 0.55f, -pan * 0.7f, 0.55f, now);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s MODEL_INDEX OUTPUT.wav [SECONDS]\n", argv[0]);
        return 2;
    }
    int model_i = atoi(argv[1]);
    if (model_i < 0 || model_i >= AMBIENT_MODEL_COUNT) {
        fprintf(stderr, "invalid model index\n");
        return 2;
    }
    float seconds = argc > 3 ? strtof(argv[3], NULL) : 14.0f;
    if (seconds < 2.0f || seconds > 300.0f) {
        fprintf(stderr, "duration must be between 2 and 300 seconds\n");
        return 2;
    }
    uint32_t frames = (uint32_t)(seconds * AMBIENT_SAMPLE_RATE);
    FILE *file = fopen(argv[2], "wb");
    if (!file) {
        perror(argv[2]);
        return 1;
    }
    write_wav_header(file, frames);

    AmbientSynth synth;
    AmbientModel model = (AmbientModel)model_i;
    ambient_synth_init(&synth, model, 0x510e527fu + (uint32_t)model_i * 977u);
    HeldNote held[32] = {{0}};
    unsigned held_count = 0u;
    uint32_t next_event = 0u;
    unsigned event = 0u;
    const uint32_t spacing = (uint32_t)((is_sustained(model) ? 3.15f : 0.56f) * AMBIENT_SAMPLE_RATE);
    int16_t block[512u * 2u];
    uint64_t sum_squares = 0u;
    int peak = 0;

    for (uint32_t now = 0u; now < frames;) {
        release_due(&synth, held, &held_count, now);
        if (now >= next_event && now < frames - AMBIENT_SAMPLE_RATE * 2u) {
            if (is_sustained(model)) schedule_sustained(&synth, held, &held_count, event, now);
            else schedule_percussive(&synth, held, &held_count, event, now);
            next_event += spacing;
            ++event;
        }

        float t = (float)now / (float)AMBIENT_SAMPLE_RATE;
        AmbientControls controls = ambient_model_default_controls(model);
        controls.motion = fminf(1.0f, controls.motion + 0.12f * sinf(t * 0.31f));
        controls.color = fminf(1.0f, fmaxf(0.0f, controls.color + 0.10f * sinf(t * 0.23f + 0.7f)));
        controls.space = fminf(0.98f, controls.space + 0.07f * sinf(t * 0.17f));
        ambient_synth_set_controls(&synth, controls);

        size_t n = frames - now;
        if (n > 512u) n = 512u;
        ambient_synth_render(&synth, block, n);
        if (fwrite(block, sizeof(int16_t) * 2u, n, file) != n) {
            fprintf(stderr, "write failed\n");
            fclose(file);
            return 1;
        }
        for (size_t i = 0; i < n * 2u; ++i) {
            int sample = block[i];
            int magnitude = sample < 0 ? -sample : sample;
            if (magnitude > peak) peak = magnitude;
            sum_squares += (uint64_t)((int64_t)sample * sample);
        }
        now += (uint32_t)n;
    }
    fclose(file);
    double rms = sqrt((double)sum_squares / ((double)frames * 2.0)) / 32768.0;
    printf("%s: %.2f s, peak %.3f, rms %.4f, state %zu bytes\n",
           ambient_model_name(model), seconds, (double)peak / 32768.0, rms,
           ambient_synth_state_bytes());
    return 0;
}
