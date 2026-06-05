/*
 * Per-Mode reverb presets + per-Vibe bias + Space/Mood macros.
 *
 * Webapp parameters are abstract ({t60, damp, size, high} for a generated
 * IR). Our Freeverb wants {size, damp, drive, wet_amp}. The mapping at the
 * bottom of this file is the bridge — chosen so the FEEL of each mode/vibe
 * combination transfers, not the literal numbers.
 */

#include "reverb_presets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Webapp's REVERB_MODE_PRESETS, 1:1 in mode order. */
typedef struct { float t60, damp, size, high; } rp_preset_t;

static const rp_preset_t MODE_PRESETS[RP_MODE_COUNT] = {
    /* t60   damp  size  high   ← mode */
    { 2.8f, 0.40f, 0.7f,  0.90f }, /* ionian     */
    { 2.2f, 0.50f, 0.6f,  0.70f }, /* dorian     */
    { 1.8f, 0.60f, 0.5f,  0.55f }, /* phrygian   */
    { 4.5f, 0.25f, 0.85f, 1.10f }, /* lydian     */
    { 3.0f, 0.35f, 0.75f, 0.95f }, /* mixolydian */
    { 2.4f, 0.55f, 0.6f,  0.65f }, /* aeolian    */
};

/* Webapp's VIBE_REVERB_BIAS, 1:1 in vibe order. */
static const rp_preset_t VIBE_BIAS[RP_VIBE_COUNT] = {
    /* t60   damp  size  high   ← vibe */
    { 1.00f, 1.00f, 1.00f, 0.70f }, /* warm     */
    { 1.30f, 0.60f, 1.10f, 1.40f }, /* bright   */
    { 0.85f, 1.40f, 0.90f, 0.50f }, /* deep     */
    { 1.60f, 0.70f, 1.25f, 1.20f }, /* floating */
};

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}
static int clampi(int x, int lo, int hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

/* Webapp's linlin() helper, faithful port. */
static float linlin(float x, float in_lo, float in_hi, float out_lo, float out_hi) {
    x = clampf(x, in_lo, in_hi);
    return out_lo + (x - in_lo) * (out_hi - out_lo) / (in_hi - in_lo);
}

reverb_settings_t reverb_presets_compute(int mode_idx, int vibe_idx,
                                         float space, float mood) {
    mode_idx = clampi(mode_idx, 0, RP_MODE_COUNT - 1);
    vibe_idx = clampi(vibe_idx, 0, RP_VIBE_COUNT - 1);
    space = clampf(space, 0.0f, 1.0f);
    mood  = clampf(mood,  0.0f, 1.0f);

    const rp_preset_t *m = &MODE_PRESETS[mode_idx];
    const rp_preset_t *b = &VIBE_BIAS[vibe_idx];

    /* Same shape as webapp computeReverb(): the space macro splits at 0.5
     * (below → ducked tail/room, above → expanded), mood gently widens. */
    float spaceTailMul = (space <= 0.5f) ? linlin(space, 0.0f, 0.5f, 0.45f, 1.0f)
                                         : linlin(space, 0.5f, 1.0f, 1.0f, 1.9f);
    float spaceSizeMul = (space <= 0.5f) ? linlin(space, 0.0f, 0.5f, 0.70f, 1.0f)
                                         : linlin(space, 0.5f, 1.0f, 1.0f, 1.25f);
    float moodSizeMul  = linlin(mood, 0.0f, 1.0f, 0.90f, 1.10f);

    /* Webapp-shape intermediate values (t60 seconds, damp 0..1, size 0..5, high 0..~1.5). */
    float t60  = m->t60  * b->t60  * spaceTailMul;
    float damp = clampf(m->damp * b->damp, 0.0f, 1.0f);
    float size = clampf(m->size * b->size * spaceSizeMul * moodSizeMul, 0.5f, 5.0f);
    float high = clampf(m->high * b->high, 0.0f, 1.5f);

    /* ---- Map webapp-shape → Freeverb-shape ----
     * The Freeverb size_0_1 maps roughly to "tail length perception". The
     * webapp has BOTH t60 and size driving this. We average their normalised
     * versions: t60 (~0.3..8 s) maps to 0..1 over the usable musical band
     * (1..6 s), and size (0.5..5) maps to 0..1. The blend captures both
     * preset's intent without forcing the user to know about either knob.
     */
    float t60_norm  = clampf((t60 - 1.0f) / 5.0f, 0.0f, 1.0f);     /* 1s→0, 6s→1 */
    float size_norm = clampf((size - 0.5f) / 4.5f, 0.0f, 1.0f);    /* 0.5→0, 5→1 */
    float fv_size   = clampf(0.5f * t60_norm + 0.5f * size_norm, 0.05f, 1.0f);

    /* Damp passes through (already 0..1 in both worlds). Add a tiny amount
     * of inverse-high so a "bright" vibe sounds bright (less damp). */
    float fv_damp = clampf(damp - 0.10f * (high - 0.70f), 0.0f, 1.0f);

    /* Drive: not a webapp param. Tie to vibe + mood so bright/floating
     * vibes get a touch more harmonic warmth, deep vibe stays cleaner. */
    float drive_base = 0.10f + b->high * 0.10f;   /* warm 0.17, bright 0.24, deep 0.15, floating 0.22 */
    float fv_drive = clampf(drive_base + mood * 0.08f, 0.0f, 0.5f);

    /* Wet amount matches the webapp's linlin(space, 0,1, 0.40, 0.70). */
    float fv_wet = linlin(space, 0.0f, 1.0f, 0.40f, 0.70f);

    reverb_settings_t out = { fv_size, fv_damp, fv_drive, fv_wet };
    return out;
}

char *reverb_presets_describe(int mode_idx, int vibe_idx, char *buf, int bufsz) {
    static const char *MODE_NAME[] = { "ionian","dorian","phrygian","lydian","mixolyd","aeolian" };
    static const char *VIBE_NAME[] = { "warm","bright","deep","floating" };
    mode_idx = clampi(mode_idx, 0, RP_MODE_COUNT - 1);
    vibe_idx = clampi(vibe_idx, 0, RP_VIBE_COUNT - 1);
    reverb_settings_t s = reverb_presets_compute(mode_idx, vibe_idx, 0.5f, 0.5f);
    snprintf(buf, (size_t)bufsz, "%-9s %-8s -> size=%.3f damp=%.3f drive=%.3f wet=%.3f",
             MODE_NAME[mode_idx], VIBE_NAME[vibe_idx],
             (double)s.size, (double)s.damp, (double)s.drive, (double)s.wet_amp);
    return buf;
}
