/* ui_scene — see ui_scene.h for what was measured and why. */
#include "ui_scene.h"
#include "ui_draw.h"
#include "oled.h"

#include <math.h>
#include <string.h>

/* ---- palette -------------------------------------------------------------
 * One dim grey for everything inert — 42,42,42, which is within a hair of the
 * #353238 that carries over half of the measured OP-1 screen — and then
 * nothing but the four encoder colours. There is no decorative colour in this
 * system: if something is coloured, an encoder moves it. */
#define DIM_R   42
#define DIM_G   42
#define DIM_B   42

const uint8_t UI_KNOB_RGB[UI_SCENE_MAX_KNOBS][3] = {
    { 244,  86,  96 },   /* EN1 · RED    */
    {  92, 146, 255 },   /* EN2 · BLUE   */
    { 242, 200,  70 },   /* EN4 · YELLOW */
};
const uint8_t UI_NAV_RGB[3] = { 60, 230, 130 };   /* EN3 · GREEN — navigation */

/* The measured OP-1 weight: 1.5 px, which is a half-width of 0.75. Every
 * stroke in every scene uses it. One weight is most of why those screens hold
 * together. */
#define STROKE 0.75f
#define DOT    1.1f             /* the rx=ry=1 ellipse, with AA headroom */

/* ---- scene box ----------------------------------------------------------
 * Above the hub arc, which stays put underneath every scene as the one
 * constant the eye can hold on to. */
#define BX0  40.0f
#define BX1 280.0f
#define BY0  30.0f
#define BY1  98.0f   /* clears the ring, whose top edge is at y = 102.5 */
#define BCX ((BX0 + BX1) * 0.5f)
#define BCY ((BY0 + BY1) * 0.5f)
#define BW  (BX1 - BX0)
#define BH  (BY1 - BY0)

typedef uint8_t cov_t;

static void sc_line(cov_t *c, int y, float x0, float y0, float x1, float y1)
{
    ui_cov_capsule(c, y, x0, y0, x1, y1, STROKE);
}
static void sc_dot(cov_t *c, int y, float x, float yy)
{
    ui_cov_disc(c, y, x, yy, DOT);
}
static void sc_circle(cov_t *c, int y, float cx, float cy, float r)
{
    ui_cov_arc(c, y, cx, cy, r, STROKE, -180.0f, 180.0f);
}

/* Deterministic hash — a texture must look the same on every frame, or the
 * grain crawls. No rand() anywhere near a per-scanline path. */
static float h01(int i, int salt)
{
    /* UNSIGNED multiplies. In int these overflow for i > ~5, which is
     * undefined behaviour the compiler may assume cannot happen: at -O1
     * this returned plausible noise, at -O2 it hung the renderer. A hash
     * is exactly where signed overflow hides, because the values are
     * meant to wrap. */
    uint32_t x = (uint32_t)i * 374761393u + (uint32_t)salt * 668265263u;
    x = (x ^ (x >> 13)) * 1274126177u;
    return (float)((x ^ (x >> 16)) & 0xFFFF) / 65535.0f;
}

/* ---- knob names ---------------------------------------------------------- */
static const char *const KNOBS[9][UI_SCENE_MAX_KNOBS] = {
    { "World",  0,       0      },
    { "Synth",  "Voice", "Cell" },
    { "Key",    "Tune",  0      },
    { "Bass",   "Color", 0      },
    { "Space",  "Shimmer", 0    },
    { "Echo",   "Blur",  0      },
    { "Atmos",  "Age",   0      },
    { "Motion", 0,       0      },
    { "FX",     0,       0      },
};

const char *ui_scene_knob_name(int group, int knob)
{
    if (group < 0 || group > 8 || knob < 0 || knob >= UI_SCENE_MAX_KNOBS)
        return "";
    const char *s = KNOBS[group][knob];
    return s ? s : "";
}

