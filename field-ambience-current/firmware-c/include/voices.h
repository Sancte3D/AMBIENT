#ifndef FAM_VOICES_H
#define FAM_VOICES_H

/*
 * Polyphonic voice pool — Step 7 foundation for the native engine.
 *
 * Each voice is (for now) a single LUT sine through an ASR amplitude
 * envelope: linear attack 0→1, sustain at 1 while gated, linear release
 * →0 on gate-off. Sustaining (not one-shot) so a held cell rings until
 * released — matches the instrument-mode "tap a cell, hear a world" model.
 *
 * Click-free by construction: the envelope ramps are applied per sample,
 * so note-on/off never produce a discontinuity.
 *
 * "source" identifies who owns a voice (e.g. a cell index 0..4). A new
 * note_on for the same source re-triggers that source's voice rather than
 * stacking; note_off(source) releases it. This is the hook the engine and
 * the cell-tap handler use; later steps swap the sine for famPadCore etc.
 * behind the same note_on/note_off/render API.
 */

#include <stdint.h>
#include <stdbool.h>

#define VOICES_MAX 8

/* Build the pool (envelope times etc.). Call once after dsp_init(). */
void voices_init(void);

/* Start (or re-trigger) the voice owned by `source` at `freq_hz`, peak
 * amplitude `amp` (0..1 of full scale, before the global mix). */
void voices_note_on(uint8_t source, float freq_hz, float amp);

/* Release the voice owned by `source` (enters the release ramp). No-op if
 * that source has no active voice. */
void voices_note_off(uint8_t source);

/* Release every voice. */
void voices_all_off(void);

/* Render `frames` interleaved stereo int16 samples into buf (L,R,L,R,...),
 * summing all active voices. Safe to call from the audio fill context. */
void voices_render(int16_t *buf, int frames);

/* Number of voices not in the idle state — for the OLED / debug. */
int voices_active_count(void);

#endif
