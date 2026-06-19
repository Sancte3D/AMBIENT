#ifndef FAM_V2_ARP_H
#define FAM_V2_ARP_H

/*
 * arp.c — gentle pentatonic arpeggio / bell layer (ADR-0014 r2).
 *
 * This is the "schöne Töne" the field was missing: soft mallet/bell notes
 * picked from the active pentatonic scale, in a slow rising (and occasionally
 * falling) pattern, with a fast-ish attack and a long exponential release —
 * the felt-piano / music-box voice of Eno-style ambient. Every note is a
 * scale tone, so it always lands consonant against the pad field.
 *
 * The bell timbre = fundamental + soft octave + faint 12th, exponential
 * decay, slight inharmonic shimmer. Polyphonic with overlapping tails so the
 * arpeggio blooms rather than stutters.
 */

#include <stdint.h>

#define ARP_POLY 12

typedef struct {
    float freq;
    float phase1, phase2, phase3;
    float env;            /* 1 → 0 exponential */
    float rise;           /* 0 → 1 attack ramp (persists across blocks) */
    float amp;
    float pan;
    int   active;
} arp_note_t;

typedef struct {
    uint32_t rng;
    float    step_phase;     /* 0..1 within a step */
    float    rate_hz;        /* steps per second */
    float    amount;         /* 0..1 master level */
    int      pattern_pos;
    int      dir;            /* +1 up, -1 down */
    arp_note_t notes[ARP_POLY];
} arp_t;

void arp_init(arp_t *a, uint32_t seed);
void arp_set_rate(arp_t *a, float hz);       /* 0 = silent */
void arp_set_amount(arp_t *a, float amt_0_1);

/* Advance the step clock by dt_s; trigger new notes from the current scale.
 * root_midi + scale provide the pentatonic tones (read from harmony_field).
 * Call once per audio block BEFORE arp_render. */
void arp_tick(arp_t *a, float dt_s);

/* Render `frames` mono bell output, MIXED-IN to L/R with pan + send split.
 * color 0..1 brightens the bell; send_scale routes a copy for reverb shimmer
 * (handled by caller via the returned send buffers is overkill — instead this
 * writes dry into L/R and the same signal*send into sL/sR). */
void arp_render_add(arp_t *a, float *L, float *R, float *sL, float *sR,
                    int frames, float color_0_1, float send_scale);

#endif
