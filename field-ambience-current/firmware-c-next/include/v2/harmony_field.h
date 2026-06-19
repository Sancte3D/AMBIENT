#ifndef FAM_V2_HARMONY_FIELD_H
#define FAM_V2_HARMONY_FIELD_H

/*
 * harmony_field.c — Engine V2 voice leading, PENTATONIC-LOCKED (ADR-0014 r2).
 *
 * r1 let 8 voices pick semitone offsets at random and even injected tension
 * intervals (minor-2nd / tritone). The result was atonal horror. r2 fixes
 * the root cause: every voice is locked to a PENTATONIC scale, which has no
 * semitone (and no tritone) intervals at all — so a harsh cluster is
 * mathematically impossible. Beauty by construction.
 *
 * The field is a wide stacked voicing (bass → high partial). Each voice owns
 * a position in pentatonic scale-steps. Low voices (bass/root/fifth) are the
 * gravity and almost never move; upper voices wander gently to ADJACENT
 * scale tones — always consonant. Movement is slow and one voice at a time
 * (voice leading, not block changes).
 *
 * Output reads (hf_voice_*): one float per voice for freq/amp/pan/drift.
 * The arp + DSP layers read the scale via hf_root_midi/hf_scale_*.
 */

#include <stdint.h>
#include <stdbool.h>

#define HF_VOICE_COUNT 8
#define HF_SCALE_LEN   5      /* pentatonic */

typedef enum {
    HF_VOICE_BASS = 0,
    HF_VOICE_ROOT,
    HF_VOICE_FIFTH,
    HF_VOICE_LOW_MID,
    HF_VOICE_COLOR,
    HF_VOICE_UPPER,
    HF_VOICE_SHIMMER,
    HF_VOICE_HIGH_PARTIAL,
} hf_voice_id_t;

typedef struct {
    bool  active;
    float freq_hz;
    float target_freq_hz;
    float amp;
    float target_amp;
    float pan;
    float drift_cents;
    float age_s;
    int   midi_note;
    int   scale_pos;        /* current absolute pentatonic step (oct*5 + deg) */
} hf_voice_t;

void hf_init(uint32_t seed, int center_midi);
void hf_set_center(int center_midi);
void hf_set_scale_minor(bool minor);          /* false = major pent, true = minor pent */
void hf_set_density(float density_0_1);
void hf_set_dissonance(float dissonance_0_1); /* now = upper-voice wander range, never clashes */
void hf_set_motion(float motion_0_1);
void hf_set_voice_target_count(float n_3_to_8);

void hf_advance(float dt_s);

const hf_voice_t *hf_voice(int idx);
int  hf_active_count(void);
void hf_new_field(uint32_t seed);

/* Scale access for the arpeggio + particle layers. */
int  hf_root_midi(void);
int  hf_scale_len(void);
int  hf_scale_semitone(int idx);   /* 0..HF_SCALE_LEN-1 */
int  hf_scale_pos_to_midi(int pos);/* pos = oct*HF_SCALE_LEN + degree */

#endif
