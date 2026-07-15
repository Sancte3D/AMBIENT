#ifndef FAM_HARMONY_H
#define FAM_HARMONY_H

/*
 * harmony.{h,c} — the HARMONIC SAFETY CORE (r19.0).
 *
 * Rewrite of the autoplay harmony thinking after the user's research pass
 * (r/ambientmusic, r/musictheory patterns; Plomp/Levelt & Sethares
 * roughness psychoacoustics; Tymoczko voice-leading geometry; the
 * Sonicware Liven Ambient Ø architecture). The core findings:
 *
 *   1. PITCH WORLD, not chord progressions. A world owns a pentatonic
 *      CORE (no semitone steps, no tritone inside the set — the two most
 *      aggressive clash classes are structurally removed) plus ONE color
 *      note that is only allowed in the high register.
 *   2. Harmonic states are not re-rolled — they MUTATE. Minimum 3 common
 *      pitch classes between consecutive states, at most 2 voices move,
 *      and common tones stay at the SAME pitch (parsimonious voice
 *      leading). Mostly the BASS reinterprets the held notes.
 *   3. REGISTER RULES: below C3 only root/fifth/octave relationships;
 *      thirds live in the mid band; 2nds/9ths/maj7/color only above C4
 *      (roughness rises sharply in low registers — Plomp/Levelt).
 *   4. COLLISION FILTER before every melody note-on: against every
 *      sustained voice reject semitone class (1/11), tritone (6), and
 *      2nds below C4. Rejected notes fall to the next-best candidate.
 *   5. The melody is a LONG voice (durations seconds, silences seconds),
 *      moving by the table: repeat > step > fourth/fifth > sixth/octave >
 *      color. Not an arpeggiator.
 *
 *   Quality gate first. Randomness last.
 *
 * Pure control-rate module: no audio, no engine calls, fixed-seed LCG,
 * fully host-testable. engine.c owns WHEN things sound; harmony.c owns
 * WHAT is allowed to sound.
 */

#include <stdint.h>

#define HARMONY_VOICES 4      /* upper harmony voices (the state, voiced) */

void harmony_init(void);

/* Configure the pitch world: tonic pitch (MIDI) + tonality. minor != 0
 * uses the minor-pentatonic world (core {0,3,5,7,10}, color 2 — the 9th),
 * else the major-pentatonic world (core {0,2,4,7,9}, color 11 — maj7).
 * Resets the state machine + voicing to state 0. */
void harmony_set_world(int tonic_midi, int minor);

/* Advance the slow state clock (call from the generative tick, any rate).
 * States dwell 24..48 s (humanized, fixed seed), then MUTATE. */
void harmony_tick(uint32_t now_ms);

/* Force one state mutation now (offline renderers / tests + r19.24 steer). */
void harmony_advance(void);

/* r19.24 New Field: reseed the mutation LCG (reproducible per seed). The
 * pitch world is unchanged; only the evolution path differs. */
void harmony_reseed(uint32_t seed);

/* --- current state ------------------------------------------------------ */

int harmony_state_index(void);        /* 0..3 */
int harmony_state_changes(void);      /* mutations since set_world */
int harmony_last_common_tones(void);  /* pcs kept on the last mutation */
int harmony_last_moved_voices(void);  /* voices that moved on last mutation */

/* Bass of the current state, placed low (MIDI 38..49). The bed/drone/bass
 * fundament. Below C3 the ONLY companions allowed are fifth + octave. */
int harmony_bass_midi(void);
int harmony_fifth_midi(void);         /* bass + 7 */

/* The four voiced upper-harmony notes (wide voicing, MIDI ~55..79,
 * common tones frozen across mutations). Returns count written. */
int harmony_voices(int *out_midi, int max);

/* --- melody -------------------------------------------------------------- */

/* Pick the next melody note. last_midi 0 = phrase opening. sustained[] =
 * every currently sounding pitch (bed, voices, drone, user notes) for the
 * collision filter. Returns a MIDI note inside the pitch world and the
 * melody register (62..86), or -1 if nothing safe exists (rare — caller
 * treats it as silence). p_color 0..1 biases the color-note option. */
int harmony_melody_next(int last_midi, const int *sustained, int n_sus,
                        float p_color);

/* True if the midi note's pitch class belongs to the world (core∪color) —
 * with the color class only accepted above C4 (MIDI 60). Test hook + the
 * melody picker's own gate. */
int harmony_in_world(int midi);

/* The collision filter alone (test hook): 1 = safe against all sustained
 * notes per the interval rules above. */
int harmony_collision_ok(int midi, const int *sustained, int n_sus);

#endif
