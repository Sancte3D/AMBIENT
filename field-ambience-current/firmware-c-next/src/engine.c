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
#include "bass.h"
#include "reverb_presets.h"
#include "brain.h"
#include "dsp.h"
#include "audio.h"                    /* AUDIO_BUFFER_FRAMES */
#include <math.h>
#include <string.h>

/* Per-layer reverb sends (match the webapp's per-voice verbSend values).
 * Pad uses the user-tunable engine_set_send; texture has its own fixed send;
 * the bass applies its two per-layer sends internally. */
#define TEXTURE_SEND  0.55f

/* Active note tracking so the bass can follow the lowest held pitch. Sources
 * are cell indices today (0..4), with headroom for MIDI later. freq 0 = idle. */
#define MAX_SOURCES   16
static float active_freq[MAX_SOURCES];

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
    bass_init();

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
}

void engine_note_on(uint8_t source, float freq_hz, float amp) {
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
void engine_set_brightness(float hz)  { pad_set_brightness(hz); }
void engine_set_texture(float v)      { texture_set_amount(dsp_clampf(v, 0.0f, 1.0f)); }
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
    bass_render_mix(dryL, dryR, sendL, sendR, frames);

    /* Reverb writes (does not add) wet from send. */
    reverb_render(sendL, sendR, wetL, wetR, frames);

    /* Sum + master soft-clip → int16. */
    const float wa = wet_amp_cur;
    for (int n = 0; n < frames; ++n) {
        float L = dryL[n] + wetL[n] * wa;
        float R = dryR[n] + wetR[n] * wa;
        L = tanhf(L * 0.9f);
        R = tanhf(R * 0.9f);
        buf[n * 2 + 0] = (int16_t)(L * 32767.0f);
        buf[n * 2 + 1] = (int16_t)(R * 32767.0f);
    }
}

int engine_active_voices(void) { return pad_active_count(); }
