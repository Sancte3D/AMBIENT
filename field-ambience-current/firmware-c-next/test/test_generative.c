/*
 * Host-side tests for the Step-12b #4 generative sequencer.
 *
 * Build via run_tests.sh, or:
 *   cc -std=c11 -I../include test_generative.c ../src/generative.c -o /tmp/gen_test
 */

#include "generative.h"

#include <stdio.h>
#include <stdlib.h>

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

static void test_fixed_progressions_cycle(void) {
    /* Program 0 is [1,4]: degrees must cycle 1,4,1,4,… */
    generative_init();
    generative_set_program(0);
    int expect[] = { 1, 4, 1, 4, 1, 4 };
    for (int i = 0; i < 6; ++i) {
        int d = generative_next_degree();
        CHECK(d == expect[i], "prog0 step %d: got %d want %d", i, d, expect[i]);
    }

    /* Program 1 is [1,5,6,4]. */
    generative_set_program(1);
    int e2[] = { 1, 5, 6, 4, 1, 5 };
    for (int i = 0; i < 6; ++i) {
        int d = generative_next_degree();
        CHECK(d == e2[i], "prog1 step %d: got %d want %d", i, d, e2[i]);
    }
}

static void test_all_degrees_in_range(void) {
    /* Fixed progressions may legitimately use degree 7 (e.g. PROGRESSIONS[3]
     * = [1,6,3,7], webapp-faithful); the brain handles degree 7 fine. The
     * Markov walk, however, is clamped to 1..6 (degree 7 has zero weight).
     * So: every program emits 1..7; Markov specifically emits only 1..6. */
    for (int p = -1; p < GEN_PROG_COUNT; ++p) {
        generative_init();
        generative_seed(12345u + p);
        generative_set_program(p);
        for (int i = 0; i < 5000; ++i) {
            int d = generative_next_degree();
            CHECK(d >= 1 && d <= 7, "program %d emitted out-of-range degree %d", p, d);
            if (p < 0) CHECK(d <= 6, "Markov emitted degree 7 (%d)", d);
        }
    }
}

static void test_markov_deterministic(void) {
    /* Same seed → identical Markov walk. */
    int a[64], b[64];
    generative_init(); generative_seed(0xABCDEFu); generative_set_program(-1);
    for (int i = 0; i < 64; ++i) a[i] = generative_next_degree();
    generative_init(); generative_seed(0xABCDEFu); generative_set_program(-1);
    for (int i = 0; i < 64; ++i) b[i] = generative_next_degree();
    for (int i = 0; i < 64; ++i) CHECK(a[i] == b[i], "Markov not deterministic at %d: %d vs %d", i, a[i], b[i]);

    /* Different seed → not identical (extremely unlikely to match 64 steps). */
    int diff = 0;
    generative_init(); generative_seed(0x999u); generative_set_program(-1);
    for (int i = 0; i < 64; ++i) if (generative_next_degree() != a[i]) ++diff;
    CHECK(diff > 0, "different seed produced identical walk (suspicious)");
}

static void test_markov_visits_and_distribution(void) {
    /* Over a long walk the Markov mode must visit several distinct degrees
     * (not get stuck) and never emit degree 7. From the transition weights,
     * degree 4 (a popular target from 1/3/6) should appear a fair amount. */
    generative_init();
    generative_seed(0x1234u);
    generative_set_program(-1);

    int hist[8] = {0};
    int N = 20000;
    for (int i = 0; i < N; ++i) hist[generative_next_degree()]++;

    int distinct = 0;
    for (int d = 1; d <= 6; ++d) if (hist[d] > 0) ++distinct;
    CHECK(distinct >= 5, "Markov stuck — only %d distinct degrees", distinct);
    CHECK(hist[7] == 0, "Markov emitted degree 7 (%d times)", hist[7]);
    CHECK(hist[4] > N / 20, "degree 4 surprisingly rare: %d / %d", hist[4], N);

    printf("  Markov histogram (deg1..6): ");
    for (int d = 1; d <= 6; ++d) printf("%d:%d ", d, hist[d]);
    printf("\n");
}

static void test_program_switch_resets(void) {
    generative_init();
    generative_set_program(0);
    generative_next_degree();                /* advance into prog0 */
    generative_set_program(2);               /* [1,4,5,1] — should restart */
    int d = generative_next_degree();
    CHECK(d == 1, "program switch did not restart at first degree: got %d", d);
}

int main(void) {
    printf("== generative (step12b #4) ==\n");
    test_fixed_progressions_cycle();
    test_all_degrees_in_range();
    test_markov_deterministic();
    test_markov_visits_and_distribution();
    test_program_switch_resets();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
