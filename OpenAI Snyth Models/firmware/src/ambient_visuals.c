#include "ambient_visuals.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_visual_names[AMBIENT_VISUAL_COUNT] = {
    "RESONANT GARDEN", "ORBITAL LOOM", "SPECTRAL CANYON", "RAIN MEMORY",
    "DREAM TOPOGRAPHY", "FOCUS RAIL", "PRISM VEINS", "CRYSTAL CHOIR",
    "RADIANT GATE", "LUMEN RIBBON", "GLYPH RELAY", "PARTICLE CURRENT",
    "SIGNAL CHAMBER", "RESONANCE ORB", "GLITCH HALO", "TWIN PULSE",
    "CHROMA FALL", "SOFTBURST"
};

static const char *const k_visual_slugs[AMBIENT_VISUAL_COUNT] = {
    "resonant-garden", "orbital-loom", "spectral-canyon", "rain-memory",
    "dream-topography", "focus-rail", "prism-veins", "crystal-choir",
    "radiant-gate", "lumen-ribbon", "glyph-relay", "particle-current",
    "signal-chamber", "resonance-orb", "glitch-halo", "twin-pulse",
    "chroma-fall", "softburst"
};

_Static_assert(sizeof(AmbientVisualState) <= AMBIENT_VISUAL_STATE_BUDGET_BYTES,
               "AmbientVisualState exceeds the embedded RAM budget");

static float clamp01(float x)
{
    return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
}

static uint32_t random_u32(AmbientVisualState *s)
{
    uint32_t x = s->random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->random_state = x ? x : 0x7f4a7c15u;
    return s->random_state;
}

static float wave(float phase)
{
    float x = phase - floorf(phase);
    x = x * 2.0f - 1.0f;
    float y = 4.0f * x * (1.0f - fabsf(x));
    return y + 0.225f * (y * fabsf(y) - y);
}

static void pixel(uint8_t *fb, int x, int y, unsigned tone)
{
    if ((unsigned)x >= AMBIENT_DISPLAY_WIDTH ||
        (unsigned)y >= AMBIENT_DISPLAY_HEIGHT) {
        return;
    }
    unsigned index = (unsigned)y * (AMBIENT_DISPLAY_WIDTH / 2u) + (unsigned)x / 2u;
    unsigned old = fb[index];
    tone &= 15u;
    if ((x & 1) == 0) {
        unsigned existing = old >> 4;
        if (tone > existing) fb[index] = (uint8_t)((old & 0x0fu) | (tone << 4));
    } else {
        unsigned existing = old & 0x0fu;
        if (tone > existing) fb[index] = (uint8_t)((old & 0xf0u) | tone);
    }
}

static void line(uint8_t *fb, int x0, int y0, int x1, int y1, unsigned tone)
{
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        pixel(fb, x0, y0, tone);
        if (x0 == x1 && y0 == y1) break;
        int twice = error * 2;
        if (twice >= dy) { error += dy; x0 += sx; }
        if (twice <= dx) { error += dx; y0 += sy; }
    }
}

static void circle(uint8_t *fb, int cx, int cy, int radius, unsigned tone)
{
    if (radius <= 0) {
        pixel(fb, cx, cy, tone);
        return;
    }
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y) {
        pixel(fb, cx + x, cy + y, tone); pixel(fb, cx + y, cy + x, tone);
        pixel(fb, cx - y, cy + x, tone); pixel(fb, cx - x, cy + y, tone);
        pixel(fb, cx - x, cy - y, tone); pixel(fb, cx - y, cy - x, tone);
        pixel(fb, cx + y, cy - x, tone); pixel(fb, cx + x, cy - y, tone);
        ++y;
        if (error < 0) {
            error += 2 * y + 1;
        } else {
            --x;
            error += 2 * (y - x) + 1;
        }
    }
}

static void glow(uint8_t *fb, int x, int y, int radius, unsigned tone)
{
    pixel(fb, x, y, tone);
    for (int r = 1; r <= radius; ++r) {
        unsigned faded = tone > (unsigned)(r * 3) ? tone - (unsigned)(r * 3) : 1u;
        circle(fb, x, y, r, faded);
    }
}

static void rectangle(uint8_t *fb, int x0, int y0, int x1, int y1,
                      unsigned tone)
{
    line(fb, x0, y0, x1, y0, tone);
    line(fb, x1, y0, x1, y1, tone);
    line(fb, x1, y1, x0, y1, tone);
    line(fb, x0, y1, x0, y0, tone);
}

static void ellipse(uint8_t *fb, int cx, int cy, int rx, int ry,
                    unsigned tone)
{
    if (rx <= 0 || ry <= 0) {
        pixel(fb, cx, cy, tone);
        return;
    }
    int previous_x = cx;
    int previous_y = cy - ry;
    for (int step = 1; step <= 64; ++step) {
        float phase = (float)step / 64.0f;
        int x = cx + (int)((float)rx * wave(phase));
        int y = cy + (int)((float)ry * wave(phase + 0.25f));
        line(fb, previous_x, previous_y, x, y, tone);
        previous_x = x;
        previous_y = y;
    }
}

