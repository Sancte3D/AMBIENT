/*
 * engine_v2.c — wires harmony_field + field_voices + texture + diffuser +
 * mod_delay + (shared) reverb + beauty_guard into a single allocation-
 * free render loop.
 *
 * Signal flow per block (ADR-0014 §Phase 2):
 *
 *   harmony_field (control rate)
 *           │
 *           ▼
 *   8 field voices → mono mix → pan → L/R   (dry+send)
 *           │
 *           ▼              send only
 *   material_texture (add) ──────────────┐
 *           │                            │
 *           ▼                            ▼
 *   diffuser                          (no diffuser on texture: keeps Air pure)
 *           │
 *           ▼
 *   mod_delay      (parallel — adds wet, dry passes through)
 *           │
 *           ▼
 *   send_L,R  →  reverb_render → wet_L,R
 *           │
 *           ▼
 *   dry + wet*amp  → beauty_guard → master_volume → int16
 */

#include "v2/engine_v2.h"
#include "v2/harmony_field.h"
#include "v2/field_voice.h"
#include "v2/motion.h"
#include "v2/material_texture.h"
#include "v2/diffuser.h"
#include "v2/mod_delay.h"
#include "v2/beauty_guard.h"
#include "v2/worlds.h"

#include "dsp.h"
#include "reverb.h"
#include "audio.h"

#include <math.h>
#include <string.h>

#define BLOCK AUDIO_BUFFER_FRAMES
#define SR    44100.0f

typedef struct {
    int      world_id;
    int      center_midi;
    float    density, motion, color, blur, texture_amt, glow;
    float    master_volume;
    bool     freeze;
    motion_t mot;
    fv_state_t voices[HF_VOICE_COUNT];
    mt_state_t texture;
    diffuser_t diff;
    mod_delay_t mdelay;
    beauty_guard_t guard;

    /* Smoothed wet amount + per-block scratch. */
    float wet_amp;
    float wet_target;

    /* Reverb size/wet/damp/drive setpoints, smoothed via reverb's own
     * coefficient smoother (we just call reverb_set / reverb_set_drive). */
    const world_t *world;
} engine_v2_t;

static engine_v2_t E;

/* Scratch buffers — file-static so we never touch heap. */
static float dryL[BLOCK], dryR[BLOCK];
static float sendL[BLOCK], sendR[BLOCK];
static float wetL[BLOCK], wetR[BLOCK];

/* Apply world preset to engine setpoints. */
static void apply_world(const world_t *w) {
    E.world = w;
    /* Reverb. */
    float wet = w->reverb_wet_lo + (w->reverb_wet_hi - w->reverb_wet_lo) * E.blur;
    reverb_set(w->reverb_size_base + 0.15f * E.blur, w->reverb_damp_base);
    reverb_set_drive(0.10f);
    E.wet_target = wet;

    /* Texture layer balance. */
    mt_set_layer_balance(&E.texture, w->air_base, w->dust_base, w->body_base);

    /* Diffuser + mod_delay scale with Blur but also world base. */
    diffuser_set_amount(&E.diff, w->diffuser_amount * (0.5f + 0.5f * E.blur));
    mod_delay_set_amount(&E.mdelay, w->mod_delay_amount * (0.5f + 0.5f * E.blur));

    /* Harmony field constraints. */
    hf_set_voice_target_count(w->voice_target_count + (E.density - 0.5f) * 2.0f);
    hf_set_dissonance(w->dissonance_limit + E.density * 0.1f);
    hf_set_motion(E.motion);

    /* Motion engine speed. */
    motion_set_speed(&E.mot, w->motion_speed * (0.5f + 1.5f * E.motion));

    /* Re-assign voice types: primary on roles 0..3, secondary on 4..7. */
    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        fv_type_t t = (i < 4) ? w->primary_voice : w->secondary_voice;
        if (E.voices[i].type != t) {
            fv_reset(&E.voices[i], t, 0xBEEF0000u + i);
        }
    }
}

void engine_v2_init(uint32_t seed) {
    memset(&E, 0, sizeof E);
    E.center_midi = 57;  /* A3 — Dorian feel start */
    E.density = 0.5f;
    E.motion  = 0.4f;
    E.color   = 0.55f;
    E.blur    = 0.5f;
    E.texture_amt = 0.20f;
    E.glow    = 0.20f;
    E.master_volume = 0.7f;
    E.freeze  = false;
    E.wet_amp = 0.30f;
    E.wet_target = 0.30f;

    motion_init(&E.mot, seed);
    mt_init(&E.texture, seed ^ 0xABCD);
    diffuser_init(&E.diff);
    mod_delay_init(&E.mdelay);
    bg_init(&E.guard);
    reverb_init();
    hf_init(seed ^ 0x4444, E.center_midi);

    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        fv_reset(&E.voices[i], FV_GLASS, 0xBEEF0000u + i);
    }
    apply_world(worlds_get(WORLD_FOG));
    E.world_id = WORLD_FOG;
}

void engine_v2_set_world(int world_id) {
    if (world_id < 0 || world_id >= worlds_count()) return;
    E.world_id = world_id;
    apply_world(worlds_get(world_id));
}

void engine_v2_set_center(int center_midi) {
    if (center_midi < 24) center_midi = 24;
    if (center_midi > 96) center_midi = 96;
    E.center_midi = center_midi;
    hf_set_center(center_midi);
}

