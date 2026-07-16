#include "ambient_palettes.h"

#include <math.h>

typedef struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Rgb;

typedef struct Hsv {
    float h;
    float s;
    float v;
} Hsv;

typedef struct PaletteDefinition {
    const char *name;
    const char *slug;
    Rgb quiet[5];
    Rgb alive[5];
} PaletteDefinition;

/*
 * Five anchors map to indices 0, 3, 7, 14, and 15. Index 14 carries the
 * saturated neon colour; index 15 is reserved for tiny specular highlights.
 */
static const PaletteDefinition k_palettes[AMBIENT_PALETTE_COUNT] = {
    {"NACRE DAWN", "nacre-dawn",
     {{1, 0, 10}, {20, 0, 80}, {80, 10, 180}, {255, 45, 185}, {255, 225, 245}},
     {{0, 2, 12}, {0, 28, 90}, {0, 100, 220}, {20, 240, 255}, {225, 255, 255}}},
    {"TIDAL PRISM", "tidal-prism",
     {{0, 2, 12}, {0, 12, 70}, {0, 70, 220}, {0, 230, 255}, {215, 255, 255}},
     {{2, 0, 18}, {20, 0, 95}, {20, 45, 240}, {40, 170, 255}, {235, 245, 255}}},
    {"EMBER MOSS", "ember-moss",
     {{1, 5, 0}, {5, 35, 0}, {40, 110, 0}, {180, 220, 0}, {255, 250, 210}},
     {{8, 1, 0}, {60, 5, 0}, {180, 35, 0}, {255, 150, 0}, {255, 235, 190}}},
    {"ION VIOLET", "ion-violet",
     {{3, 0, 16}, {30, 0, 90}, {110, 0, 210}, {245, 40, 255}, {255, 205, 255}},
     {{0, 3, 18}, {10, 10, 100}, {45, 35, 220}, {120, 110, 255}, {245, 215, 255}}},
    {"LUNAR PEACH", "lunar-peach",
     {{10, 1, 0}, {70, 5, 0}, {200, 35, 0}, {255, 125, 10}, {255, 230, 195}},
     {{8, 0, 8}, {70, 0, 35}, {200, 10, 75}, {255, 65, 125}, {255, 210, 225}}},
    {"ARCTIC BLOOM", "arctic-bloom",
     {{0, 4, 14}, {0, 25, 90}, {0, 105, 230}, {0, 235, 255}, {220, 255, 255}},
     {{2, 0, 18}, {25, 0, 100}, {55, 35, 230}, {80, 170, 255}, {235, 240, 255}}},
    {"ACID PETAL", "acid-petal",
     {{1, 8, 0}, {5, 45, 0}, {35, 150, 0}, {170, 255, 0}, {245, 255, 205}},
     {{5, 2, 0}, {45, 10, 0}, {170, 55, 0}, {255, 200, 0}, {255, 245, 195}}},
    {"COPPER RAIN", "copper-rain",
     {{6, 2, 0}, {50, 12, 0}, {150, 45, 0}, {255, 140, 20}, {255, 230, 185}},
     {{3, 5, 0}, {30, 35, 0}, {100, 105, 0}, {245, 210, 0}, {255, 245, 200}}},
    {"DEEP CORAL", "deep-coral",
     {{10, 0, 3}, {70, 0, 20}, {190, 10, 45}, {255, 60, 90}, {255, 205, 210}},
     {{5, 0, 12}, {45, 0, 75}, {150, 15, 170}, {255, 70, 230}, {255, 210, 250}}},
    {"GHOST ORCHID", "ghost-orchid",
     {{3, 0, 12}, {25, 0, 80}, {95, 10, 190}, {220, 70, 255}, {250, 215, 255}},
     {{0, 5, 12}, {0, 35, 85}, {15, 115, 185}, {80, 230, 255}, {225, 255, 255}}},
    {"SOLAR INK", "solar-ink",
     {{4, 2, 0}, {45, 15, 0}, {140, 60, 0}, {255, 180, 0}, {255, 245, 190}},
     {{8, 0, 6}, {65, 0, 35}, {190, 15, 80}, {255, 95, 50}, {255, 220, 205}}},
    {"BIOLUME", "biolume",
     {{0, 7, 6}, {0, 45, 35}, {0, 150, 110}, {0, 255, 190}, {215, 255, 235}},
     {{0, 3, 15}, {0, 25, 95}, {0, 105, 225}, {0, 225, 255}, {210, 250, 255}}}
};

