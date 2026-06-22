/*
 * Audio engine — Step 11 mix-bus owner.
 *
 * Static float scratch buffers (BLOCK frames) keep allocation out of the
 * audio path. BLOCK matches AUDIO_BUFFER_FRAMES (256) — audio.c calls in
 * chunks of that size, so we always process a whole audio block in one pass.
 *
 * `send` and `wet_amp` are smoothed per block (~120 ms) to avoid zipper
 * when the user knobs them; size/damp/drive are smoothed inside the reverb
 * itself.
 */

#include "engine.h"
#include "pad.h"
#include "reverb.h"
#include "texture.h"
#include "ambience.h"
#include "tape.h"
#include "bass.h"
#include "drone.h"
#include "reverb_presets.h"
#include "brain.h"
#include "generative.h"
#include "cells.h"
#include "dsp.h"
#include "audio.h"                    /* AUDIO_BUFFER_FRAMES */
#include <math.h>
#include <string.h>

/* Per-layer reverb sends (match the webapp's per-voice verbSend values).
 * Pad uses the user-tunable engine_set_send; texture has its own fixed send;
 * the bass applies its two per-layer sends internally. */
#define TEXTURE_SEND  0.55f
#define AMBIENCE_SEND 0.35f           /* slightly less wet than texture — ADR-0017 */

/* Active note tracking so the bass can follow the lowest held pitch. Sources
 * are cell indices today (0..4), with headroom for MIDI later. freq 0 = idle. */
#define MAX_SOURCES   16
#define GEN_SOURCE    8           /* reserved pad-voice source for the bed */
#define GEN_VOICE_AMP 0.10f
static float active_freq[MAX_SOURCES];

/* Generative-bed state. */
static bool gen_on = false;

/* Lowest currently-held frequency, or 0 if nothing is held. */
static float lowest_held(void) {
    float lo = 0.0f;
    for (int i = 0; i < MAX_SOURCES; ++i) {
        float f = active_freq[i];
        if (f > 0.0f && (lo == 0.0f || f < lo)) lo = f;
    }
    return lo;
}

/* Re-point the bass at the current lowest note, or release it if none held. */
static void refresh_bass(void) {
    float lo = lowest_held();
    if (lo > 0.0f) bass_note(lo);
    else           bass_release();
}

#define BLOCK     AUDIO_BUFFER_FRAMES

static float dryL [BLOCK];
static float dryR [BLOCK];
static float sendL[BLOCK];
static float sendR[BLOCK];
static float wetL [BLOCK];
static float wetR [BLOCK];

static float send_amount_cur, send_amount_tgt;
static float wet_amp_cur,     wet_amp_tgt;
static float reverb_size, reverb_damp;        /* cached, so the two setters
                                                  can change one independently */
static const float SMOOTH_COEF = 0.05f;       /* per-block, ~120 ms time-const */

/* Master stage (fixes the listening-test "earrape / brummt"):
 *   1. one-pole DC blocker (~35 Hz highpass) removes DC + subsonic rumble that
 *      otherwise builds up from the brown-noise bed, the bass and the reverb
 *      feedback — this is the "LeakDC" the Step-11 plan called for.
 *   2. master volume gives headroom (no on-device volume knob yet).
 *   3. a soft limiter that is PERFECTLY LINEAR below the knee and only rounds
 *      true peaks — replaces the old tanf() that distorted everything above
 *      ~0.5 (that continuous saturation was the harshness). */
#define DC_R 0.995f                           /* one-pole HP, ≈35 Hz at 44.1 k */
static float dc_x1L, dc_y1L, dc_x1R, dc_y1R;
static float master_vol_cur, master_vol_tgt;

static inline float soft_limit(float x) {
    const float k = 0.75f;                    /* clean below this */
    float a = x < 0.0f ? -x : x;
    if (a <= k) return x;                     /* transparent for normal levels */
    float over = a - k;                       /* smoothly map (k..∞) → (k..1) */
    float comp = k + (1.0f - k) * (over / (over + (1.0f - k)));
    return x < 0.0f ? -comp : comp;
}

