/*
 * Host test for r19.25 — the gesture loop (gesture.c):
 *   - IDLE→REC→PLAY→IDLE state cycle via gesture_toggle
 *   - record only captures while REC
 *   - playback replays the recorded cell events in order, at the right phase
 *   - the loop WRAPS and releases any voice it left held (no stuck notes)
 *   - CLEAR wipes and releases
 */
#include <stdio.h>
#include "gesture.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Capture replayed cell events. */
#define LOGN 64
static struct { uint8_t cell; uint8_t pressed; } s_log[LOGN];
static int s_logn = 0;
static void on_cell(uint8_t cell, bool pressed, uint32_t now_ms) {
    (void)now_ms;
    if (s_logn < LOGN) { s_log[s_logn].cell = cell;
                         s_log[s_logn].pressed = pressed ? 1 : 0; ++s_logn; }
}

int main(void) {
    printf("== gesture loop (r19.25) ==\n");
    gesture_init(on_cell);

    /* ---- 1. record only captures while REC ---- */
    gesture_record_cell(0, true, 500);
    CHECK(gesture_count() == 0, "no capture while IDLE");

    /* ---- 2. IDLE→REC, record a small phrase ---- */
    uint32_t t0 = 1000;
    gesture_toggle(t0);
    CHECK(gesture_state() == GESTURE_REC, "toggle IDLE→REC");
    gesture_record_cell(0, true,  1000);        /* t=0   */
    gesture_record_cell(0, false, 1200);        /* t=200 */
    gesture_record_cell(1, true,  1400);        /* t=400 */
    CHECK(gesture_count() == 3, "3 events recorded (%d)", gesture_count());

    /* ---- 3. REC→PLAY, loop length spans the recording ---- */
    gesture_toggle(1600);                        /* loop_len = 600 ms */
    CHECK(gesture_state() == GESTURE_PLAY, "toggle REC→PLAY");

    /* ---- 4. playback replays events at the right phase ---- */
    s_logn = 0;
    gesture_tick(1600);   /* phase 0   → cell0 press  */
    gesture_tick(1800);   /* phase 200 → cell0 release */
    gesture_tick(2000);   /* phase 400 → cell1 press  */
    CHECK(s_logn == 3, "first loop replayed 3 events (%d)", s_logn);
    CHECK(s_log[0].cell == 0 && s_log[0].pressed == 1, "ev0 = cell0 press");
    CHECK(s_log[1].cell == 0 && s_log[1].pressed == 0, "ev1 = cell0 release");
    CHECK(s_log[2].cell == 1 && s_log[2].pressed == 1, "ev2 = cell1 press");

    /* ---- 5. wrap releases the held voice (cell1), then restarts ---- */
    s_logn = 0;
    gesture_tick(2200);   /* phase 600 >= loop_len → wrap */
    /* the wrap must release cell1 (left held), then fire cell0 press again */
    int saw_c1_release = 0, saw_c0_press = 0;
    for (int i = 0; i < s_logn; ++i) {
        if (s_log[i].cell == 1 && s_log[i].pressed == 0) saw_c1_release = 1;
        if (s_log[i].cell == 0 && s_log[i].pressed == 1) saw_c0_press   = 1;
    }
    CHECK(saw_c1_release, "wrap released the held cell1 (no stuck note)");
    CHECK(saw_c0_press,   "loop restarted (cell0 press again)");

    /* ---- 6. toggle PLAY→IDLE clears ---- */
    gesture_toggle(2400);
    CHECK(gesture_state() == GESTURE_IDLE, "toggle PLAY→IDLE");
    CHECK(gesture_count() == 0, "cleared on stop");

    /* ---- 7. an empty recording collapses back to IDLE ---- */
    gesture_toggle(3000);                        /* REC */
    gesture_toggle(3200);                        /* nothing recorded → IDLE */
    CHECK(gesture_state() == GESTURE_IDLE, "empty REC→toggle = IDLE (nothing to loop)");

    /* ---- 8. gesture_clear stops + releases from PLAY ---- */
    gesture_toggle(4000);                        /* REC */
    gesture_record_cell(2, true, 4000);
    gesture_record_cell(2, false, 4300);
    gesture_toggle(4600);                        /* PLAY */
    s_logn = 0;
    gesture_tick(4600);                          /* cell2 press */
    gesture_clear(4700);
    CHECK(gesture_state() == GESTURE_IDLE, "clear → IDLE");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
