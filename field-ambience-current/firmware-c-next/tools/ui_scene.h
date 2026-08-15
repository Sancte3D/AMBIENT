/*
 * ui_scene — one small visual world per group, instead of a menu page.
 *
 * THE RULE, taken from the OP-1 and measured rather than assumed:
 *
 *     Each mode gets its own illustration. The physical controls change
 *     properties OF THAT ILLUSTRATION, rather than a generic GUI being laid
 *     over it.
 *
 * That is the difference between "FILTER / Freq 61 / Res 42" and a drawing in
 * which four recognisable features move. You stop reading and start
 * recognising, which is the only way a 39 x 21 mm screen can be operated while
 * playing.
 *
 * WHAT WAS MEASURED (op1repacker's iter-lab.svg, the one real OP-1 display
 * asset in that archive — 320 x 160, the native OP-1 canvas):
 *
 *   304 drawable elements: 273 path, 27 ellipse, 4 line — and NO rect at all
 *   294 of 304 carry fill="none"                             96.7 % outline
 *   282 of 304 are stroke-width 1.5                          93 % one weight
 *   path vocabulary: L 297, M 273, S 113, C 45, Z 11         11 closed paths
 *   19 of 27 ellipses are rx = ry = 1                        dots, not discs
 *   157 of ~300 strokes are #353238                          >half is one dim grey
 *   then, sparsely: #698eff 24, #00ed95 22, #383572 20, #ff3a5d 15, ...
 *
 * So the OP-1's coherence is not its palette, it is its DISCIPLINE: one dim
 * structural grey carrying most of the drawing, a handful of saturated accents
 * used sparingly, one stroke weight, open paths, dots as the only filled form.
 * #353238 is (53,50,56) — almost exactly the (42,42,42) the wheel already uses
 * for inert structure, which is a reassuring convergence rather than a copy.
 *
 * The archive contains no runtime: op1repacker only moves static SVG elements
 * around (move_all / move_element, rewriting x, cx, path d and polyline
 * points). How a value reaches a shape lives in the compiled firmware and is
 * not recoverable from it. What IS recoverable is the vocabulary — and the
 * element names in the tape patch (centerline, grid, loopin, loopout,
 * track_active, track_semiactive, track_inactive) say plainly that a screen is
 * a named SCENE, not a stack of widgets.
 *
 * HOW THIS DIFFERS FROM COPYING THE LOOK. We keep the discipline and our own
 * palette: the project baseline asks for black, white, grey and one controlled
 * accent, so the semantic set here is four tones, not nine hues —
 *
 *   DIM    inert scaffolding, the drawing's skeleton
 *   INFO   structure that carries information but is not being changed
 *   LIVE   the property the hand is moving right now
 *   TEXT   naming, used as little as possible
 *
 * Every scene is stroked at 1.5 px, exactly as measured, and drawn from the
 * same signed-distance primitives as the wheel, so it costs no framebuffer.
 */
#ifndef FAM_UI_SCENE_H
#define FAM_UI_SCENE_H

#include <stdint.h>

#define UI_SCENE_MAX_KNOBS 3

/* What a scene needs to draw itself. The wheel fills this in; the scene knows
 * nothing about navigation. */
typedef struct {
    int   group;                          /* which scene                     */
    int   n;                              /* live properties, 1..3           */
    int   opts[UI_SCENE_MAX_KNOBS];       /* option count, 0 = continuous    */
    float v[UI_SCENE_MAX_KNOBS];          /* 0..1, already eased             */
    int   focus;                          /* property the encoder holds, or -1 */
} ui_scene_t;

/* Draw one scanline of the scene at `alpha`. */
void ui_scene_row(const ui_scene_t *sc, int y, uint16_t *line, int alpha);

/* Short name of a scene's property, for the one line of text a scene gets. */
const char *ui_scene_knob_name(int group, int knob);

#endif