static void filled_disc(uint8_t *fb, int cx, int cy, int radius,
                        unsigned tone)
{
    for (int r = radius; r >= 0; --r) {
        unsigned lift = (unsigned)(radius - r);
        unsigned inner = tone + lift;
        circle(fb, cx, cy, r, inner > 15u ? 15u : inner);
    }
}

static uint32_t hash_u32(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

static void spawn_particle(AmbientVisualState *s, int y, int speed, unsigned tone)
{
    AmbientParticle *p = &s->particles[s->particle_cursor++ % 96u];
    p->x = (int16_t)(random_u32(s) % AMBIENT_DISPLAY_WIDTH);
    p->y = (int16_t)y;
    p->vx = (int8_t)((int)(random_u32(s) % 3u) - 1);
    p->vy = (int8_t)speed;
    p->life = (uint8_t)(40u + random_u32(s) % 110u);
    p->tone = (uint8_t)tone;
}

static void draw_garden(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], high = s->smooth[3];
    int base = 158;
    for (int i = 0; i < 15; ++i) {
        int root_x = 8 + i * 22;
        float sway = wave((float)s->frame * 0.0032f + (float)i * 0.137f);
        int height = 22 + (int)(bass * 48.0f) + (i * 17 % 31);
        int tip_x = root_x + (int)(sway * (3.0f + s->smooth[2] * 7.0f));
        int tip_y = base - height;
        int mid_x = (root_x + tip_x) / 2 - (i & 1 ? 4 : -4);
        int mid_y = (base + tip_y) / 2;
        line(fb, root_x, base, mid_x, mid_y, 4u + (unsigned)(rms * 5.0f));
        line(fb, mid_x, mid_y, tip_x, tip_y, 7u + (unsigned)(rms * 5.0f));
        int flower = 1 + (int)(high * 4.0f) + (i % 3 == 0);
        glow(fb, tip_x, tip_y, flower, 10u + (unsigned)(high * 5.0f));
        if (i & 1) {
            line(fb, mid_x, mid_y, mid_x + 7, mid_y - 5, 5u);
        }
    }
    line(fb, 0, base, 319, base, 3u + (unsigned)(bass * 4.0f));

    unsigned spawn = 1u + (unsigned)(high * 5.0f);
    for (unsigned i = 0; i < spawn; ++i) spawn_particle(s, 150, -1, 8u + i);
    for (unsigned i = 0; i < 96u; ++i) {
        AmbientParticle *p = &s->particles[i];
        if (!p->life) continue;
        p->x += p->vx;
        p->y += p->vy;
        if ((s->frame + i) % 4u == 0u) p->x += (int16_t)((int)(random_u32(s) % 3u) - 1);
        pixel(fb, p->x, p->y, p->tone);
        if (--p->life == 0u || p->y < 4) p->life = 0u;
    }
}

static void draw_orbital_loom(AmbientVisualState *s, uint8_t *fb)
{
    int cx = 160;
    int cy = 85;
    float bass = s->smooth[1], mid = s->smooth[2], high = s->smooth[3];
    for (int ring = 0; ring < 7; ++ring) {
        float fr = (float)ring;
        float rx = 25.0f + fr * 18.0f + bass * (fr + 1.0f) * 1.8f;
        float ry = 12.0f + fr * 8.4f + mid * (7.0f - fr) * 1.3f;
        float phase = (float)s->frame * (0.0023f + fr * 0.00031f);
        int previous_x = cx + (int)rx;
        int previous_y = cy;
        for (int step = 1; step <= 64; ++step) {
            float p = (float)step / 64.0f;
            float xw = wave(p + phase);
            float yw = wave(p + phase + 0.25f);
            int x = cx + (int)(rx * xw);
            int y = cy + (int)(ry * yw + wave(p * 3.0f + phase) * high * 4.0f);
            line(fb, previous_x, previous_y, x, y, 3u + (unsigned)ring + (unsigned)(mid * 4.0f));
            previous_x = x;
            previous_y = y;
        }
    }
    int spokes = 10 + (int)(high * 12.0f);
    for (int i = 0; i < spokes; ++i) {
        float p = (float)i / (float)spokes + (float)s->frame * 0.0011f;
        int x = cx + (int)((58.0f + bass * 66.0f) * wave(p));
        int y = cy + (int)((29.0f + mid * 34.0f) * wave(p + 0.25f));
        line(fb, cx, cy, x, y, 2u + (unsigned)(high * 5.0f));
        glow(fb, x, y, high > 0.45f ? 2 : 1, 10u + (unsigned)(high * 5.0f));
    }
    glow(fb, cx, cy, 4 + (int)(s->smooth[0] * 7.0f), 15u);
}

