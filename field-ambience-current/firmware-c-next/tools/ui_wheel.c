/*
 * ui_wheel — implementation. See ui_wheel.h for the measured geometry.
 *
 * Shapes are drawn from signed distance fields evaluated per pixel on the
 * current scanline. That choice is not decoration: the wheel rotates, and a
 * rotating branch drawn without antialiasing crawls badly at 320 x 170. Text
 * stays binary (ui_draw.c) because Bitcount is a grid font and AA smears its
 * lattice — the two opposite rules live side by side on purpose.
 *
 * Cost is bounded by geometry, not by the panel: each row touches at most five
 * branches, five nodes, one ring and one icon, each over its own x-span. There
 * is still no framebuffer.
 */
#include "ui_wheel.h"
#include "ui_draw.h"
#include "oled.h"
#include "baked_font.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---- palette ------------------------------------------------------------
 * Estimated from the reference render; the exact values should come from the
 * Figma export when it exists. Green is the only chromatic colour in the
 * system and marks exactly one thing: what is active or changeable. */
#define BG_R      0
#define BG_G      0
#define BG_B      0
/* ONE inactive grey, used by the branch, the inactive node and the ring alike.
 * They are one object drawn in three parts, so three near-but-not-equal greys
 * read as a rendering fault rather than as hierarchy. Neutral, too: a blue
 * cast on the darks is invisible in isolation and obvious the moment two of
 * them touch. */
#define DIM_R    42
#define DIM_G    42
#define DIM_B    42
#define SEL_R   232            /* selected node body */
#define SEL_G   232
#define SEL_B   232
#define ICON_R  126            /* icon inside an inactive node */
#define ICON_G  126
#define ICON_B  126
#define GRN_R   124
#define GRN_G   240
#define GRN_B   132

/* ---- geometry (panel px, from ui_wheel.h) ------------------------------- */
#define HX          158.0f
#define HY          171.0f     /* 1 px below the bottom edge — deliberate */
#define R_ORBIT     104.0f     /* MAIN node orbit */
#define R_NODE       22.0f
#define R_ORBIT_SUB  92.0f     /* GROUP: shorter branches, smaller nodes,
                                * so depth reads as scale, not as chrome */
#define R_NODE_SUB   15.0f
#define R_RING       64.0f     /* the shared circle */
#define RING_HALF_T   4.5f
#define BRANCH_HALF_W 4.0f
/* Branches start on the ring's CENTRELINE. Their rounded cap has radius
 * BRANCH_HALF_W, so it spans 60..68 and lands entirely inside the ring band
 * (59.5..68.5): no gap on the outside, and — the reason for the exact value —
 * no stub poking through into the black inside the circle, which is what
 * starting them further in produced. */
#define R_BRANCH0    R_RING
#define R_ORB         6.5f     /* value orb — 13 px across, not 20+ */

/* Sweep of the value arc. Not the full visible half: the ring crosses the
 * bottom edge at +-89.1 deg, so ends at +-90 would put the orb centre exactly
 * on y = 171 and clip the value readout at both extremes — 0 and 100 are
 * precisely the two values that must be unambiguous. At +-78 deg both ends sit
 * at y = 158 with the whole orb on the panel. */
#define VAL_A0      (-78.0f)
#define VAL_A1       (78.0f)

#define DEG2RAD 0.017453293f

/* ---- model -------------------------------------------------------------- */
static const char *const P_LABEL[WHEEL_PARAM_COUNT] = {
    "World","Key","Tuning","Voice","Space","Shimmer","Atmosphere","Motion",
    "Age","Echo","Blur","Synth","Cell","Bass","Color","FX"
};
static const char *const WORLD_NAMES[]  = { "Tokyo City","Crystal Coast",
                                            "Midnight Drive","After Hours" };
static const char *const KEY_NAMES[]    = { "C","C#","D","D#","E","F",
                                            "F#","G","G#","A","A#","B" };
static const char *const TUNING_NAMES[] = { "Equal","Just" };
static const char *const VOICE_NAMES[]  = { "Pad","String","Glass","Ember" };
static const char *const SYNTH_NAMES[]  = { "Ambient","Acid","FM Glass","Mist",
                                            "Storm","Orbit","Bamboo" };
