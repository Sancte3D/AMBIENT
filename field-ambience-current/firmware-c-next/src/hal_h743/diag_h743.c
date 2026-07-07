/*
 * diag_h743.c — first-power-on diagnostics screen (bring-up instrument, #4).
 *
 * NOT part of normal play. Entered by HOLDING CELL1 at power-on (checked once
 * in main_h743 before the menu starts) — a clean, unambiguous bench gesture.
 * It surfaces the things that are otherwise invisible without a debugger, so
 * BRING_UP stages 8/10 have live numbers:
 *   - audio render profiler: peak load %, worst-case cycles, deadline misses,
 *     clip count (the WCET/deadline evidence from REALTIME_AUDIO_RULES §1),
 *   - QSPI-PSRAM self-test PASS/FAIL (ADR-0022),
 *   - battery volts + active voice count.
 *
 * Pure-ish: it reads live state and draws text. No new hardware brought up
 * here except the (bench-pending) PSRAM. h743-only.
 */

#include "oled.h"
#include "audio.h"
#include "audio_profiler.h"
#include "psram.h"
#include "engine.h"
#include "h743_hal.h"

/* tiny unsigned → decimal (no printf in firmware). Returns end pointer. */
static char *u2s(char *p, uint32_t v) {
    char tmp[12]; int n = 0;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *p++ = tmp[--n];
    *p = 0; return p;
}
static void row(int y, const char *label, uint32_t v, const char *suffix) {
    char line[40]; char *p = line;
    while (*label) *p++ = *label++;
    p = u2s(p, v);
    while (*suffix) *p++ = *suffix++;
    *p = 0;
    oled_text(6, y, line, 15);
}

void diag_draw(int psram_pass, int psram_ran) {
    const struct audio_profiler *pr = audio_profiler_state();
    oled_fill(0);
    oled_text_scaled(6, 6, "BRING-UP DIAG", 12, 2);

    int y = 34;
    if (pr) {
        row(y, "load%  ", (uint32_t)(pr->peak_load * 100.0f + 0.5f), "");   y += 14;
        row(y, "maxcyc ", pr->max_cycles, "");                              y += 14;
        row(y, "misses ", pr->deadline_miss_count, "");                     y += 14;
        row(y, "clips  ", pr->clip_count, "");                              y += 14;
    } else {
        oled_text(6, y, "profiler: n/a", 10); y += 14;
    }
    row(y, "voices ", (uint32_t)engine_active_voices(), "");                y += 14;
    row(y, "batt mV", (uint32_t)(bat_adc_read_volts() * 1000.0f + 0.5f), ""); y += 14;

    if (!psram_ran)      oled_text(6, y, "psram: (hold CELL2)", 9);
    else if (psram_pass) oled_text(6, y, "psram: PASS", 15);
    else                 oled_text(6, y, "psram: FAIL", 15);
}
