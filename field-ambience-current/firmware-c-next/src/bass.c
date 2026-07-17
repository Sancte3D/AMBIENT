/*
 * famSubBass + famDeepBass — Step 8.
 *
 * Two single mono voices that track the chord root. Both use a slow linear
 * bloom + exponential release; pitch glides (portamento) when the held root
 * changes so fast cell taps don't click. famSubBass adds a slow breath LFO
 * on its amplitude; famDeepBass adds tanh body saturation between a 50 Hz
 * highpass and a 350 Hz lowpass.
 */

#include "bass.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR        ((float)DSP_SAMPLE_RATE_HZ)

#define SUB_ATTACK_S    3.0f
#define SUB_RELEASE_S   (4.0f / 3.0f)      /* setTargetAtTime tau in the webapp */
#define DEEP_ATTACK_S   2.5f
#define DEEP_RELEASE_S  (3.5f / 3.0f)

#define FREQ_GLIDE_S    0.12f              /* portamento time constant */
/* Release is an exponential with a ~1.2–1.3 s time constant, so it would take
 * ~13 s to reach 1e-5. Cut the tail to idle at −60 dB (inaudible for a bass
 * layer) — mirrors the webapp force-stopping the oscillators a few seconds
 * after release rather than waiting for true zero. */
#define SILENCE_EPS     1.0e-3f

#define SUB_SEND   0.03f
#define DEEP_SEND  0.08f

typedef enum { ENV_IDLE = 0, ENV_ATTACK, ENV_SUSTAIN, ENV_RELEASE } env_state_t;

/* ---- famSubBass ---- */
static struct {
    float sine_ph, tri_ph;
    float freq_cur, freq_tgt;             /* sub root (= lowest/4) */
    dsp_svf_t lp;                          /* lowpass 90 Hz */
    float env, amp, atkInc, relCoef;
    env_state_t state;
    float breath_ph;
} sub;

/* ---- famDeepBass ---- */
static struct {
    float sine_ph, tri_ph;
    float freq_cur, freq_tgt;             /* deep root (= lowest/2) */
    dsp_svf_t hp, lp;                      /* highpass 50, lowpass 350 */
    float env, amp, atkInc, relCoef;
    env_state_t state;
} deep;

static float freq_glide;                   /* per-sample one-pole coef */
static float depth = 0.5f;

static float depth_to_sub(float d)  { return 0.06f + d * (0.20f - 0.06f); }
static float depth_to_deep(float d) { return 0.04f + d * (0.16f - 0.04f); }

void bass_init(void) {
    memset(&sub,  0, sizeof sub);
    memset(&deep, 0, sizeof deep);

    dsp_svf_reset(&sub.lp);  dsp_svf_set(&sub.lp,  90.0f, 0.707f);
    dsp_svf_reset(&deep.hp); dsp_svf_set(&deep.hp, 50.0f, 0.707f);
    dsp_svf_reset(&deep.lp); dsp_svf_set(&deep.lp, 350.0f, 0.15f * 12.0f);

    sub.atkInc  = 1.0f / (SUB_ATTACK_S  * SR);     /* against the amp target */
    sub.relCoef = dsp_smooth_coef(SUB_RELEASE_S);
    deep.atkInc = 1.0f / (DEEP_ATTACK_S * SR);
    deep.relCoef = dsp_smooth_coef(DEEP_RELEASE_S);

    sub.state = deep.state = ENV_IDLE;
    freq_glide = dsp_smooth_coef(FREQ_GLIDE_S);
    depth = 0.5f;
}

void bass_set_depth(float d) { depth = dsp_clampf(d, 0.0f, 1.0f); }

/* r19.31 — portamento time for the root glide (HARMONY bass modes: ROOT snaps,
 * DRIFT glides slowly, FIFTH sits in between). */
void bass_set_glide(float tau_s) {
    if (tau_s < 0.001f) tau_s = 0.001f;
    freq_glide = dsp_smooth_coef(tau_s);
}

