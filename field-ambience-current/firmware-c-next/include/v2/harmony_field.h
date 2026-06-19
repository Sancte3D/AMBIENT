#ifndef FAM_V2_HARMONY_FIELD_H
#define FAM_V2_HARMONY_FIELD_H

/*
 * harmony_field.c — Engine V2 voice-leading + consonance budget (ADR-0014).
 *
 * The classic Engine-V1 brain produces block chord changes — "C → F → C" —
 * which is exactly what makes V1 sound like a church. V2 does NOT change
 * the whole chord at once. Instead it owns 8 voices that drift one at a
 * time toward new targets chosen inside a constrained interval set.
 *
 * Voice roles (fixed, assigned at init):
 *   #0   Bass        — moves rarely, follows Gravity rules.
 *   #1   Root        — usually stable, occasionally drifts to octave.
 *   #2   Fifth       — usually stable.
 *   #3   Third       — optional, can be omitted (drops amp to 0).
 *   #4   Sixth       — colour, can sit on +9 or +6.
 *   #5   Ninth       — colour, sits on +14 or +9.
 *   #6   Eleventh    — sparse, leaves and returns, soft tension.
 *   #7   High Partial — random walk ±5 cents around octave/twelfth.
 *
 * Each voice has independent retarget probability per advance() call, so
 * blocks don't move in lockstep — exactly the voice-leading rule.
 *
 * Output reads (hf_voice_*):  one float per voice for freq/amp/pan/drift.
 * The DSP layer reads these and renders; the field never touches audio.
 */

#include <stdint.h>
#include <stdbool.h>

#define HF_VOICE_COUNT 8

typedef enum {
    HF_VOICE_BASS = 0,
    HF_VOICE_ROOT,
    HF_VOICE_FIFTH,
    HF_VOICE_THIRD,
    HF_VOICE_SIXTH,
    HF_VOICE_NINTH,
    HF_VOICE_ELEVENTH,
    HF_VOICE_HIGH_PARTIAL,
} hf_voice_id_t;

typedef struct {
    bool  active;
    float freq_hz;          /* live, glide toward target */
    float target_freq_hz;
    float amp;              /* live, glide toward target */
    float target_amp;
    float pan;              /* -1..+1, stable per voice */
    float drift_cents;      /* ±0..±12 cent slow detune */
    float age_s;            /* time since last retarget */
    int   midi_note;        /* nearest semitone target (for guard checks) */
} hf_voice_t;

/* density 0..1 governs target-amp scale + voice-count (3..7 of 8 active).
 * dissonance 0..1 governs whether spice intervals (9, 11) participate. */
void hf_init(uint32_t seed, int center_midi);
void hf_set_center(int center_midi);
void hf_set_density(float density_0_1);
void hf_set_dissonance(float dissonance_0_1);
void hf_set_motion(float motion_0_1);     /* scales retarget probability + drift */
void hf_set_voice_target_count(float n_3_to_7);

/* Drives one harmonic "tick" — typically called once per audio block.
 * Voice freq/amp glide internally via dsp_smooth_coef. */
void hf_advance(float dt_s);

/* Snapshot read of voice state. Pointer is valid until next hf_advance(). */
const hf_voice_t *hf_voice(int idx);

/* Number of currently-active voices (for guards / displays). */
int hf_active_count(void);

/* Force a new field (re-roll all targets). Used by New Field button. */
void hf_new_field(uint32_t seed);

#endif
