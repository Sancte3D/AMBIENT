#include "ambient_palettes.h"

typedef struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Rgb;

typedef struct PaletteDefinition {
    const char *name;
    const char *slug;
    Rgb quiet[5];
    Rgb alive[5];
} PaletteDefinition;

/* Five anchors map to indices 0, 3, 7, 11, and 15. */
static const PaletteDefinition k_palettes[AMBIENT_PALETTE_COUNT] = {
    {"NACRE DAWN", "nacre-dawn",
     {{1, 3, 12}, {22, 18, 55}, {61, 69, 132}, {211, 122, 178}, {255, 244, 239}},
     {{1, 4, 13}, {13, 31, 66}, {40, 107, 142}, {161, 155, 226}, {244, 251, 255}}},
    {"TIDAL PRISM", "tidal-prism",
     {{0, 4, 12}, {5, 29, 70}, {13, 89, 146}, {29, 202, 195}, {231, 255, 246}},
     {{2, 2, 18}, {30, 17, 83}, {39, 93, 177}, {81, 196, 239}, {247, 246, 255}}},
    {"EMBER MOSS", "ember-moss",
     {{4, 6, 3}, {29, 39, 18}, {92, 100, 43}, {224, 143, 55}, {255, 238, 191}},
     {{7, 4, 3}, {54, 25, 20}, {117, 75, 35}, {225, 158, 74}, {255, 245, 210}}},
    {"ION VIOLET", "ion-violet",
     {{2, 1, 14}, {33, 14, 75}, {105, 35, 154}, {79, 171, 236}, {239, 244, 255}},
     {{1, 4, 16}, {18, 33, 91}, {96, 73, 191}, {203, 113, 224}, {255, 242, 255}}},
    {"LUNAR PEACH", "lunar-peach",
     {{6, 3, 9}, {57, 25, 50}, {134, 62, 87}, {241, 144, 132}, {255, 242, 222}},
     {{3, 5, 14}, {42, 39, 75}, {124, 88, 131}, {232, 164, 162}, {255, 245, 235}}},
    {"ARCTIC BLOOM", "arctic-bloom",
     {{0, 5, 13}, {8, 34, 65}, {24, 102, 137}, {95, 210, 220}, {239, 255, 255}},
     {{1, 4, 17}, {29, 24, 74}, {56, 100, 165}, {151, 195, 239}, {249, 249, 255}}},
    {"ACID PETAL", "acid-petal",
     {{3, 7, 2}, {23, 49, 14}, {74, 132, 27}, {194, 224, 65}, {251, 255, 222}},
     {{8, 2, 13}, {61, 15, 65}, {147, 39, 118}, {230, 113, 174}, {255, 236, 248}}},
    {"COPPER RAIN", "copper-rain",
     {{5, 4, 4}, {48, 26, 21}, {123, 67, 43}, {221, 139, 88}, {255, 238, 207}},
     {{1, 7, 11}, {9, 47, 59}, {28, 111, 120}, {100, 198, 185}, {227, 255, 242}}},
    {"DEEP CORAL", "deep-coral",
     {{8, 2, 5}, {65, 13, 32}, {150, 42, 58}, {241, 104, 87}, {255, 232, 190}},
     {{4, 3, 13}, {42, 24, 70}, {121, 56, 111}, {230, 119, 120}, {255, 239, 211}}},
    {"GHOST ORCHID", "ghost-orchid",
     {{3, 3, 10}, {35, 28, 65}, {99, 79, 145}, {202, 163, 224}, {255, 247, 255}},
     {{1, 6, 11}, {19, 49, 67}, {73, 121, 139}, {164, 206, 214}, {244, 255, 255}}},
    {"SOLAR INK", "solar-ink",
     {{4, 4, 6}, {38, 32, 35}, {103, 75, 48}, {226, 160, 60}, {255, 243, 193}},
     {{4, 3, 10}, {46, 24, 61}, {119, 61, 87}, {224, 139, 97}, {255, 239, 207}}},
    {"BIOLUME", "biolume",
     {{0, 6, 8}, {3, 41, 48}, {9, 108, 104}, {63, 218, 165}, {226, 255, 210}},
     {{1, 4, 13}, {10, 32, 68}, {27, 89, 151}, {77, 196, 217}, {232, 254, 255}}}
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

static Rgb sample_anchors(const Rgb anchors[5], unsigned index)
{
    static const unsigned position[5] = {0u, 3u, 7u, 11u, 15u};
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
    Rgb result = mix_rgb(quiet, alive, movement * 0.72f);
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
