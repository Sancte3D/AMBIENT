/*
 * Sample renderer — one WAV per setting, in a samples/ directory.
 *
 * For each thing you can turn on the device, plays the same short musical
 * phrase under three (or more) values of just that one parameter, with brief
 * silence between, so you can A/B them by ear. Same engine code path as the
 * firmware, so the sound is what the device produces (modulo the analog
 * chain).
 *
 * NOTE on Brightness: hardware EN2 ("Brightness") is the DISPLAY backlight
 * (SPEC §5 + §12). It does not affect audio — no sample.
 *
 * Build (from firmware-c-next/):  bash tools/render_samples.sh
 * Outputs every file under ./samples/ as <setting>.wav.
 */

#include "engine.h"
#include "brain.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#define SR     44100
#define BLOCK  256
#define CELL_AMP 0.12f                  /* matches firmware CELL_VOICE_AMP */

/* ---- WAV helpers --------------------------------------------------------- */

static void w_u32(FILE *f, uint32_t v) {
    fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f);
    fputc((v >> 16) & 0xff, f); fputc((v >> 24) & 0xff, f);
}
static void w_u16(FILE *f, uint16_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); }

/* Write the header with placeholder sizes; patched at the end once we know
 * the real frame count. */
static long wav_open_header(FILE *f) {
    fwrite("RIFF", 1, 4, f); w_u32(f, 0); /* riff size — patched */
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); w_u32(f, 16);
    w_u16(f, 1);                          /* PCM */
    w_u16(f, 2);                          /* channels */
    w_u32(f, SR);
    w_u32(f, SR * 2u * 2u);
    w_u16(f, 4);                          /* block align */
    w_u16(f, 16);
    fwrite("data", 1, 4, f); w_u32(f, 0); /* data size — patched */
    return ftell(f);                      /* start of audio */
}
static void wav_close(FILE *f, long data_start) {
    long end = ftell(f);
    uint32_t data_bytes = (uint32_t)(end - data_start);
    fseek(f, 4, SEEK_SET);     w_u32(f, 36 + data_bytes);
    fseek(f, data_start - 4, SEEK_SET); w_u32(f, data_bytes);
    fseek(f, end, SEEK_SET);
}

/* ---- Renderer -------------------------------------------------------------
 * Pull `seconds` worth of audio from the live engine into the open WAV. */
static void render_seconds(FILE *f, float seconds) {
    int frames_left = (int)(seconds * SR + 0.5f);
    int16_t buf[BLOCK * 2];
    while (frames_left > 0) {
        int n = frames_left < BLOCK ? frames_left : BLOCK;
        engine_render(buf, n);
        fwrite(buf, sizeof(int16_t), (size_t)n * 2, f);
        frames_left -= n;
    }
}
/* True silence (skips engine entirely — for clean breaks between values). */
static void render_silence(FILE *f, float seconds) {
    int frames_left = (int)(seconds * SR + 0.5f);
    static const int16_t zero[BLOCK * 2] = {0};
    while (frames_left > 0) {
        int n = frames_left < BLOCK ? frames_left : BLOCK;
        fwrite(zero, sizeof(int16_t), (size_t)n * 2, f);
        frames_left -= n;
    }
}

/* ---- Musical phrase ------------------------------------------------------
 * Slow ambient phrase used as the baseline for every A/B. Same notes each
 * time so the parameter is the only variable. ~4.5 s end-to-end. */
static void play_phrase(FILE *f) {
    /* I — let it bloom */
    int r = brain_cell_root(0);
    engine_note_on(0, dsp_midi_to_hz((float)r), CELL_AMP);
    render_seconds(f, 1.2f);
    /* add IV underneath */
    r = brain_cell_root(3);
    engine_note_on(3, dsp_midi_to_hz((float)r), CELL_AMP);
    render_seconds(f, 1.2f);
    /* release I, V comes in */
    engine_note_off(0);
    r = brain_cell_root(4);
    engine_note_on(4, dsp_midi_to_hz((float)r), CELL_AMP);
    render_seconds(f, 1.2f);
    /* release all, let it ring into the next value */
    engine_note_off(3);
    engine_note_off(4);
    render_seconds(f, 0.9f);
}

