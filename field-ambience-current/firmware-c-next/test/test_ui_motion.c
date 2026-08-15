/*
 * test_ui_motion — the motion rules from ui_motion.h, as assertions.
 *
 * Smoothness is not something a still preview or a code review can check, and
 * the ways it breaks are all silent: a curve that overshoots, a retarget that
 * jumps, a frame-based tick that runs at a different speed on a slower panel,
 * or an input swallowed while an animation is running. Each of those is a line
 * below.
 */
#include "../tools/ui_motion.h"
#include "../tools/ui_wheel.h"

#include <math.h>
#include <stdio.h>

static int failures;

#define CHECK(cond, ...) do {                            \
    if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); \
                   printf("\n"); ++failures; }            \
} while (0)

static const char *CURVE[] = { "EASE_OUT", "EASE_IN_OUT", "LINEAR" };

/* Every curve must be anchored at both ends, monotonic, and never leave
 * 0..1 — an overshoot on a value ring reads as the number briefly being wrong. */
static void test_curves(void)
{
    for (int c = 0; c < 3; ++c) {
        CHECK(ui_ease((ui_ease_t)c, 0.0f) == 0.0f, "%s does not start at 0",
              CURVE[c]);
        CHECK(ui_ease((ui_ease_t)c, 1.0f) == 1.0f, "%s does not end at 1",
              CURVE[c]);
        float prev = -1.0f;
        for (int i = 0; i <= 100; ++i) {
            float v = ui_ease((ui_ease_t)c, i / 100.0f);
            CHECK(v >= prev - 1e-6f, "%s is not monotonic at t=%.2f",
                  CURVE[c], i / 100.0f);
            CHECK(v >= -1e-6f && v <= 1.0f + 1e-6f,
                  "%s overshoots at t=%.2f: %.4f", CURVE[c], i / 100.0f, v);
            prev = v;
        }
    }
}

/* Rule 4, and the reason it matters: a hand-driven value must have visibly
 * moved by the first frame, while a system transition may start from rest. */
static void test_curve_character(void)
{
    float out = ui_ease(UI_EASE_OUT, 0.1f);
    float in  = ui_ease(UI_EASE_IN_OUT, 0.1f);
    CHECK(out > 0.2f, "EASE_OUT only reaches %.3f at t=0.1 — the first frame "
                      "after a detent would look like lag", out);
    CHECK(in < 0.1f, "EASE_IN_OUT reaches %.3f at t=0.1 — a system transition "
                     "should leave from rest", in);
    CHECK(out > in, "EASE_OUT must lead EASE_IN_OUT early on");
}

/* A tween must land exactly, and must report itself done. An asymptotic
 * approach leaves a sub-pixel drift that shows up as a shimmering edge. */
static void test_lands_exactly(void)
{
    ui_tween_t tw;
    ui_tween_reset(&tw, 0.0f);
    ui_tween_to(&tw, 40.0f, UI_SPEED_MOVE, UI_EASE_OUT);
    int frames = 0;
    while (ui_tween_tick(&tw, 8) && frames < 1000) ++frames;
    CHECK(frames < 1000, "tween never finished");
    CHECK(ui_tween_value(&tw) == 40.0f, "landed on %.6f, not 40",
          ui_tween_value(&tw));
    CHECK(!ui_tween_busy(&tw), "tween still reports busy after landing");
    CHECK(frames * 8 <= ui_motion_duration(UI_SPEED_MOVE) + 16,
          "took %d ms for a %d ms move",
          frames * 8, ui_motion_duration(UI_SPEED_MOVE));
}

/* Rule 2. Retargeting mid-flight must stay continuous: the value may change
 * direction, but it must never teleport. */
static void test_retarget_is_continuous(void)
{
    ui_tween_t tw;
    ui_tween_reset(&tw, 0.0f);
    ui_tween_to(&tw, 40.0f, UI_SPEED_MOVE, UI_EASE_OUT);

    float prev = ui_tween_value(&tw);
    float worst = 0.0f;
    for (int f = 0; f < 60; ++f) {
        if (f == 5)  ui_tween_to(&tw, 80.0f, UI_SPEED_MOVE, UI_EASE_OUT);
        if (f == 9)  ui_tween_to(&tw, -40.0f, UI_SPEED_MOVE, UI_EASE_OUT);
        if (f == 12) ui_tween_to(&tw, 120.0f, UI_SPEED_MOVE, UI_EASE_OUT);
        ui_tween_tick(&tw, 8);
        float step = fabsf(ui_tween_value(&tw) - prev);
        if (step > worst) worst = step;
        prev = ui_tween_value(&tw);
    }
    /* 160 units over 200 ms is 6.4 units per 8 ms frame at constant speed;
     * a cubic-out opens at 3x that. Anything past 30 is a discontinuity. */
    CHECK(worst < 30.0f, "retargeting jumped %.1f units in one frame", worst);
    CHECK(ui_tween_value(&tw) == 120.0f, "did not reach the final target");
}

/* Rule 3. The same elapsed time must produce the same position no matter how
 * it is chopped up, or the motion runs at a different speed on a slower
 * frame rate. */
static void test_time_based(void)
{
    ui_tween_t a, b;
    ui_tween_reset(&a, 0.0f);
    ui_tween_reset(&b, 0.0f);
    ui_tween_to(&a, 100.0f, UI_SPEED_LEVEL, UI_EASE_IN_OUT);
    ui_tween_to(&b, 100.0f, UI_SPEED_LEVEL, UI_EASE_IN_OUT);

    for (int i = 0; i < 12; ++i) ui_tween_tick(&a, 10);   /* 120 ms, fast */
    for (int i = 0; i < 4;  ++i) ui_tween_tick(&b, 30);   /* 120 ms, slow */

    CHECK(fabsf(ui_tween_value(&a) - ui_tween_value(&b)) < 1e-3f,
          "120 ms at 10 ms/frame gives %.4f, at 30 ms/frame gives %.4f",
          ui_tween_value(&a), ui_tween_value(&b));
}

