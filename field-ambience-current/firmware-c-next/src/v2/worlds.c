/*
 * worlds.c — six curated sound worlds. Static table.
 */

#include "v2/worlds.h"

static const world_t WORLDS[WORLD_COUNT] = {
    /* GLASS — klar, digital, sauber, hell, leise Partials, wenig Dissonanz. */
    {
        .name = "GLASS",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_PARTICLE,
        .reverb_size_base = 0.55f,
        .reverb_wet_lo = 0.18f, .reverb_wet_hi = 0.42f,
        .reverb_damp_base = 0.25f,    /* hell */
        .diffuser_amount = 0.55f,
        .mod_delay_amount = 0.35f,
        .air_base = 0.60f, .dust_base = 0.30f, .body_base = 0.10f,
        .motion_speed = 0.8f,
        .voice_target_count = 5.0f,
        .dissonance_limit = 0.12f,
        .color_floor = 0.55f, .color_ceiling = 1.0f,
    },
    /* WARM — analog, leicht detuned, soft sat, stabile Tiefe. */
    {
        .name = "WARM",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_GLASS,
        .reverb_size_base = 0.62f,
        .reverb_wet_lo = 0.22f, .reverb_wet_hi = 0.48f,
        .reverb_damp_base = 0.45f,
        .diffuser_amount = 0.45f,
        .mod_delay_amount = 0.35f,
        .air_base = 0.30f, .dust_base = 0.25f, .body_base = 0.45f,
        .motion_speed = 0.6f,
        .voice_target_count = 5.0f,
        .dissonance_limit = 0.2f,
        .color_floor = 0.35f, .color_ceiling = 0.85f,
    },
    /* DUST — luftig, texturiert, nicht sakral, mehr Air + Dust. */
    {
        .name = "DUST",
        .primary_voice   = FV_PARTICLE,
        .secondary_voice = FV_GLASS,
        .reverb_size_base = 0.50f,
        .reverb_wet_lo = 0.20f, .reverb_wet_hi = 0.45f,
        .reverb_damp_base = 0.30f,
        .diffuser_amount = 0.70f,
        .mod_delay_amount = 0.45f,
        .air_base = 0.50f, .dust_base = 0.45f, .body_base = 0.15f,
        .motion_speed = 1.4f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.30f,
        .color_floor = 0.40f, .color_ceiling = 0.95f,
    },
    /* FOG (DEEP) — dunkel, langsam, bassiger, cinematic, dunkler Reverb. */
    {
        .name = "DEEP FOG",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_PARTICLE,
        .reverb_size_base = 0.78f,
        .reverb_wet_lo = 0.25f, .reverb_wet_hi = 0.52f,
        .reverb_damp_base = 0.65f,    /* dunkel */
        .diffuser_amount = 0.55f,
        .mod_delay_amount = 0.55f,
        .air_base = 0.15f, .dust_base = 0.20f, .body_base = 0.65f,
        .motion_speed = 0.5f,
        .voice_target_count = 4.0f,
        .dissonance_limit = 0.20f,
        .color_floor = 0.15f, .color_ceiling = 0.55f,
    },
    /* TAPE — instabil, leicht kaputt, melancholisch, Wow + Flutter. */
    {
        .name = "TAPE",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_TAPE,
        .reverb_size_base = 0.58f,
        .reverb_wet_lo = 0.20f, .reverb_wet_hi = 0.42f,
        .reverb_damp_base = 0.55f,
        .diffuser_amount = 0.50f,
        .mod_delay_amount = 0.65f,
        .air_base = 0.25f, .dust_base = 0.40f, .body_base = 0.35f,
        .motion_speed = 1.0f,
        .voice_target_count = 5.0f,
        .dissonance_limit = 0.25f,
        .color_floor = 0.30f, .color_ceiling = 0.75f,
    },
    /* MACHINE — mechanisch, rhythmisch angedeutet (durch Dust-Trigger),
       kein Beat, leise Partikel, strukturierte Bewegung. */
    {
        .name = "MACHINE",
        .primary_voice   = FV_PARTICLE,
        .secondary_voice = FV_TAPE,
        .reverb_size_base = 0.45f,
        .reverb_wet_lo = 0.18f, .reverb_wet_hi = 0.38f,
        .reverb_damp_base = 0.35f,
        .diffuser_amount = 0.40f,
        .mod_delay_amount = 0.50f,
        .air_base = 0.20f, .dust_base = 0.60f, .body_base = 0.20f,
        .motion_speed = 1.6f,
        .voice_target_count = 5.0f,
        .dissonance_limit = 0.35f,
        .color_floor = 0.45f, .color_ceiling = 0.90f,
    },
};

const world_t *worlds_get(int id) {
    if (id < 0 || id >= WORLD_COUNT) return &WORLDS[0];
    return &WORLDS[id];
}

int worlds_count(void) { return WORLD_COUNT; }
