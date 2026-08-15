/*
 * ui_motion — the motion system. Every movement on this display goes through
 * here, so "smooth" is a property of the code rather than of each site that
 * happens to remember.
 *
 * THE RULES
 *
 * 1. THE MODEL NEVER WAITS FOR THE ANIMATION. An encoder detent changes the
 *    value immediately, in the same millisecond the edge arrives. Only the
 *    PRESENTATION eases toward it. This is what makes the instrument feel
 *    direct: the animation can never introduce latency, because nothing the
 *    user asked for is queued behind it. If the display is two frames behind,
 *    the sound is still already where the hand put it.
 *
 * 2. EVERY TRANSITION IS INTERRUPTIBLE AND RETARGETED, NEVER QUEUED. A second
 *    detent during a snap does not wait, does not replay, does not add to a
 *    backlog. It moves the target; the tween re-eases from wherever it
 *    currently is. Turning the encoder fast produces one continuous sweep, not
 *    a stutter of nine separate 200 ms animations.
 *
 * 3. TIME-BASED, NOT FRAME-BASED. Every tick takes a real dt in milliseconds.
 *    A slow frame makes the motion jump further, never slower — so a stall in
 *    the SPI blit costs a frame, not the timing of the whole gesture.
 *
 * 4. CURVE FOLLOWS CAUSE.
 *      direct manipulation (the value the hand is turning) -> EASE_OUT
 *          It must leave instantly and settle gently: any ease-IN on a
 *          hand-driven value reads as lag, because the first thing the eye
 *          checks is whether the screen moved when the finger did.
 *      system transitions (changing level, entering a group) -> EASE_IN_OUT
 *          Nothing is chasing the hand, so it can accelerate out of rest and
 *          decelerate into rest. This is what makes a level change read as one
 *          object moving rather than as two screens swapping.
 *
 * 5. DURATIONS ARE A SCALE, NOT A GUESS. Three steps, and nothing in between:
 *      UI_DUR_MICRO  90 ms   value orb, small corrections. Below ~80 ms an
 *                            ease stops being perceptible and only costs
 *                            frames; above ~120 ms a hand-driven value starts
 *                            to feel rubbery.
 *      UI_DUR_MOVE  200 ms   the wheel snapping one step.
 *      UI_DUR_LEVEL 280 ms   a level change, which moves the most pixels and
 *                            is the only place the eye needs to re-orient.
 *
 * 6. NOTHING SNAPS. The only instant change permitted is the first frame after
 *    power-up (ui_tween_reset), where there is no previous state to move from.
 */
#ifndef FAM_UI_MOTION_H
#define FAM_UI_MOTION_H

#define UI_DUR_MICRO   90
#define UI_DUR_MOVE   200
#define UI_DUR_LEVEL  280

typedef enum {
    UI_EASE_OUT = 0,    /* direct manipulation: leaves instantly, settles soft */
    UI_EASE_IN_OUT,     /* system transition: rest to rest                     */
    UI_EASE_LINEAR      /* cross-fades, where a curve would read as a flicker  */
} ui_ease_t;

/* Normalised easing, t and result both 0..1. Exposed for tests. */
float ui_ease(ui_ease_t curve, float t);

typedef struct {
    float     from, to, cur;
    float     elapsed_ms, dur_ms;
    ui_ease_t curve;
} ui_tween_t;

/* Place the tween at `v` with no motion — power-up only. */
void  ui_tween_reset(ui_tween_t *tw, float v);

/* Aim at `to`. Restarts the ease FROM THE CURRENT POSITION, so retargeting
 * mid-flight is continuous instead of a jump. A target equal to the one
 * already set is ignored, so holding a direction does not keep restarting the
 * curve. */
void  ui_tween_to(ui_tween_t *tw, float to, int dur_ms, ui_ease_t curve);

/* Advance by dt milliseconds. Returns non-zero while still moving. Lands
 * exactly on the target — an asymptotic approach that never arrives leaves a
 * sub-pixel drift that shows up as a shimmering edge. */
int   ui_tween_tick(ui_tween_t *tw, int dt_ms);

/* Jump to the end. For the host renderer, which has no frame loop. */
void  ui_tween_settle(ui_tween_t *tw);

static inline float ui_tween_value(const ui_tween_t *tw) { return tw->cur; }
static inline int   ui_tween_busy(const ui_tween_t *tw)
{
    return tw->elapsed_ms < tw->dur_ms;
}

#endif
