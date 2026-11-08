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
#include "../tools/ui_draw.h"
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
        CHECK(st.level == WHEEL_SCENE, "group %s did not open its scene",
              ui_wheel_group_name(g));

        /* A press inside a scene hands the encoder to the next property; it
         * must never descend, because there is nowhere deeper to go. */
        for (int i = 0; i < 5; ++i) {
            ui_wheel_press(&st, 0);
            CHECK(st.level == WHEEL_SCENE,
                  "a press inside %s left the scene", ui_wheel_group_name(g));
            CHECK(st.member < ui_wheel_group_size(g),
                  "focus %d is outside %s's %d properties",
                  st.member, ui_wheel_group_name(g), ui_wheel_group_size(g));
        }

        ui_wheel_press(&st, 1);
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
    static const char *LVL[] = { "MAIN", "SCENE" };

    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        wheel_state_t st;
        ui_wheel_init(&st);
        for (int i = 0; i < g; ++i) ui_wheel_turn(&st, +1, 0);
        ui_wheel_settle(&st);

        for (int step = 0; step < 2; ++step) {
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
                /* The selected node is painted in the NAVIGATION encoder's
                 * green, so probe green rather than a generic brightness — a
                 * luminance probe would also pass on the white it used to be,
                 * and the point of the change is that it is no longer white. */
                if (st.level == WHEEL_MAIN && y == 67 - 15) {
                    uint16_t px = line[158];
                    int r = ((px >> 11) & 0x1F) << 3;
                    int gg = ((px >> 5) & 0x3F) << 2;
                    sel_row_bright = (gg > 160 && gg > r + 60);
                }
            }
            CHECK(lit > 500, "%s/%s drew almost nothing (%ld px)",
                  ui_wheel_group_name(g), LVL[st.level], lit);
            if (st.level == WHEEL_MAIN)
                CHECK(sel_row_bright,
                      "%s/%s: the selected node is not at 12 o'clock",
                      ui_wheel_group_name(g), LVL[st.level]);

            ui_wheel_press(&st, 0);
            ui_wheel_settle(&st);
        }
    }
}

/* 0 and 100 are the two values that must never be ambiguous, and they are
 * exactly the two that sit closest to the bottom edge. The head dot has to stay
 * fully on the panel at both ends of every slot — mirrors ring_slot() in
 * ui_wheel.c, so a change to the sweep or the gap that pushes a slot end off
 * the panel fails here rather than on glass. */
static void test_value_extremes(void)
{
    const float HX = 158.0f, HY = 171.0f, R = 64.0f, ORB = 6.5f;
    const float A0 = -78.0f, A1 = 78.0f, GAP = 7.0f, D2R = 0.017453293f;
    const float STEP = (A1 - A0) / (float)UI_SCENE_MAX_KNOBS;

    for (int slot = 0; slot < UI_SCENE_MAX_KNOBS; ++slot)
    for (int end = 0; end < 2; ++end) {
        float pct = end ? 100.0f : 0.0f;
        float s0 = A0 + slot * STEP + GAP * 0.5f;
        float s1 = A0 + (slot + 1) * STEP - GAP * 0.5f;
        float a  = (s0 + (s1 - s0) * pct / 100.0f) * D2R;
        float cx = HX + R * sinf(a), cy = HY - R * cosf(a);
        CHECK(cx - ORB >= 0.0f && cx + ORB <= (float)OLED_WIDTH,
              "slot %d at %.0f puts the head off the side (x %.1f)",
              slot, (double)pct, cx);
        CHECK(cy + ORB <= (float)OLED_HEIGHT,
              "slot %d at %.0f clips the head at the bottom edge (y %.1f, ends "
              "%.1f, panel %d)", slot, (double)pct, cy, cy + ORB, OLED_HEIGHT);
    }
}

/* THE COLOUR MECHANISM, as behaviour rather than as pixels.
 *
 * Each scene property is bound to one physical encoder, and the binding is what
 * the colour communicates. Two things therefore have to hold, or the colour is
 * lying: turning encoder i must move property i AND NOTHING ELSE, and turning
 * an encoder the scene does not use must do nothing at all rather than fall
 * through to some other property. */