static const char *const CELL_NAMES[]   = { "Note","Harmony","Land" };
static const char *const BASS_NAMES[]   = { "Off","Root","Fifth","Drift" };
static const char *const COLOR_NAMES[]  = { "Pure","Open","Warm","Deep" };
static const char *const FX_NAMES[]     = { "Bypass","Reverb","Delay","Chorus",
                                            "Tape","Swell","Shimmer","Blur","Dream" };

static const char *const *P_OPTS[WHEEL_PARAM_COUNT] = {
    WORLD_NAMES, KEY_NAMES, TUNING_NAMES, VOICE_NAMES, 0, 0, 0, 0,
    0, 0, 0, SYNTH_NAMES, CELL_NAMES, BASS_NAMES, COLOR_NAMES, FX_NAMES
};
static const uint8_t P_NOPT[WHEEL_PARAM_COUNT] = {
    4, 12, 2, 4, 0, 0, 0, 0, 0, 0, 0, 7, 3, 4, 4, 9
};

/* Nine groups because the reference's branches sit 40 deg apart and
 * 360 / 40 = 9. Sizes are uneven on purpose: a group of one goes straight from
 * the main wheel to its value, which is the correct behaviour for World, and
 * costs no extra rule. */
typedef struct {
    const char *name;
    uint8_t     n;
    uint8_t     p[3];
} wgroup_t;

/* No group name may equal one of its own members, or the sub-wheel prints the
 * same word twice and the heading reads as a repeated row. */
static const wgroup_t GROUPS[WHEEL_GROUPS] = {
    { "World",   1, { 0 } },
    { "Sound",   3, { 11, 3, 12 } },      /* Synth, Voice, Cell   */
    { "Pitch",   2, { 1, 2 } },           /* Key, Tuning          */
    { "Harmony", 2, { 13, 14 } },         /* Bass, Color          */
    { "Room",    2, { 4, 5 } },           /* Space, Shimmer       */
    { "Time",    2, { 9, 10 } },          /* Echo, Blur           */
    { "Texture", 2, { 6, 8 } },           /* Atmosphere, Age      */
    { "Motion",  1, { 7 } },
    { "FX",      1, { 15 } },
};

int ui_wheel_group_size(int g)          { return GROUPS[g].n; }
int ui_wheel_param_of(int g, int m)     { return GROUPS[g].p[m]; }
const char *ui_wheel_group_name(int g)  { return GROUPS[g].name; }
const char *ui_wheel_param_label(int p) { return P_LABEL[p]; }

const char *ui_wheel_param_value(const wheel_state_t *st, int p)
{
    static char buf[8];
    if (P_NOPT[p]) {
        int i = st->val[p];
        if (i >= P_NOPT[p]) i = P_NOPT[p] - 1;
        return P_OPTS[p][i];
    }
    snprintf(buf, sizeof buf, "%d", st->val[p]);
    return buf;
}

/* ---- state -------------------------------------------------------------- */
void ui_wheel_init(wheel_state_t *st)
{
    static const uint8_t SEED[WHEEL_PARAM_COUNT] = {
        0, 9, 0, 0, 62, 38, 74, 45, 21, 56, 33, 0, 1, 1, 2, 8
    };
    memset(st, 0, sizeof *st);
    st->level = WHEEL_MAIN;
    st->batt  = 78;
    memcpy(st->val, SEED, sizeof SEED);
}

static int level_nodes(const wheel_state_t *st)
{
    return (st->level == WHEEL_MAIN) ? WHEEL_GROUPS
                                     : GROUPS[st->group].n;
}

void ui_wheel_turn(wheel_state_t *st, int dir, int coarse)
{
    if (st->level == WHEEL_VALUE) {
        int p = ui_wheel_param_of(st->group, st->member);
        if (P_NOPT[p]) {
            int i = (int)st->val[p] + dir;
            while (i < 0) i += P_NOPT[p];
            st->val[p] = (uint8_t)(i % P_NOPT[p]);
        } else {
            int v = (int)st->val[p] + dir * (coarse ? 10 : 2);
            if (v < 0)   v = 0;
            if (v > 100) v = 100;
            st->val[p] = (uint8_t)v;
        }
        return;
    }

    int n = level_nodes(st);
    /* The target always moves by exactly one step in the turned direction, so
     * wrapping from the last node to the first rotates one step rather than
     * spinning all the way back around. */
    st->theta_target -= dir * WHEEL_STEP_DEG;
    if (st->level == WHEEL_MAIN) {
        int g = ((int)st->group + dir) % n;
        if (g < 0) g += n;
        st->group  = (uint8_t)g;
        st->member = 0;
    } else {
        int m = ((int)st->member + dir) % n;
        if (m < 0) m += n;
        st->member = (uint8_t)m;
    }
}

