/* ui_motion — see ui_motion.h for the rules these implement. */
#include "ui_motion.h"

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

void ui_tween_to(ui_tween_t *tw, float to, int dur_ms, ui_ease_t curve)
{
    if (to == tw->to && tw->dur_ms > 0.0f) return;   /* already heading there */
    tw->from       = tw->cur;                        /* continuous retarget */
    tw->to         = to;
    tw->elapsed_ms = 0.0f;
    tw->dur_ms     = (float)(dur_ms > 0 ? dur_ms : 1);
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
