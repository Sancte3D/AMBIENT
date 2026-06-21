/*
 * Menu state machine + battery curve tests — WORLD model (r18.38).
 */

#include "menu.h"
#include "battery.h"
#include "baked_font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks = 0, g_fails = 0;
#define CHECK(c, ...) do { ++g_checks; if (!(c)) { ++g_fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Capture which engine setters were called and with what. (DRONE / HOLD /
 * SHIFT / GENERATE / CLEAR are MODIFIER BUTTONS, not menu entries — no
 * callbacks here.) */
static struct {
    int   world, drums;
    float space, tone, atmos;
} st;
static void cb_world(int v)   { st.world = v; }
static void cb_space(float v) { st.space = v; }
static void cb_tone (float v) { st.tone  = v; }
static void cb_atmos(float v) { st.atmos = v; }
static void cb_drums(int v)   { st.drums = v; }

static void init(void) {
    memset(&st, 0, sizeof st);
    menu_callbacks_t cb = { cb_world, cb_space, cb_tone, cb_atmos, cb_drums };
    menu_init(&cb);
}

static void test_browse_navigates_through_all_params(void) {
    init();
    CHECK(menu_current() == MP_WORLD,    "init param != WORLD");
    CHECK(menu_mode()    == MENU_BROWSE, "init mode != BROWSE");

    for (int i = 1; i < MP_COUNT; ++i) {
        menu_rotate(1);
        CHECK(menu_current() == (menu_param_t)i, "step %d landed on %d", i, menu_current());
    }
    menu_rotate(1);
    CHECK(menu_current() == MP_WORLD, "wrap-around forward broke (got %d)", menu_current());

    menu_rotate(-1);
    CHECK(menu_current() == MP_COUNT - 1, "wrap-around backward broke (got %d)", menu_current());
}

static void test_push_toggles_mode(void) {
    init();
    CHECK(menu_mode() == MENU_BROWSE, "");
    menu_push(); CHECK(menu_mode() == MENU_EDIT, "push didn't enter edit");
    menu_push(); CHECK(menu_mode() == MENU_BROWSE, "push didn't return to browse");
}

static void test_edit_world_cycles_4_and_loads_preset(void) {
    init();
    menu_push();                          /* enter edit on WORLD */
    CHECK(menu_mode() == MENU_EDIT, "");
    CHECK(menu_world_index() == 0, "init world != 0");
    CHECK(strcmp(menu_current_value_text(), "Tokyo City") == 0,
          "world 0 should be Tokyo City: got %s", menu_current_value_text());

    menu_rotate(1);                       /* → Crystal Coast */
    CHECK(menu_world_index() == 1, "world+1 != 1");
    CHECK(st.world == 1, "set_world not called with 1: got %d", st.world);
    /* selecting a world loads its preset → space/tone/atmos callbacks fire */
    CHECK(st.space > 0.0f, "world change should push space preset");
    CHECK(st.tone  > 0.0f, "world change should push tone preset");

    /* 4 worlds → wrap back to Tokyo City */
    for (int i = 0; i < 3; ++i) menu_rotate(1);
    CHECK(menu_world_index() == 0, "wrap to world 0 failed: %d", menu_world_index());
    CHECK(strcmp(menu_current_value_text(), "Tokyo City") == 0, "wrap text wrong");
}

static void test_continuous_steps_by_tick_and_clamps(void) {
    init();
    /* navigate to SPACE */
    for (int i = 0; i < (int)MP_SPACE; ++i) menu_rotate(1);
    CHECK(menu_current() == MP_SPACE, "didn't reach SPACE");
    menu_push();
    /* Tokyo City default space = 42. One tick = 1%. */
    CHECK(menu_value_int(MP_SPACE) == 42, "init space != 42 (got %d)", menu_value_int(MP_SPACE));
    menu_rotate(+1);
    CHECK(menu_value_int(MP_SPACE) == 43, "one tick should be 1%%: %d", menu_value_int(MP_SPACE));
    menu_rotate(+8);
    CHECK(menu_value_int(MP_SPACE) == 51, "8-tick flick should add 8%%: %d", menu_value_int(MP_SPACE));
    menu_rotate(+100);
    CHECK(menu_value_int(MP_SPACE) == 100, "did not clamp at 100");
    CHECK(st.space > 0.99f, "callback space didn't reach 1.0: %.3f", st.space);
    menu_rotate(-200);
    CHECK(menu_value_int(MP_SPACE) == 0, "did not clamp at 0");
    CHECK(st.space == 0.0f, "callback space didn't reach 0");
}