/* Point the wheel at whatever index the current level selects, taking the
 * short way round. `theta` accumulates freely as the user turns, so the wanted
 * angle has to be resolved to the 360-periodic representative nearest to where
 * the wheel already is — otherwise stepping back out of a group unwinds the
 * whole rotation that got you there. */
static void wheel_retarget(wheel_state_t *st)
{
    int   idx  = (st->level == WHEEL_MAIN) ? st->group : st->member;
    float want = -(float)idx * WHEEL_STEP_DEG;
    float d    = want - st->theta;
    d -= 360.0f * floorf(d / 360.0f + 0.5f);
    st->theta_target = st->theta + d;
}

void ui_wheel_press(wheel_state_t *st, int back)
{
    if (back) {
        if (st->level == WHEEL_VALUE)
            st->level = (GROUPS[st->group].n > 1) ? WHEEL_GROUP : WHEEL_MAIN;
        else if (st->level == WHEEL_GROUP) st->level = WHEEL_MAIN;
        wheel_retarget(st);
        return;
    }
    if (st->level == WHEEL_MAIN) {
        st->member = 0;
        /* A group of one has nothing to choose: a sub-wheel with a single
         * branch is a menu that asks a question with one answer. Drop straight
         * to the value. */
        st->level = (GROUPS[st->group].n > 1) ? WHEEL_GROUP : WHEEL_VALUE;
    } else if (st->level == WHEEL_GROUP) {
        st->level = WHEEL_VALUE;
    } else {
        st->level = (GROUPS[st->group].n > 1) ? WHEEL_GROUP : WHEEL_MAIN;
    }
    wheel_retarget(st);
}

int ui_wheel_tick(wheel_state_t *st, int dt_ms)
{
    float d = st->theta_target - st->theta;
    if (d > -0.05f && d < 0.05f) { st->theta = st->theta_target; return 0; }
    /* Exponential ease: fast off the detent, settling into the lock rather
     * than arriving at constant speed. ~120 ms to visually settle. */
    float k = 1.0f - expf(-(float)dt_ms / 40.0f);
    st->theta += d * k;
    return 1;
}

void ui_wheel_settle(wheel_state_t *st) { st->theta = st->theta_target; }

/* ---- signed-distance scanline primitives --------------------------------
 * Primitives do NOT blend into the line buffer. They accumulate COVERAGE into
 * a per-row byte mask with max(), and a whole group of same-coloured shapes is
 * blended once at the end.
 *
 * That is not an optimisation, it is the fix for a visible seam. Compositing
 * two overlapping shapes of the same colour in sequence never reaches full
 * opacity: where each covers half a pixel the result lands at 0.75 of the
 * colour, so every junction — branch into ring, stem into node, the four
 * strokes crossing in the FX icon — drew itself a darker hairline. Taking the
 * maximum of the coverages first makes a union behave like one shape.
 */
static inline int cov255(float d)          /* d = signed distance in px */
{
    float c = 0.5f - d;
    if (c <= 0.0f) return 0;
    if (c >= 1.0f) return 255;
    return (int)(c * 255.0f + 0.5f);
}

static inline void cov_put(uint8_t *cov, int x, int c)
{
    if (c > cov[x]) cov[x] = (uint8_t)c;
}

/* Blend the accumulated mask in one pass and clear it for the next group. */
static void cov_flush(uint16_t *line, uint8_t *cov, int cr, int cg, int cb,
                      int alpha)
{
    for (int x = 0; x < OLED_WIDTH; ++x) {
        if (cov[x]) {
            ui_blend_px(&line[x], cr, cg, cb, cov[x] * alpha / 255);
            cov[x] = 0;
        }
    }
}

