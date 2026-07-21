/*
 * Host-side tests for the Step-11 reverb + engine mix-bus.
 *
 * Pure C — no Pico SDK. We stub AUDIO_BUFFER_FRAMES via -D so engine.c
 * compiles without dragging in audio.c's Pico-SDK dependencies.
 *
 * Build via run_tests.sh, or directly:
 *   cc -std=c11 -I../include -DAUDIO_HEADER_STUB \
 *      test_reverb_engine.c ../src/dsp.c ../src/pad.c \
 *      ../src/reverb.c ../src/engine.c -lm -o /tmp/r_test
 *
 * Exit 0 = all pass.
 */

#include "dsp.h"
#include "reverb.h"
#include "engine.h"
#include "pad.h"
#include "cells.h"
#include "brain.h"
#include "worlds.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_fails;                                                     \
            fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__);           \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
        }                                                                  \
    } while (0)

/* ----------------------------------------------------------------- reverb */

#define SR  DSP_SAMPLE_RATE_HZ

static void render_silence_reverb(int frames) {
    enum { N = 256 };
    float in[N] = {0}, out[N], outR[N];
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        reverb_render(in, in, out, outR, n);
        left -= n;
    }
}

static void test_reverb_impulse_decays(void) {
    reverb_init();
    reverb_set(0.7f, 0.3f);
    reverb_set_drive(0.0f);                  /* drive off → linear */
    /* Let coefficients settle (per-block smoothing). */
    render_silence_reverb(SR);

    /* Big impulse, then 5 s of silence. Peak during the first 4 s must be
     * bounded; the residual after 5 s must be small (decay). */
    enum { N = 256 };
    float in[N], inR[N], out[N], outR[N];
    memset(in, 0, sizeof in); memset(inR, 0, sizeof inR);
    in[0] = 1.0f; inR[0] = 1.0f;
    reverb_render(in, inR, out, outR, N);

    float peak_early = 0.0f;
    for (int i = 0; i < N; ++i) {
        float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
        if (a > peak_early) peak_early = a;
    }

    /* Run 5 s of silent input, capture the tail level. */
    memset(in, 0, sizeof in); memset(inR, 0, sizeof inR);
    float tail_peak_last_second = 0.0f, anywhere_peak = peak_early;
    int total_frames = 5 * SR;
    int rendered = 0;
    while (rendered < total_frames) {
        int n = (total_frames - rendered) < N ? (total_frames - rendered) : N;
        reverb_render(in, inR, out, outR, n);
        for (int i = 0; i < n; ++i) {
            float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
            if (a > anywhere_peak) anywhere_peak = a;
            if (rendered + i >= 4 * SR && a > tail_peak_last_second)
                tail_peak_last_second = a;
        }
        rendered += n;
    }
    printf("  reverb impulse peak (any) = %.4f   tail @4-5s = %.6f\n",
           anywhere_peak, tail_peak_last_second);
    CHECK(anywhere_peak < 2.0f, "reverb diverged: peak=%g", anywhere_peak);
    CHECK(tail_peak_last_second < 0.05f,
          "reverb did not decay (4-5s peak too high): %g", tail_peak_last_second);
}

static void test_reverb_silent_in_silent_out(void) {
    reverb_init();
    enum { N = 256 };
    float in[N] = {0}, out[N], outR[N];
    /* run 2 s of zero input then check that output is exactly zero (no DC
     * drift, no self-oscillation). */
    int rendered = 0;
    float peak = 0.0f;
    while (rendered < 2 * SR) {
        int n = (2 * SR - rendered) < N ? (2 * SR - rendered) : N;
        reverb_render(in, in, out, outR, n);
        for (int i = 0; i < n; ++i) {
            float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
            if (a > peak) peak = a;
        }
        rendered += n;
    }
    CHECK(peak == 0.0f, "silent-in did not produce silent-out: peak=%g", peak);
}

