/*
 * composer.c — ambient composer state machine. See composer.h.
 */

#include "composer.h"

/* The composition, as a table. Tuning notes:
 *   - OPEN is the only state where high answers are common enough to be
 *     remembered; everywhere else they are an event, not a feature.
 *   - EMPTY is not silence — the bed keeps sustaining at 0.6× and the
 *     texture/atmosphere layers are untouched. It is the held breath
 *     between thoughts, and it is what makes RETURN feel like warmth.
 *   - DEEP raises the bass fundament, not the loudness: depth 0.85 with
 *     the melody receding reads as "the floor comes closer".            */
static const composer_params_t TABLE[COMPOSER_STATE_COUNT] = {
    /*            mel_density rest_add high_p bed_amp bass_depth        */
    /* CALM   */ { 0.70f,      +0.10f,  0.04f, 1.00f,  0.50f },
    /* OPEN   */ { 1.30f,      -0.10f,  0.15f, 1.05f,  0.40f },
    /* DEEP   */ { 0.45f,      +0.20f,  0.02f, 0.90f,  0.85f },
    /* EMPTY  */ { 0.15f,      +0.45f,  0.00f, 0.60f,  0.30f },
    /* RETURN */ { 1.00f,       0.00f,  0.08f, 1.00f,  0.55f },
};

static const char *NAMES[COMPOSER_STATE_COUNT] = {
    "CALM", "OPEN", "DEEP", "EMPTY", "RETURN"
};

/* State durations 40–80 s, humanized by a fixed-seed LCG so a session is
 * deterministic but never metronomic. */
#define STATE_MIN_MS 40000u
#define STATE_VAR_MS 40000u

static composer_state_t s_state;
static uint32_t s_until_ms;
static int      s_timing_valid;
static uint32_t s_rng;

static float rnd01(void) {
    s_rng = s_rng * 1664525u + 1013904223u;
    return (float)(s_rng >> 8) / 16777216.0f;
}

void composer_init(void) {
    s_state        = COMPOSER_CALM;
    s_until_ms     = 0;
    s_timing_valid = 0;
    s_rng          = 0xC0400511u;
}

void composer_tick(uint32_t now_ms) {
    if (!s_timing_valid) {
        s_until_ms = now_ms + STATE_MIN_MS +
                     (uint32_t)(rnd01() * (float)STATE_VAR_MS);
        s_timing_valid = 1;
        return;
    }
    if ((int32_t)(now_ms - s_until_ms) >= 0) {
        s_state = (composer_state_t)((s_state + 1) % COMPOSER_STATE_COUNT);
        s_until_ms = now_ms + STATE_MIN_MS +
                     (uint32_t)(rnd01() * (float)STATE_VAR_MS);
    }
}

void composer_nudge(composer_state_t target, uint32_t now_ms) {
    if (target >= COMPOSER_STATE_COUNT) return;
    s_state    = target;
    s_until_ms = now_ms + STATE_MIN_MS + (uint32_t)(rnd01() * (float)STATE_VAR_MS);
    s_timing_valid = 1;
}

void composer_reseed(uint32_t seed) { s_rng = seed ? seed : 0xC0400511u; }

const composer_params_t *composer_params(void)     { return &TABLE[s_state]; }
composer_state_t         composer_state(void)      { return s_state; }
const char              *composer_state_name(void) { return NAMES[s_state]; }
