/*
 * Generative progression sequencer — Step 12b #4.
 *
 * Ported from the webapp PROGRESSIONS / DEGREE_TRANSITIONS / nextDegreeMarkov /
 * wchoose. Degrees are 1-indexed. The MARKOV result is clamped to 1..6 (degree
 * 7 has zero weight in every transition row, matching the webapp); the FIXED
 * programs may return 7 — progression 3 {1,6,3,7} deliberately walks the
 * leading-tone chord, and brain_chord() voices degree 7 fine (r18.88 doc fix:
 * the old comment claimed 7 was never voiced).
 */

#include "generative.h"

/* Fixed progressions (1-indexed degrees), webapp order. */
static const int8_t PROG[GEN_PROG_COUNT][8] = {
    { 1, 4, -1 },                 /* slow_bed (default) */
    { 1, 5, 6, 4, -1 },
    { 1, 4, 5, 1, -1 },
    { 1, 6, 3, 7, -1 },
    { 1, 4, 6, 5, -1 },
};

/* Markov transition weights from each degree 1..7 → next degree 1..7.
 * Row i is the weight vector when the current degree is (i+1). */
static const float TRANS[7][7] = {
    { 0.10f, 0.05f, 0.05f, 0.30f, 0.25f, 0.25f, 0.0f }, /* from 1 */
    { 0.25f, 0.05f, 0.05f, 0.10f, 0.40f, 0.15f, 0.0f }, /* from 2 */
    { 0.15f, 0.10f, 0.05f, 0.30f, 0.10f, 0.30f, 0.0f }, /* from 3 */
    { 0.35f, 0.10f, 0.0f,  0.05f, 0.35f, 0.15f, 0.0f }, /* from 4 */
    { 0.50f, 0.05f, 0.0f,  0.10f, 0.05f, 0.30f, 0.0f }, /* from 5 */
    { 0.20f, 0.20f, 0.05f, 0.30f, 0.20f, 0.05f, 0.0f }, /* from 6 */
    { 0.50f, 0.10f, 0.0f,  0.20f, 0.20f, 0.0f,  0.0f }, /* from 7 */
};

static int      s_program;     /* <0 = Markov, else fixed index */
static int      s_step;        /* index into the fixed progression */
static int      s_degree;      /* current degree 1..6 */
static uint32_t s_rng;

void generative_init(void) {
    s_program = 0;             /* slow_bed by default */
    s_step    = 0;
    s_degree  = 1;
    s_rng     = 0xC0FFEEu;
}

void generative_seed(uint32_t s) { s_rng = s ? s : 1u; }

static float rng_unit(void) {
    /* LCG → [0,1). */
    s_rng = s_rng * 1664525u + 1013904223u;
    return (float)(s_rng >> 8) / (float)(1u << 24);
}

static int clampi(int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); }

void generative_set_program(int idx) {
    if (idx < 0) {
        s_program = -1;        /* Markov */
    } else {
        s_program = clampi(idx, 0, GEN_PROG_COUNT - 1);
    }
    s_step   = 0;
    s_degree = 1;
}

/* Weighted choice over degrees 1..7 using the row for `cur`, clamped to 1..6. */
static int markov_next(int cur) {
    const float *w = TRANS[clampi(cur, 1, 7) - 1];
    float sum = 0.0f;
    for (int i = 0; i < 7; ++i) sum += w[i];
    float r = rng_unit() * sum;
    for (int i = 0; i < 7; ++i) {
        r -= w[i];
        if (r <= 0.0f) return clampi(i + 1, 1, 6);
    }
    return clampi(cur, 1, 6);
}

int generative_next_degree(void) {
    if (s_program < 0) {
        s_degree = markov_next(s_degree);
    } else {
        const int8_t *p = PROG[s_program];
        /* length = first -1 sentinel */
        int len = 0;
        while (len < 8 && p[len] >= 0) ++len;
        if (len == 0) { s_degree = 1; }
        else {
            int d = p[s_step % len];
            s_step = (s_step + 1) % len;
            s_degree = clampi(d, 1, 7);
        }
    }
    return s_degree;
}

int generative_current_degree(void) { return s_degree; }