static float clamp01(float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

static float triangle(float phase)
{
    phase = clamp01(phase);
    return phase < 0.5f ? phase * 2.0f : (1.0f - phase) * 2.0f;
}

static uint8_t mix_channel(uint8_t a, uint8_t b, float amount)
{
    float value = (float)a + ((float)b - (float)a) * amount;
    if (value < 0.0f) value = 0.0f;
    if (value > 255.0f) value = 255.0f;
    return (uint8_t)(value + 0.5f);
}

static Rgb mix_rgb(Rgb a, Rgb b, float amount)
{
    Rgb result = {
        mix_channel(a.r, b.r, amount),
        mix_channel(a.g, b.g, amount),
        mix_channel(a.b, b.b, amount)
    };
    return result;
}

static Hsv rgb_to_hsv(Rgb rgb)
{
    float r = (float)rgb.r / 255.0f;
    float g = (float)rgb.g / 255.0f;
    float b = (float)rgb.b / 255.0f;
    float maximum = fmaxf(r, fmaxf(g, b));
    float minimum = fminf(r, fminf(g, b));
    float delta = maximum - minimum;
    Hsv result = {0.0f, maximum > 0.0f ? delta / maximum : 0.0f, maximum};
    if (delta > 0.00001f) {
        if (maximum == r) {
            result.h = fmodf((g - b) / delta, 6.0f) / 6.0f;
        } else if (maximum == g) {
            result.h = ((b - r) / delta + 2.0f) / 6.0f;
        } else {
            result.h = ((r - g) / delta + 4.0f) / 6.0f;
        }
        if (result.h < 0.0f) result.h += 1.0f;
    }
    return result;
}

static uint8_t hsv_channel(float value)
{
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    return (uint8_t)(value * 255.0f + 0.5f);
}

static Rgb hsv_to_rgb(Hsv hsv)
{
    float scaled = hsv.h * 6.0f;
    int sector = (int)floorf(scaled);
    float fraction = scaled - (float)sector;
    float p = hsv.v * (1.0f - hsv.s);
    float q = hsv.v * (1.0f - fraction * hsv.s);
    float t = hsv.v * (1.0f - (1.0f - fraction) * hsv.s);
    float r, g, b;
    switch (sector % 6) {
    case 0: r = hsv.v; g = t;     b = p;     break;
    case 1: r = q;     g = hsv.v; b = p;     break;
    case 2: r = p;     g = hsv.v; b = t;     break;
    case 3: r = p;     g = q;     b = hsv.v; break;
    case 4: r = t;     g = p;     b = hsv.v; break;
    default:r = hsv.v; g = p;     b = q;     break;
    }
    Rgb result = {hsv_channel(r), hsv_channel(g), hsv_channel(b)};
    return result;
}

static Rgb mix_hsv(Rgb a, Rgb b, float amount)
{
    Hsv from = rgb_to_hsv(a);
    Hsv to = rgb_to_hsv(b);
    float hue_delta = to.h - from.h;
    if (hue_delta > 0.5f) hue_delta -= 1.0f;
    if (hue_delta < -0.5f) hue_delta += 1.0f;
    Hsv result = {
        from.h + hue_delta * amount,
        from.s + (to.s - from.s) * amount,
        from.v + (to.v - from.v) * amount
    };
    if (result.h < 0.0f) result.h += 1.0f;
    if (result.h >= 1.0f) result.h -= 1.0f;
    return hsv_to_rgb(result);
}

static Rgb sample_anchors(const Rgb anchors[5], unsigned index)
{
    static const unsigned position[5] = {0u, 3u, 7u, 14u, 15u};
    if (index >= 15u) return anchors[4];
    unsigned segment = 0u;
    while (segment < 3u && index > position[segment + 1u]) ++segment;
    unsigned span = position[segment + 1u] - position[segment];
    float amount = (float)(index - position[segment]) / (float)span;
    return mix_rgb(anchors[segment], anchors[segment + 1u], amount);
}

const char *ambient_palette_name(AmbientPalette palette)
{
    return (unsigned)palette < AMBIENT_PALETTE_COUNT ? k_palettes[palette].name : "UNKNOWN";
}

const char *ambient_palette_slug(AmbientPalette palette)
{
    return (unsigned)palette < AMBIENT_PALETTE_COUNT ? k_palettes[palette].slug : "unknown";
}

void ambient_palette_rgb(AmbientPalette palette, uint8_t index, float phase,
                         uint8_t *r, uint8_t *g, uint8_t *b)
{
    if ((unsigned)palette >= AMBIENT_PALETTE_COUNT) palette = AMBIENT_PALETTE_NACRE_DAWN;
    unsigned tone = index & 15u;
    Rgb quiet = sample_anchors(k_palettes[palette].quiet, tone);
    Rgb alive = sample_anchors(k_palettes[palette].alive, tone);
    float movement = triangle(clamp01(phase));
    Rgb result = mix_hsv(quiet, alive, movement * 0.72f);
    if (r) *r = result.r;
    if (g) *g = result.g;
    if (b) *b = result.b;
}

void ambient_palette_build_rgb565(AmbientPalette palette, float phase,
                                  uint16_t lut[16])
{
    if (!lut) return;
    for (unsigned i = 0; i < 16u; ++i) {
        uint8_t r, g, b;
        ambient_palette_rgb(palette, (uint8_t)i, phase, &r, &g, &b);
        lut[i] = (uint16_t)(((uint16_t)(r >> 3) << 11) |
                           ((uint16_t)(g >> 2) << 5) |
                           (uint16_t)(b >> 3));
    }
}

size_t ambient_palette_lut_bytes(void)
{
    return AMBIENT_PALETTE_LUT_BYTES;
}
