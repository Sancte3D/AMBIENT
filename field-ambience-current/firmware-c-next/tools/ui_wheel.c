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
#include "ui_motion.h"
#include "oled.h"
#include "baked_font.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---- palette ------------------------------------------------------------
 * FOUR ENCODER COLOURS AND NOTHING ELSE (see ui_scene.h for why). GREEN is
 * EN3 = navigation: the selected node, the ring, where you are. RED / BLUE /
 * YELLOW are EN1 / EN2 / EN4 and appear only where those knobs act. Anything
 * that no knob can move is grey.
 *
 * The battery used to be green, which broke the rule the moment green became
 * navigation — a green thing in the corner that the navigation encoder does
 * not move. It is now neutral, and only leaves neutral to warn. */
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
#define ICON_R  126            /* icon inside an inactive node */
#define ICON_G  126
#define ICON_B  126
#define SHELL_R 150            /* battery shell + terminal */
#define SHELL_G 150
#define SHELL_B 150
#define BATT_R  214            /* healthy charge — neutral, see above */
#define BATT_G  214
#define BATT_B  218
/* The selected node IS the navigation encoder's colour. On the old white node
 * the eye had to work out which of nine identical discs was the big one; in
 * EN3's own green it is the same statement the value slots make, one level up:
 * this colour belongs to that knob. */
#define SEL_R   (UI_NAV_RGB[0])
#define SEL_G   (UI_NAV_RGB[1])
#define SEL_B   (UI_NAV_RGB[2])

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

#define DEG2RAD UI_DEG2RAD

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
/* Two names are shorter than the engine's own: "FM Glass" is "FM" and Cell's
 * "Harmony" is "Chord". Both changes pay for themselves twice — the readout
 * shows three values side by side and the line is 250 px at 12 px per
 * character, so a 8-character option makes some combination overflow; and
 * "Harmony" was also the name of a GROUP, so the same word meant two different
 * things one level apart. Truncating them at draw time was the alternative, and
 * "FM Glas" reads as a rendering fault where "FM" reads as a name. */
static const char *const SYNTH_NAMES[]  = { "Ambient","Acid","FM","Mist",
                                            "Storm","Orbit","Bamboo" };
static const char *const CELL_NAMES[]   = { "Note","Chord","Land" };
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

/* ---- state --------------------------------------------------------------
 * The split that makes the instrument feel direct: `group`, `member` and
 * `val[]` are the MODEL and change the instant an encoder edge arrives. The
 * tweens are PRESENTATION and only ever lag behind. No input is queued behind
 * an animation, so a fast turn produces one continuous sweep instead of a
 * backlog of nine separate snaps. See ui_motion.h. */

/* A property's value as 0..1, which is what a scene draws with. */
static float param_norm(const wheel_state_t *st, int p)
{
    if (!P_NOPT[p]) return st->val[p] / 100.0f;
    return P_NOPT[p] > 1 ? (float)st->val[p] / (float)(P_NOPT[p] - 1) : 0.0f;
}

/* Point every knob tween at the current model, instantly. Used on entering a
 * scene: the properties belong to a group that was not on screen a moment ago,
 * so there is no previous position to travel from — easing here would show a
 * drawing morphing out of the last group's numbers, which is meaningless. */
static void knobs_reset(wheel_state_t *st)
{
    const wgroup_t *g = &GROUPS[st->group];
    for (int i = 0; i < UI_SCENE_MAX_KNOBS; ++i)
        ui_tween_reset(&st->knob[i],
                       i < g->n ? param_norm(st, g->p[i]) : 0.0f);
}

void ui_wheel_init(wheel_state_t *st)
{
    static const uint8_t SEED[WHEEL_PARAM_COUNT] = {
        0, 9, 0, 0, 62, 38, 74, 45, 21, 56, 33, 0, 1, 1, 2, 8
    };
    memset(st, 0, sizeof *st);
    st->level = st->prev_level = WHEEL_MAIN;
    st->batt  = 78;
    memcpy(st->val, SEED, sizeof SEED);

    /* Power-up is the one place an instant placement is allowed: there is no
     * previous state to move from. */
    ui_tween_reset(&st->rot, 0.0f);
    ui_tween_reset(&st->morph, 1.0f);
    knobs_reset(st);
}

