#ifndef FAM_EMBER_H
#define FAM_EMBER_H

/*
 * ember.{h,c} — warm subtractive "analog" melody voice (r19.28).
 *
 * The nostalgic-mood voice the glass/FM bell could not be: real analog-style
 * synthesis instead of a static bell. PRINCIPLE learned from the Juno / OB /
 * Prophet lineage (no circuit copied, reinterpreted for this instrument):
 *
 *   - TWO band-limited saw oscillators detuned a few cents against each other
 *     plus a sub-octave triangle → the slow beating IS the analog chorus/warmth;
 *   - a resonant low-pass with its OWN decay envelope: the cutoff opens on the
 *     attack and closes as the note rings — that moving "curve profile" is the
 *     vintage vowel/wow the ear reads as alive, not dead;
 *   - a soft attack + long decay AD amp envelope: a pad-like key, not a pluck;
 *   - a gentle DELAYED vibrato LFO (fades in ~0.4 s after the attack) — the
 *     classic tape/analog touch that makes a held tone breathe.
 *
 * Same contract as glass.c / pluck.c: self-decaying voices, no note-off, fixed
 * pool, round-robin stealing, fixed increments (host-testable), renders onto
 * the melody + hall-send buses. Realtime-safe: oscillators via poly-BLEP,
 * sine via the LUT, the SVF cutoff re-set at control-rate (not per sample).
 */

#include <stdint.h>

#define EMBER_VOICES 3

void ember_init(void);

/* Strike a warm tone: freq in Hz, amp 0..1 peak-ish. Steals the oldest voice
 * when the pool is full. */
void ember_note(float freq_hz, float amp);

/* Voices still audibly ringing. */
int ember_active_count(void);

/* Mix into dry + reverb-send accumulators (stereo alternating seats). */
void ember_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames);

#endif
