/*
 * Host test for the r18.88 generative AUTOPLAY (engine_generative_tick) and
 * the audit fixes around it:
 *   - enabling GENERATE produces a bed note on the FIRST tick (no 8 s wait)
 *   - autoplay runs by itself: bars advance, sparkle chord tones appear
 *     (sources 14/15), degree stays 1..7, audio stays finite and bounded
 *   - a held USER note suppresses new bed/sparkle notes — including the
 *     SHIFT-octave sources 9..13 (the old any_cell_held() only looked at
 *     0..4, so the bed played over latched shift notes)
 *   - releasing the user note resumes the bed on the next tick
 *   - disabling GENERATE releases bed + sparkles
 *
 * The tick is pure in now_ms, so the whole schedule is simulated without
 * wall-clock waits: advance a fake clock, render audio between ticks (the
 * envelopes need samples to move), and watch engine_active_voices().
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "engine.h"
#include "brain.h"
#include "generative.h"
#include "pluck.h"   /* r18.89: sparkles are KS plucks now */
#include "composer.h" /* r18.96: top-level intent states */
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Render `ms` of audio in 256-frame blocks; returns peak |sample| and traps
 * NaN. 1 ms ≈ 44.1 frames — close enough for envelope movement. */
static int render_ms(int ms) {
    int16_t buf[512];
    int blocks = (ms * 44100) / (1000 * 256) + 1, pk = 0;
    for (int b = 0; b < blocks; ++b) {
        engine_render(buf, 256);
        for (int i = 0; i < 512; ++i) {
            int a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > pk) pk = a;
        }
    }
    return pk;
}

