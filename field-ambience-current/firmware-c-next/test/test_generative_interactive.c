/*
 * Host test for r19.24 — interactive GENERATE (engine_generative_nudge +
 * engine_generative_new_field) and the composer/harmony reseed hooks:
 *   - nudge is a no-op while GENERATE is off
 *   - each cell steers the composer to its mapped intent state
 *   - a nudge does NOT freeze the generator (it keeps advancing) and moves
 *     the harmony (state change count rises)
 *   - New Field reseeds → a different-but-reproducible evolution, same key
 */
#include <stdio.h>
#include "engine.h"
#include "composer.h"
#include "harmony.h"
#include "brain.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Capture the harmony bass sequence over N forced advances. */
static void field_seq(int *out, int n) {
    for (int i = 0; i < n; ++i) { harmony_advance(); out[i] = harmony_bass_midi(); }
}

int main(void) {
    printf("== interactive GENERATE (r19.24) ==\n");
    dsp_init(); brain_init(); engine_init();
    uint32_t now = 10000;

    /* ---- 1. nudge is a no-op while GENERATE is off ---- */
    composer_init();
    composer_state_t s0 = composer_state();
    engine_generative_nudge(1, now);     /* generate is OFF */
    CHECK(composer_state() == s0, "nudge ignored while GENERATE off");

    /* ---- 2. each cell steers the composer to its mapped intent ---- */
    engine_set_generative(true, -1);
    static const composer_state_t WANT[5] = {
        COMPOSER_RETURN, COMPOSER_OPEN, COMPOSER_DEEP, COMPOSER_CALM, COMPOSER_EMPTY
    };
    static const char *nm[5] = { "Home", "Lift", "Dark", "Open", "Tension" };
    for (int c = 0; c < 5; ++c) {
        now += 1000;
        engine_generative_nudge(c, now);
        CHECK(composer_state() == WANT[c],
              "cell %d (%s) steers to state %d (got %d)",
              c, nm[c], WANT[c], composer_state());
    }

    /* ---- 3. nudge does NOT freeze the generator + moves the harmony ---- */
    int changes_before = harmony_state_changes();
    now += 1000;
    engine_generative_nudge(0, now);
    CHECK(harmony_state_changes() > changes_before,
          "nudge mutated the harmony (%d -> %d)",
          changes_before, harmony_state_changes());
    /* the generator must still be willing to advance (no user-presence set) */
    CHECK(engine_generative_advance() != -1,
          "generator keeps playing after a nudge (not frozen)");

    /* ---- 4. New Field: reproducible per seed, different across seeds ---- */
    int key0 = brain_get_key();

    engine_generative_new_field(0xAAAA1111u);
    int a1[8]; field_seq(a1, 8);
    int keyA = brain_get_key();

    engine_generative_new_field(0xAAAA1111u);   /* same seed again */
    int a2[8]; field_seq(a2, 8);

    engine_generative_new_field(0xBBBB2222u);   /* different seed */
    int b1[8]; field_seq(b1, 8);

    int same = 1, diff = 0;
    for (int i = 0; i < 8; ++i) {
        if (a1[i] != a2[i]) same = 0;
        if (a1[i] != b1[i]) diff = 1;
    }
    CHECK(same, "same seed → identical field evolution (reproducible)");
    CHECK(diff, "different seed → different field evolution");
    CHECK(keyA == key0, "New Field keeps the pitch world (key unchanged)");
    CHECK(engine_gen_seed() == 0xBBBB2222u, "gen seed reflects the last field");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
