/*
 * worlds.c — six curated sound worlds (ADR-0014 r2).
 *
 * r2: pentatonic scale per world (no clashes possible), a gentle bell arp in
 * every world for melodic "schöne Töne", and the metallic Particle voice
 * dropped from the default voice roles (it was the horror element). Glass +
 * Tape carry the pads; the arp carries the melody.
 */

#include "v2/worlds.h"

static const world_t WORLDS[WORLD_COUNT] = {
    /* GLASS — klar, digital, hell, major pentatonic, prominent bells. */
    {
        .name = "GLASS",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_GLASS,
        .scale_minor = 0,
        .arp_rate_hz = 2.2f, .arp_amount = 0.55f,
        .reverb_size_base = 0.58f,
        .reverb_wet_lo = 0.20f, .reverb_wet_hi = 0.44f,
        .reverb_damp_base = 0.28f,
        .diffuser_amount = 0.45f,
        .mod_delay_amount = 0.30f,
        .air_base = 0.55f, .dust_base = 0.20f, .body_base = 0.25f,
        .motion_speed = 0.7f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.25f,
        .color_floor = 0.55f, .color_ceiling = 0.95f,
    },
    /* WARM — analog, leicht detuned, major pentatonic, soft bells. */
    {
        .name = "WARM",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_GLASS,
        .scale_minor = 0,
        .arp_rate_hz = 1.5f, .arp_amount = 0.42f,
        .reverb_size_base = 0.62f,
        .reverb_wet_lo = 0.22f, .reverb_wet_hi = 0.46f,
        .reverb_damp_base = 0.45f,
        .diffuser_amount = 0.40f,
        .mod_delay_amount = 0.30f,
        .air_base = 0.30f, .dust_base = 0.15f, .body_base = 0.55f,
        .motion_speed = 0.55f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.18f,
        .color_floor = 0.35f, .color_ceiling = 0.75f,
    },
    /* DUST — luftig, texturiert, major pentatonic, airy bells. */
    {
        .name = "DUST",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_TAPE,
        .scale_minor = 0,
        .arp_rate_hz = 1.9f, .arp_amount = 0.40f,
        .reverb_size_base = 0.55f,
        .reverb_wet_lo = 0.22f, .reverb_wet_hi = 0.46f,
        .reverb_damp_base = 0.32f,
        .diffuser_amount = 0.55f,
        .mod_delay_amount = 0.40f,
        .air_base = 0.55f, .dust_base = 0.30f, .body_base = 0.15f,
        .motion_speed = 1.0f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.30f,
        .color_floor = 0.45f, .color_ceiling = 0.90f,
    },
    /* DEEP FOG — dunkel, langsam, minor pentatonic, sparse deep bells. */
    {
        .name = "DEEP FOG",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_GLASS,
        .scale_minor = 1,
        .arp_rate_hz = 0.9f, .arp_amount = 0.32f,
        .reverb_size_base = 0.78f,
        .reverb_wet_lo = 0.26f, .reverb_wet_hi = 0.52f,
        .reverb_damp_base = 0.62f,
        .diffuser_amount = 0.50f,
        .mod_delay_amount = 0.45f,
        .air_base = 0.20f, .dust_base = 0.15f, .body_base = 0.65f,
        .motion_speed = 0.45f,
        .voice_target_count = 5.0f,
        .dissonance_limit = 0.15f,
        .color_floor = 0.18f, .color_ceiling = 0.50f,
    },
    /* TAPE — instabil, melancholisch, minor pentatonic, wow + flutter bells. */
    {
        .name = "TAPE",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_TAPE,
        .scale_minor = 1,
        .arp_rate_hz = 1.3f, .arp_amount = 0.38f,
        .reverb_size_base = 0.60f,
        .reverb_wet_lo = 0.22f, .reverb_wet_hi = 0.44f,
        .reverb_damp_base = 0.55f,
        .diffuser_amount = 0.45f,
        .mod_delay_amount = 0.55f,
        .air_base = 0.25f, .dust_base = 0.25f, .body_base = 0.50f,
        .motion_speed = 0.8f,
        .voice_target_count = 5.0f,
        .dissonance_limit = 0.20f,
        .color_floor = 0.28f, .color_ceiling = 0.62f,
    },
    /* MACHINE — strukturierte Bewegung, major pentatonic, faster bell pulse. */
    {
        .name = "MACHINE",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_TAPE,
        .scale_minor = 0,
        .arp_rate_hz = 2.8f, .arp_amount = 0.45f,
        .reverb_size_base = 0.48f,
        .reverb_wet_lo = 0.18f, .reverb_wet_hi = 0.40f,
        .reverb_damp_base = 0.38f,
        .diffuser_amount = 0.38f,
        .mod_delay_amount = 0.45f,
        .air_base = 0.30f, .dust_base = 0.40f, .body_base = 0.30f,
        .motion_speed = 1.3f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.28f,
        .color_floor = 0.45f, .color_ceiling = 0.85f,
    },
};

const world_t *worlds_get(int id) {
    if (id < 0 || id >= WORLD_COUNT) return &WORLDS[0];
    return &WORLDS[id];
}

int worlds_count(void) { return WORLD_COUNT; }