static void draw_canyon(AmbientVisualState *s, uint8_t *fb)
{
    float bass = s->smooth[1], mid = s->smooth[2], high = s->smooth[3];
    int horizon = 45 - (int)(bass * 9.0f);
    for (int layer = 0; layer < 13; ++layer) {
        int base = horizon + layer * 10;
        int prev_y = base;
        float flayer = (float)layer;
        for (int x = 0; x < 320; x += 2) {
            float terrain = wave((float)x * (0.0047f + flayer * 0.00018f) +
                                 (float)s->frame * 0.0022f + flayer * 0.071f);
            float detail = wave((float)x * 0.015f - (float)s->frame * 0.0013f + flayer * 0.19f);
            int y = base + (int)(terrain * (3.0f + bass * 10.0f) + detail * high * 3.0f);
            if (x) line(fb, x - 2, prev_y, x, y, 2u + (unsigned)layer + (unsigned)(mid * 2.0f));
            prev_y = y;
        }
    }
    int sun_x = 50 + (int)(wave((float)s->frame * 0.0019f) * 32.0f);
    glow(fb, sun_x, 27, 4 + (int)(high * 5.0f), 13u + (unsigned)(high * 2.0f));
    for (int x = 0; x < 320; x += 8) {
        int y = 164 - (int)(wave((float)x * 0.011f + (float)s->frame * 0.002f) * mid * 3.0f);
        line(fb, x, y, x + 5, y, 5u);
    }
}

static void draw_rain(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], high = s->smooth[3];
    unsigned spawn = 2u + (unsigned)(high * 14.0f);
    for (unsigned i = 0; i < spawn; ++i) {
        spawn_particle(s, -(int)(random_u32(s) % 40u), 2 + (int)(high * 5.0f), 6u + (unsigned)(high * 9.0f));
    }
    for (unsigned i = 0; i < 96u; ++i) {
        AmbientParticle *p = &s->particles[i];
        if (!p->life) continue;
        int old_y = p->y;
        p->y += p->vy;
        p->x += p->vx;
        line(fb, p->x, old_y - p->vy * 2, p->x + p->vx, p->y, p->tone);
        if (p->y >= 145) {
            int radius = 2 + (int)(bass * 8.0f) + (int)(i % 3u);
            circle(fb, p->x, 149, radius, 5u + (unsigned)(rms * 7.0f));
            p->life = 0u;
        } else if (--p->life == 0u) {
            p->life = 0u;
        }
    }
    for (int band = 0; band < 5; ++band) {
        int y = 150 + band * 4;
        int shift = (int)((s->frame + (uint32_t)band * 9u) % 23u);
        for (int x = -shift; x < 320; x += 23) {
            line(fb, x, y, x + 11, y, 2u + (unsigned)band);
        }
    }
}

static void draw_topography(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], mid = s->smooth[2], high = s->smooth[3];
    for (int contour = 0; contour < 17; ++contour) {
        int base = 10 + contour * 10;
        int prev_y = base;
        float fcontour = (float)contour;
        for (int x = 0; x < 320; x += 2) {
            float p = (float)x * 0.0041f + fcontour * 0.037f;
            float slow = wave(p + (float)s->frame * 0.0014f);
            float folded = wave(p * 2.31f - (float)s->frame * 0.0011f + fcontour * 0.083f);
            float focus = wave((float)x * 0.0018f + 0.21f) * wave((float)x * 0.0018f + 0.21f);
            int y = base + (int)(slow * (3.0f + bass * 8.0f) +
                                 folded * (1.0f + mid * 5.0f) * focus);
            if (x) line(fb, x - 2, prev_y, x, y, 2u + (unsigned)(contour % 8) + (unsigned)(rms * 4.0f));
            prev_y = y;
        }
    }
    int nodes = 3 + (int)(high * 9.0f);
    for (int i = 0; i < nodes; ++i) {
        float p = (float)i / (float)nodes + (float)s->frame * 0.0017f;
        int x = 160 + (int)(wave(p) * (65.0f + mid * 78.0f));
        int y = 85 + (int)(wave(p * 1.73f + 0.25f) * (28.0f + bass * 42.0f));
        glow(fb, x, y, 1 + (int)(rms * 4.0f), 11u + (unsigned)(high * 4.0f));
    }
}

