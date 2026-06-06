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

/* Master stage (hörtest fix: earrape/brummt). DC blocker (~35 Hz HP) removes
 * DC + subsonic rumble from the noise bed / bass / reverb feedback; a master
 * volume gives headroom; a soft limiter is LINEAR below the knee and only
 * rounds true peaks (the old tanf() distorted everything = the harshness). */
#define DC_R 0.995f
static float dc_x1L, dc_y1L, dc_x1R, dc_y1R;
static float master_vol_cur, master_vol_tgt;

static inline float soft_limit(float x) {
    const float k = 0.75f;
    float a = x < 0.0f ? -x : x;
    if (a <= k) return x;
    float over = a - k;
    float comp = k + (1.0f - k) * (over / (over + (1.0f - k)));
    return x < 0.0f ? -comp : comp;
}

void engine_init(void) {
    pad_init();
    reverb_init();
    texture_init();
    bass_init();

    /* Defaults per the webapp's mid-mode at space≈0.5, mood≈0.5. */
    reverb_size = 0.7f;
    reverb_damp = 0.3f;
    reverb_set(reverb_size, reverb_damp);
    reverb_set_drive(0.15f);

    send_amount_cur = send_amount_tgt = 0.45f;
    wet_amp_cur     = wet_amp_tgt     = 0.40f;

    /* Texture bed boots at 0 (silent power-up); raise via engine_set_texture
     * or the brain in Step 12. */
    texture_set_amount(0.0f);
    bass_set_depth(0.5f);
    memset(active_freq, 0, sizeof active_freq);

    dc_x1L = dc_y1L = dc_x1R = dc_y1R = 0.0f;
    master_vol_cur = master_vol_tgt = 0.6f;
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
void engine_set_master_volume(float v){ master_vol_tgt  = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_brightness(float hz)  { pad_set_brightness(hz); }
void engine_set_texture(float v)      { texture_set_amount(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_bass_depth(float v)   { bass_set_depth(dsp_clampf(v, 0.0f, 1.0f)); }

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

    /* Master: sum → DC-block → volume → soft limit → int16. */
    master_vol_cur += SMOOTH_COEF * (master_vol_tgt - master_vol_cur);
    const float wa = wet_amp_cur;
    const float mv = master_vol_cur;
    for (int n = 0; n < frames; ++n) {
        float L = dryL[n] + wetL[n] * wa;
        float R = dryR[n] + wetR[n] * wa;

        float yL = L - dc_x1L + DC_R * dc_y1L; dc_x1L = L; dc_y1L = yL;
        float yR = R - dc_x1R + DC_R * dc_y1R; dc_x1R = R; dc_y1R = yR;

        yL = soft_limit(yL * mv);
        yR = soft_limit(yR * mv);
        buf[n * 2 + 0] = (int16_t)(yL * 32767.0f);
        buf[n * 2 + 1] = (int16_t)(yR * 32767.0f);
    }
}

int engine_active_voices(void) { return pad_active_count(); }
