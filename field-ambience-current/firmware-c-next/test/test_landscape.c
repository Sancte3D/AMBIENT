/*
 * test_landscape.c — r19.27 Landscape cell mode (landscape.c).
 *
 * Verifies the role state machine through a mock layer interface:
 *   - Drone/Bed/Atmos: tap = momentary (on-press, off-release);
 *   - HOLD press = latch (stays on after release; second HOLD press = off);
 *   - a latched layer ignores the key-up (no premature off);
 *   - Motif = exactly one impulse per press, nothing on release;
 *   - Memory = one gesture toggle per press;
 *   - all_off releases every sounding layer and drops all latches.
 */
#include <stdio.h>
#include "landscape.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* Mock layer state. */
static int  drone_on, atmos_on, bed_on, bed_cell;
static int  motif_count, motif_last_cell;
static int  memory_count;

static void m_drone(bool on)            { drone_on = on ? 1 : 0; }
static void m_bed  (bool on, uint8_t c) { bed_on = on ? 1 : 0; bed_cell = c; }
static void m_motif(uint8_t c)          { ++motif_count; motif_last_cell = c; }
static void m_atmos(bool on)            { atmos_on = on ? 1 : 0; }
static void m_mem  (uint32_t now)       { (void)now; ++memory_count; }

static const landscape_iface_t IF = {
    m_drone, m_bed, m_motif, m_atmos, m_mem
};

int main(void) {
    printf("== landscape cell mode (r19.27) ==\n");
    landscape_init(&IF);

    /* ---- 1. Drone tap = momentary ---- */
    landscape_press(0, false, 100);
    CHECK(drone_on == 1, "drone on after tap press");
    landscape_release(0, 150);
    CHECK(drone_on == 0, "drone off after tap release");
    CHECK(!landscape_latched(0), "drone not latched after a tap");

    /* ---- 2. Drone HOLD = latch (survives release) ---- */
    landscape_press(0, true, 200);
    CHECK(drone_on == 1 && landscape_latched(0), "drone latched on");
    landscape_release(0, 250);
    CHECK(drone_on == 1, "latched drone survives key-up");
    landscape_press(0, true, 300);           /* second HOLD = off */
    CHECK(drone_on == 0 && !landscape_latched(0), "second HOLD unlatches drone");

    /* ---- 3. Bed passes its cell through ---- */
    landscape_press(1, false, 400);
    CHECK(bed_on == 1 && bed_cell == 1, "bed on with cell=1");
    landscape_release(1, 450);
    CHECK(bed_on == 0, "bed off on release");

    /* ---- 4. Motif = one impulse, nothing on release ---- */
    motif_count = 0;
    landscape_press(2, false, 500);
    landscape_release(2, 550);
    CHECK(motif_count == 1 && motif_last_cell == 2, "motif fired once (%d)", motif_count);
    landscape_press(2, true, 600);           /* HOLD must not change motif */
    CHECK(motif_count == 2, "motif ignores HOLD, still one-shot");

    /* ---- 5. Atmos latch, then all_off releases it ---- */
    landscape_press(3, true, 700);
    CHECK(atmos_on == 1 && landscape_latched(3), "atmos latched on");
    landscape_all_off(800);
    CHECK(atmos_on == 0 && !landscape_latched(3), "all_off releases atmos + latch");

    /* ---- 6. Memory toggles the gesture loop once per press ---- */
    memory_count = 0;
    landscape_press(4, false, 900);
    landscape_release(4, 950);
    CHECK(memory_count == 1, "memory toggled once (%d)", memory_count);

    /* ---- 7. a latched layer must not double-off on a stray release ---- */
    landscape_press(0, true, 1000);          /* latch drone */
    landscape_release(0, 1010);              /* stray up */
    CHECK(drone_on == 1, "latched drone stays on through stray release");
    landscape_all_off(1100);

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