static void test_reverb_bounded_under_load(void) {
    reverb_init();
    reverb_set(0.9f, 0.1f);                       /* long, bright tail */
    reverb_set_drive(0.5f);
    render_silence_reverb(SR / 4);                /* settle smoothing */

    enum { N = 256 };
    float in[N], inR[N], out[N], outR[N];
    /* 5 s of unit-amplitude white-ish noise (deterministic LCG). */
    uint32_t state = 0xdeadbeefu;
    float peak = 0.0f;
    int rendered = 0;
    while (rendered < 5 * SR) {
        for (int i = 0; i < N; ++i) {
            state = state * 1664525u + 1013904223u;
            in[i]  = ((int32_t)state) / 2147483648.0f;
            state = state * 1664525u + 1013904223u;
            inR[i] = ((int32_t)state) / 2147483648.0f;
        }
        reverb_render(in, inR, out, outR, N);
        for (int i = 0; i < N; ++i) {
            float a = fmaxf(fabsf(out[i]), fabsf(outR[i]));
            if (a > peak) peak = a;
        }
        rendered += N;
    }
    printf("  reverb peak under 5s noise input = %.3f\n", peak);
    CHECK(peak < 4.0f, "reverb saturated past sane bound: %g", peak);
}

/* ----------------------------------------------------------------- engine */

typedef struct { int pk_l, pk_r, diff_acc, nz_blocks; } eng_stats_t;

static eng_stats_t run_engine(int frames) {
    enum { N = 256 };
    int16_t buf[N * 2];
    eng_stats_t st = {0,0,0,0};
    int left = frames;
    while (left > 0) {
        int n = left < N ? left : N;
        engine_render(buf, n);
        int block_nonzero = 0;
        for (int i = 0; i < n; ++i) {
            int l = buf[i*2], r = buf[i*2+1];
            int al = abs(l), ar = abs(r);
            if (al > st.pk_l) st.pk_l = al;
            if (ar > st.pk_r) st.pk_r = ar;
            st.diff_acc += abs(l - r);
            if (al || ar) block_nonzero = 1;
        }
        if (block_nonzero) ++st.nz_blocks;
        left -= n;
    }
    return st;
}

static void test_engine_silent_then_note(void) {
    engine_init();
    engine_all_off();
    run_engine(SR * 6);                          /* drain any tail */

    /* "Silent" boot — since ADR-0017 Phase 3, the engine is always running
     * a tiny tape-hiss layer (~−46 dBFS, "colour not body") that the user
     * approved as part of the dreamy/warm reference. Strict pk==0 no longer
     * holds; allow up to ±300 (hiss + DC-blocker settling). Note hits stay
     * an order of magnitude above this floor, so the assertion still proves
     * the engine isn't accidentally producing audible signal at idle. */
    eng_stats_t silent = run_engine(SR);
    CHECK(silent.pk_l < 300 && silent.pk_r < 300,
          "engine idle leaking above noise floor: L=%d R=%d", silent.pk_l, silent.pk_r);

    /* Tap a cell; output rises through the pad attack + reverb buildup. */
    engine_note_on(0, 220.0f, 0.5f);
    eng_stats_t hit = run_engine((int)(2.5f * SR));
    CHECK(hit.pk_l > 1000 && hit.pk_r > 1000,
          "engine note did not produce audible output: L=%d R=%d",
          hit.pk_l, hit.pk_r);
    CHECK(hit.pk_l <= 32767 && hit.pk_r <= 32767, "engine clipped int16");
    CHECK(hit.diff_acc > 0, "engine output is mono (L==R)");
    printf("  engine peak after 2.5s note  L=%d  R=%d\n", hit.pk_l, hit.pk_r);
}