static void cov_disc(uint8_t *cov, int y, float cx, float cy, float r)
{
    float dy = (float)y - cy;
    if (dy < -r - 1.0f || dy > r + 1.0f) return;
    int x0 = (int)(cx - r - 1.0f), x1 = (int)(cx + r + 2.0f);
    if (x0 < 0) x0 = 0;
    if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;
    for (int x = x0; x < x1; ++x) {
        float dx = (float)x - cx;
        cov_put(cov, x, cov255(sqrtf(dx * dx + dy * dy) - r));
    }
}

/* Capsule: the branch. Distance to the segment, minus the half width. */
static void cov_capsule(uint8_t *cov, int y, float ax, float ay,
                        float bx, float by, float r)
{
    float ylo = (ay < by ? ay : by) - r - 1.0f;
    float yhi = (ay > by ? ay : by) + r + 1.0f;
    if ((float)y < ylo || (float)y > yhi) return;

    float xlo = (ax < bx ? ax : bx) - r - 1.0f;
    float xhi = (ax > bx ? ax : bx) + r + 2.0f;
    int x0 = (int)xlo, x1 = (int)xhi;
    if (x0 < 0) x0 = 0;
    if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;

    float ex = bx - ax, ey = by - ay;
    float ee = ex * ex + ey * ey;
    if (ee < 1e-6f) ee = 1e-6f;

    for (int x = x0; x < x1; ++x) {
        float px = (float)x - ax, py = (float)y - ay;
        float t = (px * ex + py * ey) / ee;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float qx = px - ex * t, qy = py - ey * t;
        cov_put(cov, x, cov255(sqrtf(qx * qx + qy * qy) - r));
    }
}

/* Arc of an annulus, angles measured from 12 o'clock, positive clockwise.
 * The ends are rounded so a value fill terminates like the orb, not like a
 * cut. */
static void cov_arc(uint8_t *cov, int y, float cx, float cy, float r,
                    float half_t, float a0, float a1)
{
    float ro = r + half_t;
    float dy = (float)y - cy;
    if (dy >= -ro - 1.0f && dy <= ro + 1.0f) {
        int x0 = (int)(cx - ro - 1.0f), x1 = (int)(cx + ro + 2.0f);
        if (x0 < 0) x0 = 0;
        if (x1 > OLED_WIDTH) x1 = OLED_WIDTH;
        for (int x = x0; x < x1; ++x) {
            float dx = (float)x - cx;
            float dist = sqrtf(dx * dx + dy * dy);
            float dr = fabsf(dist - r) - half_t;
            if (dr > 0.75f) continue;                 /* outside the band */
            float ang = atan2f(dx, -dy) / DEG2RAD;    /* 0 = up, + = right */
            if (ang >= a0 && ang <= a1) cov_put(cov, x, cov255(dr));
        }
    }
    for (int e = 0; e < 2; ++e) {                     /* rounded caps */
        float a = (e ? a1 : a0) * DEG2RAD;
        cov_disc(cov, y, cx + r * sinf(a), cy - r * cosf(a), half_t);
    }
}

/* ---- icons --------------------------------------------------------------
 * Local coordinates run -20..20, so one unit is 1/20 of the icon radius. Each
 * icon is one to five primitives from the same vocabulary as the interface
 * itself (disc, capsule, arc) — they are built out of the UI's own geometry
 * rather than borrowed from an icon set, which is what keeps them reading as
 * instrument marks instead of app icons.
 *
 * They never rotate: nodes are circles, and the icon is emitted in screen
 * space at the node's position, so the required icon_rotation = -theta is a
 * property of the construction rather than a correction applied afterwards. */
enum { IP_DISC, IP_CAP, IP_ARC };

typedef struct {
    uint8_t kind;
    int8_t  x0, y0, x1, y1;   /* ARC: (x0,y0) centre, x1 radius            */
    int8_t  r;                /* DISC/CAP radius, ARC half-thickness       */
    int16_t a0, a1;           /* ARC only, degrees from 12 o'clock         */
} iprim_t;

#define D(x, y, r)             { IP_DISC, (x), (y), 0, 0, (r), 0, 0 }
#define C(x0, y0, x1, y1, r)   { IP_CAP, (x0), (y0), (x1), (y1), (r), 0, 0 }
#define A(x, y, rad, t, a, b)  { IP_ARC, (x), (y), (rad), 0, (t), (a), (b) }

