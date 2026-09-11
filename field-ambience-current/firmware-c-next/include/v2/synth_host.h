#ifndef FAM_V2_SYNTH_HOST_H
#define FAM_V2_SYNTH_HOST_H

#include <stdint.h>
#include "v2/synth_engine.h"

/*
 * synth_host — the swappable sound-core host (OP-1-Field style).
 *
 * Holds the registry of selectable synth engines, routes note/param calls to
 * the active one, and owns the GLOBAL signal path that every engine shares:
 *     active_engine.render_mix → global reverb (send) → master → beauty-guard
 * So the FX live outside the sound-core; switching engines never re-plumbs FX.
 *
 * Existing identifiers are stable scene/API keys. Display names come from
 * synth_names.h; renaming a sound type must never reorder these values.
 */
typedef enum {
    SYNTH_ACID = 0,         /* Resonant: subtractive ladder-filter tone */
    SYNTH_FM_GLASS,         /* Keys: 2-op FM attack/body tone */
    SYNTH_CHORUS_MIST,      /* Ensemble: detuned saw pad + chorus */
    SYNTH_ION_STORM,        /* Pulse: sustained PWM stack */
    SYNTH_GLASS_ORBIT,      /* Morph: phase-related waveform morph */
    SYNTH_BAMBOO_CIRCUIT,   /* Pluck: coupled LPG amplitude/brightness decay */
    SYNTH_COUNT
} synth_id_t;

void        synth_host_init(void);
void        synth_host_select(synth_id_t id);
synth_id_t  synth_host_active(void);
const char *synth_host_active_name(void);

void        synth_host_note_on(int midi, float vel);
void        synth_host_note_on_hz(float hz, float vel);
/* Slots: brightness (-600..800 Hz legacy offset), resonance, sweep, envmod. */
void        synth_host_set_macro(int slot, float value);
void        synth_host_retune_hz(float hz);
void        synth_host_note_off(void);
void        synth_host_set_param(synth_param_t p, float v01);
void        synth_host_panic(void);

/* Global, shared by all engines. */
void        synth_host_set_reverb(float size_0_1, float wet_0_1);
void        synth_host_set_master(float v_0_1);

/* Render `frames` stereo samples → interleaved int16. Runs the active engine
 * → global reverb on its send bus → master mix → beauty-guard limiter. */
void        synth_host_render(int16_t *out, int frames);
/* Adds unmastered dry/send buses. Device uses its shared master and FX. */
void synth_host_render_mix(float *dry_l, float *dry_r, float *send_l, float *send_r, int frames);

#endif /* FAM_V2_SYNTH_HOST_H */
