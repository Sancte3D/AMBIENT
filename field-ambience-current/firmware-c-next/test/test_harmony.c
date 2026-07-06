/*
 * test_harmony.c — the harmonic safety core (r19.0).
 *
 * Verifies the contracts from the user's research brief, as CODE:
 *   1. pitch worlds: pentatonic core has no semitone step / no tritone;
 *      color note admitted only above C4;
 *   2. state mutations keep ≥3 common pitch classes and move ≤2 voices;
 *      common tones stay at the SAME pitch (parsimonious voice leading);
 *   3. bass lives in its register; voices stay in theirs and always
 *      belong to the current state;
 *   4. collision filter: semitone class and tritone rejected everywhere,
 *      2nds rejected below C4;
 *   5. melody picker: every pick is in-world, collision-safe, inside the
 *      melody register, never leaps beyond an octave, repeats exist.
 */

#include "harmony.h"
#include <stdio.h>
#include <string.h>

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static int pc(int m) { int v = m % 12; return v < 0 ? v + 12 : v; }
static int icl(int a, int b) {
    int d = pc(a - b);
    return d > 6 ? 12 - d : d;
}

int main(void) {
    printf("== harmonic safety core (r19.0) ==\n");

    /* ---- 1. pitch world: D minor default ---- */
    harmony_init();                       /* D minor world */
    /* core D F G A C sounds anywhere in the melody band */
    int core_d[5] = { 62, 65, 67, 69, 72 };
    for (int i = 0; i < 5; ++i)
        CHECK(harmony_in_world(core_d[i]), "core pc %d in world", core_d[i]);
    /* color E only above C4 */
    CHECK(harmony_in_world(64),  "E4 (color) allowed above C4");
    CHECK(!harmony_in_world(52), "E3 (color) rejected below C4");
    /* avoid classes are out */
    CHECK(!harmony_in_world(63), "Eb rejected");   /* D#/Eb */
    CHECK(!harmony_in_world(66), "F# rejected");
    CHECK(!harmony_in_world(71), "B rejected");

    /* the core itself contains no semitone step and no tritone */
    {
        int pcs[5] = { 2, 5, 7, 9, 0 };   /* D F G A C */
        int bad = 0;
        for (int i = 0; i < 5; ++i)
            for (int j = i + 1; j < 5; ++j) {
                int ic = icl(pcs[i], pcs[j]);
                if (ic == 1 || ic == 6) ++bad;
            }
        CHECK(bad == 0, "core is semitone/tritone free (%d)", bad);
    }

    /* ---- 2 + 3. mutations: common tones, moved voices, registers ---- */
    {
        int prev[HARMONY_VOICES], cur[HARMONY_VOICES];
        harmony_voices(prev, HARMONY_VOICES);
        for (int step = 0; step < 60; ++step) {
            harmony_advance();
            CHECK(harmony_last_common_tones() >= 3,
                  "mutation keeps >= 3 common pcs (%d at step %d)",
                  harmony_last_common_tones(), step);
            CHECK(harmony_last_moved_voices() <= 2,
                  "mutation moves <= 2 voices (%d at step %d)",
                  harmony_last_moved_voices(), step);
            harmony_voices(cur, HARMONY_VOICES);
            for (int v = 0; v < HARMONY_VOICES; ++v) {
                CHECK(cur[v] >= 55 && cur[v] <= 79,
                      "voice %d in band (%d)", v, cur[v]);
                /* a voice that kept its pitch class kept its PITCH */
                if (pc(cur[v]) == pc(prev[v]))
                    CHECK(cur[v] == prev[v],
                          "common tone frozen at pitch (%d -> %d)",
                          prev[v], cur[v]);
            }
            int bass = harmony_bass_midi();
            CHECK(bass >= 38 && bass <= 49, "bass in register (%d)", bass);
            CHECK(harmony_fifth_midi() == bass + 7, "fifth = bass + 7");
            memcpy(prev, cur, sizeof prev);
            if (fails > 12) break;
        }
        CHECK(harmony_state_changes() >= 60, "mutations counted");
    }

    /* ---- 4. collision filter ---- */
    {
        int sus1[1] = { 60 };
        CHECK(!harmony_collision_ok(61, sus1, 1), "semitone rejected");
        CHECK(!harmony_collision_ok(59, sus1, 1), "semitone (11) rejected");
        CHECK(!harmony_collision_ok(66, sus1, 1), "tritone rejected");
        CHECK(harmony_collision_ok(67, sus1, 1),  "fifth accepted");
        CHECK(harmony_collision_ok(64, sus1, 1),  "third accepted");
        int sus2[1] = { 55 };
        CHECK(!harmony_collision_ok(57, sus2, 1), "2nd below C4 rejected");
        int sus3[1] = { 72 };
        CHECK(harmony_collision_ok(74, sus3, 1),  "2nd above C4 accepted");
    }

    /* ---- 5. melody picker: 600 picks, all safe ---- */
    {
        harmony_init();
        int sus[8], nsus, last = 0, reps = 0, notes = 0, silences = 0;
        int max_leap = 0;
        for (int k = 0; k < 600; ++k) {
            if ((k % 7) == 0) harmony_advance();     /* harmony moves under it */
            nsus = 0;
            sus[nsus++] = harmony_bass_midi();
            int hv[HARMONY_VOICES];
            harmony_voices(hv, HARMONY_VOICES);
            for (int i = 0; i < HARMONY_VOICES; ++i) sus[nsus++] = hv[i];
            int t = harmony_melody_next(last, sus, nsus, 0.05f);
            if (t < 0) { ++silences; continue; }
            CHECK(harmony_in_world(t), "pick in world (%d)", t);
            CHECK(harmony_collision_ok(t, sus, nsus), "pick collision-safe (%d)", t);
            CHECK(t >= 62 && t <= 86, "pick in melody register (%d)", t);
            if (last > 0) {
                int d = t > last ? t - last : last - t;
                if (d == 0) ++reps;
                if (d > max_leap) max_leap = d;
            }
            last = t;
            ++notes;
            if (fails > 12) break;
        }
        CHECK(notes > 500, "melody actually sings (%d/600)", notes);
        CHECK(max_leap <= 12, "no leap beyond an octave (%d)", max_leap);
        CHECK(reps >= 20, "repetition exists (%d)", reps);
        CHECK(silences < 60, "silence fallback is the exception (%d)", silences);
    }

    /* ---- 6. major world (Tokyo A) ---- */
    {
        harmony_set_world(57, 0);                  /* A major */
        /* core A B C# E F# */
        CHECK(harmony_in_world(69), "A in world");
        CHECK(harmony_in_world(71), "B in world");
        CHECK(harmony_in_world(73), "C# in world");
        CHECK(harmony_in_world(76), "E in world");
        CHECK(harmony_in_world(78), "F# in world");
        CHECK(!harmony_in_world(72), "C natural rejected in A major world");
        CHECK(harmony_in_world(68),  "G# (color maj7) above C4 ok");
        CHECK(!harmony_in_world(56), "G# below C4 rejected");
        for (int step = 0; step < 20; ++step) {
            harmony_advance();
            CHECK(harmony_last_common_tones() >= 3, "major world common tones");
            CHECK(harmony_last_moved_voices() <= 2, "major world moved voices");
            if (fails > 12) break;
        }
    }

    printf("%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