/* ---- 0 · WORLD — a horizon ----------------------------------------------
 * Four worlds, four skylines. The parameter does not label the world, it IS
 * the shape of the land: a city cuts square, a coast rolls, a highway
 * converges to a point, a room is furniture. */
static void scene_world(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    int w = (int)(sc->v[0] * 3.0f + 0.5f);
    float pts[2 * 26];
    int n = 0;
    const float base = BY1 - 6.0f;

    if (w == 0) {                                   /* city: square teeth */
        float h[12] = { .18f,.55f,.32f,.78f,.45f,.95f,.30f,.62f,.40f,.85f,.25f,.50f };
        pts[n*2] = BX0; pts[n*2+1] = base; ++n;
        for (int i = 0; i < 12; ++i) {
            float x0 = BX0 + BW * i / 12.0f, x1 = BX0 + BW * (i + 1) / 12.0f;
            float yy = base - h[i] * (BH * 0.62f);
            pts[n*2] = x0; pts[n*2+1] = yy; ++n;
            pts[n*2] = x1; pts[n*2+1] = yy; ++n;
        }
        pts[n*2] = BX1; pts[n*2+1] = base; ++n;
    } else if (w == 1) {                            /* coast: rolling swell */
        for (int i = 0; i < 25; ++i) {
            float t = i / 24.0f;
            pts[n*2] = BX0 + BW * t;
            pts[n*2+1] = base - (BH * 0.30f) * (sinf(t * 6.2f) * 0.6f
                                                + sinf(t * 2.3f + 1.0f) * 0.4f + 1.0f);
            ++n;
        }
    } else if (w == 2) {                            /* highway: perspective */
        for (int i = 0; i < 25; ++i) {
            float t = i / 24.0f;
            pts[n*2] = BX0 + BW * t;
            pts[n*2+1] = base - (BH * 0.12f) * (1.0f + 0.4f * sinf(t * 3.0f));
            ++n;
        }
    } else {                                        /* after hours: interior */
        float h[8] = { .10f,.62f,.62f,.22f,.22f,.78f,.78f,.14f };
        for (int i = 0; i < 8; ++i) {
            pts[n*2] = BX0 + BW * i / 7.0f;
            pts[n*2+1] = base - h[i] * (BH * 0.55f);
            ++n;
        }
    }
    ui_cov_polyline(k[0], y, pts, n, STROKE);
    sc_line(dim, y, BX0, base, BX1, base);          /* ground */

    if (w == 2) {                                   /* vanishing point + road */
        sc_line(dim, y, BX0 + 10.0f, BY1, BCX, BY0 + 26.0f);
        sc_line(dim, y, BX1 - 10.0f, BY1, BCX, BY0 + 26.0f);
        sc_dot(k[0], y, BCX, BY0 + 26.0f);
    }
}

/* ---- 1 · SOUND — one cycle of the actual voice ---------------------------
 * Synth bends the waveform, Voice tilts its envelope, Cell puts the trigger
 * points on it. Three knobs, three visible features, no labels needed once. */
