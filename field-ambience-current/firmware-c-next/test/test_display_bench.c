/*
 * Host test for the bench display tool (tools/display_hw_test.c).
 *
 * Compiles the REAL tool source against the fake SDK headers in
 * test/pico_stubs/ and drives it with fake time + scripted GPIO:
 *   - quadrature decoder: full-cycle detents both directions, bounce immunity
 *   - tween engine: text wipe / bar slide / fill settle to exact targets
 *   - mode change: fade-out → snap → fade-in sequence completes
 *   - overlay lifecycle: shows, retargets, ALWAYS fades out by itself
 *     (regression guard for the "stuck in backlight mode" class of bug)
 *   - velocity acceleration: exact sim tier values, slow turn = 1
 *   - backlight gamma: endpoints + the 1-3 % floor (must glow, not be off)
 *   - test scenes render and produce pixels
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* fake SDK headers first, so their types exist for the stub bodies below */
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* ---- fake SDK implementations (headers come from test/pico_stubs/) ----- */
static uint32_t fake_ms = 0;
static bool     sim_pin[32];          /* scripted GPIO levels (true = high) */
static uint16_t pwm_level[32];        /* recorded backlight duty            */

typedef unsigned int uint;
void stdio_init_all(void) {}
void sleep_ms(uint32_t ms) { (void)ms; }
void gpio_init(uint p) { sim_pin[p] = true; }     /* idle high (pull-up)    */
void gpio_set_dir(uint p, bool o) { (void)p; (void)o; }
void gpio_pull_up(uint p) { (void)p; }
bool gpio_get(uint p) { return sim_pin[p]; }
void gpio_set_function(uint p, int f) { (void)p; (void)f; }
uint pwm_gpio_to_slice_num(uint g) { (void)g; return 0; }
void pwm_set_wrap(uint s, uint16_t w) { (void)s; (void)w; }
void pwm_set_gpio_level(uint g, uint16_t l) { pwm_level[g] = l; }
void pwm_set_enabled(uint s, bool e) { (void)s; (void)e; }
uint64_t get_absolute_time(void) { return fake_ms; }
uint32_t to_ms_since_boot(uint64_t t) { return (uint32_t)t; }
bool add_repeating_timer_us(int64_t d, repeating_timer_callback_t cb,
                            void *u, struct repeating_timer *o)
{ (void)d; (void)cb; (void)u; (void)o; return true; }
uint32_t save_and_disable_interrupts(void) { return 0; }
void restore_interrupts(uint32_t s) { (void)s; }
void oled_init(void) {}
void oled_show(void) {}

/* ---- unit under test (its main renamed out of the way) ----------------- */
/* Decoder tests below script FULL quadrature cycles; pin the half-step
 * latch off (the bench default flipped to 1 in r18.14 for the KY-040-class
 * 30-detent/15-PPR part — the decoder core is identical in both modes). */
#define ENC_HALF_STEP 0
#define main hw_main
#include "display_hw_test.c"
#undef main

/* ---- helpers ------------------------------------------------------------ */
static void spin_frames(int n) {
    for (int i = 0; i < n; ++i) {
        fake_ms += 16;
        tweens_tick(fake_ms);
        anim_housekeeping(fake_ms);
        render_menu_scene();
    }
}

static int fb_nonzero(void) {
    const uint8_t *fb = oled_framebuffer();
    int nz = 0;
    for (int i = 0; i < OLED_FB_SIZE; ++i) nz += (fb[i] != 0);
    return nz;
}

/* Apply one quadrature state (cur = CLK<<1 | DT) and run the sampler. */
static void quad_step(uint8_t cur) {
    sim_pin[PIN_ENC_CLK] = (cur >> 1) & 1;
    sim_pin[PIN_ENC_DT]  = cur & 1;
    sampler_1khz(NULL);
}

