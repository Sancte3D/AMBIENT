#ifndef FAM_V2_WORLDS_H
#define FAM_V2_WORLDS_H

/*
 * worlds.c — kuratierte Klang-Welten (ADR-0014 §Phase 3).
 *
 * Sechs Welten, jede mit eigenen Grenzen + Voice-Charakter. Die User-
 * Makros (Density / Motion / Color / Blur / Texture / Glow) bedeuten in
 * jeder Welt etwas anderes — gleicher Regler, anderes Verhalten.
 *
 * Die Engine fragt nach jeder World-Wahl `worlds_get()` und nutzt die
 * Felder direkt als Setpoints für die V2-Module.
 */

#include "v2/field_voice.h"

typedef enum {
    WORLD_GLASS = 0,
    WORLD_WARM,
    WORLD_DUST,
    WORLD_FOG,
    WORLD_TAPE,
    WORLD_MACHINE,
    WORLD_COUNT,
} world_id_t;

typedef struct {
    const char *name;
    fv_type_t   primary_voice;
    fv_type_t   secondary_voice;

    /* Reverb base mapping (size, wet, damp range over Blur 0..1). */
    float reverb_size_base;
    float reverb_wet_lo, reverb_wet_hi;
    float reverb_damp_base;

    /* Effect chain amounts. */
    float diffuser_amount;
    float mod_delay_amount;

    /* Texture layer balance (sum should be ~1.0). */
    float air_base, dust_base, body_base;

    /* Harmony field constraints. */
    float motion_speed;          /* multiplier for slow1/slow2 + retarget */
    float voice_target_count;    /* 3..7 */
    float dissonance_limit;      /* 0..1 */

    /* Color range — World may e.g. always live bright (Glass) or dark (Fog). */
    float color_floor, color_ceiling;
} world_t;

const world_t *worlds_get(int id);
int worlds_count(void);

#endif
