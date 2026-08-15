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
 * That measurement gives the DRAWING STYLE: one dim structural grey carrying
 * most of the picture, saturated accents used sparingly, one stroke weight,
 * open paths, dots as the only filled form. #353238 is (53,50,56) — almost
 * exactly the (42,42,42) the wheel already uses for inert structure, which is a
 * reassuring convergence rather than a copy.
 *
 * It does NOT give the mechanism. The mechanism is the four encoder colours,
 * below, and it is the load-bearing half.
 *
 * The archive contains no runtime: op1repacker only moves static SVG elements
 * around (move_all / move_element, rewriting x, cx, path d and polyline
 * points). How a value reaches a shape lives in the compiled firmware and is
 * not recoverable from it. What IS recoverable is the vocabulary — and the
 * element names in the tape patch (centerline, grid, loopin, loopout,
 * track_active, track_semiactive, track_inactive) say plainly that a screen is
 * a named SCENE, not a stack of widgets.
 *
 * SO THERE ARE EXACTLY TWO KINDS OF INK in a scene:
 *
 *   DIM (42,42,42)   inert scaffolding — the drawing's skeleton, the parts no
 *                    encoder can move: horizons, staves, reference marks.
 *   one of the four  everything an encoder moves, painted in THAT encoder's
 *   encoder colours  colour.
 *
 * There is no decorative colour and no third tone. If it is coloured, a knob
 * moves it; if it is grey, no knob does. That rule is checkable, which is why
 * it is written as a rule rather than as a mood.
 *
 * Every scene is stroked at 1.5 px, exactly as measured, and drawn from the
 * same signed-distance primitives as the wheel, so it costs no framebuffer.
 */
#ifndef FAM_UI_SCENE_H
#define FAM_UI_SCENE_H

#include <stdint.h>

#define UI_SCENE_MAX_KNOBS 3

/* THE MECHANISM, and it is the whole thing.
 *
 * On the OP-1 every screen paints its controllable elements in the FOUR
 * ENCODER COLOURS. iter.png is the purest statement of it: four little needles,
 * blue / green / white / red, and nothing else on the screen. filter.png does
 * the same with FREQ+RES in one colour and DRIVE in another. You never read a
 * label because you learn "the red one moves that".
 *
 * iter-lab.svg proves the second half: it is the SAME four needles, dressed as
 * a chemistry set. The elaborate drawing is a costume over the four coloured
 * value objects — so the drawing is optional and the colour mapping is not.
 * A bespoke illustration with one accent colour, which is what the first pass
 * built, keeps the expensive half and drops the load-bearing one.
 *
 * AMBIENT's four encoders are RED, BLUE, GREEN, YELLOW.
 *
 *   GREEN is reserved for navigation — the wheel, the selection, where you
 *   are. It is never a scene property, because "where am I" must never be
 *   confusable with "what am I changing".
 *   RED, BLUE, YELLOW are the three scene properties, in that order.
 *
 * Three scene knobs is not a compromise, it is what the hardware has: EN3
 * (Display) holds navigation permanently, leaving EN1/EN2/EN4. Every group in
 * the wheel already carries one to three parameters, so the fit is exact. At
 * the top level those same three encoders keep their global roles — Drive,
 * Brightness, Volume — so the globals are always one press away. */
extern const uint8_t UI_KNOB_RGB[UI_SCENE_MAX_KNOBS][3];
extern const uint8_t UI_NAV_RGB[3];

/* What a scene needs to draw itself. The wheel fills this in; the scene knows
 * nothing about navigation. */
/* There is deliberately NO "focus" field. All of a scene's properties are live
 * simultaneously, because all three encoders are — that is the entire point of
 * the colour mapping. A focused property would reintroduce the modal selection
 * the scene exists to remove. */
typedef struct {
    int   group;                          /* which scene                     */
    int   n;                              /* live properties, 1..3           */
    int   opts[UI_SCENE_MAX_KNOBS];       /* option count, 0 = continuous    */
    float v[UI_SCENE_MAX_KNOBS];          /* 0..1, already eased             */
} ui_scene_t;

/* Draw one scanline of the scene at `alpha`. */
void ui_scene_row(const ui_scene_t *sc, int y, uint16_t *line, int alpha);

/* Short name of a scene's property, for the one line of text a scene gets. */
const char *ui_scene_knob_name(int group, int knob);

#endif
