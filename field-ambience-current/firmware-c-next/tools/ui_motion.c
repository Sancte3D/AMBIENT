/* ui_motion — see ui_motion.h for the rules these implement. */
#include "ui_motion.h"

/* target_ms, min_frames — see the speed-class note in the header. */
static const struct { int ms; int frames; } SPEED[UI_SPEED_COUNT] = {
    {  90,  8 },   /* MICRO */
    { 200, 12 },   /* MOVE  */
    { 280, 14 },   /* LEVEL */
};

/* Assume a 60 fps panel until told otherwise, so the host renderer and the
 * tests get the crisp target timings without having to configure anything. */
static float s_frame_ms = 16.7f;

void ui_motion_set_frame_ms(float ms)
{
    /* Below ~4 ms is a measurement artefact (an idle loop that drew nothing);
     * above 100 ms the panel is not animating anything anyway and letting it
     * stretch the durations without bound would make every motion glacial. */
    if (ms < 4.0f)   ms = 4.0f;
    if (ms > 100.0f) ms = 100.0f;
    /* Smoothed: one slow frame must not stretch the next motion. */
    s_frame_ms += (ms - s_frame_ms) * 0.15f;
}

float ui_motion_frame_ms(void) { return s_frame_ms; }

int ui_motion_duration(ui_speed_t speed)
{
    if (speed < 0 || speed >= UI_SPEED_COUNT) speed = UI_SPEED_MOVE;
    /* Rounded UP: truncating here turns "at least 8 frames" into 7.96 frames,
     * which is 7 after the division and exactly the off-by-one that lets a
     * motion sit one sample below the threshold it was supposed to clear. */
    float floor_ms = SPEED[speed].frames * s_frame_ms;
    float ms = (float)SPEED[speed].ms;
    return (int)((ms > floor_ms ? ms : floor_ms) + 0.999f);
}

float ui_ease(ui_ease_t curve, float t)
{
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    switch (curve) {
        case UI_EASE_OUT: {
            /* Cubic out. Starts at 3x speed, so the screen has visibly moved
             * within the first frame after the detent — that first frame is
             * the whole perception of latency. */
            float u = 1.0f - t;
            return 1.0f - u * u * u;
        }
        case UI_EASE_IN_OUT: {
            /* Symmetric cubic, rest to rest. */
            if (t < 0.5f) return 4.0f * t * t * t;
            float u = -2.0f * t + 2.0f;
            return 1.0f - u * u * u / 2.0f;
        }
        default:
            return t;
    }
}

void ui_tween_reset(ui_tween_t *tw, float v)
{
    tw->from = tw->to = tw->cur = v;
    tw->elapsed_ms = tw->dur_ms = 0.0f;
    tw->curve = UI_EASE_OUT;
}

void ui_tween_to(ui_tween_t *tw, float to, ui_speed_t speed, ui_ease_t curve)
{
    if (to == tw->to && tw->dur_ms > 0.0f) return;   /* already heading there */
    tw->from       = tw->cur;                        /* continuous retarget */
    tw->to         = to;
    tw->elapsed_ms = 0.0f;
    tw->dur_ms     = (float)ui_motion_duration(speed);
    tw->curve      = curve;
}

int ui_tween_tick(ui_tween_t *tw, int dt_ms)
{
    if (tw->elapsed_ms >= tw->dur_ms) {
        tw->cur = tw->to;
        return 0;
    }
    tw->elapsed_ms += (float)(dt_ms > 0 ? dt_ms : 1);
    if (tw->elapsed_ms >= tw->dur_ms) {
        tw->elapsed_ms = tw->dur_ms;
        tw->cur = tw->to;                 /* land exactly, never asymptotically */
        return 0;
    }
    float t = ui_ease(tw->curve, tw->elapsed_ms / tw->dur_ms);
    tw->cur = tw->from + (tw->to - tw->from) * t;
    return 1;
}

void ui_tween_settle(ui_tween_t *tw)
{
    tw->cur = tw->to;
    tw->elapsed_ms = tw->dur_ms;
}
