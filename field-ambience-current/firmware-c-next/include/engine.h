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
#include <stdbool.h>

void engine_init(void);

/* Cell-tap forwarders (thin wrappers around pad_note_on/off for now;
 * the harmonic brain in Step 12 will interpose voicing logic here). */
void engine_note_on(uint8_t source, float freq_hz, float amp);
void engine_note_off(uint8_t source);
void engine_all_off(void);

/* ADR-0013 — feed one normalised Hall position sample (0=rest, 1=bottom-out)
 * for cell `cell` (0..4) at `now_ms`. The cell-velocity model (cells.c) turns
 * the position stream into note events: a PRESS sounds that cell's chord root
 * (via the harmonic brain) at a velocity-scaled amplitude; a RELEASE stops it.
 * This is the single cell entry point the STM32 ADC loop calls; the RP2040
 * bench can synthesise positions from its digital buttons. Returns true when a
 * note started or stopped this sample (for LED / debug). */
bool engine_cell_sample(uint8_t cell, float pos_0_1, uint32_t now_ms);

/* Live-tunable engine knobs (smoothed by the reverb internally; the wet/
 * send mix is smoothed here). All are 0..1; defaults applied at engine_init. */
void engine_set_reverb_size(float size_0_1);
void engine_set_reverb_damp(float damp_0_1);
void engine_set_reverb_drive(float drive_0_1);
void engine_set_wet_amp(float wet_0_1);
void engine_set_send(float send_0_1);          /* pad reverb send */
void engine_set_master_volume(float vol_0_1);  /* master level (VOLUME encoder) */
void engine_set_brightness(float hz);          /* pass-through to pad */
void engine_set_texture(float amount_0_1);     /* famTexture bed amount */
void engine_set_atmosphere(float amount_0_1);  /* per-world ambience layer (ADR-0017) */
void engine_set_motion(float amount_0_1);      /* Pad LFO depth (perform macro) */
void engine_set_age(float amount_0_1);         /* tape hiss + saturation combo */
void engine_set_echo(float amount_0_1);        /* tape-style stereo delay macro */
void engine_set_blur(float amount_0_1);        /* granular cloud / smear macro */
void engine_set_bass_depth(float depth_0_1);   /* famSubBass/DeepBass depth */
void engine_set_world(int world_idx);          /* pick ambience generator (ADR-0017) */

/* Step 12b #1 — musical state. Setting any of these recomputes the four
 * Freeverb parameters (size/damp/drive/wet) from the per-mode preset +
 * vibe bias + space/mood macros (reverb_presets.h). Updates are applied
 * live; the reverb's own coefficient smoothing prevents zipper, matching
 * the "sound darf nicht konkurrieren" rule for global parameters. */
void engine_set_mode(int mode_idx);            /* 0..5 (ionian..aeolian) */
void engine_set_vibe(int vibe_idx);            /* 0..3 (warm..floating) */
void engine_set_space(float space_0_1);        /* room size / tail macro */
void engine_set_mood(float mood_0_1);          /* darkness↔brightness macro */

/* Step 12b — key tonic (MIDI). Updates the harmonic brain (future cells use
 * the new key) and glides the drone root live; already-held cells keep their
 * pitch ("global follows, held notes freeze" rule). */
void engine_set_key(int tonic_midi);

/* Step 12b #2 — drone toggle (DRONE modifier). Blooms a sustained root pad
 * in/out; it follows engine_set_key live with portamento. */
void engine_set_drone(bool on);

/* Step 12b #3 — pad timbre (warm/strings/brass = 0/1/2). All sounding voices
 * glide together into the new timbre (smoothed global crossfade), so warm→
 * brass never leaves the two timbres competing in the air. */
void engine_set_pad_voice(int voice_idx);

/* Step 12b #4 — generative bed. on=false stops it (releases its voice).
 * program <0 selects Markov auto, >=0 selects a fixed progression index. */
void engine_set_generative(bool on, int program);

/* Advance the generative bed one step: pick the next degree, sound its chord
 * root as a pad voice (a reserved source), and let the bass follow. Call from
 * the bar timer. Returns the new degree (1..6) or -1 when generative is off.
 * No-op while any cell is held — live playing overrides the bed. */
int engine_generative_advance(void);

/* The renderer audio.c registers via audio_set_renderer(). Writes `frames`
 * interleaved stereo int16 samples (L,R,L,R,…). Audio-context safe. */
void engine_render(int16_t *buf, int frames);

/* Pass-through for main.c's OLED stat line. */
int engine_active_voices(void);

#endif
