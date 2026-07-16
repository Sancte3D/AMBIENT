#ifndef SANCTE_AMBIENT_VISUALS_H
#define SANCTE_AMBIENT_VISUALS_H

/* 320 x 170, packed 4-bit luminance: two horizontal pixels per byte. */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMBIENT_DISPLAY_WIDTH  320u
#define AMBIENT_DISPLAY_HEIGHT 170u
#define AMBIENT_DISPLAY_BYTES  (AMBIENT_DISPLAY_WIDTH * AMBIENT_DISPLAY_HEIGHT / 2u)
#define AMBIENT_VISUAL_STATE_BUDGET_BYTES 2048u

typedef enum AmbientVisual {
    AMBIENT_VISUAL_RESONANT_GARDEN = 0,
    AMBIENT_VISUAL_ORBITAL_LOOM,
    AMBIENT_VISUAL_SPECTRAL_CANYON,
    AMBIENT_VISUAL_RAIN_MEMORY,
    AMBIENT_VISUAL_DREAM_TOPOGRAPHY,
    AMBIENT_VISUAL_FOCUS_RAIL,
    AMBIENT_VISUAL_PRISM_VEINS,
    AMBIENT_VISUAL_CRYSTAL_CHOIR,
    AMBIENT_VISUAL_RADIANT_GATE,
    AMBIENT_VISUAL_LUMEN_RIBBON,
    AMBIENT_VISUAL_GLYPH_RELAY,
    AMBIENT_VISUAL_PARTICLE_CURRENT,
    AMBIENT_VISUAL_SIGNAL_CHAMBER,
    AMBIENT_VISUAL_RESONANCE_ORB,
    AMBIENT_VISUAL_GLITCH_HALO,
    AMBIENT_VISUAL_TWIN_PULSE,
    AMBIENT_VISUAL_CHROMA_FALL,
    AMBIENT_VISUAL_SOFTBURST,
    AMBIENT_VISUAL_COUNT
} AmbientVisual;

typedef struct AmbientVisualInput {
    float rms;
    float bass;
    float mid;
    float high;
    float centroid;
    float beat_phase;
    uint8_t sound_model;
} AmbientVisualInput;

typedef struct AmbientParticle {
    int16_t x;
    int16_t y;
    int8_t vx;
    int8_t vy;
    uint8_t life;
    uint8_t tone;
} AmbientParticle;

typedef struct AmbientVisualState {
    AmbientParticle particles[96];
    int16_t ridge[AMBIENT_DISPLAY_WIDTH];
    uint32_t random_state;
    uint32_t frame;
    float smooth[6];
    uint8_t visual;
    uint8_t particle_cursor;
    uint8_t reserved[2];
} AmbientVisualState;

const char *ambient_visual_name(AmbientVisual visual);
const char *ambient_visual_slug(AmbientVisual visual);
void ambient_visual_init(AmbientVisualState *state, AmbientVisual visual,
                         uint32_t seed);
void ambient_visual_render(AmbientVisualState *state, uint8_t *packed_framebuffer,
                           AmbientVisualInput input);
size_t ambient_visual_state_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