static void scene_sound(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    int shape = (int)(sc->v[0] * 6.0f + 0.5f);      /* 7 synth cores */
    float tilt = sc->v[1];                          /* voice            */
    int   trig = 1 + (int)(sc->v[2] * 2.0f + 0.5f); /* cell mode 1..3   */

    sc_line(dim, y, BX0, BCY, BX1, BCY);            /* zero line */

    float pts[2 * 61];
    int n = 0;
    for (int i = 0; i <= 60; ++i) {
        float t = i / 60.0f;
        float ph = t * 6.2831853f * 2.0f;
        float v;
        switch (shape) {
            case 0: v = sinf(ph); break;                              /* ambient */
            case 1: v = 2.0f * (t * 2.0f - floorf(t * 2.0f)) - 1.0f; break; /* saw */
            case 2: v = sinf(ph) > 0 ? 1.0f : -1.0f; break;           /* square  */
            case 3: v = sinf(ph) + 0.35f * sinf(ph * 3.0f); break;    /* mist    */
            case 4: v = sinf(ph + 2.0f * sinf(ph)); break;            /* storm/FM*/
            case 5: v = sinf(ph) * cosf(ph * 0.5f); break;            /* orbit   */
            default: v = 1.0f - 4.0f * fabsf(t * 2.0f - floorf(t * 2.0f) - 0.5f);
                     break;                                            /* bamboo */
        }
        /* envelope tilt: 0 = level, 1 = decaying */
        float env = 1.0f - tilt * t * 0.85f;
        pts[n*2]   = BX0 + BW * t;
        pts[n*2+1] = BCY - v * env * (BH * 0.36f);
        ++n;
    }
    ui_cov_polyline(k[1], y, pts, n, STROKE);

    for (int t_i = 0; t_i < trig; ++t_i) {          /* cell triggers */
        float t = (t_i + 0.5f) / trig;
        int i = (int)(t * 60.0f);
        sc_dot(k[2], y, pts[i*2], pts[i*2+1]);
        sc_line(k[2], y, pts[i*2], pts[i*2+1] + 4.0f, pts[i*2], BY1);
    }
}

/* ---- 2 · PITCH — the tuning made visible --------------------------------
 * Twelve pitches on a ring. Equal temperament spaces them evenly; just
 * intonation does not — and that is the whole point of the parameter, so the
 * SPACING is the readout. No number can show that as fast. */
static void scene_pitch(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    /* Just-intonation ratios as cents/1200, the real uneven ladder. */
    static const float JUST[12] = {
        0.0f, .1130f, .1699f, .2680f, .3219f, .4150f,
        .5145f, .5850f, .6781f, .7370f, .8301f, .8930f
    };
    int   key  = (int)(sc->v[0] * 11.0f + 0.5f);
    int   just = sc->v[1] > 0.5f;
    float r    = BH * 0.36f;

    sc_circle(dim, y, BCX, BCY, r);
    for (int i = 0; i < 12; ++i) {
        float f = just ? JUST[i] : (float)i / 12.0f;
        float a = f * 6.2831853f - 1.5707963f;
        float x = BCX + r * cosf(a), yy = BCY + r * sinf(a);
        if (i == key) {
            ui_cov_disc(k[0], y, x, yy, 3.2f);
            sc_line(k[0], y, BCX, BCY, x, yy);
        } else {
            sc_dot(just ? k[1] : dim, y, x, yy);
        }
    }
    /* The reference ring — where equal temperament would have put them —
     * sits INSIDE. Outside it ran past the top and bottom of the scene box and
     * collided with the label above and the ring below. */
    if (just)
        for (int i = 0; i < 12; ++i) {
            float a = (float)i / 12.0f * 6.2831853f - 1.5707963f;
            sc_dot(dim, y, BCX + (r - 7.0f) * cosf(a),
                           BCY + (r - 7.0f) * sinf(a));
        }
}

/* ---- 3 · HARMONY — a chord as a stack ----------------------------------- */
static void scene_harmony(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    int bass  = (int)(sc->v[0] * 3.0f + 0.5f);      /* Off/Root/Fifth/Drift */
    int color = (int)(sc->v[1] * 3.0f + 0.5f);      /* Pure/Open/Warm/Deep  */
    static const float SPREAD[4][3] = {
        { 0.00f, 0.28f, 0.55f },                     /* pure  */
        { 0.00f, 0.42f, 0.80f },                     /* open  */
        { 0.00f, 0.24f, 0.44f },                     /* warm  */
        { 0.00f, 0.50f, 0.95f },                     /* deep  */
    };
    for (int i = 0; i < 5; ++i) {                    /* the stave */
        float yy = BY0 + 8.0f + i * (BH - 16.0f) / 4.0f;
        sc_line(dim, y, BX0, yy, BX1, yy);
    }
    float top = BY1 - 8.0f, span = BH - 16.0f;
    for (int i = 0; i < 3; ++i) {
        float yy = top - SPREAD[color][i] * span;
        ui_cov_disc(k[1], y, BCX - 26.0f + i * 26.0f, yy, 2.6f);
    }
    if (bass > 0) {                                  /* the bass voice */
        float yy = top + 4.0f;
        float x  = BCX - 52.0f;
        ui_cov_disc(k[0], y, x, yy, 3.4f);
        if (bass >= 2) sc_line(k[0], y, x, yy, x + 26.0f, top - SPREAD[color][1] * span);
        if (bass == 3) sc_line(k[0], y, x, yy, x, yy - 12.0f);
    }
}

