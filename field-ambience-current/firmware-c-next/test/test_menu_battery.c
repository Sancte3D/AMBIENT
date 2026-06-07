/*
 * Step 12b #7 tests — menu state machine + battery curve.
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

/* Capture which engine setters were called and with what. (VOLUME / DRONE /
 * GENERATE are NOT in the menu — they have dedicated hardware controls per
 * SPEC §5 / §7, no callbacks here.) */
static struct {
    int   key, mode, vibe, voice;
    float texture, bass, space, mood;
} st;
static void cb_key  (int v)             { st.key = v; }
static void cb_mode (int v)             { st.mode = v; }
static void cb_vibe (int v)             { st.vibe = v; }
static void cb_voice(int v)             { st.voice = v; }
static void cb_tex  (float v)           { st.texture = v; }
static void cb_bass (float v)           { st.bass = v; }
static void cb_space(float v)           { st.space = v; }
static void cb_mood (float v)           { st.mood = v; }

static void init(void) {
    memset(&st, 0, sizeof st);
    menu_callbacks_t cb = {
        cb_key, cb_mode, cb_vibe, cb_voice,
        cb_tex, cb_bass, cb_space, cb_mood
    };
    menu_init(&cb);
}

static void test_browse_navigates_through_all_params(void) {
    init();
    CHECK(menu_current() == MP_KEY,  "init param != KEY");
    CHECK(menu_mode()    == MENU_BROWSE, "init mode != BROWSE");

    /* Going forward through all params and wrapping returns to KEY. */
    for (int i = 1; i < MP_COUNT; ++i) {
        menu_rotate(1);
        CHECK(menu_current() == (menu_param_t)i, "step %d landed on %d", i, menu_current());
    }
    menu_rotate(1);
    CHECK(menu_current() == MP_KEY, "wrap-around forward broke (got %d)", menu_current());

    /* Backwards from KEY goes to the last param. */
    menu_rotate(-1);
    CHECK(menu_current() == MP_COUNT - 1, "wrap-around backward broke (got %d)", menu_current());
}

static void test_push_toggles_mode(void) {
    init();
    CHECK(menu_mode() == MENU_BROWSE, "");
    menu_push(); CHECK(menu_mode() == MENU_EDIT, "push didn't enter edit");
    menu_push(); CHECK(menu_mode() == MENU_BROWSE, "push didn't return to browse");
}

static void test_edit_key_fires_callback_and_cycles_12(void) {
    init();
    /* navigate to KEY, enter edit */
    menu_push();
    CHECK(menu_mode() == MENU_EDIT, "");
    menu_rotate(1);                       /* +1 semitone */
    CHECK(strcmp(menu_current_value_text(), "C#") == 0,
          "key+1 should be C#: got %s", menu_current_value_text());
    CHECK(st.key == 61, "set_key not called with 61: got %d", st.key);

    /* 12 forward → back to C */
    for (int i = 0; i < 11; ++i) menu_rotate(1);
    CHECK(strcmp(menu_current_value_text(), "C") == 0, "wrap to C failed");
    CHECK(st.key == 60, "wrap should set 60: got %d", st.key);
}

static void test_continuous_clamps_and_steps_by_5(void) {
    init();
    /* navigate to TEXTURE (index 6) */
    for (int i = 0; i < (int)MP_TEXTURE; ++i) menu_rotate(1);
    CHECK(menu_current() == MP_TEXTURE, "didn't reach TEXTURE");
    menu_push();
    /* default is 20% */
    CHECK(menu_value_int(MP_TEXTURE) == 20, "init texture != 20");
    menu_rotate(+1);
    CHECK(menu_value_int(MP_TEXTURE) == 25, "step should be 5");
    /* fast scroll up to 100, then attempt to overshoot — clamps */
    menu_rotate(+30);
    CHECK(menu_value_int(MP_TEXTURE) == 100, "did not clamp at 100");
    CHECK(st.texture > 0.99f, "callback texture didn't reach 1.0: %.3f", st.texture);
    menu_rotate(-100);
    CHECK(menu_value_int(MP_TEXTURE) == 0, "did not clamp at 0");
    CHECK(st.texture == 0.0f, "callback texture didn't reach 0");
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
    /* monotone non-increasing as voltage drops */
    int prev = 100;
    for (int mv = 4200; mv >= 3000; mv -= 10) {
        int p = battery_pct_from_voltage(mv / 1000.0f);
        CHECK(p <= prev, "non-monotone at %dmV: prev %d, now %d", mv, prev, p);
        prev = p;
    }
}

static void test_bfont_width(void) {
    /* empty = 0; non-empty > 0; longer wider; the 40px face wider than 24px. */
    CHECK(bfont_width(&font_hn_value, "") == 0, "empty width != 0");
    int wc = bfont_width(&font_hn_value, "C");
    CHECK(wc > 0, "single glyph width not positive: %d", wc);
    CHECK(bfont_width(&font_hn_value, "CC") > wc, "two glyphs not wider than one");
    CHECK(bfont_width(&font_hn_value, "Lydian") > bfont_width(&font_hn_value_small, "Lydian"),
          "40px face not wider than 24px");
    CHECK(font_hn_label.ascent > 0 && font_hn_value.ascent > 0, "fonts not initialised");
}

int main(void) {
    printf("== menu / battery (step12b #7) ==\n");
    test_bfont_width();
    test_browse_navigates_through_all_params();
    test_push_toggles_mode();
    test_edit_key_fires_callback_and_cycles_12();
    test_continuous_clamps_and_steps_by_5();
    test_battery_curve();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
