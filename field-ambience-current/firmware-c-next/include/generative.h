#ifndef FAM_GENERATIVE_H
#define FAM_GENERATIVE_H

/*
 * Generative progression sequencer — Step 12b #4.
 *
 * Pure control-rate logic: produces a stream of scale degrees (1..6) that an
 * outer timer turns into chord changes. Two modes, ported from the webapp:
 *   - fixed progression : cycle one of PROGRESSIONS (e.g. I–IV, I–V–vi–IV)
 *   - Markov auto       : weighted-random next degree from DEGREE_TRANSITIONS
 *
 * No audio, no engine deps. The RNG is a seedable LCG so the Markov walk is
 * fully deterministic + host-testable.
 */

#include <stdint.h>

#define GEN_PROG_COUNT 5      /* number of fixed progressions */

void generative_init(void);

/* Seed the Markov RNG (deterministic). Optional — init seeds a default. */
void generative_seed(uint32_t s);

/* Select the progression source:
 *   idx < 0  → Markov auto mode
 *   idx >= 0 → fixed PROGRESSIONS[idx] (clamped to 0..GEN_PROG_COUNT-1)
 * Resets the step/degree so the next call starts the chosen sequence. */
void generative_set_program(int idx);

/* Advance one step and return the new current scale degree (1..6). */
int generative_next_degree(void);

/* The current degree without advancing (1..6). */
int generative_current_degree(void);

#endif