/* ---- 4 · ROOM — how far the sound goes ---------------------------------- */
static void scene_room(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    float space = sc->v[0], shim = sc->v[1];
    float rmax  = 14.0f + space * (BH * 0.52f);

    sc_dot(dim, y, BCX, BCY + BH * 0.22f);
    for (int i = 1; i <= 5; ++i) {                   /* the room, opening up */
        float r = rmax * i / 5.0f;
        ui_cov_arc(dim, y, BCX, BCY + BH * 0.22f, r, STROKE, -78.0f, 78.0f);
    }
    ui_cov_arc(k[0], y, BCX, BCY + BH * 0.22f, rmax, STROKE, -78.0f, 78.0f);

    int n = (int)(shim * 6.0f + 0.5f);               /* shimmer: upper partials */
    for (int i = 0; i < n; ++i) {
        float r = rmax * (0.35f + 0.13f * i);
        float a = (-60.0f + 24.0f * i) * UI_DEG2RAD;
        float x = BCX + r * sinf(a), yy = BCY + BH * 0.22f - r * cosf(a);
        sc_dot(k[1], y, x, yy);
    }
}

/* ---- 5 · TIME — repeats, and how hard they smear ------------------------ */
static void scene_time(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    float echo = sc->v[0], blur = sc->v[1];
    float base = BY1 - 10.0f;

    sc_line(dim, y, BX0, base, BX1, base);
    sc_line(dim, y, BX0 + 6.0f, base, BX0 + 6.0f, BY0 + 6.0f);   /* the source */

    int n = 1 + (int)(echo * 9.0f + 0.5f);
    for (int i = 1; i <= n; ++i) {
        float t   = (float)i / (float)(n + 1);
        float x   = BX0 + 6.0f + (BW - 12.0f) * t;
        float amp = (BH - 16.0f) * (1.0f - t) * (1.0f - t);
        /* blur widens each repeat into a band instead of a tick, which is
         * what a granular smear actually does to a transient */
        int w = 1 + (int)(blur * 5.0f + 0.5f);
        for (int kk = 0; kk < w; ++kk) {
            float xx = x + (kk - (w - 1) * 0.5f) * 2.4f;
            /* the centre tick belongs to Echo, the spread around it to Blur */
            sc_line((w > 1 && kk != (w - 1) / 2) ? k[1] : k[0],
                    y, xx, base, xx, base - amp);
        }
    }
}

/* ---- 6 · TEXTURE — grain, and how worn it is ---------------------------- */
static void scene_texture(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    float atmos = sc->v[0], age = sc->v[1];
    int   n     = 12 + (int)(atmos * 78.0f);

    sc_line(dim, y, BX0, BY0, BX1, BY0);
    sc_line(dim, y, BX0, BY1, BX1, BY1);

    for (int i = 0; i < n; ++i) {
        float gx = BX0 + 4.0f + h01(i, 1) * (BW - 8.0f);
        float gy = BY0 + 4.0f + h01(i, 2) * (BH - 8.0f);
        /* age eats holes in the field and jitters what is left */
        if (h01(i, 3) < age * 0.45f) continue;
        float shift = (h01(i, 4) - 0.5f) * age * 14.0f;
        int   moved = (shift > 1.5f || shift < -1.5f);
        gy += shift;
        if (gy < BY0 + 2.0f || gy > BY1 - 2.0f) continue;
        /* a grain the age has moved belongs to Age, the rest to Atmosphere */
        ui_cov_disc(moved ? k[1] : k[0], y, gx, gy, 0.9f);
    }
}

