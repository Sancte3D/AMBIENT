#ifndef FAM_TUNING_H
#define FAM_TUNING_H

/*
 * tuning.{h,c} — instrument tuning layer (r19.6).
 *
 * PRINCIPLE from the Sonicware Liven Ambient Ø v1.5 ("Pure Intonation —
 * harmonies without beating tones") and classic just intonation: tune the
 * sustained harmony to SIMPLE FREQUENCY RATIOS relative to the key tonic
 * (fifth 3:2, major third 5:4, sixth 5:3 …) instead of equal temperament
 * (2^(n/12)). For pure intervals the partials of the two notes coincide
 * exactly → no beating → a glassy, "locked" standing harmony. This is the
 * ideal case for THIS instrument: one slowly-moving tonal centre held for
 * minutes (the drone + the pentatonic bed), exactly what JI wants.
 *
 * JI is anchored to ONE tonic (the current KEY). Every tonal voice routes
 * its MIDI→Hz through tuning_hz() so the whole standing harmony locks
 * together — mixing ET and JI voices would beat WORSE than either alone,
 * so it is all-or-nothing per the global mode.
 *
 * Mode EQUAL is BIT-EXACT equal temperament (the bench-tuned reference is
 * untouched); JUST switches the ratios on. Pure module, host-testable.
 */

/* Tuning mode. 0 = equal temperament (default, reference), 1 = just. */
void tuning_set_mode(int just);
int  tuning_mode(void);

/* Anchor tonic (MIDI note of the current key). */
void tuning_set_key(int tonic_midi);

/* MIDI (float, fractional allowed) → Hz. EQUAL = dsp_midi_to_hz (bit-exact);
 * JUST = pure-ratio interval from the key tonic. */
float tuning_hz(float midi);

#endif