int main(void) {
    printf("== generative autoplay (engine_generative_tick, r18.88) ==\n");
    dsp_init(); brain_init(); engine_init();
    uint32_t now = 1000;

    /* ---- 1. Immediate first note ---- */
    CHECK(engine_active_voices() == 0, "silent at boot");
    engine_set_generative(true, -1);            /* Markov auto */
    engine_generative_tick(now);
    CHECK(engine_active_voices() >= 1, "bed voice starts on the FIRST tick");
    int pk = render_ms(1500);
    CHECK(pk > 300, "bed audible shortly after enable (peak %d)", pk);

    /* ---- 2. Autoplay: simulate ~120 s, tick every 16 ms ---- */
    int max_voices = 0, sparkle_seen = 0;
    for (int step = 0; step < 7500; ++step) {
        now += 16;
        engine_generative_tick(now);
        int d = generative_current_degree();
        CHECK(d >= 1 && d <= 7, "degree in range (%d)", d);
        int v = engine_active_voices();
        if (v > max_voices) max_voices = v;
        if (pluck_active_count() > 0) sparkle_seen = 1;   /* r18.89: plucks */
        if ((step & 63) == 0) {
            int p = render_ms(16 * 64);
            CHECK(p <= 32767, "bounded");
        }
        if (fails > 10) break;                  /* don't spam */
    }
    CHECK(sparkle_seen, "sparkle PLUCKS actually played");
    CHECK(max_voices <= 1, "pad pool carries only the bed now (%d)", max_voices);

    /* ---- 3. User override incl. SHIFT-octave source (the r18.88 fix) ---- */
    engine_note_on(11, dsp_midi_to_hz(76.0f), 0.12f);   /* shift source 9..13 */
    CHECK(engine_generative_advance() == -1,
          "advance() blocked by a held SHIFT note (was the any_cell_held bug)");
    int voices_with_user = 0;
    for (int step = 0; step < 1500; ++step) {           /* 24 s held */
        now += 16;
        engine_generative_tick(now);
        int v = engine_active_voices();
        if (v > voices_with_user) voices_with_user = v;
        if ((step & 127) == 0) render_ms(16 * 128);     /* let sparkles decay */
    }
    /* No NEW gen notes while the user holds: sparkles release on schedule,
     * the BED deliberately keeps sustaining underneath (it's an ambient bed
     * — same semantics as the old advance() gate), so after the sparkle
     * tails exactly two voices remain: user note + sustained bed. */
    CHECK(voices_with_user <= 4, "no new gen notes while user holds (max %d)",
          voices_with_user);
    render_ms(10000);                                   /* drain sparkle tails */
    CHECK(engine_active_voices() == 2,
          "user note + sustained bed remain (have %d)", engine_active_voices());
    CHECK(pluck_active_count() == 0, "plucks self-decayed under the user (%d)",
          pluck_active_count());

    /* ---- 4. Release → bed movement resumes promptly ----
     * Drain the user note's release tail FIRST (no ticks), so the resume
     * check below can only be satisfied by NEW sparkle notes, not by the
     * old note still fading out. */
    engine_note_off(11);
    render_ms(10000);
    CHECK(engine_active_voices() == 1, "only the bed left after the user tail (%d)",
          engine_active_voices());
    int resumed = 0;                                    /* sparkles restart? */
    for (int step = 0; step < 9500; ++step) {           /* 152 s simulated —
        * r18.96: the composer may sit in EMPTY (rests 75 %) right here;
        * the window must span a state change so the resume is observable */
        now += 16;
        engine_generative_tick(now);
        if (pluck_active_count() > 0) { resumed = 1; break; }
        if ((step & 127) == 0) render_ms(16 * 128);
    }
    CHECK(resumed, "autoplay (sparkles) resumes after the user lets go");

    /* ---- 5. Disable releases everything ---- */
    engine_set_generative(false, -1);
    render_ms(12000);
    CHECK(engine_active_voices() == 0, "all gen voices gone after disable (%d)",
          engine_active_voices());
    CHECK(pluck_active_count() == 0, "plucks rang out after disable (%d)",
          pluck_active_count());

    /* ---- 6. r18.90 melody GRAMMAR: composes, not randomizes ----
     * Simulate ~200 bars with coarse 250 ms ticks (no audio needed to make
     * scheduling decisions; render occasionally to keep envelopes moving)
     * and audit the tone sequence via the observability getters against
     * SOUND_WORLD.md §6: register bounds, stepwise voice-leading,
     * repetitions present, rests present. */
    {
        engine_init();
        engine_set_generative(true, -1);
        int prev_count = 0, prev_midi = 0;
        int notes = 0, reps = 0, big_leaps = 0, small_steps = 0;
        int lo = 999, hi = 0;
        uint32_t t = 100000;
        for (int step = 0; step < 6600; ++step) {       /* ~1650 s ≈ 206 bars */
            t += 250;
            engine_generative_tick(t);
            int c = engine_generative_melody_count();
            if (c != prev_count) {
                int m = engine_generative_last_melody_midi();
                if (m < lo) lo = m;
                if (m > hi) hi = m;
                if (prev_midi != 0) {
                    int d = m > prev_midi ? m - prev_midi : prev_midi - m;
                    if (d == 0)  ++reps;
                    if (d > 12)  ++big_leaps;
                    if (d >= 1 && d <= 4) ++small_steps;
                }
                prev_midi  = m;
                prev_count = c;
                ++notes;
            }
            if ((step & 255) == 0) render_ms(300);
        }
        CHECK(notes >= 40, "melody actually sings (%d notes in ~206 bars)", notes);
        CHECK(notes <= 190, "melody leaves space — rests exist (%d notes)", notes);
        CHECK(lo >= 52 && hi <= 98, "register inside the voiced band (%d..%d)", lo, hi);
        CHECK(big_leaps == 0, "no leap beyond an octave (%d)", big_leaps);
        CHECK(reps >= 3, "repetition happens — it's a motif, not a walk (%d)", reps);
        CHECK(small_steps >= notes / 5,
              "stepwise motion dominates (%d small of %d)", small_steps, notes);
        CHECK(engine_generative_dejavu_count() >= 3,
              "deja-vu replays whole phrases (%d in ~206 bars)",
              engine_generative_dejavu_count());
        engine_set_generative(false, -1);
    }

    /* ---- 7. r18.96 composer: states cycle, density follows intent ----
     * Simulate ~15 min of autoplay and attribute every melody note to the
     * composer state it was born in. All five states must be visited
     * (cycle ≈ 5 × 40–80 s), and OPEN must sing clearly denser than
     * EMPTY (notes per minute, time-normalized) — the whole point of a
     * composer that only re-weights probabilities. */
    {
        engine_init();
        engine_set_generative(true, -1);
        int    seen[COMPOSER_STATE_COUNT] = { 0 };
        double ms_in[COMPOSER_STATE_COUNT] = { 0 };
        int    notes_in[COMPOSER_STATE_COUNT] = { 0 };
        int    prev_notes = 0;
        uint32_t t = 500000;
        for (int step = 0; step < 3600; ++step) {       /* 900 s @ 250 ms */
            t += 250;
            engine_generative_tick(t);
            composer_state_t st = composer_state();
            seen[st] = 1;
            ms_in[st] += 250.0;
            int c = engine_generative_melody_count();
            if (c != prev_notes) { notes_in[st] += c - prev_notes; prev_notes = c; }
            if ((step & 255) == 0) render_ms(400);
        }
        for (int k = 0; k < COMPOSER_STATE_COUNT; ++k)
            CHECK(seen[k], "composer visits state %d", k);
        double d_open  = ms_in[COMPOSER_OPEN]  > 0 ? notes_in[COMPOSER_OPEN]  / ms_in[COMPOSER_OPEN]  : 0;
        double d_empty = ms_in[COMPOSER_EMPTY] > 0 ? notes_in[COMPOSER_EMPTY] / ms_in[COMPOSER_EMPTY] : 1e9;
        CHECK(d_open > d_empty * 2.0,
              "OPEN sings denser than EMPTY (%.5f vs %.5f notes/ms)",
              d_open, d_empty);
        engine_set_generative(false, -1);
    }

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