static const iprim_t IC_WORLD[]   = { A(0,0,16,2,-180,180), C(-16,0,16,0,2) };
static const iprim_t IC_SYNTH[]   = { C(-16,6,-6,6,2), C(-6,6,-6,-6,2),
                                      C(-6,-6,6,-6,2), C(6,-6,6,6,2),
                                      C(6,6,16,6,2) };
static const iprim_t IC_KEY[]     = { D(-10,9,4), D(7,9,4),
                                      C(-10,9,-10,-11,2), C(7,9,7,-11,2),
                                      C(-10,-11,7,-11,2) };
static const iprim_t IC_HARMONY[] = { C(-6,-10,6,-10,3), C(-13,0,13,0,3),
                                      C(-9,10,9,10,3) };
static const iprim_t IC_SPACE[]   = { A(0,10,9,2,-75,75), A(0,10,17,2,-75,75) };
static const iprim_t IC_ECHO[]    = { D(-11,0,6), D(2,0,4), D(12,0,2) };
static const iprim_t IC_TEXTURE[] = { D(-12,-8,3), D(0,-13,3), D(11,-4,3),
                                      D(-6,8,3), D(9,10,3) };
static const iprim_t IC_MOTION[]  = { A(-8,0,9,2,-90,90), A(8,0,9,2,90,270) };
static const iprim_t IC_FX[]      = { C(0,-16,0,16,2), C(-16,0,16,0,2),
                                      C(-11,-11,11,11,2), C(-11,11,11,-11,2) };

#undef D
#undef C
#undef A

typedef struct { const iprim_t *p; uint8_t n; } icon_t;
#define ICON(a) { a, (uint8_t)(sizeof(a) / sizeof((a)[0])) }

static const icon_t ICONS[WHEEL_GROUPS] = {
    ICON(IC_WORLD), ICON(IC_SYNTH), ICON(IC_KEY), ICON(IC_HARMONY),
    ICON(IC_SPACE), ICON(IC_ECHO), ICON(IC_TEXTURE), ICON(IC_MOTION),
    ICON(IC_FX),
};
#undef ICON

static void cov_icon(uint8_t *cov, int y, const icon_t *ic,
                     float cx, float cy, float rad)
{
    float s = rad / 20.0f;
    for (int i = 0; i < ic->n; ++i) {
        const iprim_t *q = &ic->p[i];
        switch (q->kind) {
            case IP_DISC:
                cov_disc(cov, y, cx + q->x0 * s, cy + q->y0 * s, q->r * s);
                break;
            case IP_CAP:
                cov_capsule(cov, y, cx + q->x0 * s, cy + q->y0 * s,
                            cx + q->x1 * s, cy + q->y1 * s, q->r * s);
                break;
            default:
                cov_arc(cov, y, cx + q->x0 * s, cy + q->y0 * s, q->x1 * s,
                        q->r * s, (float)q->a0, (float)q->a1);
                break;
        }
    }
}

/* ---- widgets ------------------------------------------------------------ */

/* One solid pill, exactly as the reference draws it. A dim track with a
 * proportional fill on top makes two rounded caps meet in the middle of a
 * 26 x 12 px shape, and at any partial charge that reads as a blob rather than
 * as a battery. Charge is carried by COLOUR instead — the only thing legible
 * at this size anyway, and it keeps the mark to one shape. */
static void wheel_battery(uint16_t *line, int y, int pct)
{
    const int x = 267, w = 26, top = 19, h = 12;
    int r = GRN_R, g = GRN_G, b = GRN_B;
    if (pct <= 10)      { r = 244; g =  90; b =  76; }   /* critical */
    else if (pct <= 25) { r = 246; g = 190; b =  84; }   /* low      */
    ui_row_pill(line, y, top, h, x, w, r, g, b, 255);
}

static void text_centre(uint16_t *line, int y, int ytop,
                        const bakedfont_t *f, const char *s,
                        int cr, int cg, int cb, int a)
{
    char buf[32];
    const char *t = ui_fit_text(f, s, 240, buf, sizeof buf);
    ui_row_text(line, y, ytop, (OLED_WIDTH - ui_text_w(f, t)) / 2, f, t,
                cr, cg, cb, a);
}