void bass_note(float lowest_freq_hz) {
    if (lowest_freq_hz < 1.0f) return;
    float sub_f  = lowest_freq_hz * 0.25f;         /* 2 octaves down */
    float deep_f = lowest_freq_hz * 0.5f;          /* 1 octave down  */

    bool was_idle = (sub.state == ENV_IDLE && deep.state == ENV_IDLE);

    sub.freq_tgt  = sub_f;
    deep.freq_tgt = deep_f;

    if (was_idle) {
        /* Bloom from silence: snap pitch, restart envelopes. */
        sub.freq_cur  = sub_f;  deep.freq_cur = deep_f;
        sub.amp  = depth_to_sub(depth);
        deep.amp = depth_to_deep(depth);
        sub.env = deep.env = 0.0001f;
        sub.state = deep.state = ENV_ATTACK;
    } else {
        /* Already sounding: just glide to the new root (handled per sample);
         * refresh amp targets in case depth changed. */
        sub.amp  = depth_to_sub(depth);
        deep.amp = depth_to_deep(depth);
        if (sub.state  == ENV_RELEASE) sub.state  = ENV_ATTACK;
        if (deep.state == ENV_RELEASE) deep.state = ENV_ATTACK;
    }
}

void bass_release(void) {
    if (sub.state  != ENV_IDLE) sub.state  = ENV_RELEASE;
    if (deep.state != ENV_IDLE) deep.state = ENV_RELEASE;
}

bool bass_active(void) {
    return sub.state != ENV_IDLE || deep.state != ENV_IDLE;
}

/* Advance a linear-attack / exponential-release envelope one sample. */
static float env_step(env_state_t *st, float *env, float amp,
                      float atkInc, float relCoef) {
    switch (*st) {
        case ENV_ATTACK:
            *env += atkInc;
            if (*env >= amp) { *env = amp; *st = ENV_SUSTAIN; }
            break;
        case ENV_SUSTAIN:
            /* r18.88 AUDIT-FIX: sustain used to hold the OLD level forever —
             * a bass_note() while sounding refreshes `amp` from the current
             * depth, but nothing moved env toward it, so a depth change (or
             * a bed retrigger at new depth) was inaudible until the next
             * full release. Drift toward amp with the release time constant
             * (both directions — swells and settles musically). */
            *env += relCoef * (amp - *env);
            break;
        case ENV_RELEASE:
            *env -= relCoef * *env;
            if (*env <= SILENCE_EPS) { *env = 0.0f; *st = ENV_IDLE; }
            break;
        default: break;
    }
    return *env;
}

void bass_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R, int frames) {
    if (sub.state == ENV_IDLE && deep.state == ENV_IDLE) return;

    for (int n = 0; n < frames; ++n) {
        float out = 0.0f, send = 0.0f;

        /* famSubBass */
        if (sub.state != ENV_IDLE) {
            sub.freq_cur += freq_glide * (sub.freq_tgt - sub.freq_cur);
            float e = env_step(&sub.state, &sub.env, sub.amp, sub.atkInc, sub.relCoef);

            sub.breath_ph += 0.04f / SR;
            if (sub.breath_ph >= 1.0f) sub.breath_ph -= 1.0f;
            float breath = 1.0f + 0.08f * dsp_sin(sub.breath_ph);

            float s = dsp_sin(sub.sine_ph)
                    + dsp_tri(sub.tri_ph) * 0.08f;
            float lp = dsp_svf_lp(&sub.lp, s);
            float v = lp * e * breath;

            sub.sine_ph += sub.freq_cur / SR;        if (sub.sine_ph >= 1.0f) sub.sine_ph -= 1.0f;
            sub.tri_ph  += 2.0f * sub.freq_cur / SR; if (sub.tri_ph  >= 1.0f) sub.tri_ph  -= 1.0f;

            out  += v;
            send += v * SUB_SEND;
        }

        /* famDeepBass */
        if (deep.state != ENV_IDLE) {
            deep.freq_cur += freq_glide * (deep.freq_tgt - deep.freq_cur);
            float e = env_step(&deep.state, &deep.env, deep.amp, deep.atkInc, deep.relCoef);

            float mix = dsp_sin(deep.sine_ph)
                      + dsp_tri(deep.tri_ph) * 0.06f;
            float sat = tanhf(mix * 2.2f);            /* body saturation */
            float hp  = dsp_svf_hp(&deep.hp, sat);
            float lp  = dsp_svf_lp(&deep.lp, hp);
            float v   = lp * e;

            deep.sine_ph += deep.freq_cur / SR;        if (deep.sine_ph >= 1.0f) deep.sine_ph -= 1.0f;
            deep.tri_ph  += 2.0f * deep.freq_cur / SR; if (deep.tri_ph  >= 1.0f) deep.tri_ph  -= 1.0f;

            out  += v;
            send += v * DEEP_SEND;
        }

        /* Mono → both channels. */
        dry_L[n]  += out;  dry_R[n]  += out;
        send_L[n] += send; send_R[n] += send;
    }
}