int main(void) {
    for (int i = 0; i < 32; ++i) sim_pin[i] = true;

    /* ---- quadrature decoder -------------------------------------------- */
    quad_step(3);                              /* settle at rest            */
    ev_detents = 0;
    quad_step(2); quad_step(0); quad_step(1); quad_step(3);   /* CW cycle   */
    CHECK(ev_detents == ENC_DIR, "CW detent: got %d", ev_detents);
    ev_detents = 0;
    quad_step(1); quad_step(0); quad_step(2); quad_step(3);   /* CCW cycle  */
    CHECK(ev_detents == -ENC_DIR, "CCW detent: got %d", ev_detents);
    ev_detents = 0;
    /* contact bounce mid-cycle must neither lose nor double the detent */
    quad_step(1); quad_step(3); quad_step(1);  /* bounce 3↔1                */
    quad_step(0); quad_step(2); quad_step(3);
    CHECK(ev_detents == -ENC_DIR, "bounced detent: got %d", ev_detents);
    ev_detents = 0;
    /* a partial wiggle that returns to rest must count nothing */
    quad_step(1); quad_step(3);
    CHECK(ev_detents == 0, "wiggle counted: got %d", ev_detents);

    /* ---- menu + animations --------------------------------------------- */
    menu_init(NULL);
    battery_set_pct(72);
    battery_set_usb_present(false);
    anim[A_BART]  = (float)menu_current();
    anim[A_FILLT] = (float)menu_value_int(menu_current());
    render_menu_scene();
    CHECK(fb_nonzero() > 100, "initial frame empty");

    for (int i = 0; i < MP_COUNT + 2; ++i) {   /* browse with wipe running  */
        menu_rotate(1);
        on_cur_change(1, fake_ms);
        spin_frames(18);                       /* ~290 ms — settled         */
        CHECK(anim[A_TEXTALPHA] > 0.99f, "wipe not settled (i=%d)", i);
        CHECK(anim[A_BART] >= -0.01f && anim[A_BART] <= (float)MP_COUNT,
              "barT out of range: %f", (double)anim[A_BART]);
    }

    while (menu_current() != MP_SPACE) {     /* go to a continuous param  */
        menu_rotate(1); on_cur_change(1, fake_ms); spin_frames(2);
    }
    spin_frames(20);
    menu_push(); on_mode_change(fake_ms);
    spin_frames(25);                           /* fade → snap → fade-in     */
    CHECK(menu_mode() == MENU_EDIT, "not in edit");
    CHECK(!barseq.pending, "bar sequence stuck");
    CHECK(anim[A_BARALPHA] > 0.99f, "bar alpha not restored");

    int v0 = menu_value_int(MP_SPACE);
    menu_rotate(8);                            /* accelerated burst         */
    on_value_change(fake_ms);
    spin_frames(20);
    CHECK(menu_value_int(MP_SPACE) == v0 + 8, "value: %d", menu_value_int(MP_SPACE));
    CHECK((int)(anim[A_FILLT] + 0.5f) == v0 + 8, "fill lagged: %f", (double)anim[A_FILLT]);

    menu_push(); on_mode_change(fake_ms);
    spin_frames(25);
    CHECK(menu_mode() == MENU_BROWSE, "not back in browse");

    /* ---- overlay lifecycle (stuck-bug regression guard) -----------------
     * The overlay is ALWAYS transient (sim's Drive/Volume/Brightness model):
     * it must fade out by itself after the idle hold. The SHIFT-mode latch
     * lives in the main loop, NOT here — so auto-fade is correct even while
     * the mode is on. An explicit dismiss (SHIFT tap to exit, or leaving
     * the menu scene) must also tear it down early. */
    show_overlay("Backlight", 80, fake_ms);
    spin_frames(10);
    CHECK(overlay.active && anim[A_OVALPHA] > 0.9f, "overlay not shown");
    show_overlay("Backlight", 60, fake_ms);    /* retarget without restart  */
    spin_frames(15);
    CHECK((int)(anim[A_OVFILL] + 0.5f) == 60, "retarget: %f", (double)anim[A_OVFILL]);
    spin_frames(90);                           /* > idle 1100 ms + fade     */
    CHECK(!overlay.active, "overlay STUCK after idle");
    CHECK(anim[A_OVALPHA] < 0.02f, "overlay alpha stuck: %f", (double)anim[A_OVALPHA]);

    /* re-showing after a turn must re-arm the idle timer (fresh 1100 ms) */
    show_overlay("Backlight", 50, fake_ms);
    spin_frames(40);                           /* 640 ms — not expired yet  */
    show_overlay("Backlight", 55, fake_ms);    /* turn re-arms the timer    */
    spin_frames(40);                           /* 640 ms after re-arm       */
    CHECK(overlay.active, "overlay died before re-armed timeout");
    spin_frames(60);                           /* now well past the re-arm  */
    CHECK(!overlay.active, "overlay STUCK after re-armed idle");

    show_overlay("Backlight", 30, fake_ms);
    spin_frames(5);
    dismiss_overlay(fake_ms);                  /* early dismiss path        */
    spin_frames(15);
    CHECK(!overlay.active, "overlay STUCK after dismiss");

    /* ---- velocity acceleration (sim ACCEL_TIERS) ------------------------ */
    fake_ms += 1000; (void)accel_ticks(1, fake_ms);
    fake_ms += 10;  CHECK(accel_ticks(1, fake_ms)  ==  8, "tier 28");
    fake_ms += 40;  CHECK(accel_ticks(1, fake_ms)  ==  5, "tier 60");
    fake_ms += 100; CHECK(accel_ticks(1, fake_ms)  ==  3, "tier 120");
    fake_ms += 200; CHECK(accel_ticks(1, fake_ms)  ==  2, "tier 240");
    fake_ms += 500; CHECK(accel_ticks(1, fake_ms)  ==  1, "slow = fine");
    fake_ms += 10;  CHECK(accel_ticks(-1, fake_ms) == -8, "sign carried");

    /* ---- backlight gamma + floor ---------------------------------------- */
    backlight_pct = 0;   backlight_apply();
    CHECK(pwm_level[PIN_BL] == 0, "0%% must be off, duty=%u", pwm_level[PIN_BL]);
    backlight_pct = 1;   backlight_apply();
    CHECK(pwm_level[PIN_BL] >= 1, "1%% must glow, duty=%u", pwm_level[PIN_BL]);
    backlight_pct = 3;   backlight_apply();
    CHECK(pwm_level[PIN_BL] >= 1, "3%% must glow, duty=%u", pwm_level[PIN_BL]);
    backlight_pct = 100; backlight_apply();
    CHECK(pwm_level[PIN_BL] == BL_PWM_WRAP, "100%% = full, duty=%u", pwm_level[PIN_BL]);
    backlight_pct = 50;  backlight_apply();
    CHECK(pwm_level[PIN_BL] < BL_PWM_WRAP / 2, "gamma 2 expected, duty=%u",
          pwm_level[PIN_BL]);

    /* ---- test scenes ----------------------------------------------------- */
    render_ramp();    CHECK(fb_nonzero() > 100,  "RAMP empty");
    render_checker(); CHECK(fb_nonzero() > 1000, "CHECKER empty");
    render_type();    CHECK(fb_nonzero() > 100,  "TYPE empty");

    printf("== display bench (tools/display_hw_test.c) ==\n");
    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