static void test_engine_reverb_tail_outlives_dry(void) {
    /* With a healthy wet-send, releasing the note should leave audible
     * reverb tail for several seconds — the whole point of Step 11.
     * r19.41: the master hall lives in the effects engine now — Space and
     * Atmosphere are the wet controls (the legacy wet_amp/reverb_size knobs
     * only configure the V2 tank). Same intent, new knobs. */
    engine_init();
    engine_set_send(0.8f); engine_set_wet_amp(0.7f);
    engine_set_space(0.85f);
    engine_set_atmosphere(0.80f);
    engine_all_off();
    run_engine(SR * 6);

    engine_note_on(1, 440.0f, 0.5f);
    run_engine((int)(2.5f * SR));                /* let it bloom */
    engine_note_off(1);

    /* Wait for the dry pad release to mostly finish (~3 s), then measure
     * the reverb tail energy. */
    run_engine((int)(3.5f * SR));
    eng_stats_t tail = run_engine(SR);            /* 1 s window for tail level */
    CHECK(tail.pk_l > 200 || tail.pk_r > 200,
          "reverb tail too quiet after dry release: L=%d R=%d",
          tail.pk_l, tail.pk_r);
    printf("  reverb tail 3.5-4.5s after release  L=%d  R=%d\n", tail.pk_l, tail.pk_r);

    /* And it does decay: 15 s later the level must have dropped by ≥6 dB
     * from the tail window. r19.41: an ABSOLUTE silence check is no longer
     * possible here — Atmosphere now also drives the per-world ambience
     * layer (wind/waves), a constant bed that never decays by design
     * (measured floor ~680 LSB at this setting). The relative check still
     * catches a runaway / never-decaying hall (end ≈ tail) while
     * tolerating the bed. */
    run_engine((int)(15.0f * SR));
    eng_stats_t end = run_engine(SR / 2);
    CHECK(end.pk_l < tail.pk_l / 2 && end.pk_r < tail.pk_r / 2,
          "reverb tail never decays: tail L=%d R=%d → end L=%d R=%d",
          tail.pk_l, tail.pk_r, end.pk_l, end.pk_r);
}

static void test_engine_overload_holds_int16(void) {
    engine_init();
    engine_set_send(0.9f); engine_set_wet_amp(0.9f);
    engine_all_off();
    run_engine(SR * 6);

    for (int s = 0; s < PAD_MAX; ++s)
        engine_note_on((uint8_t)s, dsp_midi_to_hz(48.0f + s * 2), 1.0f);
    eng_stats_t st = run_engine(SR * 2);
    CHECK(st.pk_l <= 32767 && st.pk_r <= 32767,
          "engine wrapped int16 under overload: L=%d R=%d", st.pk_l, st.pk_r);
    CHECK(st.pk_l > 20000, "engine seems silent under overload: L=%d", st.pk_l);
    printf("  engine peak under full load  L=%d  R=%d\n", st.pk_l, st.pk_r);
}

/* Step 12b #5 — live parameter changes (the "sound darf nicht konkurrieren"
 * rule) exercised at the engine level. With a cell held, slamming every
 * global control (key/mode/vibe/space/mood/pad-voice/drone/generative) must:
 *   - never kill or restart the held voice (it keeps sounding),
 *   - never click or blow past int16,
 *   - keep the engine producing continuous audio.
 * The held-note-freezes property is structural: the engine stores each
 * source's pitch at note_on and never re-pitches a sounding voice, so a key
 * change only affects notes triggered afterwards. */