static void draw_focus_rail(AmbientVisualState *s, uint8_t *fb)
{
    unsigned focus = (s->frame / 32u) % 5u;
    unsigned pulse = (unsigned)(s->smooth[0] * 3.0f);
    int status_x = (int)((s->frame * 3u) % AMBIENT_DISPLAY_WIDTH);
    line(fb, status_x - 5, 3, status_x + 5, 3, 9u + pulse);
    for (int row = 0; row < 5; ++row) {
        int y = 23 + row * 29;
        unsigned selected = (unsigned)row == focus;
        unsigned tone = selected ? 13u + (pulse > 2u ? 2u : pulse) : 3u + (unsigned)(row & 1);
        if (selected) {
            line(fb, 0, y - 13, 319, y - 13, 5u + pulse);
            line(fb, 0, y + 13, 319, y + 13, 5u + pulse);
        }
        circle(fb, 25, y, selected ? 10 : 8, tone);
        line(fb, 20, y, 30, y, tone);
        line(fb, 25, y - 5, 25, y + 5, tone > 2u ? tone - 2u : tone);
        rectangle(fb, 45, y - 10, 78, y + 10, tone);
        line(fb, 52, y, 70, y, selected ? 15u : tone);
        rectangle(fb, 86, y - 11, 165 + row * 7, y + 11, tone);
        line(fb, 96, y - 3, 142 + row * 4, y - 3, tone);
        line(fb, 96, y + 4, 130 + row * 5, y + 4, tone > 2u ? tone - 2u : tone);

        int icon_x = 230 + row * 17;
        switch (row) {
        case 0:
            ellipse(fb, icon_x, y, 11, 8, tone);
            line(fb, icon_x - 8, y, icon_x + 8, y, tone);
            break;
        case 1:
            line(fb, icon_x, y - 10, icon_x + 10, y, tone);
            line(fb, icon_x + 10, y, icon_x, y + 10, tone);
            line(fb, icon_x, y + 10, icon_x - 10, y, tone);
            line(fb, icon_x - 10, y, icon_x, y - 10, tone);
            break;
        case 2:
            rectangle(fb, icon_x - 9, y - 9, icon_x + 9, y + 9, tone);
            line(fb, icon_x - 9, y + 5, icon_x + 9, y - 5, tone);
            break;
        case 3:
            circle(fb, icon_x, y, 9, tone);
            circle(fb, icon_x, y, 3, selected ? 15u : tone);
            break;
        default:
            line(fb, icon_x - 11, y + 7, icon_x - 4, y - 6, tone);
            line(fb, icon_x - 4, y - 6, icon_x + 3, y + 6, tone);
            line(fb, icon_x + 3, y + 6, icon_x + 11, y - 7, tone);
            break;
        }
    }
}

static void draw_prism_veins(AmbientVisualState *s, uint8_t *fb)
{
    float bass = s->smooth[1], mid = s->smooth[2], high = s->smooth[3];
    line(fb, 0, 85, 319, 85, 3u + (unsigned)(mid * 4.0f));
    for (int x = 4; x < 320; x += 7) {
        uint32_t noise = hash_u32((uint32_t)x * 193u + s->frame / 3u);
        float motion = wave((float)x * 0.018f + (float)s->frame * 0.0031f);
        float sparkle = fabsf(wave((float)x * 0.041f - (float)s->frame * 0.0063f));
        int center = 85 + (int)(motion * (7.0f + mid * 12.0f));
        int height = 5 + (int)(sparkle * (14.0f + high * 48.0f)) +
                     (int)(noise % 17u) + (int)(bass * 12.0f);
        unsigned tone = 7u + (noise >> 8u) % 9u;
        line(fb, x, center - height, x, center + height, tone);
        line(fb, x - 1, center - height / 2, x - 1, center + height / 2,
             tone > 4u ? tone - 4u : 2u);
        line(fb, x + 1, center - height / 3, x + 1, center + height / 3,
             tone > 6u ? tone - 6u : 2u);
        if ((noise & 7u) == 0u) {
            glow(fb, x, center - height, 2, 15u);
            glow(fb, x, center + height, 2, 15u);
        }
    }
}

static void draw_crystal_choir(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], high = s->smooth[3];
    int center = 86 + (int)(wave((float)s->frame * 0.0022f) * 5.0f);
    for (int x = 2; x < 320; x += 5) {
        uint32_t noise = hash_u32((uint32_t)x * 811u);
        float choir = fabsf(wave((float)x * 0.026f + (float)s->frame * 0.0047f));
        float shimmer = fabsf(wave((float)x * 0.071f - (float)s->frame * 0.0081f));
        int reach = 7 + (int)(choir * choir * (28.0f + high * 62.0f)) +
                    (int)(shimmer * 13.0f) + (int)(bass * 10.0f);
        unsigned tone = 8u + (noise % 8u);
        line(fb, x, center - reach, x, center + reach, tone);
        line(fb, x - 1, center - reach / 2, x - 1, center + reach / 2,
             tone > 5u ? tone - 5u : 3u);
        if (((unsigned)x / 5u + s->frame / 5u) % 11u == 0u) {
            glow(fb, x, center - reach, 2 + (int)(rms * 2.0f), 15u);
            glow(fb, x, center + reach, 2 + (int)(rms * 2.0f), 15u);
        }
    }
    glow(fb, 160, center, 3 + (int)(rms * 6.0f), 15u);
}

