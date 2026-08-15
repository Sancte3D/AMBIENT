/*
 * ui_motion — the motion system. Every movement on this display goes through
 * here, so "smooth" is a property of the code rather than of each call site
 * that happens to remember.
 *
 * THE MISTAKE THIS FILE EXISTS TO PREVENT
 *
 * Durations were first written as fixed milliseconds — 90 / 200 / 280 — which
 * are reasonable numbers for a 60 fps screen and meaningless here. The bench
 * panel runs SPI at 24 MHz, so a 320x170 RGB565 frame is 108,800 bytes =
 * 36.3 ms of wire time before anything is drawn. At that rate a 90 ms ease is
 * TWO AND A HALF FRAMES. Two frames is not a motion, it is a jump with an
 * intermediate position, and it looks exactly as jerky as it sounds.
 *
 * So a duration is not a number here, it is a SPEED CLASS, resolved against
 * the frame time the device is actually achieving:
 *
 *     duration = max(target_ms, min_frames * measured_frame_ms)
 *
 * On a fast panel the target wins and the motion is crisp. On a slow one the
 * floor wins and the motion stretches until it has enough samples to read as
 * movement. Feed the real measurement in with ui_motion_set_frame_ms() and the
 * timing corrects itself instead of having to be re-guessed per panel.
 *
 * THE OTHER RULES
 *
 * 1. THE MODEL NEVER WAITS FOR THE ANIMATION. An encoder detent changes the
 *    value in the same millisecond the edge arrives; only the PRESENTATION
 *    eases toward it. An animation therefore cannot introduce latency, because
 *    nothing the user asked for is queued behind it.
 *
 * 2. EVERY TRANSITION IS INTERRUPTIBLE AND RETARGETED, NEVER QUEUED. A second
 *    detent during a snap moves the target; the tween re-eases from wherever it
 *    is. Turning fast is one continuous sweep, not a backlog of nine snaps.
 *
 * 3. TIME-BASED, NOT FRAME-BASED. Every tick takes a real dt. A slow frame
 *    makes the motion jump further, never run slower.
 *
 * 4. CURVE FOLLOWS CAUSE.
 *      direct manipulation (the value the hand is turning) -> EASE_OUT
 *          It must leave instantly and settle gently: an ease-IN on a
 *          hand-driven value reads as lag, because the first thing the eye
 *          checks is whether the screen moved when the finger did.
 *      system transitions (changing level, entering a group) -> EASE_IN_OUT
 *          Nothing is chasing the hand, so it can go rest to rest. This is what
 *          makes a level change read as one object moving rather than as two
 *          screens swapping.
 *
 * 5. NOTHING SNAPS. The only instant placement allowed is the first frame after
 *    power-up (ui_tween_reset), where there is no previous state to move from.
 */
#ifndef FAM_UI_MOTION_H
#define FAM_UI_MOTION_H

/* Speed classes. The ms figures are the targets for a panel fast enough to
 * honour them; min_frames is what the motion needs in order to read as motion
 * at all, and it wins on a slow panel. */
typedef enum {
    UI_SPEED_MICRO = 0,   /*  90 ms /  8 frames — value orb, small corrections */
    UI_SPEED_MOVE,        /* 200 ms / 12 frames — the wheel snapping one step  */
    UI_SPEED_LEVEL,       /* 280 ms / 14 frames — a level change               */
    UI_SPEED_COUNT
} ui_speed_t;

typedef enum {
    UI_EASE_OUT = 0,    /* direct manipulation: leaves instantly, settles soft */
    UI_EASE_IN_OUT,     /* system transition: rest to rest                     */
    UI_EASE_LINEAR      /* cross-fades, where a curve would read as a flicker  */
} ui_ease_t;

/* Tell the motion system how long a frame is actually taking, in ms. Call it
 * every frame with the measured value; it is smoothed internally, so a single
 * slow frame does not stretch everything. Clamped to a sane range. */
void  ui_motion_set_frame_ms(float ms);
float ui_motion_frame_ms(void);

/* Resolved duration of a speed class at the current frame time. */
int   ui_motion_duration(ui_speed_t speed);

/* Normalised easing, t and result both 0..1. Exposed for tests. */
float ui_ease(ui_ease_t curve, float t);

typedef struct {
    float     from, to, cur;
    float     elapsed_ms, dur_ms;
    ui_ease_t curve;
} ui_tween_t;

/* Place the tween at `v` with no motion — power-up only. */
void  ui_tween_reset(ui_tween_t *tw, float v);

/* Aim at `to` at the given speed class. Restarts the ease FROM THE CURRENT
 * POSITION, so retargeting mid-flight is continuous instead of a jump. A
 * target equal to the one already set is ignored, so holding a direction does
 * not keep restarting the curve. */
void  ui_tween_to(ui_tween_t *tw, float to, ui_speed_t speed, ui_ease_t curve);

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