/* Reset to a neutral baseline so each setting test starts from the same state. */
static void baseline(void) {
    engine_init();
    /* engine_init sets sensible defaults already; keep them. */
}

/* ---- Per-setting renders ------------------------------------------------- */
/* Each opens its own WAV, plays the phrase under different values of just one
 * parameter, and closes. Brief silence between values, longer at start/end. */

static void path_join(char *out, size_t n, const char *dir, const char *name) {
    snprintf(out, n, "%s/%s.wav", dir, name);
}
static FILE *open_wav(const char *dir, const char *name, long *data_start) {
    char path[1024]; path_join(path, sizeof path, dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(1); }
    *data_start = wav_open_header(f);
    return f;
}
static void close_wav(FILE *f, long data_start, const char *name) {
    wav_close(f, data_start);
    long end = ftell(f);
    fclose(f);
    fprintf(stderr, "  samples/%s.wav  %ld kB\n", name, (end + 1023) / 1024);
}

/* The phrase wants a moment for texture+drone+reverb to settle. */
static void settle_before_phrase(FILE *f) { render_silence(f, 0.4f); render_seconds(f, 0.4f); }

/* MODE — 6 modes in sequence (Ionian → Aeolian) */
static void render_mode(const char *dir) {
    long ds; FILE *f = open_wav(dir, "mode", &ds);
    baseline();
    engine_set_texture(0.2f);                  /* gentle bed */
    for (int m = 0; m < 6; ++m) {
        engine_set_mode(m);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, "mode");
}

/* VIBE — 4 vibes (chord families: Warm/Bright/Deep/Floating) */
static void render_vibe(const char *dir) {
    long ds; FILE *f = open_wav(dir, "vibe", &ds);
    baseline();
    engine_set_texture(0.2f);
    for (int v = 0; v < 4; ++v) {
        engine_set_vibe(v);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, "vibe");
}

/* VOICE — 3 pad timbres */
static void render_voice(const char *dir) {
    long ds; FILE *f = open_wav(dir, "voice", &ds);
    baseline();
    engine_set_texture(0.15f);
    for (int v = 0; v < 3; ++v) {
        engine_set_pad_voice(v);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, "voice");
}

/* KEY — three keys (C, F, A) so you hear the tonal shift without a 12-key marathon */
static void render_key(const char *dir) {
    long ds; FILE *f = open_wav(dir, "key", &ds);
    baseline();
    engine_set_texture(0.15f);
    const int KEYS[3] = { 60, 65, 69 };        /* C4, F4, A4 */
    for (int i = 0; i < 3; ++i) {
        engine_set_key(KEYS[i]);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, "key");
}

/* For the four 0..100 % macros: phrase at low / mid / high. */
static void render_continuous(const char *dir, const char *name,
                              void (*apply)(float)) {
    long ds; FILE *f = open_wav(dir, name, &ds);
    baseline();
    engine_set_texture(0.15f);
    const float LEVELS[3] = { 0.0f, 0.5f, 1.0f };
    for (int i = 0; i < 3; ++i) {
        apply(LEVELS[i]);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, name);
}

/* DRONE — phrase with drone off, then on, then off again */
static void render_drone(const char *dir) {
    long ds; FILE *f = open_wav(dir, "drone", &ds);
    baseline();
    engine_set_texture(0.15f);

    settle_before_phrase(f);
    play_phrase(f);                            /* drone off */
    render_silence(f, 0.6f);

    engine_set_drone(true);
    settle_before_phrase(f); render_seconds(f, 1.0f);  /* hear the drone alone */
    play_phrase(f);                            /* drone + cells */
    render_silence(f, 0.6f);

    engine_set_drone(false);
    render_seconds(f, 1.5f);                   /* drone fade-out */
    close_wav(f, ds, "drone");
}

