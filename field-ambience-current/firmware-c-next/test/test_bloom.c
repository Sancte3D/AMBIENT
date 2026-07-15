/*
 * Host test for r19.23 — Chord Bloom cell mode (bloom.c):
 *   - a press schedules staggered onsets (first immediate, rest later)
 *   - after enough ticks the whole chord is sounding on the 0..4 pool
 *   - voice-leading pulls the new chord's centroid within a semitone-octave
 *     of the previous one (shortest path)
 *   - HOLD latches the chord past release; no-hold releases it
 *   - mono-chord: a new press replaces, never stacks past the pool
 *   - CLEAR/all-off silences and resets
 *   - the generator yields while a bloom key is physically down
 */
#include <stdio.h>
#include <stdlib.h>
#include "bloom.h"
#include "engine.h"
#include "brain.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Tick the bloom scheduler across `ms` in small steps (like the main loop). */
static void run_ms(uint32_t *now, uint32_t ms) {
    for (uint32_t t = 0; t < ms; t += 8) { *now += 8; bloom_tick(*now); }
}

int main(void) {
    printf("== chord bloom (r19.23) ==\n");
    dsp_init(); brain_init(); engine_init(); bloom_init();
    uint32_t now = 10000;

    /* ---- 1. press schedules onsets; not all sound instantly ---- */
    bloom_press(0, 0.6f, false, now);
    bloom_tick(now);                                  /* first note fires now */
    int v_immediate = engine_active_voices();
    CHECK(v_immediate >= 1, "first chord tone sounds immediately (%d)", v_immediate);
    CHECK(bloom_pending() >= 1, "later tones still pending (%d)", bloom_pending());

    /* ---- 2. after the full bloom, the chord is voiced on the pool ---- */
    run_ms(&now, 900);
    CHECK(bloom_pending() == 0, "all onsets fired (%d pending)", bloom_pending());
    int v_full = engine_active_voices();
    CHECK(v_full >= 3, "full chord has >=3 voices (%d)", v_full);
    CHECK(v_full <= 5, "chord capped at the 5-voice pool (%d)", v_full);

    /* ---- 3. voice-leading: new chord centroid near the previous one ---- */
    int c_a = bloom_centroid();
    bloom_press(3, 0.6f, false, now);                 /* different degree */
    run_ms(&now, 900);
    int c_b = bloom_centroid();
    CHECK(abs(c_b - c_a) <= 6,
          "voice-leading keeps the register (|%d-%d|=%d <= 6)",
          c_b, c_a, abs(c_b - c_a));

    /* ---- 4. mono-chord: still within the pool, active cell tracks ---- */
    CHECK(bloom_active_cell() == 3, "active cell follows the last press");
    CHECK(engine_active_voices() <= 5, "no stacking past the pool (%d)",
          engine_active_voices());

    /* ---- 5. no-hold release silences the chord ---- */
    bloom_release(3, now);
    run_ms(&now, 200);
    /* engine voices go to 0 as the release tails decay; check the pool was
     * released (active cell cleared) rather than waiting the full reverb. */
    CHECK(bloom_active_cell() == -1, "no-hold release cleared the chord");

    /* ---- 6. HOLD latches past release ---- */
    bloom_init(); engine_init();
    now = 20000;
    bloom_press(1, 0.6f, true, now);                  /* HOLD latched */
    run_ms(&now, 900);
    int held = engine_active_voices();
    CHECK(held >= 3, "held chord sounds (%d)", held);
    bloom_release(1, now);                            /* finger up */
    run_ms(&now, 300);
    CHECK(bloom_active_cell() == 1, "HOLD keeps the chord active after release");
    CHECK(engine_active_voices() >= 3, "held chord still sounds (%d)",
          engine_active_voices());

    /* ---- 7. all-off resets ---- */
    bloom_all_off();
    CHECK(bloom_active_cell() == -1, "all_off clears active cell");
    CHECK(bloom_pending() == 0,      "all_off clears pending");

    /* ---- 8. generator yields while a bloom key is down ---- */
    bloom_init(); engine_init();
    now = 30000;
    engine_set_generative(true, -1);
    bloom_press(2, 0.6f, false, now);                 /* key DOWN → presence */
    run_ms(&now, 100);
    CHECK(engine_generative_advance() == -1,
          "generator yields while a bloom key is held");
    bloom_release(2, now);                            /* key UP → presence off */

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