/* Rule 1 and 2 together, at the level that matters: input must reach the model
 * immediately and must never be dropped because an animation is running. */
static void test_no_input_is_swallowed(void)
{
    wheel_state_t st;
    ui_wheel_init(&st);
    ui_wheel_settle(&st);

    /* 20 detents with no frames in between at all. */
    for (int i = 0; i < 20; ++i) ui_wheel_turn(&st, +1, 0);
    CHECK(st.group == 20 % WHEEL_GROUPS,
          "20 fast detents landed on group %d, expected %d — input was "
          "swallowed while the wheel was still moving",
          st.group, 20 % WHEEL_GROUPS);

    /* And the value: the model must be at the new value before a single frame
     * has been drawn. */
    ui_wheel_init(&st);
    ui_wheel_settle(&st);
    while (ui_wheel_group_size(st.group) < 2) ui_wheel_turn(&st, +1, 0);
    ui_wheel_press(&st, 0);
    ui_wheel_settle(&st);
    ui_wheel_press(&st, 0);
    ui_wheel_settle(&st);

    int p = ui_wheel_param_of(st.group, st.member);
    int before = st.val[p];
    for (int i = 0; i < 5; ++i) ui_wheel_turn(&st, +1, 0);
    CHECK(st.val[p] != before,
          "five detents in the value state did not change the model");
}

/* Rule 6: a level change is animated, not instant — and it finishes. */
static void test_level_change_animates(void)
{
    wheel_state_t st;
    ui_wheel_init(&st);
    ui_wheel_settle(&st);
    while (ui_wheel_group_size(st.group) < 2) ui_wheel_turn(&st, +1, 0);
    ui_wheel_settle(&st);

    ui_wheel_press(&st, 0);
    CHECK(st.prev_level == WHEEL_MAIN && st.level == WHEEL_GROUP,
          "level change did not keep the outgoing level for the cross-fade");
    CHECK(ui_wheel_tick(&st, 8), "level change was not animated at all");

    int frames = 1;
    while (ui_wheel_tick(&st, 8) && frames < 500) ++frames;
    CHECK(frames < 500, "level change never settled");
    CHECK(st.prev_level == st.level,
          "the outgoing level was never released after the cross-fade");
    CHECK(frames * 8 <= ui_motion_duration(UI_SPEED_LEVEL) + 24,
          "level change took %d ms", frames * 8);
}

/* The duration scale must stay ordered, or "micro" stops meaning micro. */
static void test_duration_scale(void)
{
    ui_motion_set_frame_ms(16.7f);
    for (int i = 0; i < 60; ++i) ui_motion_set_frame_ms(16.7f);
    int mi = ui_motion_duration(UI_SPEED_MICRO);
    int mo = ui_motion_duration(UI_SPEED_MOVE);
    int lv = ui_motion_duration(UI_SPEED_LEVEL);
    CHECK(mi < mo, "micro (%d) is not shorter than move (%d)", mi, mo);
    CHECK(mo < lv, "move (%d) is not shorter than a level change (%d)", mo, lv);
    CHECK(mi >= 60, "micro below ~60 ms is imperceptible and only costs frames");
}

/* THE ONE THAT WOULD HAVE CAUGHT THE JERKY MOTION.
 *
 * A duration in milliseconds means nothing on its own; what matters is how
 * many frames it gets. The bench panel is 24 MHz, so a 320x170 RGB565 frame is
 * 36.3 ms of wire time — at which the original fixed 90 ms ease had TWO AND A
 * HALF frames to run in. Whatever the frame time, every speed class must still
 * resolve to enough samples to read as movement. */
static void test_enough_frames_at_any_rate(void)
{
    static const float RATES[] = { 8.0f, 16.7f, 25.0f, 36.3f, 50.0f, 80.0f };
    static const char *NAME[] = { "MICRO", "MOVE", "LEVEL" };
    const int MIN_FRAMES = 8;

    for (size_t r = 0; r < sizeof RATES / sizeof RATES[0]; ++r) {
        /* settle the smoothing filter on this rate */
        for (int i = 0; i < 200; ++i) ui_motion_set_frame_ms(RATES[r]);

        for (int sp = 0; sp < UI_SPEED_COUNT; ++sp) {
            int dur    = ui_motion_duration((ui_speed_t)sp);
            int frames = (int)(dur / RATES[r]);
            CHECK(frames >= MIN_FRAMES,
                  "at %.1f ms/frame, %s resolves to %d ms = %d frames — under "
                  "%d frames it reads as a jump, not a motion",
                  (double)RATES[r], NAME[sp], dur, frames, MIN_FRAMES);
        }
    }
    for (int i = 0; i < 200; ++i) ui_motion_set_frame_ms(16.7f);   /* restore */
}

int main(void)
{
    test_curves();
    test_curve_character();
    test_lands_exactly();
    test_retarget_is_continuous();
    test_time_based();
    test_no_input_is_swallowed();
    test_level_change_animates();
    test_duration_scale();
    test_enough_frames_at_any_rate();

    if (failures) {
        printf("test_ui_motion: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_motion: OK — curves anchored and monotonic, tweens land "
           "exactly, retargets stay continuous, timing is dt-based, no input "
           "is swallowed\n");
    return 0;
}