static void test_knobs_are_independent(void)
{
    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        int n = ui_wheel_group_size(g);
        for (int knob = 0; knob < UI_SCENE_MAX_KNOBS; ++knob) {
            wheel_state_t st;
            ui_wheel_init(&st);
            for (int i = 0; i < g; ++i) ui_wheel_turn(&st, +1, 0);
            ui_wheel_settle(&st);
            ui_wheel_press(&st, 0);
            ui_wheel_settle(&st);

            uint8_t before[WHEEL_PARAM_COUNT];
            memcpy(before, st.val, sizeof before);
            uint8_t grp = st.group;

            /* ONE detent. Three would wrap a 3-option list straight back to
             * where it started and read as "the knob does nothing". */
            ui_wheel_turn_knob(&st, knob, +1, 0);

            CHECK(st.group == grp && st.level == WHEEL_SCENE,
                  "turning knob %d in %s navigated — scene knobs must never "
                  "move the wheel", knob, ui_wheel_group_name(g));

            for (int p = 0; p < WHEEL_PARAM_COUNT; ++p) {
                int owned = (knob < n && p == ui_wheel_param_of(g, knob));
                if (owned) continue;
                CHECK(st.val[p] == before[p],
                      "turning knob %d in %s also changed %s (%d -> %d)",
                      knob, ui_wheel_group_name(g), ui_wheel_param_label(p),
                      before[p], st.val[p]);
            }
            if (knob < n) {
                int p = ui_wheel_param_of(g, knob);
                CHECK(st.val[p] != before[p],
                      "knob %d in %s moved nothing — %s stayed at %d",
                      knob, ui_wheel_group_name(g),
                      ui_wheel_param_label(p), before[p]);
            }
        }

        /* And a scene knob must be inert on the wheel itself, where those
         * encoders hold the globals instead. */
        wheel_state_t st;
        ui_wheel_init(&st);
        uint8_t before[WHEEL_PARAM_COUNT];
        memcpy(before, st.val, sizeof before);
        for (int knob = 0; knob < UI_SCENE_MAX_KNOBS; ++knob)
            ui_wheel_turn_knob(&st, knob, +1, 0);
        CHECK(memcmp(before, st.val, sizeof before) == 0,
              "a scene knob changed a parameter from the top-level wheel");
    }
}

/* NO VALUE MAY EVER BE TRUNCATED, so the names have to fit by construction.
 *
 * Three values sit side by side on one 250 px line at 12 px per character, and
 * the previous version simply cut whatever ran over — which is how "Tokyo City"
 * became "Tokyo Ci". A cut word reads as a rendering fault, and there is no
 * space left to grow into, so the constraint belongs on the NAMES: this walks
 * the widest reachable value of every property of every group and fails if any
 * group's worst case overflows. It fails the day someone adds an option whose
 * name is too long, which is the only moment the fix is cheap.
 *
 * Mirrors the layout constants in compose_scene(). */
static void test_readout_never_truncates(void)
{
    const bakedfont_t *f = &font_hn_value_small;
    const int VLEFT = 6, RIGHT = 256, GAPX = 12;

    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        wheel_state_t st;
        ui_wheel_init(&st);
        int n = ui_wheel_group_size(g), total = 0;
        char worst[UI_SCENE_MAX_KNOBS][24] = { { 0 } };

        for (int m = 0; m < n; ++m) {
            int p = ui_wheel_param_of(g, m), wide = 0;
            /* 0..100 covers every option index (the accessor clamps past the
             * end of a list) and every continuous reading. */
            for (int v = 0; v <= 100; ++v) {
                st.val[p] = (uint8_t)v;
                const char *s = ui_wheel_param_value(&st, p);
                int w = ui_text_w(f, s);
                if (w > wide) {
                    wide = w;
                    snprintf(worst[m], sizeof worst[m], "%s", s);
                }
            }
            total += wide + (m ? GAPX : 0);
        }
        CHECK(total <= RIGHT - VLEFT,
              "%s's widest readout needs %d px but the line is %d: \"%s\" "
              "\"%s\" \"%s\" — shorten an option name rather than let the "
              "draw path cut it",
              ui_wheel_group_name(g), total, RIGHT - VLEFT,
              worst[0], worst[1], worst[2]);
    }
}

/* ui_wheel_param_value formats continuous values into one shared static
 * buffer. The scene readout holds up to three of them at once, so aliasing
 * showed Room's 62 and 38 as "38 38" — a readout that looks like it works. */
static void test_values_do_not_alias(void)
{
    wheel_state_t st;
    ui_wheel_init(&st);
    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        int n = ui_wheel_group_size(g);
        if (n < 2) continue;
        for (int m = 0; m < n; ++m) st.val[ui_wheel_param_of(g, m)] =
            (uint8_t)(11 + m * 17);

        char seen[UI_SCENE_MAX_KNOBS][16];
        for (int m = 0; m < n; ++m)
            snprintf(seen[m], sizeof seen[m], "%s",
                     ui_wheel_param_value(&st, ui_wheel_param_of(g, m)));
        for (int m = 0; m < n; ++m) {
            char want[16];
            snprintf(want, sizeof want, "%s",
                     ui_wheel_param_value(&st, ui_wheel_param_of(g, m)));
            CHECK(strcmp(seen[m], want) == 0,
                  "%s property %d read back as \"%s\" but is \"%s\" — the "
                  "value buffer is shared and the caller kept the pointer",
                  ui_wheel_group_name(g), m, seen[m], want);
        }
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
    test_knobs_are_independent();
    test_readout_never_truncates();
    test_values_do_not_alias();

    if (failures) {
        printf("test_ui_wheel: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_wheel: OK — %d groups partition %d parameters, "
           "navigation closes, snaps take the short way, %d screens compose, "
           "and each of the %d scene encoders moves its own property only\n",
           WHEEL_GROUPS, WHEEL_PARAM_COUNT, WHEEL_GROUPS * 2,
           UI_SCENE_MAX_KNOBS);
    return 0;
}
