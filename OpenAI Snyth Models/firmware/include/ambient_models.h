#ifndef SANCTE_AMBIENT_MODELS_H
#define SANCTE_AMBIENT_MODELS_H

/*
 * Sancte3D original ambient synthesis collection.
 *
 * Design constraints:
 * - C11, allocation-free, deterministic, 44.1 kHz stereo.
 * - No samples, wavetables, copied presets, melodies, or third-party DSP.
 * - One active model at a time; the complete state stays below 32 KiB.
 *
 * The public API is deliberately independent of the product firmware.  The
 * adapter described in docs/INTEGRATION.md can therefore be reviewed before
 * this code is wired into an interrupt-driven audio callback.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMBIENT_SAMPLE_RATE 44100u
#define AMBIENT_MAX_VOICES  10u
#define AMBIENT_STATE_BUDGET_BYTES 32768u

typedef enum AmbientModel {
    AMBIENT_ACID_RAIN = 0,
    AMBIENT_FM_GLASS,
    AMBIENT_CHORUS_MIST,
    AMBIENT_ION_STORM,
    AMBIENT_GLASS_ORBIT,
    AMBIENT_BAMBOO_CIRCUIT,
    AMBIENT_NACRE_HORIZON,
    AMBIENT_TIDEGLASS,
    AMBIENT_LUMEN_SWARM,
    AMBIENT_HOLLOW_CHOIR,
    AMBIENT_MODEL_COUNT
} AmbientModel;

typedef struct AmbientControls {
    float color;    /* 0 dark / damped .. 1 bright / open */
    float motion;   /* 0 still .. 1 animated */
    float space;    /* 0 intimate .. 1 long spatial decay */
    float texture;  /* model-specific density / imperfection */
    float width;    /* stereo spread */
    float level;    /* final gain; 0 .. 1 */
} AmbientControls;

typedef struct AmbientVoice {
    float phase[4];
    float frequency;
    float velocity;
    float envelope;
    float tail;
    float pan;
    float filter;
    float aux[4];
    uint32_t age;
    uint16_t note_id;
    uint16_t pluck_position;
    uint16_t pluck_length;
    uint8_t gate;
    uint8_t active;
    uint8_t pluck_slot;
    uint8_t reserved;
} AmbientVoice;

/*
 * Public so firmware may place it in a selected RAM bank without allocation.
 * Treat fields as private and use the functions below.
 */
typedef struct AmbientSynth {
    AmbientVoice voices[AMBIENT_MAX_VOICES];
    AmbientControls controls;
    AmbientControls target;
    float fdn_damping[4];
    float dc_l;
    float dc_r;
    float dc_prev_l;
    float dc_prev_r;
    float master_l;
    float master_r;
    float lfo_phase;
    float background_phase[4];
    uint32_t fdn_position[4];
    uint32_t chorus_position;
    uint32_t sample_clock;
    uint32_t random_state;
    uint16_t next_note_id;
    uint8_t model;
    uint8_t reserved[5];
    int16_t fdn[4][2048];
    int16_t chorus[2][512];
    int16_t pluck[4][1024];
} AmbientSynth;

/* Defaults are musical starting points, not recreations of any commercial preset. */
AmbientControls ambient_model_default_controls(AmbientModel model);
const char *ambient_model_name(AmbientModel model);
const char *ambient_model_slug(AmbientModel model);

void ambient_synth_init(AmbientSynth *synth, AmbientModel model, uint32_t seed);
void ambient_synth_set_model(AmbientSynth *synth, AmbientModel model);
void ambient_synth_set_controls(AmbientSynth *synth, AmbientControls controls);

/* Returns a non-zero note id. Frequency is clamped to a safe musical range. */
uint16_t ambient_synth_note_on(AmbientSynth *synth, float frequency_hz,
                               float velocity_0_1, float pan_minus1_1);
void ambient_synth_note_off(AmbientSynth *synth, uint16_t note_id);
void ambient_synth_all_notes_off(AmbientSynth *synth);

/* Writes interleaved signed 16-bit stereo. Safe for arbitrary block sizes. */
void ambient_synth_render(AmbientSynth *synth, int16_t *interleaved_stereo,
                          size_t frames);

size_t ambient_synth_state_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