static void draw_radiant_gate(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], mid = s->smooth[2], high = s->smooth[3];
    int cx = 160;
    int cy = 85;
    int rays = 26 + (int)(high * 18.0f);
    for (int i = 0; i < rays; ++i) {
        float p = (float)i / (float)rays + (float)s->frame * 0.0017f;
        float radial = 1.0f + 0.13f * wave(p * 5.0f + (float)s->frame * 0.003f);
        int inner_x = cx + (int)(23.0f * wave(p));
        int inner_y = cy + (int)(34.0f * wave(p + 0.25f));
        int outer_x = cx + (int)(190.0f * radial * wave(p));
        int outer_y = cy + (int)(105.0f * radial * wave(p + 0.25f));
        unsigned tone = 4u + (unsigned)(i % 7) + (unsigned)(rms * 4.0f);
        if (tone > 15u) tone = 15u;
        line(fb, inner_x, inner_y, outer_x, outer_y, tone);
        if ((i % 4) == 0) {
            line(fb, inner_x + 1, inner_y, outer_x + 2, outer_y, tone > 4u ? tone - 4u : 2u);
        }
    }
    for (int ring = 0; ring < 7; ++ring) {
        int breathe = (int)(wave((float)s->frame * 0.003f + (float)ring * 0.11f) *
                            (2.0f + mid * 4.0f));
        ellipse(fb, cx, cy, 18 + ring * 4 + breathe,
                30 + ring * 5 - breathe, 5u + (unsigned)ring + (unsigned)(rms * 2.0f));
    }
    glow(fb, cx, cy, 3 + (int)(rms * 5.0f), 15u);
}

static void draw_lumen_ribbon(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], high = s->smooth[3];
    int previous_x = -1;
    int previous_y = 85;
    for (unsigned i = 0; i < 25u; ++i) {
        uint32_t travel = (s->frame + i * 15u) % 370u;
        int x = (int)travel - 25;
        float phase = (float)i * 0.071f - (float)s->frame * 0.0042f;
        int y = 85 + (int)(wave(phase) * (7.0f + bass * 8.0f));
        int radius = 3 + (int)(rms * 3.0f) + (int)(i % 3u == 0u);
        unsigned tone = 6u + (i % 7u) + (unsigned)(high * 3.0f);
        if (tone > 15u) tone = 15u;
        filled_disc(fb, x, y, radius, tone > 4u ? tone - 3u : tone);
        circle(fb, x, y, radius + 1, tone);
        if (previous_x >= 0 && abs(x - previous_x) < 20) {
            line(fb, previous_x, previous_y, x, y, tone > 5u ? tone - 5u : 2u);
        }
        int fiber = 7 + (int)(high * 9.0f) + (int)(i % 4u);
        int direction = (i & 1u) ? 1 : -1;
        line(fb, x, y + radius, x + direction * (3 + (int)(i % 4u)), y + fiber,
             tone > 4u ? tone - 4u : 3u);
        if ((i % 3u) == 0u) {
            line(fb, x, y - radius, x - direction * 3, y - fiber / 2,
                 tone > 6u ? tone - 6u : 2u);
        }
        previous_x = x;
        previous_y = y;
    }
}

static void draw_glyph_relay(AmbientVisualState *s, uint8_t *fb)
{
    static const int16_t points[][2] = {
        {18, 117}, {48, 117}, {48, 94}, {77, 94}, {77, 116},
        {108, 116}, {108, 77}, {137, 77}, {137, 98}, {170, 98},
        {170, 61}, {204, 61}, {204, 82}, {239, 82}, {239, 47},
        {291, 47}
    };
    unsigned count = (unsigned)(sizeof(points) / sizeof(points[0]));
    unsigned reveal = (s->frame / 5u) % (count + 7u);
    unsigned active = reveal < count ? reveal : count;
    unsigned pulse = (unsigned)(s->smooth[0] * 4.0f);
    for (unsigned i = 1u; i < active; ++i) {
        unsigned tone = 8u + (i % 6u) + pulse;
        if (tone > 15u) tone = 15u;
        line(fb, points[i - 1u][0], points[i - 1u][1],
             points[i][0], points[i][1], tone);
        line(fb, points[i - 1u][0], points[i - 1u][1] + 4,
             points[i][0], points[i][1] + 4, tone > 6u ? tone - 6u : 2u);
        if ((i % 3u) == 0u) {
            circle(fb, points[i][0], points[i][1], 3, 15u);
        }
    }
    unsigned head = active ? active - 1u : 0u;
    glow(fb, points[head][0], points[head][1], 2 + (int)(s->smooth[3] * 3.0f), 15u);
    uint32_t packet_step = s->frame % ((count - 1u) * 5u);
    unsigned packet_segment = packet_step / 5u;
    int packet_phase = (int)(packet_step % 5u);
    int packet_x = (points[packet_segment][0] * (5 - packet_phase) +
                    points[packet_segment + 1u][0] * packet_phase) / 5;
    int packet_y = (points[packet_segment][1] * (5 - packet_phase) +
                    points[packet_segment + 1u][1] * packet_phase) / 5;
    glow(fb, packet_x, packet_y, 2 + (int)(s->smooth[0] * 3.0f), 15u);
    for (int node = 0; node < 5; ++node) {
        int x = 36 + node * 61;
        int y = 145 + (int)(wave((float)node * 0.19f + (float)s->frame * 0.003f) * 7.0f);
        rectangle(fb, x - 4, y - 4, x + 4, y + 4, 3u + (unsigned)node);
    }
}