/* Step 12b #1 — musical state driving the preset-based reverb mapping. */
static int   musical_mode  = 0;     /* ionian */
static int   musical_vibe  = 0;     /* warm */
static float musical_space = 0.5f;
static float musical_mood  = 0.5f;

/* Recompute Freeverb settings from current mode/vibe/space/mood and push to
 * the reverb. Cached size/damp keep manual reverb setters orthogonal. */
static void recompute_reverb_from_presets(void) {
    reverb_settings_t s = reverb_presets_compute(musical_mode, musical_vibe,
                                                 musical_space, musical_mood);
    reverb_size = s.size;
    reverb_damp = s.damp;
    reverb_set(reverb_size, reverb_damp);
    reverb_set_drive(s.drive);
    wet_amp_tgt = s.wet_amp;        /* still smoothed per block in render */
}

void engine_init(void) {
    pad_init();
    reverb_init();
    texture_init();
    ambience_init();
    tape_init();
    bass_init();
    drone_init();

    /* Musical state defaults: C ionian / warm / space=mood=0.5. The reverb
     * parameters fall out of the preset table — same shape as the webapp's
     * mid-mode landing point. */
    musical_mode  = 0;
    musical_vibe  = 0;
    musical_space = 0.5f;
    musical_mood  = 0.5f;
    send_amount_cur = send_amount_tgt = 0.45f;
    wet_amp_cur     = wet_amp_tgt     = 0.40f;
    recompute_reverb_from_presets();
    wet_amp_cur = wet_amp_tgt;       /* snap at boot, no glide-from-silence */

    /* Texture bed boots at 0 (silent power-up); raise via engine_set_texture
     * or the brain in Step 12. */
    texture_set_amount(0.0f);
    bass_set_depth(0.5f);
    memset(active_freq, 0, sizeof active_freq);
    generative_init();
    cells_init();                    /* ADR-0013 cell-velocity state */
    gen_on = false;

    /* Master stage: DC-block cleared, moderate default volume (no on-device
     * volume knob bound yet — keeps headphones from being slammed). */
    dc_x1L = dc_y1L = dc_x1R = dc_y1R = 0.0f;
    master_vol_cur = master_vol_tgt = 0.6f;
}

/* Tier A #2: tiny LCG for micro-humanisation. Inside JND so it doesn't drift
 * audibly, but enough that two consecutive identical cell taps aren't
 * bit-identical → no "mechanical" feel on repeats. */
static uint32_t humanize_rng = 0xA5C3F19Du;
static inline float humanize_rand_unit(void){      /* in [-1, +1] */
    humanize_rng = humanize_rng * 1664525u + 1013904223u;
    return ((int32_t)humanize_rng) * (1.0f / 2147483648.0f);
}

void engine_note_on(uint8_t source, float freq_hz, float amp) {
    /* ±0.5 cent pitch jitter, ±0.3 % amp jitter. Bass / drone get the same
     * freq downstream (refresh_bass) so the jitter is consistent per press. */
    float pitch_jitter = humanize_rand_unit() * (0.5f / 1200.0f);   /* cents */
    float amp_jitter   = humanize_rand_unit() * 0.003f;
    freq_hz *= 1.0f + pitch_jitter;                    /* 2^(jit) ≈ 1+jit at tiny jit */
    amp     *= 1.0f + amp_jitter;
    pad_note_on(source, freq_hz, amp);
    if (source < MAX_SOURCES) active_freq[source] = freq_hz;
    refresh_bass();
}
void engine_note_off(uint8_t source) {
    pad_note_off(source);
    if (source < MAX_SOURCES) active_freq[source] = 0.0f;
    refresh_bass();
}
void engine_all_off(void) {
    pad_all_off();
    memset(active_freq, 0, sizeof active_freq);
    bass_release();
}