/* The circle itself. Its span depends on what job it is doing:
 *
 *   hub (MAIN/GROUP)  -92..+92, i.e. the whole visible half with both caps off
 *                     the bottom edge. It has to reach past the outermost
 *                     branch, or the +-80 deg arms emerge from nothing —
 *                     stopping it at the value sweep left them floating.
 *   value             VAL_A0..VAL_A1, because there the ends are the limits of
 *                     the parameter and must be visible as ends.
 */
#define HUB_A0 (-92.0f)
#define HUB_A1  (92.0f)

/* Node centre for a wheel angle. */
static void node_xy(float deg, float orbit, float *nx, float *ny)
{
    float a = deg * DEG2RAD;
    *nx = HX + orbit * sinf(a);
    *ny = HY - orbit * cosf(a);
}

/* Is this node worth scanning at all? One fully off the panel still costs its
 * bounding box otherwise. */
static int node_visible(float nx, float ny, float nr)
{
    if (nx < -nr - 12.0f || nx > OLED_WIDTH + nr + 12.0f) return 0;
    if (ny - nr - 12.0f > (float)OLED_HEIGHT) return 0;
    return 1;
}

/* Branches, hub and node bodies all go into ONE coverage mask before being
 * blended, so the wheel behaves as a single object: no seam where a branch
 * enters the ring, and the ring is never notched by an arm crossing it. */
static void cov_branch(uint8_t *cov, int y, float deg, float orbit)
{
    float nx, ny, a = deg * DEG2RAD;
    node_xy(deg, orbit, &nx, &ny);
    if (!node_visible(nx, ny, 24.0f)) return;
    cov_capsule(cov, y, HX + R_BRANCH0 * sinf(a), HY - R_BRANCH0 * cosf(a),
                nx, ny, BRANCH_HALF_W);
}

static void cov_node_body(uint8_t *cov, int y, float deg, float orbit, float nr)
{
    float nx, ny;
    node_xy(deg, orbit, &nx, &ny);
    if (!node_visible(nx, ny, nr)) return;
    cov_disc(cov, y, nx, ny, nr);
}

static void cov_node_icon(uint8_t *cov, int y, float deg, float orbit,
                          float nr, const icon_t *ic)
{
    float nx, ny;
    node_xy(deg, orbit, &nx, &ny);
    if (!node_visible(nx, ny, nr)) return;
    cov_icon(cov, y, ic, nx, ny, nr * 0.42f);
}

/* ---- levels ------------------------------------------------------------- */
static void compose_main(const wheel_state_t *st, int y, uint16_t *line)
{
    static uint8_t cov[OLED_WIDTH];        /* cov_flush leaves it zeroed */

    for (int i = 0; i < WHEEL_GROUPS; ++i)
        cov_branch(cov, y, i * WHEEL_STEP_DEG + st->theta, R_ORBIT);
    cov_arc(cov, y, HX, HY, R_RING, RING_HALF_T, HUB_A0, HUB_A1);
    for (int i = 0; i < WHEEL_GROUPS; ++i)
        if (i != st->group)
            cov_node_body(cov, y, i * WHEEL_STEP_DEG + st->theta,
                          R_ORBIT, R_NODE);
    cov_flush(line, cov, DIM_R, DIM_G, DIM_B, 255);

    for (int i = 0; i < WHEEL_GROUPS; ++i)
        if (i != st->group)
            cov_node_icon(cov, y, i * WHEEL_STEP_DEG + st->theta,
                          R_ORBIT, R_NODE, &ICONS[i]);
    cov_flush(line, cov, ICON_R, ICON_G, ICON_B, 255);

    float sel_deg = st->group * WHEEL_STEP_DEG + st->theta;
    cov_node_body(cov, y, sel_deg, R_ORBIT, R_NODE);
    cov_flush(line, cov, SEL_R, SEL_G, SEL_B, 255);

    cov_node_icon(cov, y, sel_deg, R_ORBIT, R_NODE, &ICONS[st->group]);
    cov_flush(line, cov, 0, 0, 0, 255);

    text_centre(line, y, 18, &font_hn_value_small,
                ui_wheel_group_name(st->group), 255, 255, 255, 235);
}

