#ifndef FAM_V2_ENGINE_V2_H
#define FAM_V2_ENGINE_V2_H

/*
 * engine_v2.c — Engine V2 renderer (ADR-0014).
 *
 * Drop-in alternative zu engine_render() aus V1. Selber Aufruf-Vertrag:
 * writes `frames` interleaved stereo int16 samples, allokationsfrei,
 * bounded.
 *
 * V1 wird NICHT verändert. V2 sitzt neben V1 — der Toggle, welche der
 * beiden audio.c aufruft, wird Phase 3 (Display-UX). Bis dahin nutzt die
 * Firmware weiter V1; V2 ist über tools/render_v2_wav.c oder Host-Tests
 * hörbar.
 */

#include <stdint.h>
#include <stdbool.h>

void engine_v2_init(uint32_t seed);

/* User Macros — alle 0..1, sofort wirksam. */
void engine_v2_set_world(int world_id);
void engine_v2_set_center(int center_midi);
void engine_v2_set_density(float n);
void engine_v2_set_motion(float n);
void engine_v2_set_color(float n);
void engine_v2_set_blur(float n);
void engine_v2_set_texture(float n);
void engine_v2_set_glow(float n);
void engine_v2_set_master_volume(float n);

/* Performance Features. */
void engine_v2_set_freeze(bool on);
void engine_v2_new_field(uint32_t seed);

/* Block renderer — int16 interleaved stereo, matches engine_render shape. */
void engine_v2_render(int16_t *buf, int frames);

/* For host tests / display: how many voices currently active. */
int  engine_v2_active_voices(void);

#endif