/* ---- 7 · MOTION — the drift, as an orbit -------------------------------- */
static void scene_motion(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    float m = sc->v[0];
    float rx = BW * 0.30f, ry = BH * 0.34f;

    sc_line(dim, y, BCX - rx - 8.0f, BCY, BCX + rx + 8.0f, BCY);
    sc_line(dim, y, BCX, BCY - ry - 6.0f, BCX, BCY + ry + 6.0f);

    /* A Lissajous whose ratio opens with the parameter: at 0 it is a flat
     * line, at 1 a wide slow loop. The figure IS the amount of movement. */
    float pts[2 * 81];
    int n = 0;
    float ratio = 1.0f + m * 2.0f;
    for (int i = 0; i <= 80; ++i) {
        float t = i / 80.0f * 6.2831853f;
        pts[n*2]   = BCX + rx * sinf(t * ratio);
        pts[n*2+1] = BCY + ry * sinf(t) * (0.08f + 0.92f * m);
        ++n;
    }
    ui_cov_polyline(k[0], y, pts, n, STROKE);
}

/* ---- 8 · FX — the chain, and which link is lit -------------------------- */
static void scene_fx(const ui_scene_t *sc, int y, cov_t *dim, cov_t **k)
{
    int sel = (int)(sc->v[0] * 8.0f + 0.5f);         /* 9 effects */
    float yy = BCY;

    sc_line(dim, y, BX0, yy, BX1, yy);
    for (int i = 0; i < 9; ++i) {
        float x = BX0 + 12.0f + (BW - 24.0f) * i / 8.0f;
        if (i == sel) {
            sc_circle(k[0], y, x, yy, 9.0f);
            /* the tick hangs DOWN: upward it would run into the one line of
             * type this scene is allowed */
            sc_line(k[0], y, x, yy + 9.0f, x, BY1 - 2.0f);
        } else {
            sc_dot(dim, y, x, yy);
        }
    }
    /* bypass reads as the signal passing straight through, untouched */
    if (sel == 0) sc_line(dim, y, BX0, yy - 5.0f, BX1, yy - 5.0f);
}

/* ---- entry point --------------------------------------------------------
 * Three coverage masks, one per semantic tone, each flushed once. Grouping by
 * MEANING rather than by draw order is what keeps a scene from turning into a
 * pile of overlapping strokes at different opacities. */
void ui_scene_row(const ui_scene_t *sc, int y, uint16_t *line, int alpha)
{
    static cov_t dim[OLED_WIDTH];
    static cov_t kc[UI_SCENE_MAX_KNOBS][OLED_WIDTH];
    cov_t *k[UI_SCENE_MAX_KNOBS] = { kc[0], kc[1], kc[2] };

    switch (sc->group) {
        case 0: scene_world  (sc, y, dim, k); break;
        case 1: scene_sound  (sc, y, dim, k); break;
        case 2: scene_pitch  (sc, y, dim, k); break;
        case 3: scene_harmony(sc, y, dim, k); break;
        case 4: scene_room   (sc, y, dim, k); break;
        case 5: scene_time   (sc, y, dim, k); break;
        case 6: scene_texture(sc, y, dim, k); break;
        case 7: scene_motion (sc, y, dim, k); break;
        default: scene_fx    (sc, y, dim, k); break;
    }

    /* Inert structure first, then each encoder's own colour — one flush per
     * tone, so a union of same-coloured strokes never seams. */
    ui_cov_flush(line, dim, DIM_R, DIM_G, DIM_B, alpha);
    for (int i = 0; i < UI_SCENE_MAX_KNOBS; ++i)
        ui_cov_flush(line, kc[i], UI_KNOB_RGB[i][0], UI_KNOB_RGB[i][1],
                     UI_KNOB_RGB[i][2], alpha);
}