float ui_wheel_theta(const wheel_state_t *st)        { return st->rot.cur; }
float ui_wheel_theta_target(const wheel_state_t *st) { return st->rot.to; }

void ui_wheel_turn_knob(wheel_state_t *st, int knob, int dir, int coarse)
{
    if (st->level != WHEEL_SCENE) return;
    if (knob < 0 || knob >= ui_wheel_group_size(st->group)) return;

    int p = ui_wheel_param_of(st->group, knob);
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
    /* Only the turned property moves, and it follows the hand. */
    ui_tween_to(&st->knob[knob], param_norm(st, p), UI_SPEED_MICRO, UI_EASE_OUT);
}

void ui_wheel_turn(wheel_state_t *st, int dir, int coarse)
{
    if (st->level == WHEEL_SCENE) {
        ui_wheel_turn_knob(st, st->member, dir, coarse);
        return;
    }

    /* The target always moves by exactly one step in the turned direction, so
     * wrapping from the last node to the first rotates one step rather than
     * spinning all the way back around. */
    ui_tween_to(&st->rot, st->rot.to - dir * WHEEL_STEP_DEG,
                UI_SPEED_MOVE, UI_EASE_OUT);
    int g = ((int)st->group + dir) % WHEEL_GROUPS;
    if (g < 0) g += WHEEL_GROUPS;
    st->group  = (uint8_t)g;
    st->member = 0;
}

/* Point the wheel at the selected group, taking the short way round. The angle
 * accumulates freely as the user turns, so the wanted angle has to be resolved
 * to the 360-periodic representative nearest to where the wheel already is —
 * otherwise stepping back out of a scene unwinds the whole rotation that got
 * you there. */
static void wheel_retarget(wheel_state_t *st, ui_speed_t speed, ui_ease_t curve)
{
    float want = -(float)st->group * WHEEL_STEP_DEG;
    float d    = want - st->rot.cur;
    d -= 360.0f * floorf(d / 360.0f + 0.5f);
    ui_tween_to(&st->rot, st->rot.cur + d, speed, curve);
}

/* Begin a level change: the outgoing level keeps drawing while the incoming
 * one fades in, so the change reads as one thing becoming another rather than
 * two screens swapping. */
static void level_to(wheel_state_t *st, wheel_level_t to)
{
    if (to == st->level) return;
    st->prev_level = st->level;
    st->level      = to;
    ui_tween_reset(&st->morph, 0.0f);
    ui_tween_to(&st->morph, 1.0f, UI_SPEED_LEVEL, UI_EASE_IN_OUT);
    wheel_retarget(st, UI_SPEED_LEVEL, UI_EASE_IN_OUT);
}

void ui_wheel_press(wheel_state_t *st, int back)
{
    if (back) {
        if (st->level != WHEEL_MAIN) level_to(st, WHEEL_MAIN);
        else                         wheel_retarget(st, UI_SPEED_MOVE, UI_EASE_IN_OUT);
        return;
    }
    if (st->level == WHEEL_MAIN) {
        st->member = 0;
        knobs_reset(st);
        level_to(st, WHEEL_SCENE);
        return;
    }
    /* Inside a scene the press does not descend — there is nowhere deeper to
     * go. On the BENCH it hands the single encoder to the next property; on the
     * product the three encoders already hold all three and this press is free
     * for something else. */
    st->member = (uint8_t)((st->member + 1) % GROUPS[st->group].n);
}

int ui_wheel_tick(wheel_state_t *st, int dt_ms)
{
    int busy = 0;
    busy |= ui_tween_tick(&st->rot, dt_ms);
    busy |= ui_tween_tick(&st->morph, dt_ms);
    for (int i = 0; i < UI_SCENE_MAX_KNOBS; ++i)
        busy |= ui_tween_tick(&st->knob[i], dt_ms);
    if (!ui_tween_busy(&st->morph)) st->prev_level = st->level;
    return busy;
}

