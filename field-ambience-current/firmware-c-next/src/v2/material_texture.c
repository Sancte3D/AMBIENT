/*
 * material_texture.c — Air + Dust + Body texture layers.
 */

#include "v2/material_texture.h"
#include "dsp.h"
#include <math.h>

#define SR 44100.0f

/* Linear congruential noise generator — cheap, white-ish. */
static inline float lcg_noise(uint32_t *s) {
    *s = (*s * 1664525u + 1013904223u);
    return ((int32_t)*s) * (1.0f / 2147483648.0f);
}

void mt_init(mt_state_t *m, uint32_t seed) {
    m->lcg = seed ? seed : 0x12345678u;
    dsp_svf_reset(&m->air_bp);
    dsp_svf_reset(&m->body_lp);
    dsp_svf_reset(&m->dust_bp);
    dsp_svf_set(&m->air_bp,  6000.0f, 1.2f);
    dsp_svf_set(&m->body_lp,  150.0f, 0.6f);
    dsp_svf_set(&m->dust_bp, 2500.0f, 3.0f);
    m->dust_env = 0.0f;
    m->dust_rate_phase = 0.0f;
}

static float s_air = 0.5f, s_dust = 0.3f, s_body = 0.2f;

void mt_set_layer_balance(mt_state_t *m,
                          float air_base, float dust_base, float body_base) {
    (void)m;
    if (air_base  < 0.0f) air_base  = 0.0f;
    if (air_base  > 1.0f) air_base  = 1.0f;
    if (dust_base < 0.0f) dust_base = 0.0f;
    if (dust_base > 1.0f) dust_base = 1.0f;
    if (body_base < 0.0f) body_base = 0.0f;
    if (body_base > 1.0f) body_base = 1.0f;
    s_air  = air_base;
    s_dust = dust_base;
    s_body = body_base;
}

void mt_render_add(mt_state_t *m, float *L, float *R, int frames,
                   float texture_macro, float motion_walk)
{
    if (texture_macro < 0.0f) texture_macro = 0.0f;
    if (texture_macro > 1.0f) texture_macro = 1.0f;

    /* Master gain heuristic — texture_macro 0.18 → ~-22 dBFS noise floor. */
    const float master = texture_macro * 0.4f;

    /* Per-layer amplitudes, scaled by macro. */
    const float A_air  = s_air  * master * 0.55f;
    const float A_dust = s_dust * master * 0.45f;
    const float A_body = s_body * master * 0.70f;

    /* Dust trigger rate scales gently with macro — denser texture, more dust. */
    const float dust_hz = 6.0f + texture_macro * 10.0f;

    /* Small stereo pan tilt from motion (±10°). */
    float panL = 0.5f - motion_walk * 0.1f;
    float panR = 0.5f + motion_walk * 0.1f;
    if (panL < 0.1f) panL = 0.1f;
    if (panL > 0.9f) panL = 0.9f;
    if (panR < 0.1f) panR = 0.1f;
    if (panR > 0.9f) panR = 0.9f;

    for (int i = 0; i < frames; ++i) {
        /* Decorrelated noise for L vs R so the texture has a real stereo
         * image, not just a panned mono signal. */
        float n_air_L = lcg_noise(&m->lcg);
        float n_air_R = lcg_noise(&m->lcg);
        float n_body  = lcg_noise(&m->lcg);
        float n_dust  = lcg_noise(&m->lcg);

        /* AIR: high band-pass noise — share the BP filter but feed with
         * decorrelated noise; the filter state carries some correlation,
         * but a separate sample per channel is enough to break L==R. */
        float airL = dsp_svf_bp(&m->air_bp, n_air_L);
        float airR = dsp_svf_bp(&m->air_bp, n_air_R);

        /* BODY: low-pass noise. */
        float body = dsp_svf_lp(&m->body_lp, n_body * 0.5f);

        /* DUST: triggered envelope, bandpass on n_dust. */
        m->dust_rate_phase += dust_hz / SR;
        if (m->dust_rate_phase >= 1.0f) {
            m->dust_rate_phase -= 1.0f;
            if ((m->lcg & 0xFu) < 6u) m->dust_env = 1.0f;
            (void)lcg_noise(&m->lcg);
        }
        m->dust_env *= 0.992f;
        float dust = dsp_svf_bp(&m->dust_bp, n_dust) * m->dust_env;

        float Lout = airL * A_air + body * A_body + dust * A_dust;
        float Rout = airR * A_air + body * A_body + dust * A_dust;

        L[i] += Lout * panL * 2.0f;
        R[i] += Rout * panR * 2.0f;
    }
}
