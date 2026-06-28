/*
 * harmonic_bass — "5th-harmonic" / Exceeder-style electro synth bass.
 *
 * The trick (per the user / the Mason – Exceeder, Gold Dust sound): a root
 * oscillator plus a second oscillator tuned **+28 semitones** (2 octaves + a
 * major third). 2^(28/12) ≈ 5.04, which lands almost exactly on the 5th
 * overtone of the root — that overtone is what makes the bass sing with the
 * aggressive, electric "bite" of electro/house bass.
 *
 * Signal chain (mono):
 *   sub sine (root −1 oct) ─┐
 *   root saw  ──────────────┤→ mix → tanh(drive) → SVF lowpass (tone)
 *   +28 saw (the bite) ─────┘                       → SVF highpass (tighten)
 *                                                   → AR envelope → out
 * Portamento glides the root pitch on a new note while sounding → the bendy
 * slides. Band-limited saws (polyBLEP) keep it from aliasing into fizz.
 *
 * Mirrors the bass.c voice contract: init / note / release / render_mix that
 * ADDS into the dry + reverb-send buffers. Host-testable, no Pico SDK.
 */

#include "harmonic_bass.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR ((float)DSP_SAMPLE_RATE_HZ)

enum { ENV_IDLE = 0, ENV_ATTACK = 1, ENV_RELEASE = 2 };

#define ATTACK_S    0.012f      /* fast electro pluck, not a click */
#define RELEASE_TAU 0.12f       /* exp tail ≈ 0.8 s to silence */
#define SILENCE_EPS 1.0e-4f
#define SEND_LEVEL  0.05f       /* a touch of reverb so it sits in the world */
#define SVF_UPDATE  32          /* control-rate cutoff refresh (samples) */

static struct {
    float sub_ph, root_ph, harm_ph;   /* oscillator phases (turns) */
    float freq_cur, freq_tgt;         /* root freq, glided */
    float harm_ratio;                 /* 2^(28/12) ≈ 5.04 */
    float glide_coef;                 /* portamento one-pole coef */
    float env, atk_inc, rel_coef;     /* AR amplitude envelope */
    int   state;
    dsp_svf_t lp, hp;
    int   svf_ctr;
    /* parameters */
    float bite;                       /* +28 osc level (0..~0.7) */
    float drive;                      /* tanh pre-gain (1..~4) */
    float cutoff_hz;                  /* tone lowpass */
    float level;                      /* master 0..1 */
} h;

void harmonic_bass_init(void) {
    memset(&h, 0, sizeof h);
    h.harm_ratio = powf(2.0f, 28.0f / 12.0f);   /* +28 semitones */
    h.glide_coef = dsp_smooth_coef(0.06f);
    h.atk_inc    = 1.0f / (ATTACK_S * SR);
    h.rel_coef   = dsp_smooth_coef(RELEASE_TAU);
    dsp_svf_reset(&h.lp); dsp_svf_set(&h.lp, 1200.0f, 0.9f);
    dsp_svf_reset(&h.hp); dsp_svf_set(&h.hp, 35.0f, 0.707f);
    h.bite      = 0.42f;
    h.drive     = 1.7f;
    h.cutoff_hz = 1400.0f;
    h.level     = 0.9f;
    h.state     = ENV_IDLE;
}

void harmonic_bass_note(float freq_hz) {
    if (freq_hz < 1.0f) return;
    if (h.state == ENV_IDLE) h.freq_cur = freq_hz;   /* from silence: snap */
    h.freq_tgt = freq_hz;                            /* else: glide (bendy) */
    h.state    = ENV_ATTACK;
}

void harmonic_bass_release(void) {
    if (h.state != ENV_IDLE) h.state = ENV_RELEASE;
}

void harmonic_bass_set_bite(float v)  { h.bite      = dsp_clampf(v, 0.0f, 1.0f) * 0.70f; }
void harmonic_bass_set_drive(float v) { h.drive     = 1.0f + dsp_clampf(v, 0.0f, 1.0f) * 3.0f; }
void harmonic_bass_set_tone(float v)  { h.cutoff_hz = 400.0f + dsp_clampf(v, 0.0f, 1.0f) * 3600.0f; }
void harmonic_bass_set_level(float v) { h.level     = dsp_clampf(v, 0.0f, 1.0f); }
void harmonic_bass_set_glide(float v) {
    h.glide_coef = dsp_smooth_coef(0.008f + dsp_clampf(v, 0.0f, 1.0f) * 0.25f);
}

void harmonic_bass_render_mix(float *dL, float *dR,
                              float *sL, float *sR, int frames) {
    if (h.state == ENV_IDLE && h.env < SILENCE_EPS) return;   /* fully idle */

    for (int n = 0; n < frames; ++n) {
        /* portamento on the root */
        h.freq_cur += h.glide_coef * (h.freq_tgt - h.freq_cur);

        /* AR envelope */
        if (h.state == ENV_ATTACK) {
            h.env += h.atk_inc;
            if (h.env >= 1.0f) h.env = 1.0f;
        } else if (h.state == ENV_RELEASE) {
            h.env += h.rel_coef * (0.0f - h.env);
            if (h.env < SILENCE_EPS) { h.env = 0.0f; h.state = ENV_IDLE; }
        }

        /* per-sample phase increments */
        const float dt_r = h.freq_cur / SR;
        const float dt_h = (h.freq_cur * h.harm_ratio) / SR;
        const float dt_s = (h.freq_cur * 0.5f) / SR;

        /* control-rate cutoff refresh (the SVF is too costly per sample) */
        if ((h.svf_ctr++ % SVF_UPDATE) == 0) dsp_svf_set(&h.lp, h.cutoff_hz, 0.9f);

        /* oscillators: sub depth + root body + the +28 singing bite */
        const float sub  = dsp_sin(h.sub_ph);
        const float root = dsp_poly_saw(h.root_ph, dt_r);
        const float harm = dsp_poly_saw(h.harm_ph, dt_h);
        float mix = sub * 0.55f + root * 0.80f + harm * h.bite;

        /* drive = the bite/saturation, then tone shaping */
        float driven = tanhf(mix * h.drive);
        float lp = dsp_svf_lp(&h.lp, driven);
        float hp = dsp_svf_hp(&h.hp, lp);            /* tighten sub-rumble/DC */
        /* final soft-clip limiter: the resonant SVF can ring past the tanh
         * edges and overshoot > 1.0; keep the voice bounded so the DAC never
         * hard-clips (tanh is near-linear for quiet parts, so tone is kept). */
        float out = tanhf(hp) * h.env * h.level * 0.85f;

        /* advance phases */
        h.root_ph += dt_r; if (h.root_ph >= 1.0f) h.root_ph -= 1.0f;
        h.harm_ph += dt_h; if (h.harm_ph >= 1.0f) h.harm_ph -= 1.0f;
        h.sub_ph  += dt_s; if (h.sub_ph  >= 1.0f) h.sub_ph  -= 1.0f;

        dL[n] += out;            dR[n] += out;
        sL[n] += out * SEND_LEVEL; sR[n] += out * SEND_LEVEL;
    }
}