void ui_wheel_settle(wheel_state_t *st)
{
    ui_tween_settle(&st->rot);
    ui_tween_settle(&st->morph);
    for (int i = 0; i < UI_SCENE_MAX_KNOBS; ++i) ui_tween_settle(&st->knob[i]);
    st->prev_level = st->level;
}

/* Signed-distance scanline primitives (cov_disc / cov_capsule / cov_arc /
 * cov_flush) live in ui_draw.c — ui_scene.c draws from the same set, and a
 * second copy of an antialiased arc is exactly the kind of thing that
 * silently diverges. */

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
                ui_cov_disc(cov, y, cx + q->x0 * s, cy + q->y0 * s, q->r * s);
                break;
            case IP_CAP:
                ui_cov_capsule(cov, y, cx + q->x0 * s, cy + q->y0 * s,
                            cx + q->x1 * s, cy + q->y1 * s, q->r * s);
                break;
            default:
                ui_cov_arc(cov, y, cx + q->x0 * s, cy + q->y0 * s, q->x1 * s,
                        q->r * s, (float)q->a0, (float)q->a1);
                break;
        }
    }
}

/* ---- widgets ------------------------------------------------------------ */

/* Rounded-rectangle SDF, for the one widget that is not part of the wheel. */
static float rrect_sd(float px, float py, float hw, float hh, float r)
{
    float qx = fabsf(px) - (hw - r), qy = fabsf(py) - (hh - r);
    float ax = qx > 0.0f ? qx : 0.0f, ay = qy > 0.0f ? qy : 0.0f;
    float outside = sqrtf(ax * ax + ay * ay);
    float inside  = (qx > qy ? qx : qy);
    if (inside > 0.0f) inside = 0.0f;
    return outside + inside - r;
}

/* Battery: a clean rounded rectangle with a terminal nub and a gradient fill.
 *
 * Two earlier versions failed for the same reason — a dim track pill with a
 * proportional fill pill on top puts two rounded caps in the middle of a
 * 24 x 12 px shape, and at any partial charge that reads as a blob rather than
 * as a battery. A rectangle has a flat fill edge, so the charge boundary is a
 * straight line and the silhouette stays a battery at every level. The
 * gradient runs vertically inside the fill (lighter at the top), which is what
 * keeps a 20-px-wide solid block from looking like a printed swatch. */