static void draw_particle_current(AmbientVisualState *s, uint8_t *fb)
{
    float bass = s->smooth[1], mid = s->smooth[2], high = s->smooth[3];
    int previous_x = 160;
    int previous_y = 85;
    for (unsigned i = 0; i < 96u; ++i) {
        float u = (float)i / 96.0f;
        float p = u * 2.35f + (float)s->frame * 0.0019f;
        float spread = 14.0f + u * (88.0f + bass * 28.0f);
        int jitter = s->ridge[(i * 37u) % AMBIENT_DISPLAY_WIDTH] / 2;
        int x = 160 + (int)(wave(p) * spread) + jitter;
        int y = 85 + (int)(wave(p + 0.25f) * spread * (0.39f + mid * 0.10f)) +
                (int)(wave(u * 6.0f - (float)s->frame * 0.0027f) * (4.0f + high * 8.0f));
        unsigned tone = 5u + (i % 9u) + (unsigned)(high * 3.0f);
        if (tone > 15u) tone = 15u;
        pixel(fb, x, y, tone);
        circle(fb, x, y, 1, tone > 3u ? tone - 3u : tone);
        if ((i % 4u) == 0u) glow(fb, x, y, 1 + (int)(high * 2.0f), tone);
        if (i && (i % 2u) == 0u) {
            line(fb, previous_x, previous_y, x, y, tone > 8u ? tone - 8u : 2u);
        }
        previous_x = x;
        previous_y = y;
    }
}

static void draw_signal_chamber(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], high = s->smooth[3];
    int horizon = 61;
    for (int x = -32; x <= 352; x += 24) {
        line(fb, 160, horizon, x, 169, 3u + (unsigned)(rms * 3.0f));
    }
    for (int band = 0; band < 7; ++band) {
        int y = horizon + 5 + band * band * 2;
        if (y < 170) line(fb, 0, y, 319, y, 2u + (unsigned)band);
    }
    for (unsigned i = 0; i < 96u; ++i) {
        uint32_t noise = hash_u32(i * 2654435761u);
        int speed = 1 + (int)((noise >> 8u) % 4u) + (int)(high * 4.0f);
        int x = (int)(noise % AMBIENT_DISPLAY_WIDTH);
        int y = (int)(((noise >> 16u) + s->frame * (uint32_t)speed) % AMBIENT_DISPLAY_HEIGHT);
        unsigned tone = 5u + (noise >> 24u) % 11u;
        line(fb, x, y - speed * 2, x, y, tone);
        if ((i % 13u) == 0u && y > horizon) {
            line(fb, x - 5, y + 4, x, y - 4, tone > 5u ? tone - 5u : 2u);
            line(fb, x, y - 4, x + 5, y + 4, tone > 5u ? tone - 5u : 2u);
            line(fb, x + 5, y + 4, x - 5, y + 4, tone > 5u ? tone - 5u : 2u);
        }
    }
}

static void draw_resonance_orb(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], mid = s->smooth[2], high = s->smooth[3];
    int cx = 160;
    int cy = 85;
    for (int trace = 0; trace < 6; ++trace) {
        int previous_x = cx;
        int previous_y = cy - 45;
        for (int step = 1; step <= 96; ++step) {
            float p = (float)step / 96.0f;
            float grain = wave(p * (7.0f + (float)trace * 0.63f) +
                               (float)s->frame * (0.0021f + (float)trace * 0.0003f));
            float ripple = wave(p * 19.0f - (float)s->frame * 0.0041f + (float)trace * 0.13f);
            float radius = 42.0f + bass * 15.0f + grain * (3.0f + mid * 8.0f) +
                           ripple * high * 3.0f + (float)trace * 1.2f;
            int x = cx + (int)(wave(p) * radius);
            int y = cy + (int)(wave(p + 0.25f) * radius * (0.76f + rms * 0.10f));
            unsigned tone = 3u + (unsigned)trace * 2u + (unsigned)(rms * 3.0f);
            if (tone > 15u) tone = 15u;
            line(fb, previous_x, previous_y, x, y, tone);
            previous_x = x;
            previous_y = y;
        }
    }
    glow(fb, cx - 51 - (int)(bass * 10.0f), cy, 2, 15u);
    glow(fb, cx + 51 + (int)(bass * 10.0f), cy, 2, 15u);
}

