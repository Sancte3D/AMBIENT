/*
 * worlds.c — six curated sound worlds (ADR-0014 r3).
 *
 * r3: "nix kirche, sondern entfaltung wie crystal castles das macht mit deren
 * beats". The worlds now span a range from pure ambient (Glass/Fog) to
 * full beat-driven Crystal-Castles energy (Crystal/Tape/Dust): tempo-locked
 * 16th arps, synth drums, lo-fi grit. Minor pentatonic carries the
 * melancholic-energetic CC feel; everything stays clash-free (pentatonic).
 */

#include "v2/worlds.h"
#include "v2/beat.h"

static const world_t WORLDS[WORLD_COUNT] = {
    /* GLASS — bright pure ambient, no drums, slow quarter-note bells. */
    {
        .name = "GLASS",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_GLASS,
        .scale_minor = 0,
        .bpm = 92.0f,
        .arp_division = 4, .arp_amount = 0.50f,
        .beat_pattern = BEAT_PATTERN_FOUR, .beat_amount = 0.0f,
        .grit = 0.05f,
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
    /* WARM — analog ambient, no drums, gentle 8th bells. */
    {
        .name = "WARM",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_GLASS,
        .scale_minor = 0,
        .bpm = 88.0f,
        .arp_division = 2, .arp_amount = 0.40f,
        .beat_pattern = BEAT_PATTERN_FOUR, .beat_amount = 0.0f,
        .grit = 0.12f,
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
    /* DUST — light four-on-floor, driving 8th arp, airy + a little grit. */
    {
        .name = "DUST",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_TAPE,
        .scale_minor = 0,
        .bpm = 124.0f,
        .arp_division = 2, .arp_amount = 0.46f,
        .beat_pattern = BEAT_PATTERN_FOUR, .beat_amount = 0.55f,
        .grit = 0.30f,
        .reverb_size_base = 0.55f,
        .reverb_wet_lo = 0.20f, .reverb_wet_hi = 0.42f,
        .reverb_damp_base = 0.32f,
        .diffuser_amount = 0.45f,
        .mod_delay_amount = 0.40f,
        .air_base = 0.50f, .dust_base = 0.30f, .body_base = 0.20f,
        .motion_speed = 1.0f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.30f,
        .color_floor = 0.45f, .color_ceiling = 0.90f,
    },
    /* DEEP FOG — dark pure ambient, no drums, very slow. */
    {
        .name = "DEEP FOG",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_GLASS,
        .scale_minor = 1,
        .bpm = 76.0f,
        .arp_division = 4, .arp_amount = 0.32f,
        .beat_pattern = BEAT_PATTERN_HALF, .beat_amount = 0.0f,
        .grit = 0.08f,
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
    /* TAPE — half-time, heavy + lo-fi, melancholic minor, gritty bells. */
    {
        .name = "TAPE",
        .primary_voice   = FV_TAPE,
        .secondary_voice = FV_TAPE,
        .scale_minor = 1,
        .bpm = 116.0f,
        .arp_division = 2, .arp_amount = 0.42f,
        .beat_pattern = BEAT_PATTERN_HALF, .beat_amount = 0.62f,
        .grit = 0.55f,
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
    /* CRYSTAL — full Crystal-Castles: driving broken beat, 16th arps, grit,
     * melancholic minor pentatonic, high energy. The "geil spielbar" world. */
    {
        .name = "CRYSTAL",
        .primary_voice   = FV_GLASS,
        .secondary_voice = FV_TAPE,
        .scale_minor = 1,
        .bpm = 138.0f,
        .arp_division = 1, .arp_amount = 0.58f,
        .beat_pattern = BEAT_PATTERN_CC, .beat_amount = 0.80f,
        .grit = 0.65f,
        .reverb_size_base = 0.50f,
        .reverb_wet_lo = 0.18f, .reverb_wet_hi = 0.40f,
        .reverb_damp_base = 0.38f,
        .diffuser_amount = 0.35f,
        .mod_delay_amount = 0.50f,
        .air_base = 0.35f, .dust_base = 0.35f, .body_base = 0.30f,
        .motion_speed = 1.3f,
        .voice_target_count = 6.0f,
        .dissonance_limit = 0.28f,
        .color_floor = 0.50f, .color_ceiling = 0.92f,
    },
};

const world_t *worlds_get(int id) {
    if (id < 0 || id >= WORLD_COUNT) return &WORLDS[0];
    return &WORLDS[id];
}

int worlds_count(void) { return WORLD_COUNT; }