static void wheel_battery(uint16_t *line, int y, int pct)
{
    const float bx0 = 264.0f, bx1 = 288.0f;      /* body */
    const float by0 = 19.0f,  by1 = 31.0f;
    const float cx = (bx0 + bx1) * 0.5f, cy = (by0 + by1) * 0.5f;
    const float hw = (bx1 - bx0) * 0.5f, hh = (by1 - by0) * 0.5f;
    const float rad = 3.0f, wall = 1.5f;

    /* Neutral while healthy: colour in this system means "an encoder moves
     * this", and no encoder moves the battery. It earns colour only when it
     * needs the eye — which is also why the warning states now read as
     * warnings instead of as one more coloured thing among several. */
    int r = BATT_R, g = BATT_G, b = BATT_B;
    if (pct <= 10)      { r = 244; g =  90; b =  76; }   /* critical */
    else if (pct <= 25) { r = 246; g = 190; b =  84; }   /* low      */

    float fy = (float)y;
    if (fy < by0 - 2.0f || fy > by1 + 2.0f) return;

    /* nub */
    if (fy >= cy - 3.0f && fy <= cy + 3.0f)
        ui_row_pill(line, y, (int)(cy - 3.0f), 6, (int)bx1, 4,
                    SHELL_R, SHELL_G, SHELL_B, 255);

    /* Inner cavity, and the charge boundary inside it. */
    const float ihw = hw - wall - 1.0f, ihh = hh - wall - 1.0f;
    float fill_x1 = cx - ihw + (2.0f * ihw) * (float)pct / 100.0f;

    for (int x = (int)(bx0 - 2.0f); x <= (int)(bx1 + 5.0f); ++x) {
        if (x < 0 || x >= OLED_WIDTH) continue;
        float px = (float)x - cx, py = fy - cy;

        float outer = rrect_sd(px, py, hw, hh, rad);
        float inner = rrect_sd(px, py, hw - wall, hh - wall, rad - wall * 0.6f);

        /* shell: the ring between outer and inner */
        int shell = ui_cov255(outer);
        int hole  = ui_cov255(inner);
        if (shell > hole) ui_blend_px(&line[x], SHELL_R, SHELL_G, SHELL_B,
                                      shell - hole);

        /* charge: the cavity, clipped to the level, with a vertical gradient */
        if (pct > 0 && px <= fill_x1 - cx) {
            int cav = ui_cov255(rrect_sd(px, py, ihw, ihh, rad - wall));
            /* soften the vertical charge edge by one pixel so it does not
             * crawl a whole pixel at a time while the value moves */
            float edge = (fill_x1 - cx) - px;
            if (edge < 1.0f) cav = (int)(cav * (edge < 0.0f ? 0.0f : edge));
            if (cav) {
                float t = (py + ihh) / (2.0f * ihh);      /* 0 top, 1 bottom */
                float k = 1.18f - 0.36f * t;              /* lighter at top  */
                int rr = (int)(r * k), gg = (int)(g * k), bb = (int)(b * k);
                if (rr > 255) rr = 255;
                if (gg > 255) gg = 255;
                if (bb > 255) bb = 255;
                ui_blend_px(&line[x], rr, gg, bb, cav);
            }
        }
    }
}

static void text_centre(uint16_t *line, int y, int ytop,
                        const bakedfont_t *f, const char *s,
                        int cr, int cg, int cb, int a)
{
    if (a <= 0) return;
    char buf[32];
    const char *t = ui_fit_text(f, s, 240, buf, sizeof buf);
    ui_row_text(line, y, ytop, (OLED_WIDTH - ui_text_w(f, t)) / 2, f, t,
                cr, cg, cb, a);
}

/* The circle itself. Its span depends on what job it is doing:
 *
 *   hub (MAIN/GROUP)  -92..+92, i.e. the whole visible half with both caps off
 *                     the bottom edge. It has to reach past the outermost
 *                     branch, or the +-80 deg arms emerge from nothing.
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
    ui_cov_capsule(cov, y, HX + R_BRANCH0 * sinf(a), HY - R_BRANCH0 * cosf(a),
                nx, ny, BRANCH_HALF_W);
}

static void cov_node_body(uint8_t *cov, int y, float deg, float orbit, float nr)
{
    float nx, ny;
    node_xy(deg, orbit, &nx, &ny);
    if (nr < 0.6f || !node_visible(nx, ny, nr)) return;
    ui_cov_disc(cov, y, nx, ny, nr);
}

static void cov_node_icon(uint8_t *cov, int y, float deg, float orbit,
                          float nr, const icon_t *ic)
{
    float nx, ny;
    node_xy(deg, orbit, &nx, &ny);
    if (nr < 4.0f || !node_visible(nx, ny, nr)) return;
    cov_icon(cov, y, ic, nx, ny, nr * 0.42f);
}

/* ---- levels -------------------------------------------------------------
 * Each level draws at an alpha, so a level change cross-fades the outgoing
 * wheel against the incoming one over UI_DUR_LEVEL. Node radii scale with that
 * alpha too: the outgoing set shrinks away and the incoming set grows in, which
 * is what makes the change read as ONE object transforming rather than two
 * screens swapping. The ring is common to every level and never fades, so
 * there is always a fixed thing for the eye to hold on to. */

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