static void test_drums_toggle(void) {
    init();
    for (int i = 0; i < (int)MP_DRUMS; ++i) menu_rotate(1);
    CHECK(menu_current() == MP_DRUMS, "didn't reach DRUMS");
    menu_push();
    CHECK(menu_value_index(MP_DRUMS) == 0, "drums init != Off");
    CHECK(strcmp(menu_current_value_text(), "Off") == 0, "drums text != Off");
    menu_rotate(+1);
    CHECK(menu_value_index(MP_DRUMS) == 1, "drums didn't toggle to On");
    CHECK(strcmp(menu_current_value_text(), "On") == 0, "drums text != On");
    CHECK(st.drums == 1, "set_drums not called with 1");
    menu_rotate(+1);                      /* wraps back to Off */
    CHECK(menu_value_index(MP_DRUMS) == 0, "drums didn't wrap to Off");
}

static void test_browse_nav_ignores_accel(void) {
    init();
    menu_rotate(+5);
    CHECK(menu_current() == MP_SPACE, "browse should advance 1 slot on +5, got %d", menu_current());
    menu_rotate(-3);
    CHECK(menu_current() == MP_WORLD, "browse should retreat 1 slot on -3, got %d", menu_current());
}

static void test_world_does_not_drift(void) {
    /* Spin the world value a lot; index must stay inside 0..3. */
    init();
    menu_push();  /* enter edit on WORLD */
    for (int i = 0; i < 10000; ++i) menu_rotate(+1);
    CHECK(menu_world_index() >= 0 && menu_world_index() < MENU_WORLD_COUNT,
          "world drifted to %d after 10k +1 turns", menu_world_index());
    for (int i = 0; i < 20000; ++i) menu_rotate(-1);
    CHECK(menu_world_index() >= 0 && menu_world_index() < MENU_WORLD_COUNT,
          "world drifted to %d after 20k -1 turns", menu_world_index());
}

static void test_subtitle_present(void) {
    init();
    for (int w = 0; w < MENU_WORLD_COUNT; ++w) {
        menu_push();
        int guard = 0;
        while (menu_world_index() != w && guard++ < MENU_WORLD_COUNT * 2) menu_rotate(+1);
        menu_push();
        CHECK(menu_world_subtitle() != NULL && menu_world_subtitle()[0] != '\0',
              "world %d has empty subtitle", w);
    }
}

/* --- battery curve --- */

static void test_battery_curve(void) {
    CHECK(battery_pct_from_voltage(4.3f) == 100, "high clamp");
    CHECK(battery_pct_from_voltage(4.20f) == 100, "");
    CHECK(battery_pct_from_voltage(4.10f) >= 90 && battery_pct_from_voltage(4.10f) <= 95, "4.10V");
    CHECK(battery_pct_from_voltage(3.80f) == 60, "");
    CHECK(battery_pct_from_voltage(3.70f) == 40, "");
    CHECK(battery_pct_from_voltage(3.60f) == 20, "");
    CHECK(battery_pct_from_voltage(3.40f) == 5, "");
    CHECK(battery_pct_from_voltage(3.00f) == 0, "");
    CHECK(battery_pct_from_voltage(2.5f) == 0, "low clamp");
    int prev = 100;
    for (int mv = 4200; mv >= 3000; mv -= 10) {
        int p = battery_pct_from_voltage(mv / 1000.0f);
        CHECK(p <= prev, "non-monotone at %dmV: prev %d, now %d", mv, prev, p);
        prev = p;
    }
}

static void test_bfont_width(void) {
    CHECK(bfont_width(&font_hn_value, "") == 0, "empty width != 0");
    int wc = bfont_width(&font_hn_value, "C");
    CHECK(wc > 0, "single glyph width not positive: %d", wc);
    CHECK(bfont_width(&font_hn_value, "CC") > wc, "two glyphs not wider than one");
    CHECK(bfont_width(&font_hn_value, "Tokyo") > bfont_width(&font_hn_value_small, "Tokyo"),
          "40px face not wider than 24px");
    CHECK(font_hn_label.ascent > 0 && font_hn_value.ascent > 0, "fonts not initialised");
}

int main(void) {
    printf("== menu (world model) / battery ==\n");
    test_bfont_width();
    test_browse_navigates_through_all_params();
    test_push_toggles_mode();
    test_edit_world_cycles_4_and_loads_preset();
    test_continuous_steps_by_tick_and_clamps();
    test_drums_toggle();
    test_browse_nav_ignores_accel();
    test_world_does_not_drift();
    test_subtitle_present();
    test_battery_curve();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