static void compose_group(const wheel_state_t *st, int y, uint16_t *line)
{
    static uint8_t cov[OLED_WIDTH];
    const wgroup_t *g = &GROUPS[st->group];
    int p = ui_wheel_param_of(st->group, st->member);

    for (int i = 0; i < g->n; ++i)
        cov_branch(cov, y, i * WHEEL_STEP_DEG + st->theta, R_ORBIT_SUB);
    cov_arc(cov, y, HX, HY, R_RING, RING_HALF_T, HUB_A0, HUB_A1);
    /* Sub-nodes carry no icon. The group icon on every branch would say the
     * same thing three times, and a second icon set for 16 parameters is more
     * marks than a 15 px node can hold. Depth sheds detail: icons on the main
     * wheel, plain discs below it, no discs at all in the value state. */
    for (int i = 0; i < g->n; ++i)
        if (i != st->member)
            cov_node_body(cov, y, i * WHEEL_STEP_DEG + st->theta,
                          R_ORBIT_SUB, R_NODE_SUB);
    cov_flush(line, cov, DIM_R, DIM_G, DIM_B, 255);

    /* Preview the selected parameter's amount, but WITHOUT the orb: the orb is
     * the thing the encoder moves, and it belongs to the value state. Here it
     * would also sit under the selected branch at mid-range values. */
    if (!P_NOPT[p] && st->val[p] > 0) {
        float a = VAL_A0 + (VAL_A1 - VAL_A0) * (float)st->val[p] / 100.0f;
        cov_arc(cov, y, HX, HY, R_RING, RING_HALF_T, VAL_A0, a);
        cov_flush(line, cov, GRN_R, GRN_G, GRN_B, 255);
    }

    cov_node_body(cov, y, st->member * WHEEL_STEP_DEG + st->theta,
                  R_ORBIT_SUB, R_NODE_SUB);
    cov_flush(line, cov, SEL_R, SEL_G, SEL_B, 255);

    /* Group name stays quiet above the parameter name: depth is carried by
     * scale and position, not by a breadcrumb. */
    text_centre(line, y, 12, &font_hn_value_small, g->name, 255, 255, 255, 110);
    text_centre(line, y, 36, &font_hn_value_small, ui_wheel_param_label(p),
                255, 255, 255, 245);
}

static void compose_value(const wheel_state_t *st, int y, uint16_t *line)
{
    static uint8_t cov[OLED_WIDTH];
    int p = ui_wheel_param_of(st->group, st->member);
    int continuous = !P_NOPT[p];
    int pct = continuous ? st->val[p]
                         : (P_NOPT[p] > 1
                            ? st->val[p] * 100 / (P_NOPT[p] - 1) : 0);

    cov_arc(cov, y, HX, HY, R_RING, RING_HALF_T, VAL_A0, VAL_A1);
    cov_flush(line, cov, DIM_R, DIM_G, DIM_B, 255);

    float a = VAL_A0 + (VAL_A1 - VAL_A0) * (float)pct / 100.0f;
    if (pct > 0) cov_arc(cov, y, HX, HY, R_RING, RING_HALF_T, VAL_A0, a);
    cov_disc(cov, y, HX + R_RING * sinf(a * DEG2RAD),
             HY - R_RING * cosf(a * DEG2RAD), R_ORB);
    cov_flush(line, cov, GRN_R, GRN_G, GRN_B, 255);

    text_centre(line, y, 22, &font_hn_value_small, ui_wheel_param_label(p),
                255, 255, 255, 160);

    /* Long option names ("Midnight Drive" is 252 px at 30 ppem against 240 px
     * of room) drop to the 20 ppem face rather than being cut: a value is the
     * one string on the screen that must never be guessed at. Truncation stays
     * as the backstop below that. */
    const char *v = ui_wheel_param_value(st, p);
    const bakedfont_t *fv = &font_hn_value;
    int ytop = 56;
    if (ui_text_w(fv, v) > 240) { fv = &font_hn_value_small; ytop = 62; }
    text_centre(line, y, ytop, fv, v, 255, 255, 255, 255);
}

void ui_wheel_compose_row(const wheel_state_t *st, int y, uint16_t *line)
{
    uint16_t bg = ui_pack565(BG_R, BG_G, BG_B);
    for (int x = 0; x < OLED_WIDTH; ++x) line[x] = bg;

    switch (st->level) {
        case WHEEL_MAIN:  compose_main(st, y, line);  break;
        case WHEEL_GROUP: compose_group(st, y, line); break;
        default:          compose_value(st, y, line); break;
    }
    wheel_battery(line, y, st->batt);
}