static void compose_main(const wheel_state_t *st, int y, uint16_t *line,
                         int alpha, int talpha)
{
    static uint8_t cov[OLED_WIDTH];
    float theta = st->rot.cur;
    float k     = 0.55f + 0.45f * (float)alpha / 255.0f;   /* grow / shrink */
    float nr    = R_NODE * k;

    for (int i = 0; i < WHEEL_GROUPS; ++i)
        cov_branch(cov, y, i * WHEEL_STEP_DEG + theta, R_ORBIT);
    for (int i = 0; i < WHEEL_GROUPS; ++i)
        if (i != st->group)
            cov_node_body(cov, y, i * WHEEL_STEP_DEG + theta, R_ORBIT, nr);
    ui_cov_flush(line, cov, DIM_R, DIM_G, DIM_B, alpha);

    for (int i = 0; i < WHEEL_GROUPS; ++i)
        if (i != st->group)
            cov_node_icon(cov, y, i * WHEEL_STEP_DEG + theta, R_ORBIT, nr,
                          &ICONS[i]);
    ui_cov_flush(line, cov, ICON_R, ICON_G, ICON_B, alpha);

    float sel_deg = st->group * WHEEL_STEP_DEG + theta;
    cov_node_body(cov, y, sel_deg, R_ORBIT, nr);
    ui_cov_flush(line, cov, SEL_R, SEL_G, SEL_B, alpha);

    cov_node_icon(cov, y, sel_deg, R_ORBIT, nr, &ICONS[st->group]);
    ui_cov_flush(line, cov, 0, 0, 0, alpha);

    text_centre(line, y, 18, &font_hn_value_small,
                ui_wheel_group_name(st->group), 255, 255, 255, 235 * talpha / 255);
}

/* The scene replaces the old sub-wheel and value screen both. Every property of
 * the group is visible at once as a feature of one drawing, in the colour of
 * the encoder that moves it — so there is nothing to select and nothing to
 * read. */
static void compose_scene(const wheel_state_t *st, int y, uint16_t *line,
                          int alpha, int talpha)
{
    const wgroup_t *g = &GROUPS[st->group];

    ui_scene_t sc;
    sc.group = st->group;
    sc.n     = g->n;
    for (int i = 0; i < UI_SCENE_MAX_KNOBS; ++i) {
        sc.opts[i] = i < g->n ? P_NOPT[g->p[i]] : 0;
        sc.v[i]    = i < g->n ? st->knob[i].cur : 0.0f;
        if (sc.v[i] < 0.0f) sc.v[i] = 0.0f;
        if (sc.v[i] > 1.0f) sc.v[i] = 1.0f;
    }
    ui_scene_row(&sc, y, line, alpha);

    /* ONE line of type, and the property NAMES are gone from it. Three names
     * plus three values do not fit in 216 px at this size, and more to the
     * point they are no longer needed: each value is painted in the colour of
     * the encoder that moves it, and the encoders are read left to right in
     * exactly that order — RED, BLUE, YELLOW. The colour is the label, which is
     * the whole reason the OP-1's screens can be operated without reading.
     *
     * Right-aligned as a group against the battery (which owns x >= 262), so a
     * long word grows leftwards into empty space instead of into the corner.
     *
     * NOTHING IS TRUNCATED HERE, and nothing needs to be: the option names were
     * chosen so that the widest reachable combination of every group fits in
     * VLEFT..RIGHT, and test_ui_wheel.c walks all of them and fails if one does
     * not. Fixing the width at the source is the only way to keep the promise —
     * a fit-to-budget call would silently start cutting words the day someone
     * adds an eighth synth core with a long name. */
    const bakedfont_t *f = &font_hn_value_small;
    const int VLEFT = 6, RIGHT = 256, GAPX = 12, LEFT = 40;
    /* COPIED, not aliased: ui_wheel_param_value formats continuous values into
     * one shared static buffer, so holding three of its return pointers made
     * all three numbers show the last one — Room printed "38 38" for 62 and 38.
     * A scene with three identical-looking numbers is worse than no numbers,
     * because it looks like a working readout. */
    char vs[UI_SCENE_MAX_KNOBS][16];
    int vw[UI_SCENE_MAX_KNOBS], total = 0;

    for (int i = 0; i < g->n; ++i) {
        const char *s = ui_wheel_param_value(st, g->p[i]);
        size_t len = strlen(s);
        if (len >= sizeof vs[i]) len = sizeof vs[i] - 1;
        memcpy(vs[i], s, len);
        vs[i][len] = '\0';
        vw[i] = ui_text_w(f, vs[i]);
        total += vw[i] + (i ? GAPX : 0);
    }

    int x = RIGHT - total;
    if (x < VLEFT) x = VLEFT;      /* the test proves this unreachable */
    for (int i = 0; i < g->n; ++i) {
        ui_row_text(line, y, 10, x, f, vs[i], UI_KNOB_RGB[i][0],
                    UI_KNOB_RGB[i][1], UI_KNOB_RGB[i][2], talpha);
        /* BENCH ONLY: with one physical encoder something has to say which
         * value the turn will hit. On the product all three are live and this
         * underline is absent — see wheel_state_t.one_encoder. */
        if (st->one_encoder && i == st->member) {
            static uint8_t cov[OLED_WIDTH];
            ui_cov_capsule(cov, y, (float)x, 25.0f, (float)(x + vw[i]), 25.0f,
                           0.75f);
            ui_cov_flush(line, cov, UI_KNOB_RGB[i][0], UI_KNOB_RGB[i][1],
                         UI_KNOB_RGB[i][2], talpha);
        }
        x += vw[i] + GAPX;
    }

    /* The scene name is DROPPED rather than truncated when the line is full.
     * "Soun" and "Textur" read as a rendering fault; nothing at all reads as a
     * deliberately quiet line — and the name is the least load-bearing thing
     * on screen, because the drawing already says where you are. */
    int room = (RIGHT - total) - LEFT - GAPX;
    if (ui_text_w(f, g->name) <= room)
        ui_row_text(line, y, 10, LEFT, f, g->name, 255, 255, 255,
                    110 * talpha / 255);
}

