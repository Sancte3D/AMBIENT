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
#include "horn.h"    /* default Alps World uses its curated horn */
#include "worlds.h"
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

static uint32_t phrase_clock, phrase_started, phrase_ended;
static int phrase_world, phrase_pairs, phrase_live, phrase_has_end;
static void phrase_event(int on, uint8_t source, float hz, float amp) {
    (void)hz; (void)amp;
    if (source != 15) return;
    const world_phrase_t *p = worlds_phrase(phrase_world);
    if (on == 1) {
        if (phrase_has_end) {
            /* OPEN can shorten the base rest by 20%; other states and
             * phrase boundaries only add space. Tick quantization: 100 ms. */
            CHECK(phrase_clock - phrase_ended + 100 >= p->rest_min * 800u,
                  "world %d has space between melodic holds", phrase_world);
        }
        phrase_started = phrase_clock; phrase_live = 1;
    } else if (on == 0 && phrase_live) {
        uint32_t hold = phrase_clock - phrase_started;
        CHECK(hold >= p->note_min * 1000u && hold <= p->note_max * 1000u + 100u,
              "world %d melodic hold %u ms matches its phrase", phrase_world, hold);
        ++phrase_pairs; phrase_live = 0;
        phrase_ended = phrase_clock; phrase_has_end = 1;
    }
}

static void test_world_phrases(void) {
    for (phrase_world = 0; phrase_world < WORLD_COUNT; ++phrase_world) {
        engine_init(); engine_set_world(phrase_world);
        engine_set_gen_seed(0x5EEDBA55u);
        engine_set_generative(true, -1);
        phrase_live = phrase_has_end = phrase_pairs = 0;
        engine_set_note_hook(phrase_event);
        /* This is a scheduler audit, not a long audio export. */
        for (phrase_clock = 1000; phrase_clock < 901000; phrase_clock += 100) {
            engine_generative_tick(phrase_clock);
            if (phrase_clock % 1000 == 0) render_ms(20);
        }
        engine_set_note_hook(NULL); /* explicit stop may shorten a hold */
        engine_set_generative(false, -1);
        CHECK(phrase_pairs >= 5, "world %d completed enough phrases (%d)",
              phrase_world, phrase_pairs);
        printf("  World %d: %d completed melodic holds audited\n", phrase_world, phrase_pairs);
    }
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
        if (horn_active_count() > 0) sparkle_seen = 1;   /* r18.89: plucks */
        if ((step & 63) == 0) {
            int p = render_ms(16 * 64);
            CHECK(p <= 32767, "bounded");
        }
        if (fails > 10) break;                  /* don't spam */
    }
    CHECK(sparkle_seen, "Alps melody voice actually played");
    /* r18.99: the bed is a CHOIR now — the three Eno loops join one by
     * one (staggered entries at 0.40/0.62/0.81 of their periods), so the
     * pad pool grows past the single bed voice but never past bed + 3. */
    CHECK(max_voices >= 2, "Eno loops actually joined the bed (%d)", max_voices);
    CHECK(max_voices <= 5, "pad pool = bed + 3 Eno loops + melody max (%d)", max_voices);

    /* ---- 3. User override — r19.20: the gate is PHYSICAL key presence
     * (controls.c edges), not "any active voice". The device path calls
     * engine_set_user_presence(true) on key-down; simulate exactly that. */
    engine_note_on(11, dsp_midi_to_hz(76.0f), 0.12f);   /* shift source 9..13 */
    engine_set_user_presence(true);                     /* finger is DOWN */
    CHECK(engine_generative_advance() == -1,
          "advance() blocked while a key is physically held");
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
    CHECK(voices_with_user <= 6, "no new gen notes while user holds (max %d)",
          voices_with_user);
    render_ms(10000);                                   /* drain sparkle tails */
    /* r18.99: user + bed + up to 3 sustaining Eno loops (loops freeze while
     * the user plays — they neither retrigger nor release under a hold). */
    CHECK(engine_active_voices() >= 2 && engine_active_voices() <= 5,
          "user note + bed choir remain (have %d)", engine_active_voices());
    CHECK(horn_active_count() == 0, "Alps voice self-decayed under the user (%d)",
          horn_active_count());

    /* ---- 4. Release → bed movement resumes promptly ----
     * Drain the user note's release tail FIRST (no ticks), so the resume
     * check below can only be satisfied by NEW sparkle notes, not by the
     * old note still fading out. */
    engine_note_off(11);
    engine_set_user_presence(false);                    /* finger lifted */
    render_ms(10000);
    CHECK(engine_active_voices() >= 1 && engine_active_voices() <= 4,
          "only the bed choir left after the user tail (%d)",
          engine_active_voices());
    int resumed = 0;                                    /* sparkles restart? */
    for (int step = 0; step < 9500; ++step) {           /* 152 s simulated —
        * r18.96: the composer may sit in EMPTY (rests 75 %) right here;
        * the window must span a state change so the resume is observable */
        now += 16;
        engine_generative_tick(now);
        if (horn_active_count() > 0) { resumed = 1; break; }
        if ((step & 127) == 0) render_ms(16 * 128);
    }
    CHECK(resumed, "autoplay (sparkles) resumes after the user lets go");

    /* ---- 4b. Low-level injected-note regression (physical cells are now
     * locked during Generate). Former r19.20 HOLD+GENERATE regression: a LATCHED voice (note on,
     * finger UP → no presence) must NOT freeze the composer. Before r19.20
     * the gate was any_user_note(): one latched cell paused autoplay
     * forever. Now the generator keeps composing around the latch. */
    engine_note_on(2, dsp_midi_to_hz(64.0f), 0.12f);    /* latched: no presence */
    int gen_moved = 0;
    for (int step = 0; step < 9500; ++step) {
        now += 16;
        engine_generative_tick(now);
        if (horn_active_count() > 0) { gen_moved = 1; break; }
        if ((step & 127) == 0) render_ms(16 * 128);
    }
    CHECK(gen_moved, "generator keeps playing around a latched voice (r19.20)");
    engine_note_off(2);
    render_ms(6000);

    /* ---- 5. Disable releases everything ---- */
    engine_set_generative(false, -1);
    render_ms(12000);
    CHECK(engine_active_voices() == 0, "all gen voices gone after disable (%d)",
          engine_active_voices());
    CHECK(horn_active_count() == 0, "Alps voice rang out after disable (%d)",
          horn_active_count());

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

    /* r19.33 — player-priority "listening": the generator holds off while a
     * key is down AND for ~8 s after the last release, then returns. */
    {
        engine_init();
        engine_set_generative(true, -1);
        uint32_t t = 500000;
        engine_generative_tick(t);                     /* running */
        CHECK(!engine_generative_suppressed(), "not suppressed with no player");

        engine_set_user_presence(true);                /* player presses */
        t += 250; engine_generative_tick(t);
        CHECK(engine_generative_suppressed(), "suppressed while a key is down");
        CHECK(engine_generative_advance() == -1, "no bed advance while playing");

        engine_set_user_presence(false);               /* player lifts */
        t += 250; engine_generative_tick(t);
        CHECK(engine_generative_suppressed(), "still held off right after release");

        t += 4000; engine_generative_tick(t);          /* 4 s later — inside hold-off */
        CHECK(engine_generative_suppressed(), "held off 4 s after release (return delay)");

        t += 5000; engine_generative_tick(t);          /* ~9 s after release */
        CHECK(!engine_generative_suppressed(), "returns after the hold-off elapses");
        engine_set_generative(false, -1);
    }

    test_world_phrases();
    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
