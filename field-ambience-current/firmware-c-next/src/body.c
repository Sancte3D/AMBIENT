/*
 * body.c — modal resonant body. See body.h for the concept notes.
 */

#include "body.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR ((float)DSP_SAMPLE_RATE_HZ)

/* One material: base frequency + up to 8 (ratio, T60 s, gain) modes.
 * These numbers are THIS instrument's tuning — the material identities
 * follow the worlds (Tokyo wood, Coast glass, Drive dark metal, Hours
 * felt), the ratio flavours follow the physics rules of thumb: wood =
 * dense inharmonic, glass/bars = stretched, metal = clustered low,
 * felt = few fast-dying modes. */
typedef struct { float ratio, t60, gain; } mode_def_t;
typedef struct {
    float base_hz;
    int   n;
    mode_def_t m[BODY_MAX_MODES];
} material_t;

static const material_t MATERIALS[4] = {
    { 180.0f, 8, {  /* Tokyo — warm wood, dense inharmonic */
        { 1.00f, 0.80f, 1.00f }, { 1.47f, 0.65f, 0.70f },
        { 2.09f, 0.50f, 0.55f }, { 2.56f, 0.42f, 0.50f },
        { 3.24f, 0.34f, 0.40f }, { 4.07f, 0.28f, 0.30f },
        { 5.13f, 0.24f, 0.25f }, { 6.33f, 0.20f, 0.20f } } },
    { 240.0f, 6, {  /* Coast — glass, stretched partials, longer ring */
        { 1.00f, 1.20f, 1.00f }, { 2.32f, 0.95f, 0.65f },
        { 4.25f, 0.70f, 0.50f }, { 6.63f, 0.50f, 0.35f },
        { 9.38f, 0.35f, 0.25f }, { 12.10f, 0.25f, 0.15f } } },
    { 140.0f, 6, {  /* Drive — dark metal, clustered low modes */
        { 1.00f, 0.55f, 1.00f }, { 1.58f, 0.45f, 0.75f },
        { 2.24f, 0.38f, 0.55f }, { 2.92f, 0.30f, 0.40f },
        { 3.91f, 0.24f, 0.28f }, { 5.02f, 0.18f, 0.18f } } },
    { 160.0f, 4, {  /* Hours — felt/paper, soft fast-dying thump */
        { 1.00f, 0.28f, 1.00f }, { 1.79f, 0.20f, 0.60f },
        { 2.67f, 0.15f, 0.40f }, { 3.46f, 0.10f, 0.25f } } },
};

/* Two-pole resonator state per mode per channel:
 *   y = 2 R cosθ · y1 − R² · y2 + g (x − x2)
 * (the x − x2 zero pair keeps the DC and Nyquist gains at zero — a
 * band-ish mode, not a peaking lowpass). */
typedef struct { float a1, a2, g, y1, y2, x1, x2; } res_t;

static res_t s_res[2][BODY_MAX_MODES];
static int   s_n = 0;
static float s_wet = 0.38f;
static int   s_world = -1;

static void mode_setup(res_t *r, float f_hz, float t60, float gain) {
    if (f_hz > SR * 0.45f) { r->g = 0.0f; r->a1 = r->a2 = 0.0f; return; }
    float R  = expf(-6.91f / (t60 * SR));          /* −60 dB in t60      */
    float th = 6.28318530718f * f_hz / SR;
    r->a1 = 2.0f * R * cosf(th);
    r->a2 = -R * R;
    /* normalise so a unit impulse rings at ≈`gain` peak: the resonator's
     * peak response scales like 1/(1−R) — pre-scale it away. */
    r->g  = gain * (1.0f - R);
    r->y1 = r->y2 = r->x1 = r->x2 = 0.0f;
}

void body_set_world(int world_idx) {
    if (world_idx < 0) world_idx = 0;
    if (world_idx > 3) world_idx = 3;
    if (world_idx == s_world) return;
    s_world = world_idx;
    const material_t *mt = &MATERIALS[world_idx];
    s_n = mt->n;
    for (int m = 0; m < s_n; ++m) {
        float f = mt->base_hz * mt->m[m].ratio;
        mode_setup(&s_res[0][m], f,          mt->m[m].t60, mt->m[m].gain);
        mode_setup(&s_res[1][m], f * 1.007f, mt->m[m].t60, mt->m[m].gain);
    }
}

void body_init(void) {
    s_world = -1;
    s_wet   = 0.38f;
    memset(s_res, 0, sizeof s_res);
    body_set_world(0);
}

void body_set_amount(float v01) {
    s_wet = dsp_clampf(v01, 0.0f, 1.0f);
}

void body_process(float *L, float *R, int frames) {
    if (s_wet <= 0.0f) return;                     /* bit-exact bypass   */
    const float wet = s_wet, dry = 1.0f;           /* body ADDS resonance */
    for (int n = 0; n < frames; ++n) {
        float x[2] = { L[n], R[n] };
        float y[2] = { 0.0f, 0.0f };
        for (int c = 0; c < 2; ++c) {
            for (int m = 0; m < s_n; ++m) {
                res_t *r = &s_res[c][m];
                float v = r->a1 * r->y1 + r->a2 * r->y2
                        + r->g * (x[c] - r->x2);
                r->y2 = r->y1; r->y1 = v;
                r->x2 = r->x1; r->x1 = x[c];
                y[c] += v;
            }
        }
        L[n] = x[0] * dry + y[0] * wet;
        R[n] = x[1] * dry + y[1] * wet;
    }
}