/* The value ring is divided into THREE FIXED SLOTS, one per scene encoder, and
 * a knob's slot never moves between scenes. Left slot = EN1 = red, middle =
 * EN2 = blue, right = EN4 = yellow, which is the same left-to-right order as
 * the three readouts above it and as the encoders under the user's hand.
 *
 * Fixed rather than shared out among however many properties the scene has: a
 * slot that changes size and position per group teaches nothing, whereas a
 * slot that is always in the same place becomes a position the hand knows. A
 * group with one property simply leaves the other two slots dim, which is also
 * the honest statement — those knobs do nothing here. */
#define VAL_SLOT_GAP 7.0f

static void ring_slot(int i, float *s0, float *s1)
{
    float step = (VAL_A1 - VAL_A0) / (float)UI_SCENE_MAX_KNOBS;
    *s0 = VAL_A0 + i * step + VAL_SLOT_GAP * 0.5f;
    *s1 = VAL_A0 + (i + 1) * step - VAL_SLOT_GAP * 0.5f;
}

/* The ring is the one thing every level shares, and it never fades. In the
 * wheel it is the hub the branches grow out of; in a scene it becomes the
 * horizon the drawing sits on AND the precise readout of all three properties
 * at once. The drawing carries the character, the arcs carry the amounts —
 * they answer different questions, so both earn their place. */