/* GENERATE — off, Markov, then a fixed progression */
static void render_generate(const char *dir) {
    long ds; FILE *f = open_wav(dir, "generate", &ds);
    baseline();
    engine_set_texture(0.15f);
    engine_set_mode(0);                        /* Ionian baseline */

    /* baseline (no generate, just the phrase) */
    settle_before_phrase(f);
    play_phrase(f);
    render_silence(f, 0.6f);

    /* Markov: advance every ~2 s for 12 s */
    engine_set_generative(true, -1);
    for (int i = 0; i < 6; ++i) {
        engine_generative_advance();
        render_seconds(f, 2.0f);
    }
    render_silence(f, 0.6f);

    /* Fixed progression I-V-vi-IV (program index 1 in render_wav semantics): */
    engine_set_generative(true, 1);
    for (int i = 0; i < 6; ++i) {
        engine_generative_advance();
        render_seconds(f, 2.0f);
    }
    engine_set_generative(false, 0);
    render_seconds(f, 1.5f);                   /* tail */
    close_wav(f, ds, "generate");
}

/* VOLUME — same phrase at low / medium / loud master levels */
static void render_volume(const char *dir) {
    long ds; FILE *f = open_wav(dir, "volume", &ds);
    baseline();
    engine_set_texture(0.15f);
    const float V[3] = { 0.2f, 0.6f, 1.0f };
    for (int i = 0; i < 3; ++i) {
        engine_set_master_volume(V[i]);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, "volume");
}

/* DRIVE — reverb-stage tanh drive (the "Drive" encoder) */
static void render_drive(const char *dir) {
    long ds; FILE *f = open_wav(dir, "drive", &ds);
    baseline();
    engine_set_texture(0.15f);
    const float D[3] = { 0.0f, 0.5f, 1.0f };
    for (int i = 0; i < 3; ++i) {
        engine_set_reverb_drive(D[i]);
        settle_before_phrase(f);
        play_phrase(f);
        render_silence(f, 0.6f);
    }
    close_wav(f, ds, "drive");
}

/* ---- Main ---------------------------------------------------------------- */
static void texture_apply(float v) { engine_set_texture(v); }
static void bass_apply   (float v) { engine_set_bass_depth(v); }
static void space_apply  (float v) { engine_set_space(v); }
static void mood_apply   (float v) { engine_set_mood(v); }

int main(int argc, char **argv) {
    const char *dir = (argc > 1) ? argv[1] : "samples";
    mkdir(dir, 0755);

    dsp_init();
    brain_init();

    fprintf(stderr, "rendering samples to %s/\n", dir);
    render_mode(dir);
    render_vibe(dir);
    render_voice(dir);
    render_key(dir);
    render_continuous(dir, "texture", texture_apply);
    render_continuous(dir, "bass",    bass_apply);
    render_continuous(dir, "space",   space_apply);
    render_continuous(dir, "mood",    mood_apply);
    render_drone(dir);
    render_generate(dir);
    render_volume(dir);
    render_drive(dir);

    /* Also drop a tiny note about Brightness so nobody wonders why it isn't here. */
    char path[1024]; snprintf(path, sizeof path, "%s/README.txt", dir);
    FILE *r = fopen(path, "w");
    if (r) {
        fprintf(r,
            "Field Ambience — sample WAVs (one per setting).\n"
            "\n"
            "Each file plays the SAME short ambient phrase three (or more) times,\n"
            "varying ONLY that setting between repeats, so you can A/B by ear.\n"
            "\n"
            "Not included: Brightness. The hardware Brightness encoder (EN2) drives\n"
            "the display backlight (SPEC §5 + §12), which doesn't affect audio.\n");
        fclose(r);
    }
    fprintf(stderr, "done.\n");
    return 0;
}
