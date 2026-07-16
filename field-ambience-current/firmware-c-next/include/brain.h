#ifndef FAM_BRAIN_H
#define FAM_BRAIN_H

/*
 * Harmonic brain — Step 12a. The music-theory layer that turns a cell index
 * into a real scale/mode/family-derived pitch, replacing the placeholder
 * C-minor pentatonic in main.c. Pure integer theory, no audio — a faithful
 * port of the webapp's SCALES / CHORD_FAMILIES / chordAtDegree / voiceCentered.
 *
 * Cell taps sound a SINGLE pad voice on the chord ROOT (the lowest note of the
 * centre-voiced chord for that degree) — exactly what the webapp `cellOn`
 * does. The full chord is available via brain_chord for a future cell-hold
 * feature, but the default tap is one voice per cell.
 *
 * State (key / mode / vibe) defaults to the constitutional values and is
 * settable for the Step-12b encoder/menu bindings.
 */

#include <stdint.h>

#define BRAIN_MODE_COUNT  6      /* ionian, dorian, phrygian, lydian, mixo, aeolian */
#define BRAIN_VIBE_COUNT  4      /* warm, bright, deep, floating → chord family */
#define BRAIN_MAX_CHORD   6      /* widest family (min11) has 6 notes */

void brain_init(void);

void brain_set_key(int tonic_midi);     /* default 60 (C4) */
void brain_set_mode(int mode_idx);      /* 0..5, clamped */
void brain_set_vibe(int vibe_idx);      /* 0..3 → chord family, clamped */

int  brain_get_mode(void);
int  brain_get_vibe(void);
int  brain_get_key(void);

/* Voiced chord (centre-shifted toward MIDI ~64) for a degree 1..7. Writes up
 * to `max` MIDI notes into out_midi, returns the count written. */
int brain_chord(int degree, int *out_midi, int max);

/* The MIDI pitch a cell tap should sound (cell 0..4). r19.26: a strictly
 * ascending pentatonic degree above the key (major or minor pentatonic per the
 * world's mode) — no semitone/tritone between any two cell roots, always
 * rising left→right. Shift = +1 octave is applied by the caller. */
int brain_cell_root(int cell);

#endif