static void compose_ring(const wheel_state_t *st, int y, uint16_t *line,
                         float scene_weight)
{
    static uint8_t cov[OLED_WIDTH];

    /* The span contracts from the full visible half toward the value sweep as
     * a scene takes over, so the ends arrive as ends instead of appearing. */
    /* The ring THINS as a scene takes over. At the hub's 9 px it is six times
     * the scene's 1.5 px monoline and simply shouts over the drawing — the
     * loudest thing on screen would be the readout rather than the instrument.
     * At 3 px it still reads as the horizon and as the value, without
     * competing. */
    float a0 = lerp(HUB_A0, VAL_A0, scene_weight);
    float a1 = lerp(HUB_A1, VAL_A1, scene_weight);
    float ht = lerp(RING_HALF_T, 1.5f, scene_weight);
    ui_cov_arc(cov, y, HX, HY, R_RING, ht, a0, a1);
    ui_cov_flush(line, cov, DIM_R, DIM_G, DIM_B, 255);

    if (scene_weight <= 0.01f) return;

    int n     = GROUPS[st->group].n;
    int alpha = (int)(scene_weight * 255.0f);

    for (int i = 0; i < n; ++i) {
        float s0, s1;
        ring_slot(i, &s0, &s1);

        float v = st->knob[i].cur;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        float a = s0 + (s1 - s0) * v;

        /* Only draw the travelled part; at v = 0 the arc would otherwise be a
         * round cap sitting at the slot's start, which reads as a small value
         * rather than as none. The head dot alone is the correct mark for 0. */
        if (a - s0 > 0.5f) ui_cov_arc(cov, y, HX, HY, R_RING, ht, s0, a);
        ui_cov_disc(cov, y, HX + R_RING * sinf(a * DEG2RAD),
                    HY - R_RING * cosf(a * DEG2RAD), 3.2f * scene_weight);
        ui_cov_flush(line, cov, UI_KNOB_RGB[i][0], UI_KNOB_RGB[i][1],
                     UI_KNOB_RGB[i][2], alpha);
    }
}

static void compose_level(const wheel_state_t *st, wheel_level_t lv,
                          int y, uint16_t *line, int alpha, int talpha)
{
    if (alpha <= 0) return;
    switch (lv) {
        case WHEEL_MAIN:  compose_main(st, y, line, alpha, talpha);  break;
        default:          compose_scene(st, y, line, alpha, talpha); break;
    }
}

/* Text does NOT cross-fade with the geometry.
 *
 * Two labels at 50 % opacity in nearly the same place are not a transition,
 * they are two unreadable words on top of each other — and the level change
 * moves a heading from y=18 to y=12 while adding a second line at y=36, so
 * the overlap is guaranteed rather than occasional. Shapes may dissolve
 * through one another because they read as one object deforming; words cannot.
 *
 * So the outgoing text is gone by 45 % of the transition and the incoming text
 * does not start until 55 %. There is a moment with no label at all, which is
 * correct: during a level change the label is precisely the thing that is no
 * longer true. */
static int text_out_alpha(float m)
{
    float a = (0.45f - m) / 0.45f;
    if (a <= 0.0f) return 0;
    if (a >= 1.0f) return 255;
    return (int)(a * 255.0f);
}

static int text_in_alpha(float m)
{
    float a = (m - 0.55f) / 0.45f;
    if (a <= 0.0f) return 0;
    if (a >= 1.0f) return 255;
    return (int)(a * 255.0f);
}

void ui_wheel_compose_row(const wheel_state_t *st, int y, uint16_t *line)
{
    uint16_t bg = ui_pack565(BG_R, BG_G, BG_B);
    for (int x = 0; x < OLED_WIDTH; ++x) line[x] = bg;

    float m = st->morph.cur;
    if (m < 0.0f) m = 0.0f;
    if (m > 1.0f) m = 1.0f;

    /* How much of a scene is on screen right now — drives the orb and the
     * ring's span, so both grow in with the level instead of popping. */
    float sw = (st->level == WHEEL_SCENE ? m : 0.0f)
             + (st->prev_level == WHEEL_SCENE ? 1.0f - m : 0.0f);

    compose_ring(st, y, line, sw);

    if (st->prev_level != st->level)
        compose_level(st, st->prev_level, y, line,
                      (int)((1.0f - m) * 255.0f), text_out_alpha(m));
    compose_level(st, st->level, y, line,
                  (int)(m * 255.0f),
                  st->prev_level == st->level ? 255 : text_in_alpha(m));

    wheel_battery(line, y, st->batt);
}