static float clamp01(float v) { if (v<0) return 0; if (v>1) return 1; return v; }

void engine_v2_set_density(float n)  { E.density = clamp01(n);   if (E.world) apply_world(E.world); }
void engine_v2_set_motion(float n)   { E.motion  = clamp01(n);   if (E.world) apply_world(E.world); }
void engine_v2_set_color(float n)    { E.color   = clamp01(n); }
void engine_v2_set_blur(float n)     { E.blur    = clamp01(n);   if (E.world) apply_world(E.world); }
void engine_v2_set_texture(float n)  { E.texture_amt = clamp01(n); }
void engine_v2_set_glow(float n)     { E.glow    = clamp01(n); }
void engine_v2_set_master_volume(float n) { E.master_volume = clamp01(n); }
void engine_v2_set_freeze(bool on)   { E.freeze = on; }

void engine_v2_new_field(uint32_t seed) {
    hf_new_field(seed ? seed : 0xDEADu);
}

int engine_v2_active_voices(void) {
    return hf_active_count();
}

static inline int16_t to_i16(float x) {
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return (int16_t)(x * 32767.0f);
}

void engine_v2_render(int16_t *buf, int frames) {
    if (frames > BLOCK) frames = BLOCK;

    float dt_block = (float)frames / SR;

    /* Control-rate: advance motion + harmony (unless frozen). */
    motion_advance(&E.mot, dt_block);
    if (!E.freeze) {
        hf_advance(dt_block);
    }

    /* Effective Color = base color * world ceiling-floor + glow lift. */
    const world_t *w = E.world;
    float color_eff = w->color_floor +
                      (w->color_ceiling - w->color_floor) * E.color;
    float glow_lift = E.glow * 0.2f;
    color_eff = color_eff + glow_lift;
    if (color_eff > 1.0f) color_eff = 1.0f;

    float mot_walk = motion_walk(&E.mot);
    float mot_slow = motion_slow1(&E.mot);

    /* Per-block control reads from motion to add to voice freq smoothly.
     * We'll use mot_slow as a tiny pitch drift (±0.5%) applied to all voices. */
    float pitch_drift = 1.0f + mot_slow * 0.005f;

    /* Clear scratch. */
    for (int i = 0; i < frames; ++i) {
        dryL[i] = dryR[i] = 0.0f;
        sendL[i] = sendR[i] = 0.0f;
    }

    /* Sum 8 field voices into dry + send. */
    for (int v = 0; v < HF_VOICE_COUNT; ++v) {
        const hf_voice_t *hv = hf_voice(v);
        if (!hv || !hv->active || hv->amp < 0.0005f) continue;

        float f = hv->freq_hz * pitch_drift;
        float a = hv->amp;
        float pan = hv->pan;
        float gL = 0.5f * (1.0f - pan);
        float gR = 0.5f * (1.0f + pan);

        fv_state_t *st = &E.voices[v];

        /* Per-voice reverb send: bass less, high partials more (the wet
         * field shimmers, the bass stays dry). */
        float send_scale =
            (v == HF_VOICE_BASS)         ? 0.10f :
            (v == HF_VOICE_HIGH_PARTIAL) ? 0.85f : 0.55f;
        /* Glow boosts upper-voice send. */
        if (v >= HF_VOICE_NINTH) send_scale += E.glow * 0.2f;

        for (int i = 0; i < frames; ++i) {
            float s = fv_render(st, f, a, color_eff, mot_walk, mot_slow);
            dryL[i] += s * gL;
            dryR[i] += s * gR;
            sendL[i] += s * gL * send_scale;
            sendR[i] += s * gR * send_scale;
        }
    }

    /* Texture: adds to BOTH dry and send (texture goes through reverb too). */
    mt_render_add(&E.texture, dryL, dryR, frames, E.texture_amt, mot_walk);
    /* A muted copy into send for distant texture tail. */
    for (int i = 0; i < frames; ++i) {
        sendL[i] += dryL[i] * 0.0f;  /* texture already in dry; send stays voice-only */
        sendR[i] += dryR[i] * 0.0f;
    }

    /* Diffuser on dry path (smears voice attacks into a cloud). */
    diffuser_process(&E.diff, dryL, dryR, frames);

    /* Mod Delay on dry path (parallel — adds wet, dry remains). */
    mod_delay_process(&E.mdelay, dryL, dryR, frames);

    /* Reverb on send path → wetL/R. */
    reverb_render(sendL, sendR, wetL, wetR, frames);

    /* Smooth wet amp toward target (~120 ms). */
    float wet_coef = 1.0f - expf(-1.0f / (0.120f * SR / (float)frames));
    E.wet_amp += wet_coef * (E.wet_target - E.wet_amp);

    float vol = E.master_volume * 1.1f;   /* small headroom into BG */

    /* Mix dry + wet, apply master volume into beauty guard. */
    for (int i = 0; i < frames; ++i) {
        dryL[i] = (dryL[i] + wetL[i] * E.wet_amp) * vol;
        dryR[i] = (dryR[i] + wetR[i] * E.wet_amp) * vol;
    }

    /* Beauty Guard last — soft compress + tanh limiter. */
    bg_process(&E.guard, dryL, dryR, frames);

    /* Interleave to int16. */
    for (int i = 0; i < frames; ++i) {
        buf[2*i + 0] = to_i16(dryL[i]);
        buf[2*i + 1] = to_i16(dryR[i]);
    }
}
