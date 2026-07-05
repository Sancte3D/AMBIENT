#ifndef FAM_COMPOSER_H
#define FAM_COMPOSER_H

/*
 * composer.{h,c} — the ambient COMPOSER above the sound engine (r18.96).
 *
 * PRINCIPLE studied from Atmoscapia's publicly described architecture
 * (layered generative ambient with an evolution engine) and Brian Eno's
 * generative systems, reconstructed for this instrument (no code exists
 * to copy — the principle is the point):
 *
 *   A generative piece feels COMPOSED when a slow top-level intent moves
 *   underneath it. Not new notes — new PROBABILITIES. The composer never
 *   places a note and never touches the audio path; it only re-weights
 *   the decisions the melody grammar and the layers were already making.
 *
 * Five states, cycling over minutes (40–80 s each, humanized):
 *
 *   CALM   → the piece breathes at its resting rate
 *   OPEN   → more light: melody denser, rare high answers appear
 *   DEEP   → the floor rises: bass fundament up, melody recedes
 *   EMPTY  → almost nothing: long rests, bed quiet — the held breath
 *   RETURN → coming home: normal density, warmth back
 *
 * Each state is ONLY this table:
 *   mel_density  multiplier on the grammar's per-bar note probability
 *   rest_add     added to the 30 % rest-phrase probability
 *   high_p       chance a melody tone answers +2 octaves (very rare)
 *   bed_amp      multiplier on the generative bed voice amplitude
 *   bass_depth   target for the bass fundament (engine applies per bar)
 *
 * The module is pure (no engine calls, fixed-seed LCG, host-testable);
 * engine_generative_tick() reads the table and applies it. Active ONLY
 * in autoplay — a playing human overrides everything, as always.
 */

#include <stdint.h>

typedef enum {
    COMPOSER_CALM = 0,
    COMPOSER_OPEN,
    COMPOSER_DEEP,
    COMPOSER_EMPTY,
    COMPOSER_RETURN,
    COMPOSER_STATE_COUNT
} composer_state_t;

typedef struct {
    float mel_density;   /* × on note probability      */
    float rest_add;      /* + on rest-phrase probability */
    float high_p;        /* p of a +24 high answer       */
    float bed_amp;       /* × on bed voice amplitude     */
    float bass_depth;    /* bass fundament target 0..1   */
} composer_params_t;

void composer_init(void);

/* Advance the state clock. Call from the generative tick (any rate). */
void composer_tick(uint32_t now_ms);

/* The current state's probability table (always valid). */
const composer_params_t *composer_params(void);

composer_state_t composer_state(void);
const char      *composer_state_name(void);   /* for a future UI readout */

#endif
