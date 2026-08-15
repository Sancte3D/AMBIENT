/* ui_encoder — see ui_encoder.h. */
#include "ui_encoder.h"

/* Indexed by (previous << 2) | current, each index being (A<<1)|B.
 * +1 / -1 are the legal single-step transitions of the Gray code; 0 covers
 * both "no change" and the impossible double transition, which is what a
 * missed sample or a violent bounce looks like. Treating those as zero is
 * deliberate: guessing a direction from an illegal transition is how a decoder
 * starts inventing steps that the hand never made.
 *
 * Positive is the Gray order 00 -> 01 -> 11 -> 10. Which way the knob has to
 * turn for that is a wiring question, not a decoder question: swap A and B if
 * the wheel goes the wrong way. */
static const int8_t QDEC[16] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0
};

void ui_enc_init(ui_enc_t *e, int a, int b)
{
    e->state   = (uint8_t)(((a & 1) << 1) | (b & 1));
    e->accum   = 0;
    e->detents = 0;
}

int ui_enc_update(ui_enc_t *e, int a, int b)
{
    uint8_t cur = (uint8_t)(((a & 1) << 1) | (b & 1));
    if (cur == e->state) return 0;

    int8_t step = QDEC[(e->state << 2) | cur];
    e->state = cur;
    if (step == 0) return 0;

    e->accum = (int8_t)(e->accum + step);

    if (e->accum >= UI_ENC_STATES_PER_DETENT) {
        e->accum = 0;
        ++e->detents;
        return +1;
    }
    if (e->accum <= -UI_ENC_STATES_PER_DETENT) {
        e->accum = 0;
        --e->detents;
        return -1;
    }
    return 0;
}

int32_t ui_enc_take(ui_enc_t *e)
{
    int32_t d = e->detents;
    e->detents -= d;
    return d;
}
