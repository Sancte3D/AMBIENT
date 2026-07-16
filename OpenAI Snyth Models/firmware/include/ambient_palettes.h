#ifndef SANCTE_AMBIENT_PALETTES_H
#define SANCTE_AMBIENT_PALETTES_H

/*
 * Animated 16-colour LUTs for the existing packed 4-bit framebuffer.
 *
 * A framebuffer nibble remains one byte-pair pixel; only the 16-entry RGB565
 * conversion table changes.  This enables true multicolour frames with zero
 * additional framebuffer memory and 32 bytes of driver-side LUT storage.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMBIENT_PALETTE_LUT_BYTES 32u

typedef enum AmbientPalette {
    AMBIENT_PALETTE_NACRE_DAWN = 0,
    AMBIENT_PALETTE_TIDAL_PRISM,
    AMBIENT_PALETTE_EMBER_MOSS,
    AMBIENT_PALETTE_ION_VIOLET,
    AMBIENT_PALETTE_LUNAR_PEACH,
    AMBIENT_PALETTE_ARCTIC_BLOOM,
    AMBIENT_PALETTE_ACID_PETAL,
    AMBIENT_PALETTE_COPPER_RAIN,
    AMBIENT_PALETTE_DEEP_CORAL,
    AMBIENT_PALETTE_GHOST_ORCHID,
    AMBIENT_PALETTE_SOLAR_INK,
    AMBIENT_PALETTE_BIOLUME,
    AMBIENT_PALETTE_COUNT
} AmbientPalette;

const char *ambient_palette_name(AmbientPalette palette);
const char *ambient_palette_slug(AmbientPalette palette);

/* phase_0_1 may animate slowly; a static UI may pass 0.0f. */
void ambient_palette_rgb(AmbientPalette palette, uint8_t index_0_15,
                         float phase_0_1, uint8_t *r, uint8_t *g, uint8_t *b);
void ambient_palette_build_rgb565(AmbientPalette palette, float phase_0_1,
                                  uint16_t lut_16[16]);
size_t ambient_palette_lut_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
