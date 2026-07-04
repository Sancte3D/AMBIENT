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
#include "echo.h"
#include "blur.h"
#include "bass.h"
#include "drone.h"
#include "reverb_presets.h"
#include "brain.h"
#include "worlds.h"
#include "generative.h"
#include "cells.h"
#include "pluck.h"
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

/* Generative-bed state. Sparkle melody scheduler (r18.88): sources 14/15,
 * see the autoplay block above engine_generative_tick(). */
static bool gen_on = false;
#define SPARK_COUNT      2
#define SPARK_SRC0       14              /* sources 14/15 — see MAX_SOURCES */
#define GEN_TICK_BAR_MS  8000u
static bool     gen_timing_valid = false;
static uint32_t gen_next_bar_ms  = 0;
static uint32_t gen_tick_rng     = 0x5EEDBA55u;
static struct {
    bool     on_pending;
    uint32_t on_ms;
    float    hz, amp;
} spark[SPARK_COUNT];

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

/* r18.89 — master DRIVE stage. The DRIVE encoder used to reach only the
 * reverb-input saturation, so on a dry-ish patch the knob did almost
 * nothing. Now it drives the WHOLE mix through an asymmetric soft
 * saturator (dsp_drive_shape: tanh with a bias skew → even harmonics)
 * with small-signal makeup, placed BEFORE the DC blocker (the bias makes
 * a touch of DC — the blocker eats it) and before the tape stage, so
 * drive pushes INTO the tape knee like a real chain. 0 = bit-transparent
 * bypass. */
