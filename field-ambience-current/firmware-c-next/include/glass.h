#ifndef FAM_GLASS_H
#define FAM_GLASS_H

/*
 * glass.{h,c} — 2-operator FM "glass" melody voice (r18.98).
 *
 * The second melodic instrument colour next to the Karplus-Strong string
 * (pluck.c). PRINCIPLE learned from Chowning's FM synthesis and the DX7
 * tine/bell patch family (and Plaits' FM model as the modern reference) —
 * no code copied, reinterpreted for this instrument:
 *
 *   - a sine carrier is phase-modulated by a sine at an INHARMONIC ratio
 *     (3.5307): the sidebands land between the harmonics → glass/bell;
 *   - the modulation INDEX decays much faster (~130 ms) than the amplitude
 *     (~2.5 s): the strike is bright and complex, the ring is nearly pure —
 *     that envelope asymmetry IS the percussive-metal illusion;
 *   - velocity drives the index more than the level: soft = warm chime,
 *     hard = glassy clang. (DX7 velocity→mod-depth mapping, relearned.)
 *
 * Same contract as pluck.c: voices self-decay, no note-off, fixed pool,
 * round-robin stealing, fixed seeds, host-testable. Renders onto the same
 * melody bus (through the modal body) as the string.
 */

#include <stdint.h>

#define GLASS_VOICES 2

void glass_init(void);

/* Strike a glass tone: freq in Hz, amp 0..1 peak-ish. Steals the oldest
 * voice when the pool is full. */
void glass_note(float freq_hz, float amp);

/* Voices still audibly ringing. */
int glass_active_count(void);

/* Mix into dry + reverb-send accumulators (stereo alternating like the
 * string plucks: call-answer movement). */
void glass_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames);

#endif
