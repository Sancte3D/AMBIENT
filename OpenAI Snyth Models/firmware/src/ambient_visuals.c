#include "ambient_visuals.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_visual_names[AMBIENT_VISUAL_COUNT] = {
    "RESONANT GARDEN", "ORBITAL LOOM", "SPECTRAL CANYON", "RAIN MEMORY",
    "DREAM TOPOGRAPHY"
};

static const char *const k_visual_slugs[AMBIENT_VISUAL_COUNT] = {
    "resonant-garden", "orbital-loom", "spectral-canyon", "rain-memory",
    "dream-topography"
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
    default: break;
    }
    ++state->frame;
}

size_t ambient_visual_state_bytes(void)
{
    return sizeof(AmbientVisualState);
}
