#ifndef FAM_VOICELEAD_H
#define FAM_VOICELEAD_H

/*
 * voicelead.c — minimal-movement chord voicing (r19.29).
 *
 * Given the chord currently sounding (previous absolute MIDI pitches) and the
 * pitch-CLASSES of the next chord, place each new pitch class in the octave
 * that moves the least from the voice nearest it. Common tones (a pitch class
 * already sounding) therefore stay on the SAME pitch — parsimonious voice
 * leading — while the voices that must change move by the smallest interval.
 *
 * PRINCIPLE (learned, reinterpreted — no product copied): HiChord's nearest-
 * inversion selection + Orchid's "same pitch classes, different register"
 * idea, plus our own Harmonic-Safety common-tone rule (harmony.c) applied to
 * MANUAL play. Pure integer theory, control-rate, host-testable.
 *
 * With no previous chord (prev_n == 0) the targets are spread ascending in the
 * octave nearest `anchor`, so the first press lands in a sane register.
 */

/* Voice the `n` target pitch classes into out[] as absolute MIDI, near the
 * `prev_n` previous pitches (or `anchor` when prev_n == 0). Returns n.
 * pc entries are 0..11 (higher is fine; taken mod 12). n is clamped to 4. */
int voicelead(const int *prev, int prev_n,
              const int *pc,   int n,
              int anchor, int *out);

#endif