static void draw_glitch_halo(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], mid = s->smooth[2], high = s->smooth[3];
    int cx = 160;
    int cy = 85;
    for (unsigned i = 0; i < 84u; ++i) {
        uint32_t noise = hash_u32(i * 2246822519u + (s->frame / 3u) * 3266489917u);
        float p = (float)i / 84.0f + (float)s->frame * 0.0015f;
        float radius = 40.0f + (float)(noise % 53u) +
                       wave(p * 7.0f + (float)s->frame * 0.003f) * (4.0f + mid * 9.0f);
        int x = cx + (int)(wave(p) * radius * 1.35f);
        int y = cy + (int)(wave(p + 0.25f) * radius * 0.76f);
        int size = 2 + (int)((noise >> 8u) % 7u) + (int)(rms * 2.0f);
        unsigned tone = 4u + (noise >> 16u) % 12u;
        rectangle(fb, x - size, y - size, x + size, y + size, tone);
        if ((noise & 15u) == 0u) glow(fb, x, y, 1 + (int)(high * 2.0f), 15u);
    }
    ellipse(fb, cx, cy, 46 + (int)(rms * 9.0f), 35 + (int)(rms * 6.0f), 4u);
}

static void draw_twin_pulse(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], mid = s->smooth[2];
    int cy = 85;
    line(fb, 3, cy, 316, cy, 10u + (unsigned)(rms * 5.0f));
    for (int layer = 0; layer < 6; ++layer) {
        int previous_top = cy;
        int previous_bottom = cy;
        for (int offset = -156; offset <= 156; offset += 2) {
            float distance = fabsf((float)offset) / 156.0f;
            float lobes = fabsf(wave(distance * (2.2f + mid * 1.4f) -
                                      (float)s->frame * 0.0019f));
            float envelope = (1.0f - distance) * (0.30f + lobes * 0.70f);
            float scale = 1.0f - (float)layer * 0.12f;
            int amplitude = (int)(envelope * scale * (29.0f + bass * 45.0f));
            int x = 160 + offset;
            int top = cy - amplitude;
            int bottom = cy + amplitude;
            unsigned tone = 5u + (unsigned)layer * 2u + (unsigned)(rms * 2.0f);
            if (tone > 15u) tone = 15u;
            if (offset > -156) {
                line(fb, x - 2, previous_top, x, top, tone);
                line(fb, x - 2, previous_bottom, x, bottom, tone);
            }
            previous_top = top;
            previous_bottom = bottom;
        }
    }
    glow(fb, 160, cy, 3 + (int)(rms * 7.0f), 15u);
}

static void draw_chroma_fall(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], bass = s->smooth[1], mid = s->smooth[2];
    int cx = 160 + (int)(wave((float)s->frame * 0.0017f) * 7.0f);
    for (int shell = 0; shell < 11; ++shell) {
        int rx = 16 + shell * 6;
        int ry = 24 + shell * 7 + (int)(bass * (float)(shell + 2));
        int cy = 35 + shell * 5 +
                 (int)(wave((float)s->frame * 0.0026f + (float)shell * 0.07f) *
                       (3.0f + mid * 4.0f));
        unsigned tone = 5u + (unsigned)shell + (unsigned)(rms * 2.0f);
        if (tone > 15u) tone = 15u;
        ellipse(fb, cx, cy, rx, ry, tone);
        if ((shell & 1) == 0) {
            line(fb, cx - rx, cy, cx - rx - shell * 2, 169, tone > 6u ? tone - 6u : 2u);
            line(fb, cx + rx, cy, cx + rx + shell * 2, 169, tone > 6u ? tone - 6u : 2u);
        }
    }
    for (int streak = -2; streak <= 2; ++streak) {
        int x = cx + streak * 8;
        int end = 104 + (int)(bass * 48.0f) - abs(streak) * 9;
        line(fb, x, 9, x, end, 10u + (unsigned)(2 - abs(streak)));
    }
    glow(fb, cx, 30, 4 + (int)(rms * 7.0f), 15u);
}