static void test_engine_live_params_no_glitch(void) {
    engine_init();
    engine_all_off();
    run_engine(SR * 6);

    /* Hold a cell. */
    engine_note_on(0, 220.0f, 0.5f);
    run_engine((int)(2.0f * SR));                 /* reach sustain */
    int voices_before = engine_active_voices();
    CHECK(voices_before >= 1, "held voice not active before param storm");

    /* Slam every global control while the cell is still held. */
    engine_set_key(62);
    engine_set_mode(5);          /* aeolian */
    engine_set_vibe(2);          /* deep */
    engine_set_space(0.9f);
    engine_set_mood(0.15f);
    engine_set_pad_voice(2);     /* brass */
    engine_set_drone(true);
    engine_set_generative(true, -1);

    /* Render across the change: bounded, click-free, still alive. */
    eng_stats_t storm = run_engine((int)(3.0f * SR));
    CHECK(storm.pk_l <= 32767 && storm.pk_r <= 32767, "param storm wrapped int16");
    CHECK(storm.pk_l > 500, "engine went silent through param storm: L=%d", storm.pk_l);
    CHECK(engine_active_voices() >= 1, "held voice was killed by a global change");
    printf("  live-param storm peak  L=%d  R=%d  voices=%d\n",
           storm.pk_l, storm.pk_r, engine_active_voices());

    /* Advance the generative bed a few bars — must stay bounded. */
    for (int i = 0; i < 6; ++i) {
        int deg = engine_generative_advance();
        CHECK(deg == -1 || (deg >= 1 && deg <= 7), "generative degree out of range: %d", deg);
        eng_stats_t g = run_engine(SR);
        CHECK(g.pk_l <= 32767 && g.pk_r <= 32767, "generative bar wrapped int16");
    }

    /* Note: a cell IS held, so the generative bed must NO-OP (holds override).
     * Lift the cell, then it should start advancing. */
    engine_note_off(0);
    run_engine((int)(4.0f * SR));                 /* let the held voice release */
    int deg = engine_generative_advance();
    CHECK(deg >= 1 && deg <= 7, "generative did not run after cells released: %d", deg);
    eng_stats_t bed = run_engine((int)(2.0f * SR));
    CHECK(bed.pk_l <= 32767 && bed.pk_r <= 32767, "generative bed wrapped int16");
    printf("  generative bed after release: degree=%d peak L=%d\n", deg, bed.pk_l);
}

/* Master stage regression (fixes the listening-test earrape/brummt):
 * 1. the output must be DC-free (mean ≈ 0) even with the bass + brown-noise
 *    texture running — the one-pole DC blocker guarantees this.
 * 2. master volume must scale the level (0 → silent, 1 → louder than 0.3).
 * 3. output stays int16-bounded. */
static void test_engine_master_clean(void) {
    enum { N = 256 };
    int16_t buf[N * 2];
    engine_init();
    engine_set_texture(0.2f);                 /* the noise bed that builds rumble */
    engine_note_on(0, 130.81f, 0.5f);         /* C3 → bass goes low */
    engine_note_on(2, 196.0f, 0.5f);

    /* settle ~3 s */
    for (int i = 0; i < 3 * SR / N; ++i) engine_render(buf, N);

    /* Measure DC (mean) + peak over 2 s. */
    double sum = 0; long cnt = 0; int peak = 0;
    for (int i = 0; i < 2 * SR / N; ++i) {
        engine_render(buf, N);
        for (int k = 0; k < N; ++k) {
            sum += buf[k*2]; ++cnt;
            int a = abs(buf[k*2]); if (a > peak) peak = a;
        }
    }
    double mean = sum / (double)cnt;
    printf("  master: DC mean=%.1f LSB  peak=%d\n", mean, peak);
    CHECK(fabs(mean) < 60.0, "master output has DC offset: mean=%.1f LSB", mean);
    CHECK(peak <= 32767, "master clipped int16");
    CHECK(peak > 500, "master unexpectedly silent: peak=%d", peak);

    /* Volume scaling: 0 → silence, 1 → louder. */
    engine_set_master_volume(0.0f);
    for (int i = 0; i < SR / N; ++i) engine_render(buf, N);   /* smooth down */
    int pk0 = 0;
    for (int i = 0; i < SR / N; ++i) { engine_render(buf, N);
        for (int k = 0; k < N; ++k) { int a = abs(buf[k*2]); if (a > pk0) pk0 = a; } }
    CHECK(pk0 < 50, "master volume 0 not silent: peak=%d", pk0);

    engine_set_master_volume(1.0f);
    for (int i = 0; i < SR / N; ++i) engine_render(buf, N);   /* smooth up */
    int pk1 = 0;
    for (int i = 0; i < SR / N; ++i) { engine_render(buf, N);
        for (int k = 0; k < N; ++k) { int a = abs(buf[k*2]); if (a > pk1) pk1 = a; } }
    CHECK(pk1 > pk0, "master volume 1 not louder than 0: %d vs %d", pk1, pk0);
    printf("  master volume scaling: vol0 peak=%d  vol1 peak=%d\n", pk0, pk1);
}

