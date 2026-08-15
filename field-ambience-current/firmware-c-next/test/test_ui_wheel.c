/*
 * test_ui_wheel — guards for the radial navigation system.
 *
 * The wheel replaces a list with geometry, which moves the failure modes. A
 * list either fits or does not; a wheel can lose a parameter in a group nobody
 * reaches, spin the long way round when stepping back out, or park the value
 * orb where the panel edge cuts it in half. None of those show up as a compile
 * error and all three are invisible in a still preview, so they are checked
 * here instead.
 */
#include "../tools/ui_wheel.h"
#include "oled.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static int failures;

#define CHECK(cond, ...) do {                            \
    if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); \
                   printf("\n"); ++failures; }            \
} while (0)

/* Every one of the 16 parameters must live in exactly one group. A parameter
 * in two groups edits the same value from two places; a parameter in none is
 * unreachable on the device with no visible symptom. */
static void test_group_partition(void)
{
    int seen[WHEEL_PARAM_COUNT];
    memset(seen, 0, sizeof seen);
    int total = 0;

    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        int n = ui_wheel_group_size(g);
        CHECK(n >= 1, "group %s is empty", ui_wheel_group_name(g));
        total += n;
        for (int m = 0; m < n; ++m) {
            int p = ui_wheel_param_of(g, m);
            CHECK(p >= 0 && p < WHEEL_PARAM_COUNT,
                  "group %s member %d is param %d", ui_wheel_group_name(g), m, p);
            if (p >= 0 && p < WHEEL_PARAM_COUNT) ++seen[p];
        }
    }
    CHECK(total == WHEEL_PARAM_COUNT,
          "groups hold %d parameters, expected %d", total, WHEEL_PARAM_COUNT);
    for (int p = 0; p < WHEEL_PARAM_COUNT; ++p)
        CHECK(seen[p] == 1, "parameter %s appears in %d groups",
              ui_wheel_param_label(p), seen[p]);
}

/* A group whose name equals one of its own members prints the same word twice
 * on the sub-wheel, and the heading stops reading as a heading.
 *
 * Only groups that actually open a sub-wheel can show it. A single-member
 * group goes straight from the main wheel to the value and never renders both
 * names, so World/World and FX/FX are fine as they stand — but they would stop
 * being fine the moment a second parameter joined them, which is why the rule
 * is scoped rather than dropped. */
static void test_no_name_collision(void)
{
    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        if (ui_wheel_group_size(g) < 2) continue;
        for (int m = 0; m < ui_wheel_group_size(g); ++m) {
            const char *gn = ui_wheel_group_name(g);
            const char *pn = ui_wheel_param_label(ui_wheel_param_of(g, m));
            CHECK(strcmp(gn, pn) != 0,
                  "group \"%s\" contains a parameter with the same name", gn);
        }
    }
}

/* Turning must close on itself, and stepping in and back out must land on the
 * group you came from. */
static void test_navigation(void)
{
    wheel_state_t st;
    ui_wheel_init(&st);
    int seen[WHEEL_GROUPS];
    memset(seen, 0, sizeof seen);
    for (int i = 0; i < WHEEL_GROUPS; ++i) {
        seen[st.group] = 1;
        ui_wheel_turn(&st, +1, 0);
    }
    CHECK(st.group == 0, "%d turns did not return to group 0 (at %d)",
          WHEEL_GROUPS, st.group);
    for (int g = 0; g < WHEEL_GROUPS; ++g)
        CHECK(seen[g], "group %s unreachable", ui_wheel_group_name(g));

    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        ui_wheel_init(&st);
        for (int i = 0; i < g; ++i) ui_wheel_turn(&st, +1, 0);
        ui_wheel_settle(&st);
        CHECK(st.group == g, "walked to %d, landed on %d", g, st.group);

        ui_wheel_press(&st, 0);
        /* A one-member group must skip the sub-wheel entirely. */
        if (ui_wheel_group_size(g) == 1)
            CHECK(st.level == WHEEL_VALUE,
                  "single-member group %s did not go straight to the value",
                  ui_wheel_group_name(g));
        else
            CHECK(st.level == WHEEL_GROUP, "group %s did not open",
                  ui_wheel_group_name(g));

        while (st.level != WHEEL_MAIN) ui_wheel_press(&st, 1);
        CHECK(st.group == g, "stepping back out of %s changed the group to %d",
              ui_wheel_group_name(g), st.group);
    }
}

/* Stepping between levels must take the short way round. theta accumulates
 * freely as the user turns, so a naive retarget unwinds every rotation that
 * got you there — visible as the whole wheel spinning back. */