static void draw_softburst(AmbientVisualState *s, uint8_t *fb)
{
    float rms = s->smooth[0], mid = s->smooth[2], high = s->smooth[3];
    int cx = 160;
    int cy = 85;
    int rays = 34 + (int)(high * 20.0f);
    for (int i = 0; i < rays; ++i) {
        uint32_t noise = hash_u32((uint32_t)i * 1597334677u);
        float p = (float)i / (float)rays + (float)s->frame * 0.0012f;
        float start = 25.0f + (float)(noise % 54u) + mid * 12.0f;
        float length = 8.0f + (float)((noise >> 8u) % 31u) + high * 22.0f;
        int x0 = cx + (int)(wave(p) * start * 1.55f);
        int y0 = cy + (int)(wave(p + 0.25f) * start * 0.86f);
        int x1 = cx + (int)(wave(p) * (start + length) * 1.55f);
        int y1 = cy + (int)(wave(p + 0.25f) * (start + length) * 0.86f);
        unsigned tone = 5u + (noise >> 16u) % 11u;
        line(fb, x0, y0, x1, y1, tone);
        glow(fb, x0, y0, 1 + (int)(rms * 2.0f), tone);
        glow(fb, x1, y1, 1 + (int)(rms * 2.0f), tone);
    }
    for (int ring = 0; ring < 4; ++ring) {
        ellipse(fb, cx, cy, 15 + ring * 5, 10 + ring * 4,
                3u + (unsigned)ring + (unsigned)(rms * 2.0f));
    }
}

const char *ambient_visual_name(AmbientVisual visual)
{
    return (unsigned)visual < AMBIENT_VISUAL_COUNT ? k_visual_names[visual] : "UNKNOWN";
}

const char *ambient_visual_slug(AmbientVisual visual)
{
    return (unsigned)visual < AMBIENT_VISUAL_COUNT ? k_visual_slugs[visual] : "unknown";
}

void ambient_visual_init(AmbientVisualState *state, AmbientVisual visual, uint32_t seed)
{
    if (!state) return;
    if ((unsigned)visual >= AMBIENT_VISUAL_COUNT) visual = AMBIENT_VISUAL_RESONANT_GARDEN;
    memset(state, 0, sizeof(*state));
    state->visual = (uint8_t)visual;
    state->random_state = seed ? seed : 0x1f123bb5u;
    for (unsigned x = 0; x < AMBIENT_DISPLAY_WIDTH; ++x) {
        state->ridge[x] = (int16_t)((int)(random_u32(state) % 9u) - 4);
    }
}

void ambient_visual_render(AmbientVisualState *state, uint8_t *fb, AmbientVisualInput input)
{
    if (!state || !fb) return;
    float target[6] = {clamp01(input.rms), clamp01(input.bass), clamp01(input.mid),
                       clamp01(input.high), clamp01(input.centroid), clamp01(input.beat_phase)};
    for (unsigned i = 0; i < 6u; ++i) {
        state->smooth[i] += (target[i] - state->smooth[i]) * (i == 5u ? 0.36f : 0.18f);
    }
    memset(fb, 0, AMBIENT_DISPLAY_BYTES);
    switch ((AmbientVisual)state->visual) {
    case AMBIENT_VISUAL_RESONANT_GARDEN:  draw_garden(state, fb); break;
    case AMBIENT_VISUAL_ORBITAL_LOOM:     draw_orbital_loom(state, fb); break;
    case AMBIENT_VISUAL_SPECTRAL_CANYON:  draw_canyon(state, fb); break;
    case AMBIENT_VISUAL_RAIN_MEMORY:      draw_rain(state, fb); break;
    case AMBIENT_VISUAL_DREAM_TOPOGRAPHY: draw_topography(state, fb); break;
    case AMBIENT_VISUAL_FOCUS_RAIL:        draw_focus_rail(state, fb); break;
    case AMBIENT_VISUAL_PRISM_VEINS:       draw_prism_veins(state, fb); break;
    case AMBIENT_VISUAL_CRYSTAL_CHOIR:     draw_crystal_choir(state, fb); break;
    case AMBIENT_VISUAL_RADIANT_GATE:      draw_radiant_gate(state, fb); break;
    case AMBIENT_VISUAL_LUMEN_RIBBON:      draw_lumen_ribbon(state, fb); break;
    case AMBIENT_VISUAL_GLYPH_RELAY:       draw_glyph_relay(state, fb); break;
    case AMBIENT_VISUAL_PARTICLE_CURRENT:  draw_particle_current(state, fb); break;
    case AMBIENT_VISUAL_SIGNAL_CHAMBER:    draw_signal_chamber(state, fb); break;
    case AMBIENT_VISUAL_RESONANCE_ORB:     draw_resonance_orb(state, fb); break;
    case AMBIENT_VISUAL_GLITCH_HALO:       draw_glitch_halo(state, fb); break;
    case AMBIENT_VISUAL_TWIN_PULSE:        draw_twin_pulse(state, fb); break;
    case AMBIENT_VISUAL_CHROMA_FALL:       draw_chroma_fall(state, fb); break;
    case AMBIENT_VISUAL_SOFTBURST:         draw_softburst(state, fb); break;
    default: break;
    }
    ++state->frame;
}

size_t ambient_visual_state_bytes(void)
{
    return sizeof(AmbientVisualState);
}