/* ADR-0013 — a fast Hall press must end up louder than a slow one, end to end
 * through the engine (not just in the cells unit). Ramps a normalised position
 * across the velocity band in `band_ms`, then renders ~0.6 s and returns the
 * peak. */
static int press_and_peak(uint8_t cell, float band_ms) {
    static uint32_t ms = 100000;
    engine_all_off();
    run_engine(SR * 4);                               /* drain previous tail */
    const float band = CELL_VEL_BAND_HI - CELL_VEL_BAND_LO;
    const float step = band / band_ms;                /* per 1 ms sample */
    for (float pos = 0.0f; pos < 1.0f; pos += step)
        engine_cell_sample(cell, pos, ++ms);
    engine_cell_sample(cell, 1.0f, ++ms);
    eng_stats_t st = run_engine((int)(0.6f * SR));
    int pk = st.pk_l > st.pk_r ? st.pk_l : st.pk_r;
    /* release so the next call starts clean */
    for (float pos = 1.0f; pos > 0.0f; pos -= 0.1f)
        engine_cell_sample(cell, pos, ++ms);
    return pk;
}

static void test_engine_cell_velocity(void) {
    engine_init();
    int soft = press_and_peak(0, 60.0f);   /* slow press */
    int loud = press_and_peak(0, 5.0f);    /* fast press */
    printf("  cell velocity: slow press peak=%d  fast press peak=%d\n", soft, loud);
    CHECK(soft > 0, "slow cell press produced audio: %d", soft);
    CHECK(loud > soft, "fast press louder than slow (loud=%d soft=%d)", loud, soft);
    CHECK(loud <= 32767, "velocity press clipped int16");
}

/* Per-world musical identity: selecting a world must push its key/mode/vibe
 * into the harmonic brain so cell taps actually play in that world's key. */
static void test_engine_world_changes_key(void) {
    engine_init();
    /* boot world 0 (Tokyo) — brain should already be A-ionian-warm */
    CHECK(brain_get_key()  == worlds_get(0)->key_midi, "boot key != Tokyo (%d)", brain_get_key());
    CHECK(brain_get_mode() == worlds_get(0)->mode,     "boot mode != Tokyo");
    CHECK(brain_get_vibe() == worlds_get(0)->vibe,     "boot vibe != Tokyo");

    int prev_root = brain_cell_root(0);
    int distinct_roots = 0, last_root = prev_root;

    for (int wi = 0; wi < worlds_count(); ++wi) {
        engine_set_world(wi);
        const world_t *w = worlds_get(wi);
        CHECK(brain_get_key()  == w->key_midi, "world %d key not applied (got %d want %d)",
              wi, brain_get_key(), w->key_midi);
        CHECK(brain_get_mode() == w->mode, "world %d mode not applied", wi);
        CHECK(brain_get_vibe() == w->vibe, "world %d vibe not applied", wi);
        int r = brain_cell_root(0);
        if (r != last_root) ++distinct_roots;
        last_root = r;
        printf("  world %d (%s): cell0 root MIDI=%d  key=%d mode=%d vibe=%d\n",
               wi, w->name, r, brain_get_key(), brain_get_mode(), brain_get_vibe());
    }
    /* At least 3 of the 4 worlds should voice cell 0 to a different root —
     * they're in different keys, so the harmony genuinely differs. */
    CHECK(distinct_roots >= 2, "worlds barely differ harmonically (distinct=%d)", distinct_roots);

    /* And after touching all worlds, going back to Tokyo restores its key. */
    engine_set_world(0);
    CHECK(brain_get_key() == worlds_get(0)->key_midi, "return-to-Tokyo key not restored");
}

int main(void) {
    dsp_init();

    printf("== reverb ==\n");
    test_reverb_impulse_decays();
    test_reverb_silent_in_silent_out();
    test_reverb_bounded_under_load();

    printf("== engine ==\n");
    test_engine_silent_then_note();
    test_engine_reverb_tail_outlives_dry();
    test_engine_overload_holds_int16();
    test_engine_live_params_no_glitch();
    test_engine_master_clean();
    test_engine_cell_velocity();
    test_engine_world_changes_key();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