static float drive_cur, drive_tgt;

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
    echo_init();
    blur_init();
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
    gen_timing_valid = false;        /* r18.88 autoplay scheduler reset */
    memset(spark, 0, sizeof spark);

    /* Master stage: DC-block cleared, moderate default volume (no on-device
     * volume knob bound yet — keeps headphones from being slammed). */
    dc_x1L = dc_y1L = dc_x1R = dc_y1R = 0.0f;
    master_vol_cur = master_vol_tgt = 0.6f;
    drive_cur = drive_tgt = 0.0f;
    pluck_init();                    /* r18.89 sparkle plucks */

    /* Boot world 0 (Tokyo) — align the harmonic identity with the displayed
     * world so the first cell tap already plays in A-major, not the bare
     * C-ionian default. Sets brain key/mode/vibe + ambience world + reverb
     * preset; produces no sound (no notes held). */
    engine_set_world(0);
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
void engine_set_drive(float v)        { drive_tgt       = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_brightness(float hz)  { pad_set_brightness(hz); }
void engine_set_texture(float v)      { texture_set_amount(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_atmosphere(float v)   { ambience_set_level(dsp_clampf(v, 0.0f, 1.0f)); }
/* World change applies BOTH the texture layer (ambience) AND the harmonic
 * identity (key/mode/vibe) so each world sounds musically distinct, not just
 * texturally. The cell taps then play in the world's key/mode (brain), the
 * drone follows the root, and the reverb character shifts via the per-mode/
 * vibe preset table. Values live in worlds.c (audition-derived). */
void engine_set_world(int idx) {
    ambience_set_world(idx);
    const world_t *w = worlds_get(idx);
    engine_set_key ((int)w->key_midi);   /* brain key + drone root          */
    engine_set_mode((int)w->mode);       /* brain mode + reverb recompute   */
    engine_set_vibe((int)w->vibe);       /* brain vibe + reverb recompute   */
}
void engine_set_bass_depth(float v)   { bass_set_depth(dsp_clampf(v, 0.0f, 1.0f)); }

/* Perform-macros: combine multiple internal params under one user knob. */
void engine_set_motion(float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    /* user 0..1 → pad-LFO depth 0..2 (centre 0.5 = default movement). */
    pad_set_motion(v * 2.0f);
}
void engine_set_age(float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    /* user 0..1 → hiss 0..0.015 (≈ -36 dBFS at max) + sat drive 1.0..1.40.
     * v=0.30 lands near the dreamy_warm reference (hiss 0.005, drive 1.10). */
    tape_set_hiss_amount(v * 0.015f);
    tape_set_saturation_drive(1.0f + v * 0.40f);
    tape_set_crackle(v);             /* r18.89: vinyl ticks join the macro */
}

/* Echo macro — single setter, internally maps to time + feedback + wet +
 * tone in echo.c. See echo.c::recompute_internals for the mapping. */
void engine_set_echo(float v) {
    echo_set_amount(dsp_clampf(v, 0.0f, 1.0f));
}

/* Blur macro — granular smear. See blur.c::recompute_params for the
 * internal mapping (density + grain size + pitch jitter + wet). */
void engine_set_blur(float v) {
    blur_set_amount(dsp_clampf(v, 0.0f, 1.0f));
}

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

/* r18.88 AUDIT-FIX: this used to check only cells 0..4 — the Shift-octave
 * latch voices live on sources 9..13 (ADR-0008 r2, controls.c SHIFT_SRC),
 * so the generative bed kept playing OVER the user's held shift notes.
 * "User is playing" = any cell source, base or shift octave. The bed's own
 * voice (8) and the sparkle sources (14/15) are deliberately excluded. */
static bool any_user_note(void) {
    for (int i = 0; i < 5; ++i)  if (active_freq[i] > 0.0f) return true;
    for (int i = 9; i < 14; ++i) if (active_freq[i] > 0.0f) return true;
    return false;
}

/* --- Generative autoplay (r18.88) -----------------------------------------
 * The GENERATE modifier was always meant to make the instrument PLAY BY
 * ITSELF (passive mode = music without hands). The old wiring gave one
 * chord-root swell per fixed 8 s bar, with up to 8 s of silence after
 * enabling. engine_generative_tick() replaces the caller-side bar timer:
 *
 *   - the FIRST bed note sounds on the first tick after enabling (no wait),
 *   - bar length is humanized (±10 % per bar, LCG — never metronomic),
 *   - each bar scatters 0-2 "sparkle" chord tones an octave up (sources
 *     14/15, ~3 s ring, quiet), so the bed breathes as actual music,
 *   - live playing still overrides everything: while any user note is held
 *     (base or shift) no new bed/sparkle notes start; ringing sparkles are
 *     still released on schedule. When the user lets go, the bed resumes on
 *     the next tick.
 *
 * All timing derives from the passed now_ms — hardware-independent and
 * host-testable. The old engine_generative_advance() stays as the manual
 * step API (offline renderers, tests). State lives up by gen_on. */

static float gen_rand01(void) {
    gen_tick_rng = gen_tick_rng * 1664525u + 1013904223u;
    return (float)(gen_tick_rng >> 8) / 16777216.0f;
}

static void spark_silence_all(void) {
    /* Plucks self-decay in ~3 s — just cancel anything not yet fired. */
    for (int i = 0; i < SPARK_COUNT; ++i) spark[i].on_pending = false;
}

void engine_set_generative(bool on, int program) {
    generative_set_program(program);
    if (on && !gen_on) gen_timing_valid = false;   /* first tick plays NOW */
    gen_on = on;
    if (!on) {
        engine_note_off(GEN_SOURCE);    /* release the bed voice */
        spark_silence_all();
    }
}

int engine_generative_advance(void) {
    if (!gen_on) return -1;
    if (any_user_note()) return -1;          /* live playing overrides the bed */
    int deg  = generative_next_degree();     /* 1..7 (fixed prog 3 uses the 7th) */
    int midi = brain_cell_root(deg - 1);     /* root of that degree's voiced chord */
    engine_note_on((uint8_t)GEN_SOURCE, dsp_midi_to_hz((float)midi), GEN_VOICE_AMP);
    return deg;
}

void engine_generative_tick(uint32_t now_ms) {
    if (!gen_on) return;

    if (any_user_note()) {
        /* Player takes over: drop scheduled note-ons and re-arm the bar so
         * the bed resumes promptly (not up to 8 s late) after release. */
        for (int i = 0; i < SPARK_COUNT; ++i) spark[i].on_pending = false;
        gen_timing_valid = false;
        return;
    }

    if (!gen_timing_valid) {           /* just enabled / just released */
        gen_next_bar_ms  = now_ms;     /* fire the bar immediately */
        gen_timing_valid = true;
    }

    if ((int32_t)(now_ms - gen_next_bar_ms) >= 0) {
        int deg = engine_generative_advance();
        /* Humanized bar: 90..110 % of the base length. */
        uint32_t bar = GEN_TICK_BAR_MS * (uint32_t)(90 + (int)(gen_rand01() * 21.0f)) / 100u;
        if (deg > 0) {
            /* Scatter chord-tone sparkles inside this bar: an upper chord
             * tone (never the root), +1 octave, quiet, ~3 s ring. */
            int chord[BRAIN_MAX_CHORD];
            int n = brain_chord(deg, chord, BRAIN_MAX_CHORD);
            static const float POS_LO[SPARK_COUNT] = { 0.25f, 0.60f };
            static const float POS_HI[SPARK_COUNT] = { 0.55f, 0.85f };
            static const float PROB[SPARK_COUNT]   = { 0.75f, 0.40f };
            for (int i = 0; i < SPARK_COUNT && n > 1; ++i) {
                if (gen_rand01() >= PROB[i]) continue;
                int tone = chord[1 + (int)(gen_rand01() * (float)(n - 1)) % (n - 1)];
                spark[i].hz  = dsp_midi_to_hz((float)(tone + 12));
                spark[i].amp = 0.05f + gen_rand01() * 0.03f;
                spark[i].on_ms = now_ms + (uint32_t)((POS_LO[i] +
                                  gen_rand01() * (POS_HI[i] - POS_LO[i])) * (float)bar);
                spark[i].on_pending = true;
            }
        }
        gen_next_bar_ms = now_ms + bar;
    }

    for (int i = 0; i < SPARK_COUNT; ++i) {
        if (spark[i].on_pending && (int32_t)(now_ms - spark[i].on_ms) >= 0) {
            /* r18.89: sparkles are Karplus-Strong PLUCKS now, not pad
             * voices — a second instrument colour over the bed (bell/koto
             * blooming into the reverb) instead of "more pad". Plucks
             * self-decay (~3 s T60), so no note-off bookkeeping. */
            pluck_note(spark[i].hz, spark[i].amp * 2.2f);
            spark[i].on_pending = false;
        }
    }
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
    pluck_render_mix(dryL, dryR, sendL, sendR, frames);  /* r18.89 sparkles */

    /* Echo sits AFTER all generators but BEFORE the reverb so the delay
     * picks up the whole mix; send_amount=0.40 routes a copy of the
     * delayed signal into the reverb send so the echo also picks up room. */
    echo_render_mix(dryL, dryR, sendL, sendR, frames, 0.40f);

    /* Blur (granular cloud) — captures the dry+echo mix and re-emits as a
     * cloud of grains. Sits after echo so it grains the echoes too; before
     * the reverb so the cloud picks up room. send_amount=0.50 — cloud +
     * reverb is the classic "frozen-shimmer" wash. */
    blur_render_mix(dryL, dryR, sendL, sendR, frames, 0.50f);

    /* Tape character (ADR-0017 Phase 3): subtle hiss into the DRY bus only —
     * we don't want the reverb to amplify the noise floor. */
    tape_hiss_render_add(dryL, dryR, frames);

    /* r18.89: vinyl crackle (AGE macro) — dry bus only, like the hiss. */
    tape_crackle_render_add(dryL, dryR, frames);

    /* Reverb writes (does not add) wet from send. */
    reverb_render(sendL, sendR, wetL, wetR, frames);

    /* Master: sum + DC-block + volume → outL/outR; then tanh warmth
     * (block call); then soft-limit safety net → int16. Two-pass keeps
     * the saturation block-call cheap (one loop body, one tanh call/sample). */
    master_vol_cur += SMOOTH_COEF * (master_vol_tgt - master_vol_cur);
    drive_cur      += SMOOTH_COEF * (drive_tgt      - drive_cur);
    const float wa = wet_amp_cur;
    const float mv = master_vol_cur;

    /* r18.89 master drive (per block: curve params + makeup are constant
     * inside a 5.8 ms block; the amount itself is smoothed above). */
    const bool  drv_on   = drive_cur > 1.0e-3f;
    const float drv_g    = 1.0f + 3.5f * drive_cur;          /* 1 .. 4.5  */
    const float drv_bias = 0.28f * drive_cur;                /* asymmetry */
    const float drv_mk   = dsp_drive_makeup(drv_g, drv_bias);
    const float drv_mix  = drive_cur < 0.25f ? drive_cur * 4.0f : 1.0f;

    static float outL[BLOCK], outR[BLOCK];
    for (int n = 0; n < frames; ++n) {
        float L = dryL[n] + wetL[n] * wa;
        float R = dryR[n] + wetR[n] * wa;

        if (drv_on) {
            /* dry/wet fade over the first quarter of the knob so tiny
             * settings colour instead of switch. */
            float dL = dsp_drive_shape(L, drv_g, drv_bias) * drv_mk;
            float dR = dsp_drive_shape(R, drv_g, drv_bias) * drv_mk;
            L += drv_mix * (dL - L);
            R += drv_mix * (dR - R);
        }

        /* one-pole DC blocker per channel: y = x - x1 + R·y1 (also eats the
         * small DC offset the drive bias introduces) */
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
