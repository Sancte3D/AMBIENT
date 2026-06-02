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
#include "dsp.h"
#include "audio.h"                    /* AUDIO_BUFFER_FRAMES */
#include <math.h>
#include <string.h>

/* Per-layer reverb sends (match the webapp's per-voice verbSend values).
 * Pad uses the user-tunable engine_set_send; texture has its own fixed send. */
#define TEXTURE_SEND  0.55f

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

void engine_init(void) {
    pad_init();
    reverb_init();
    texture_init();

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
}

void engine_note_on(uint8_t source, float freq_hz, float amp) {
    pad_note_on(source, freq_hz, amp);
}
void engine_note_off(uint8_t source)  { pad_note_off(source); }
void engine_all_off(void)             { pad_all_off(); }

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
