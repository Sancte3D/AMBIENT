/*
 * ui_encoder — bounce-immune quadrature decoding for the navigation encoder.
 *
 * WHY THIS REPLACES EDGE COUNTING. The previous bench counted falling edges on
 * A alone and read B inside the interrupt. Both halves of that are wrong on a
 * mechanical encoder:
 *
 *   - A single detent does not produce one clean falling edge. The contacts
 *     bounce, so one physical click fires the interrupt several times and the
 *     UI jumps two or three steps. A time-based debounce only papers over it:
 *     too short and bounces still count, too long and a fast turn is dropped.
 *   - B is sampled at the exact moment A is transitioning, which is when B is
 *     least settled, so the direction itself is unreliable.
 *
 * A state machine has neither problem. Each (previous, current) pair of the
 * two-bit Gray code is either a step forward, a step backward, or impossible.
 * A bounce walks forward and immediately back, so the two cancel exactly —
 * there is nothing to tune and nothing to drop. Direction comes from the
 * transition rather than from a snapshot of the other pin.
 *
 * DETENTS, NOT TRANSITIONS. A common EC11 runs through all four Gray states
 * between adjacent detents, so raw transitions must be divided by four or one
 * click moves four steps. That divisor is the only thing here that depends on
 * the part: set UI_ENC_STATES_PER_DETENT to 2 for a half-step encoder, 1 for
 * one that rests on every state.
 *
 * Pure integer logic and no hardware dependency, so the whole thing is
 * exercised on the host — including bounce, which is not reproducible by hand.
 */
#ifndef FAM_UI_ENCODER_H
#define FAM_UI_ENCODER_H

#include <stdint.h>

#ifndef UI_ENC_STATES_PER_DETENT
#define UI_ENC_STATES_PER_DETENT 4
#endif

typedef struct {
    uint8_t state;      /* last (A<<1)|B                                   */
    int8_t  accum;      /* sub-steps since the last emitted detent         */
    int32_t detents;    /* net detents, consumed by ui_enc_take()          */
} ui_enc_t;

void ui_enc_init(ui_enc_t *e, int a, int b);

/* Feed a new pin reading. Call it on every edge of EITHER pin — the decoder
 * needs both to see a complete transition. Returns the number of whole detents
 * this sample completed (-1, 0 or +1). */
int  ui_enc_update(ui_enc_t *e, int a, int b);

/* Atomically-ish read and clear the pending detent count. */
int32_t ui_enc_take(ui_enc_t *e);

#endif