/* ADR-0013 — Hall cell sample → velocity note. The cell index doubles as the
 * pad-voice source (0..4), matching the digital-cell path, so re-pressing a
 * cell re-blooms its own voice rather than stacking. */
bool engine_cell_sample(uint8_t cell, float pos_0_1, uint32_t now_ms) {
    cell_event_t ev = cells_update(cell, pos_0_1, now_ms);
    if (ev.kind == CELL_EVENT_PRESS) {
        int midi = brain_cell_root(ev.cell);
        engine_note_on(ev.cell, dsp_midi_to_hz((float)midi), ev.amp);
        return true;
    }
    if (ev.kind == CELL_EVENT_RELEASE) {
        engine_note_off(ev.cell);
        return true;
    }
    return false;
}

void engine_set_reverb_size(float v) {
    reverb_size = dsp_clampf(v, 0.0f, 1.0f);
    reverb_set(reverb_size, reverb_damp);
}
void engine_set_reverb_damp(float v) {
    reverb_damp = dsp_clampf(v, 0.0f, 1.0f);
    reverb_set(reverb_size, reverb_damp);
}
void engine_set_reverb_drive(float v) { reverb_set_drive(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_wet_amp(float v)      { wet_amp_tgt    = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_send(float v)         { send_amount_tgt = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_master_volume(float v){ master_vol_tgt  = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_brightness(float hz)  { pad_set_brightness(hz); }
void engine_set_texture(float v)      { texture_set_amount(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_atmosphere(float v)   { ambience_set_level(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_world(int idx)        { ambience_set_world(idx); }
void engine_set_bass_depth(float v)   { bass_set_depth(dsp_clampf(v, 0.0f, 1.0f)); }

/* Step 12b #1 — musical-state setters. Each triggers a preset recompute so
 * the reverb shifts character with the mode/vibe/macro change. The reverb
 * itself smooths internally (~120 ms), so the transition is glide-not-step
 * — matches the "sound darf nicht konkurrieren" rule for global params.
 * Also keeps the harmonic brain's view of mode/vibe in sync so cells played
 * after the change pick up the new harmony. */
void engine_set_mode(int mode_idx) {
    if (mode_idx < 0)               mode_idx = 0;
    if (mode_idx >= RP_MODE_COUNT)  mode_idx = RP_MODE_COUNT - 1;
    musical_mode = mode_idx;
    brain_set_mode(mode_idx);
    recompute_reverb_from_presets();
}
void engine_set_vibe(int vibe_idx) {
    if (vibe_idx < 0)               vibe_idx = 0;
    if (vibe_idx >= RP_VIBE_COUNT)  vibe_idx = RP_VIBE_COUNT - 1;
    musical_vibe = vibe_idx;
    brain_set_vibe(vibe_idx);
    recompute_reverb_from_presets();
}
void engine_set_space(float v) {
    musical_space = dsp_clampf(v, 0.0f, 1.0f);
    recompute_reverb_from_presets();
}
void engine_set_mood(float v) {
    musical_mood = dsp_clampf(v, 0.0f, 1.0f);
    recompute_reverb_from_presets();
}
void engine_set_key(int tonic_midi) {
    brain_set_key(tonic_midi);
    drone_set_root_midi(tonic_midi);   /* glides live if the drone is sounding */
}
void engine_set_drone(bool on) { drone_enable(on); }

/* PAD_VOICE_MIXES from the webapp: warm / strings / brass. */
void engine_set_pad_voice(int voice_idx) {
    static const float MIX[] = { 0.0f, 0.6f, 1.2f };
    if (voice_idx < 0) voice_idx = 0;
    if (voice_idx > 2) voice_idx = 2;
    pad_set_voice_mix(MIX[voice_idx]);
}

static bool any_cell_held(void) {
    for (int i = 0; i < 5; ++i) if (active_freq[i] > 0.0f) return true;
    return false;
}

void engine_set_generative(bool on, int program) {
    generative_set_program(program);
    gen_on = on;
    if (!on) engine_note_off(GEN_SOURCE);   /* release the bed voice */
}

int engine_generative_advance(void) {
    if (!gen_on) return -1;
    if (any_cell_held()) return -1;          /* live playing overrides the bed */
    int deg  = generative_next_degree();     /* 1..6 */
    int midi = brain_cell_root(deg - 1);     /* root of that degree's voiced chord */
    engine_note_on((uint8_t)GEN_SOURCE, dsp_midi_to_hz((float)midi), GEN_VOICE_AMP);
    return deg;
}

void engine_render(int16_t *buf, int frames) {
    /* audio.c always calls with frames == AUDIO_BUFFER_FRAMES, but be safe. */
    if (frames > BLOCK) frames = BLOCK;

    /* Smooth per-block engine controls. */
    send_amount_cur += SMOOTH_COEF * (send_amount_tgt - send_amount_cur);
    wet_amp_cur     += SMOOTH_COEF * (wet_amp_tgt     - wet_amp_cur);

    /* Clear the dry + send accumulators (pad ADDS into them). */
    memset(dryL,  0, sizeof(float) * frames);
    memset(dryR,  0, sizeof(float) * frames);
    memset(sendL, 0, sizeof(float) * frames);
    memset(sendR, 0, sizeof(float) * frames);

    pad_render_mix(dryL, dryR, sendL, sendR, frames, send_amount_cur);
    texture_render_mix(dryL, dryR, sendL, sendR, frames, TEXTURE_SEND);
    ambience_render_mix(dryL, dryR, sendL, sendR, frames, AMBIENCE_SEND);
    bass_render_mix(dryL, dryR, sendL, sendR, frames);
    drone_render_mix(dryL, dryR, sendL, sendR, frames);

    /* Tape character (ADR-0017 Phase 3): subtle hiss into the DRY bus only —
     * we don't want the reverb to amplify the noise floor. */
    tape_hiss_render_add(dryL, dryR, frames);

    /* Reverb writes (does not add) wet from send. */
    reverb_render(sendL, sendR, wetL, wetR, frames);

    /* Master: sum + DC-block + volume → outL/outR; then tanh warmth
     * (block call); then soft-limit safety net → int16. Two-pass keeps
     * the saturation block-call cheap (one loop body, one tanh call/sample). */
    master_vol_cur += SMOOTH_COEF * (master_vol_tgt - master_vol_cur);
    const float wa = wet_amp_cur;
    const float mv = master_vol_cur;

    static float outL[BLOCK], outR[BLOCK];
    for (int n = 0; n < frames; ++n) {
        float L = dryL[n] + wetL[n] * wa;
        float R = dryR[n] + wetR[n] * wa;

        /* one-pole DC blocker per channel: y = x - x1 + R·y1 */
        float yL = L - dc_x1L + DC_R * dc_y1L; dc_x1L = L; dc_y1L = yL;
        float yR = R - dc_x1R + DC_R * dc_y1R; dc_x1R = R; dc_y1R = yR;

        outL[n] = yL * mv;
        outR[n] = yR * mv;
    }
    /* Warm tanh saturation — "tape" colour. Always-on at the dreamy_warm
     * reference drive (1.10). Peaks roll into the tanh knee, even harmonics
     * appear, makeup-gain implicit (×0.78 inside). */
    tape_saturation_process(outL, outR, frames);
    for (int n = 0; n < frames; ++n) {
        /* soft_limit is now a safety net for the rare residual peak above
         * 0.75 — saturation usually already keeps us in range. */
        float yL = soft_limit(outL[n]);
        float yR = soft_limit(outR[n]);
        buf[n * 2 + 0] = (int16_t)(yL * 32767.0f);
        buf[n * 2 + 1] = (int16_t)(yR * 32767.0f);
    }
}

int engine_active_voices(void) { return pad_active_count(); }
