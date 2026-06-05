/*
 * Polyphonic voice pool — Step 7.
 *
 * Sine + ASR envelope per voice. Envelope ramps are per-sample so there are
 * no clicks. Voice allocation: a note_on for an existing source re-uses that
 * source's slot; otherwise the first idle slot; otherwise steal the quietest
 * voice (lowest envelope level) so a steal is the least audible.
 */

#include "voices.h"
#include "dsp.h"
#include <string.h>

/* Envelope timing (seconds). Slow, per the Sound Constitution — no snappy
 * attacks. These are deliberately gentle; the real pad in Step 9 has its own
 * longer attacks driven by the BLOOM macro. */
#define ATTACK_S   0.8f
#define RELEASE_S  2.5f

typedef enum { ENV_IDLE = 0, ENV_ATTACK, ENV_SUSTAIN, ENV_RELEASE } env_state_t;

typedef struct {
    bool        used;          /* allocated to a source (may be releasing) */
    uint8_t     source;        /* owner id (e.g. cell index) */
    float       phase;         /* turns [0,1) */
    float       phase_inc;     /* freq / SR */
    float       amp;           /* peak amplitude */
    float       env;           /* current envelope level 0..1 */
    env_state_t state;
} voice_t;

static voice_t voices[VOICES_MAX];

static float atk_inc;   /* per-sample attack increment */
static float rel_dec;   /* per-sample release decrement */

void voices_init(void) {
    memset(voices, 0, sizeof voices);
    atk_inc = 1.0f / (ATTACK_S  * (float)DSP_SAMPLE_RATE_HZ);
    rel_dec = 1.0f / (RELEASE_S * (float)DSP_SAMPLE_RATE_HZ);
}

/* Find the slot already owned by `source`, else -1. */
static int find_source(uint8_t source) {
    for (int i = 0; i < VOICES_MAX; ++i)
        if (voices[i].used && voices[i].source == source) return i;
    return -1;
}

/* Pick a slot for a new note: idle first, else quietest voice. */
static int alloc_slot(void) {
    int best = -1;
    float best_env = 1e9f;
    for (int i = 0; i < VOICES_MAX; ++i) {
        if (!voices[i].used) return i;
        if (voices[i].env < best_env) { best_env = voices[i].env; best = i; }
    }
    return best;
}

void voices_note_on(uint8_t source, float freq_hz, float amp) {
    int i = find_source(source);
    if (i < 0) i = alloc_slot();
    if (i < 0) return;
    voice_t *v = &voices[i];
    /* Keep the current phase if re-triggering the same source (no click);
     * start fresh phase only for a newly allocated slot. */
    if (!(v->used && v->source == source)) v->phase = 0.0f;
    v->used      = true;
    v->source    = source;
    v->phase_inc = freq_hz / (float)DSP_SAMPLE_RATE_HZ;
    v->amp       = dsp_clampf(amp, 0.0f, 1.0f);
    v->state     = ENV_ATTACK;
}

void voices_note_off(uint8_t source) {
    int i = find_source(source);
    if (i < 0) return;
    if (voices[i].state != ENV_IDLE) voices[i].state = ENV_RELEASE;
}

void voices_all_off(void) {
    for (int i = 0; i < VOICES_MAX; ++i)
        if (voices[i].used && voices[i].state != ENV_IDLE)
            voices[i].state = ENV_RELEASE;
}

void voices_render(int16_t *buf, int frames) {
    for (int n = 0; n < frames; ++n) {
        float mix = 0.0f;
        for (int i = 0; i < VOICES_MAX; ++i) {
            voice_t *v = &voices[i];
            if (!v->used) continue;

            /* advance envelope */
            switch (v->state) {
                case ENV_ATTACK:
                    v->env += atk_inc;
                    if (v->env >= 1.0f) { v->env = 1.0f; v->state = ENV_SUSTAIN; }
                    break;
                case ENV_RELEASE:
                    v->env -= rel_dec;
                    if (v->env <= 0.0f) {
                        v->env = 0.0f;
                        v->state = ENV_IDLE;
                        v->used = false;
                    }
                    break;
                case ENV_SUSTAIN:
                case ENV_IDLE:
                default:
                    break;
            }
            if (!v->used) continue;

            mix += dsp_sin(v->phase) * v->env * v->amp;

            v->phase += v->phase_inc;
            if (v->phase >= 1.0f) v->phase -= 1.0f;
        }

        /* Soft-clip the sum so a fistful of cells can't wrap the int16. */
        if (mix >  1.0f) mix =  1.0f;
        if (mix < -1.0f) mix = -1.0f;
        int16_t s = (int16_t)(mix * 32767.0f);
        buf[n * 2 + 0] = s;   /* L */
        buf[n * 2 + 1] = s;   /* R */
    }
}

int voices_active_count(void) {
    int c = 0;
    for (int i = 0; i < VOICES_MAX; ++i)
        if (voices[i].used && voices[i].state != ENV_IDLE) ++c;
    return c;
}