static void test_short_way(void)
{
    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        wheel_state_t st;
        ui_wheel_init(&st);
        for (int i = 0; i < g; ++i) ui_wheel_turn(&st, +1, 0);
        ui_wheel_settle(&st);

        ui_wheel_press(&st, 0);
        CHECK(fabsf(ui_wheel_theta_target(&st) - ui_wheel_theta(&st)) <= 180.0f + 1e-3f,
              "entering %s rotates %.1f deg — the long way",
              ui_wheel_group_name(g), fabsf(ui_wheel_theta_target(&st) - ui_wheel_theta(&st)));

        ui_wheel_settle(&st);
        while (st.level != WHEEL_MAIN) {
            ui_wheel_press(&st, 1);
            CHECK(fabsf(ui_wheel_theta_target(&st) - ui_wheel_theta(&st)) <= 180.0f + 1e-3f,
                  "leaving %s rotates %.1f deg — the long way",
                  ui_wheel_group_name(g), fabsf(ui_wheel_theta_target(&st) - ui_wheel_theta(&st)));
            ui_wheel_settle(&st);
        }
    }
}

/* The ease must actually terminate, and land exactly on the target rather than
 * creeping toward it forever. */
static void test_ease_settles(void)
{
    wheel_state_t st;
    ui_wheel_init(&st);
    ui_wheel_turn(&st, +1, 0);
    int frames = 0;
    while (ui_wheel_tick(&st, 8) && frames < 500) ++frames;
    CHECK(frames < 500, "the snap never settled");
    CHECK(ui_wheel_theta(&st) == ui_wheel_theta_target(&st),
          "settled at %.4f but the target is %.4f",
          ui_wheel_theta(&st), ui_wheel_theta_target(&st));
    CHECK(frames <= 60, "the snap took %d frames (~%d ms) — too slow to feel "
                        "like a detent", frames, frames * 8);
}

/* Composing must not fall over, must draw something at every level, and the
 * selected node must sit at 12 o'clock fully on the panel. */
static void test_compose(void)
{
    static const char *LVL[] = { "MAIN", "GROUP", "VALUE" };

    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        wheel_state_t st;
        ui_wheel_init(&st);
        for (int i = 0; i < g; ++i) ui_wheel_turn(&st, +1, 0);
        ui_wheel_settle(&st);

        for (int step = 0; step < 3; ++step) {
            long lit = 0;
            uint16_t line[OLED_WIDTH];
            int sel_row_bright = 0;

            for (int y = 0; y < OLED_HEIGHT; ++y) {
                ui_wheel_compose_row(&st, y, line);
                for (int x = 0; x < OLED_WIDTH; ++x)
                    if (line[x] != 0) ++lit;
                /* Sample the selected node's BODY, not its centre: the centre
                 * is where the icon is, drawn in black on the light disc, so
                 * probing it reports background for every icon with a stroke
                 * through the middle. Node centres are (158, 67) r=22 in MAIN
                 * and (158, 79) r=15 in GROUP; 0.7 r above centre is inside
                 * the disc and outside the icon (icon radius is 0.42 r). */
                if (st.level != WHEEL_VALUE &&
                    y == ((st.level == WHEEL_MAIN) ? 67 - 15 : 79 - 10)) {
                    uint16_t px = line[158];
                    int r = ((px >> 11) & 0x1F) << 3;
                    sel_row_bright = (r > 160);
                }
            }
            CHECK(lit > 500, "%s/%s drew almost nothing (%ld px)",
                  ui_wheel_group_name(g), LVL[st.level], lit);
            if (st.level != WHEEL_VALUE)
                CHECK(sel_row_bright,
                      "%s/%s: the selected node is not at 12 o'clock",
                      ui_wheel_group_name(g), LVL[st.level]);

            ui_wheel_press(&st, 0);
            ui_wheel_settle(&st);
        }
    }
}

/* 0 and 100 are the two values that must never be ambiguous, and they are
 * exactly the two that sit closest to the bottom edge. The orb has to stay
 * fully on the panel at both ends. */
static void test_value_extremes(void)
{
    const float HX = 158.0f, HY = 171.0f, R = 64.0f, ORB = 6.5f;
    const float A0 = -78.0f, A1 = 78.0f, D2R = 0.017453293f;

    for (int pct = 0; pct <= 100; pct += 100) {
        float a  = (A0 + (A1 - A0) * pct / 100.0f) * D2R;
        float cx = HX + R * sinf(a), cy = HY - R * cosf(a);
        CHECK(cx - ORB >= 0.0f && cx + ORB <= (float)OLED_WIDTH,
              "value %d puts the orb off the side (x %.1f)", pct, cx);
        CHECK(cy + ORB <= (float)OLED_HEIGHT,
              "value %d clips the orb at the bottom edge (y %.1f, orb ends "
              "%.1f, panel %d)", pct, cy, cy + ORB, OLED_HEIGHT);
    }
}

int main(void)
{
    test_group_partition();
    test_no_name_collision();
    test_navigation();
    test_short_way();
    test_ease_settles();
    test_compose();
    test_value_extremes();

    if (failures) {
        printf("test_ui_wheel: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_wheel: OK — %d groups partition %d parameters, "
           "navigation closes, snaps take the short way, %d screens compose\n",
           WHEEL_GROUPS, WHEEL_PARAM_COUNT, WHEEL_GROUPS * 3);
    return 0;
}
