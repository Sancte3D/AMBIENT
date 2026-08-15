/*
 * ui_wheel — the radial navigation system.
 *
 * ONE INTERFACE OBJECT. The circle is not decoration around a menu; it IS the
 * menu, and it is also the parameter. Three states of the same geometry:
 *
 *   WHEEL_MAIN    branches carry GROUPS
 *   WHEEL_GROUP   branches carry the PARAMETERS of the chosen group
 *   WHEEL_VALUE   branches retract, the shared circle becomes the value ring
 *
 * The user never moves a cursor between items. The structure itself rotates
 * under a fixed selection point at 12 o'clock, which makes the encoder
 * physical: turn moves the structure, press descends, turn changes the value.
 *
 * GEOMETRY, measured off the reference render rather than invented. The card
 * in that image spans 720 px for the panel's 320, so panel = image * 0.4444:
 *
 *   hub centre        (455, 500) -> (158, 171)   i.e. 1 px BELOW the bottom
 *                                                edge, which is the whole
 *                                                point: the wheel continues
 *                                                outside the screen
 *   node orbit R       235 -> 104
 *   node radius         50 -> 22
 *   value ring R       145 ->  64
 *   branch width        18 ->   8
 *   battery pill    x 700..760, y 158..185 -> x 267..293, y 19..31
 *
 * The five nodes in the reference sit at -81, -40, 0, +40.5, +81.5 degrees,
 * i.e. an even 40.4 deg step. 360 / 40 = 9, so the reference is drawn as a
 * NINE-branch wheel and WHEEL_GROUPS follows that measurement. Changing the
 * group count is one table edit; changing it also changes the step, and with
 * it how many neighbours stay on screen (at 72 deg for five groups, both
 * neighbours fall off the sides).
 *
 * ICONS DO NOT ROTATE. Nodes are circles, so they are rotation-invariant, and
 * icons are emitted in screen space at the node's position — the required
 * icon_rotation = -theta falls out of the construction instead of being
 * applied as a correction. Only the selected node carries plain text.
 */
#ifndef FAM_UI_WHEEL_H
#define FAM_UI_WHEEL_H

#include <stdint.h>
#include "ui_motion.h"
#include "ui_scene.h"

#define WHEEL_GROUPS       9
#define WHEEL_PARAM_COUNT 16
#define WHEEL_STEP_DEG    (360.0f / WHEEL_GROUPS)

/* Two levels, not three. The old middle level was a sub-wheel of abstract
 * parameter names leading to a separate value screen; the scene replaces both.
 * A scene shows every property of the group at once as features of one
 * drawing, so there is nothing left to descend into. */
typedef enum {
    WHEEL_MAIN = 0,     /* the wheel of groups */
    WHEEL_SCENE         /* one group's own small visual world */
} wheel_level_t;

typedef struct {
    wheel_level_t level;                  /* where we are going              */
    wheel_level_t prev_level;             /* what is still fading out        */
    uint8_t    group;                     /* 0..WHEEL_GROUPS-1               */
    uint8_t    member;                    /* which property the encoder holds */

    /* Presentation only. The model (group/member/val) is updated the moment
     * the encoder edge arrives; these lag behind it and never gate it. */
    ui_tween_t rot;                       /* wheel angle, degrees            */
    ui_tween_t morph;                     /* 0 = prev_level, 1 = level       */
    /* One tween per scene property, so every feature of the drawing moves,
     * not just the one being turned. */
    ui_tween_t knob[UI_SCENE_MAX_KNOBS];  /* shown 0..1                      */

    uint8_t  val[WHEEL_PARAM_COUNT];
    uint8_t  batt;                        /* 0..100                          */
} wheel_state_t;

void ui_wheel_init(wheel_state_t *st);

/* Encoder detent. In MAIN/GROUP this rotates the structure by one step; in
 * VALUE it moves the value. */
void ui_wheel_turn(wheel_state_t *st, int dir, int coarse);
/* Encoder press: descend a level. `back` climbs back out instead — that is
 * what a long hold does, so there is always a way out of the level you are in
 * without hunting for a modifier. */
void ui_wheel_press(wheel_state_t *st, int back);

/* Advance every tween by dt milliseconds. Returns non-zero while anything is
 * still moving, so the caller knows whether to keep drawing frames. */
int  ui_wheel_tick(wheel_state_t *st, int dt_ms);

/* Current and target wheel angle, for tests and diagnostics. */
float ui_wheel_theta(const wheel_state_t *st);
float ui_wheel_theta_target(const wheel_state_t *st);

/* Jump straight to the settled position — used by the host renderer, which
 * has no frame loop. */
void ui_wheel_settle(wheel_state_t *st);

/* Model access, for tests and the host renderer. */
int         ui_wheel_group_size(int g);
int         ui_wheel_param_of(int g, int member);   /* -> 0..15 */
const char *ui_wheel_group_name(int g);
const char *ui_wheel_param_label(int p);
const char *ui_wheel_param_value(const wheel_state_t *st, int p);

/* One finished RGB565 scanline. `line` holds 320 entries. */
void ui_wheel_compose_row(const wheel_state_t *st, int y, uint16_t *line);

#endif
