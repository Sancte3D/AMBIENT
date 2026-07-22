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
 * callbacks here. Tone + Drums dropped in r18.58 — Tone duplicated the
 * Brightness encoder, Drums needs an adaptive-drums engine we don't have.) */
static struct {
    int   world, key, tuning, voice;
    float space, shim, atmos, motion, age, echo, blur;
} st;
static void cb_world (int v)   { st.world  = v; }
static void cb_key   (int v)   { st.key    = v; }
static void cb_tuning(int v)   { st.tuning = v; }
static void cb_voice (int v)   { st.voice  = v; }
static void cb_space (float v) { st.space  = v; }
static void cb_shim  (float v) { st.shim   = v; }
static void cb_atmos (float v) { st.atmos  = v; }
static void cb_motion(float v) { st.motion = v; }
static void cb_age   (float v) { st.age    = v; }
static void cb_echo  (float v) { st.echo   = v; }
static void cb_blur  (float v) { st.blur   = v; }

static void init(void) {
    memset(&st, 0, sizeof st);
    menu_callbacks_t cb = {
        .set_world = cb_world,   .set_key  = cb_key,  .set_tuning = cb_tuning,
        .set_voice = cb_voice,
        .set_space = cb_space,   .set_shimmer = cb_shim,
        .set_atmosphere = cb_atmos,
        .set_motion = cb_motion, .set_age = cb_age,
        .set_echo = cb_echo,     .set_blur = cb_blur,
    };
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

static void test_edit_world_cycles_5_and_loads_preset(void) {
    init();
    menu_push();                          /* enter edit on WORLD */
    CHECK(menu_mode() == MENU_EDIT, "");
    CHECK(menu_world_index() == 0, "init world != 0");
    CHECK(strcmp(menu_current_value_text(), "Alps") == 0,
          "world 0 should be Alps: got %s", menu_current_value_text());

    menu_rotate(1);                       /* → Open Sea */
    CHECK(menu_world_index() == 1, "world+1 != 1");
    CHECK(st.world == 1, "set_world not called with 1: got %d", st.world);
    /* selecting a world loads its preset → space/atmos/motion/age callbacks fire */
    CHECK(st.space  > 0.0f, "world change should push space preset");
    CHECK(st.motion > 0.0f, "world change should push motion preset");
    CHECK(st.age    > 0.0f, "world change should push age preset");

    /* 5 worlds → wrap back to Alps */
    for (int i = 0; i < 4; ++i) menu_rotate(1);
    CHECK(menu_world_index() == 0, "wrap to world 0 failed: %d", menu_world_index());
    CHECK(strcmp(menu_current_value_text(), "Alps") == 0, "wrap text wrong");
}

static void test_continuous_steps_by_tick_and_clamps(void) {
    init();
    /* navigate to SPACE */
    for (int i = 0; i < (int)MP_SPACE; ++i) menu_rotate(1);
    CHECK(menu_current() == MP_SPACE, "didn't reach SPACE");
    menu_push();
    /* Alps default space = 55. One tick = 1%. */
    CHECK(menu_value_int(MP_SPACE) == 55, "init space != 55 (got %d)", menu_value_int(MP_SPACE));
    menu_rotate(+1);
    CHECK(menu_value_int(MP_SPACE) == 56, "one tick should be 1%%: %d", menu_value_int(MP_SPACE));
    menu_rotate(+8);
    CHECK(menu_value_int(MP_SPACE) == 64, "8-tick flick should add 8%%: %d", menu_value_int(MP_SPACE));
    menu_rotate(+100);
    CHECK(menu_value_int(MP_SPACE) == 100, "did not clamp at 100");
    CHECK(st.space > 0.99f, "callback space didn't reach 1.0: %.3f", st.space);
    menu_rotate(-200);
    CHECK(menu_value_int(MP_SPACE) == 0, "did not clamp at 0");
    CHECK(st.space == 0.0f, "callback space didn't reach 0");
}

static void test_motion_and_age_macros(void) {
    init();
    /* navigate to MOTION (slot 3) and bump it up; check the engine setter
     * fires with the expected scaled value. Same for AGE (slot 4). */
    for (int i = 0; i < (int)MP_MOTION; ++i) menu_rotate(1);
    CHECK(menu_current() == MP_MOTION, "didn't reach MOTION");
    menu_push();
    int v0 = menu_value_int(MP_MOTION);
    menu_rotate(+25);
    CHECK(menu_value_int(MP_MOTION) == v0 + 25, "motion value didn't move");
    CHECK(st.motion > 0.0f && st.motion <= 1.0f, "motion cb out of range: %g", (double)st.motion);

    menu_push();  /* back to browse */
    menu_rotate(+1);
    CHECK(menu_current() == MP_AGE, "didn't reach AGE");
    menu_push();
    int a0 = menu_value_int(MP_AGE);
    menu_rotate(+10);
    CHECK(menu_value_int(MP_AGE) == a0 + 10, "age value didn't move");
    CHECK(st.age > 0.0f && st.age <= 1.0f, "age cb out of range: %g", (double)st.age);
}

static void test_browse_nav_ignores_accel(void) {
    init();
    menu_rotate(+5);
    CHECK(menu_current() == MP_KEY, "browse should advance 1 slot on +5, got %d", menu_current());
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

/* r18.98 — KEY (12 pitch classes, world default on world change) and VOICE
 * (Pad/String/Glass, a player's global choice that world changes keep). */
static void test_key_and_voice_slots(void) {
    init();
    /* KEY sits right after WORLD, boots on Alps' tonic G (pc 7) */
    menu_rotate(1);
    CHECK(menu_current() == MP_KEY, "slot 1 should be KEY (got %d)", menu_current());
    CHECK(menu_value_count(MP_KEY) == 12, "KEY has 12 options");
    CHECK(strcmp(menu_current_value_text(), "G") == 0,
          "boot key is Alps' G: got %s", menu_current_value_text());
    menu_push();
    menu_rotate(1);                       /* G -> G# */
    CHECK(st.key == 8, "set_key not fired with G#=8 (got %d)", st.key);
    CHECK(strcmp(menu_current_value_text(), "G#") == 0, "value text follows");
    menu_rotate(-2);                      /* G# -> F# */
    CHECK(st.key == 6, "key steps down to F#=6 (got %d)", st.key);
    menu_push();

    /* TUNING (r19.6): slot 2, 2 options, defaults to Equal */
    menu_rotate(1);
    CHECK(menu_current() == MP_TUNING, "slot 2 should be TUNING (got %d)", menu_current());
    CHECK(menu_value_count(MP_TUNING) == 2, "TUNING has 2 options");
    CHECK(strcmp(menu_current_value_text(), "Equal") == 0,
          "default tuning is Equal: got %s", menu_current_value_text());
    menu_push();
    menu_rotate(1);
    CHECK(st.tuning == 1 && strcmp(menu_current_value_text(), "Just") == 0,
          "set_tuning Just (got %d)", st.tuning);
    menu_push();

    /* VOICE: slot 3, 4 options (Pad/String/Glass/Ember r19.28), defaults to Pad */
    menu_rotate(1);
    CHECK(menu_current() == MP_VOICE, "slot 3 should be VOICE (got %d)", menu_current());
    CHECK(menu_value_count(MP_VOICE) == 4, "VOICE has 4 options");
    CHECK(strcmp(menu_current_value_text(), "Pad") == 0,
          "default voice is Pad: got %s", menu_current_value_text());
    menu_push();
    menu_rotate(1);
    CHECK(st.voice == 1, "set_voice String (got %d)", st.voice);
    menu_rotate(1);
    CHECK(st.voice == 2 &&
          strcmp(menu_current_value_text(), "Glass") == 0, "set_voice Glass");
    menu_push();

    /* world change: KEY snaps to the new world's tonic, VOICE stays.
     * VOICE is slot 3 now → three single-step retreats back to WORLD. */
    menu_rotate(-1); menu_rotate(-1); menu_rotate(-1);
    CHECK(menu_current() == MP_WORLD, "back on WORLD (got %d)", menu_current());
    menu_push();
    menu_rotate(1);                       /* -> Open Sea (D, key kept) */
    CHECK(st.key == 2, "world change pushes the new tonic D=2 (got %d)", st.key);
    CHECK(st.voice == 2, "world change must NOT reset the voice (got %d)", st.voice);
    menu_push();
    menu_rotate(1);                       /* -> KEY slot */
    CHECK(strcmp(menu_current_value_text(), "D") == 0,
          "KEY slot shows the world tonic: got %s", menu_current_value_text());
}

int main(void) {
    printf("== menu (world model) / battery ==\n");
    test_bfont_width();
    test_key_and_voice_slots();
    test_browse_navigates_through_all_params();
    test_push_toggles_mode();
    test_edit_world_cycles_5_and_loads_preset();
    test_continuous_steps_by_tick_and_clamps();
    test_motion_and_age_macros();
    test_browse_nav_ignores_accel();
    test_world_does_not_drift();
    test_subtitle_present();
    test_battery_curve();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
