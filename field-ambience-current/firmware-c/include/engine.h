#ifndef FAM_ENGINE_H
#define FAM_ENGINE_H

/*
 * Audio engine — Step 11 mix-bus owner.
 *
 * Replaces pad_render as the renderer that audio.c calls. Per audio block:
 *
 *   pad → dry_LR (direct, full level)
 *   pad → send_LR (scaled, controls reverb amount per voice → matches the
 *                  webapp's `verbSend` parameter)
 *   send → famReverbMaster → wet_LR
 *   master_LR = dry_LR + wet_LR * wet_amp
 *   master → tanh soft-clip → int16 interleaved
 *
 * Defaults (medium hall — matches `computeReverb` mid-mode at space=0.5,
 * mood=0.5): size=0.7, damp=0.3, drive=0.15, wet_amp=0.4, send=0.45.
 *
 * The engine forwards note_on/off to pad — `main.c` only calls engine_*
 * APIs after Step 11.
 */

#include <stdint.h>

void engine_init(void);

/* Cell-tap forwarders (thin wrappers around pad_note_on/off for now;
 * the harmonic brain in Step 12 will interpose voicing logic here). */
void engine_note_on(uint8_t source, float freq_hz, float amp);
void engine_note_off(uint8_t source);
void engine_all_off(void);

/* Live-tunable engine knobs (smoothed by the reverb internally; the wet/
 * send mix is smoothed here). All are 0..1; defaults applied at engine_init. */
void engine_set_reverb_size(float size_0_1);
void engine_set_reverb_damp(float damp_0_1);
void engine_set_reverb_drive(float drive_0_1);
void engine_set_wet_amp(float wet_0_1);
void engine_set_send(float send_0_1);          /* pad reverb send */
void engine_set_brightness(float hz);          /* pass-through to pad */
void engine_set_texture(float amount_0_1);     /* famTexture bed amount */

/* The renderer audio.c registers via audio_set_renderer(). Writes `frames`
 * interleaved stereo int16 samples (L,R,L,R,…). Audio-context safe. */
void engine_render(int16_t *buf, int frames);

/* Pass-through for main.c's OLED stat line. */
int engine_active_voices(void);

#endif
