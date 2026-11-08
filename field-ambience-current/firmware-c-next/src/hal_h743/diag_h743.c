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
#ifdef FAM_SOUND_AMBIENT_V3
#include "diag.h"
#include "engine_product.h"
#include "tuning.h"
#include "worlds.h"
#endif

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
    const struct audio_profiler *irq = h743_audio_irq_profiler_state();
    oled_fill(0);
    oled_text_scaled(6, 6, "BRING-UP DIAG", 12, 2);

    int y = 34;
    if (pr) {
        row(y, "render% ", (uint32_t)(pr->peak_load * 100.0f + 0.5f), "");   y += 12;
        row(y, "R cyc   ", pr->max_cycles, "");                              y += 12;
        row(y, "R over  ", pr->deadline_miss_count, "");                     y += 12;
        row(y, "clips  ", pr->clip_count, "");                              y += 12;
    } else {
        oled_text(6, y, "profiler: n/a", 10); y += 12;
    }
    if (irq) {
        row(y, "IRQ C%  ", (uint32_t)(irq->peak_load * 100.0f + 0.5f), ""); y += 12;
        row(y, "I cyc   ", irq->max_cycles, ""); y += 12;
        row(y, "I over  ", irq->deadline_miss_count, ""); y += 12;
    }
    row(y, "voices ", (uint32_t)engine_active_voices(), "");                y += 12;
    row(y, "batt mV", (uint32_t)(bat_adc_read_volts() * 1000.0f + 0.5f), ""); y += 12;

    if (!psram_ran)      oled_text(6, y, "psram: (hold CELL2)", 9);
    else if (psram_pass) oled_text(6, y, "psram: PASS", 15);
    else                 oled_text(6, y, "psram: FAIL", 15);
}

#ifdef FAM_SOUND_AMBIENT_V3
/* ---- Per-World render load bench (ambient voicing) ------------------------
 * Hold CELL1 alone at power-on. Every World gets 24 s: 12 s Generate with the
 * DRONE bed, then 12 s of three Cells struck every 3 s with the bed, at Room
 * .5 and Nature .6, so its voices, bed and effects all run. The table shows
 * per World:
 *   pk   the worst render load: exact when the profiler's worst case rises
 *        inside that World (max_cycles only grows), else the sampled peak;
 *   av   the average of the renders this loop sampled;
 *   x    deadline misses (a render that overran its 11.6 ms window: a click);
 *   clp  output samples at full scale.
 * Load is render time over the block's real-time budget (512 frames at
 * 44.1 kHz); 100 % means no time left. Rounds repeat and keep the worst. */
typedef struct { float peak, sum; uint32_t n, misses, clips; } wl_stat_t;
static wl_stat_t wl_stat[WORLD_COUNT];

static char *wl_str(char *p, const char *s) { while (*s) *p++ = *s++; *p = 0; return p; }
static char *wl_pct(char *p, float load) {
    uint32_t v = load > 0 ? (uint32_t)(load * 100.0f + 0.5f) : 0u;
    if (v < 10) *p++ = ' ';
    if (v < 100) *p++ = ' ';
    p = u2s(p, v); *p++ = '%'; *p = 0; return p;
}
static void wl_strike(bool on) {
    static const uint8_t cells[3] = {0, 2, 4};
    for (int k = 0; k < 3; ++k) {
        uint8_t c = cells[k];
        if (on) (void)engine_try_note_on(c, tuning_hz((float)engine_product_cell_midi(c, false)), .6f);
        else engine_note_off(c);
    }
}
static void wl_draw(int world, int phase, uint32_t in_phase_ms, uint32_t round) {
    const struct audio_profiler *pr = audio_profiler_state();
    const struct audio_profiler *irq = h743_audio_irq_profiler_state();
    char line[48], *p;
    oled_fill(0);
    p = wl_str(line, "WORLD LOAD  round "); u2s(p, round);
    oled_text_scaled(6, 4, line, 12, 2);
    int y = 30;
    for (int w = 0; w < WORLD_COUNT; ++w, y += 12) {
        const wl_stat_t *st = &wl_stat[w];
        p = line; const char *name = worlds_get(w)->name; int len = 0;
        while (name[len] && len < 9) { *p++ = name[len]; ++len; }
        while (len++ < 10) *p++ = ' ';
        p = wl_str(p, "pk"); p = wl_pct(p, st->peak);
        p = wl_str(p, " av"); p = wl_pct(p, st->n ? st->sum / (float)st->n : 0.0f);
        p = wl_str(p, " x"); p = u2s(p, st->misses);
        p = wl_str(p, " clp"); u2s(p, st->clips);
        oled_text(6, y, line, w == world ? 15 : 10);
    }
    p = wl_str(line, "now: "); p = wl_str(p, worlds_get(world)->name);
    p = wl_str(p, phase ? "  3 cells+drone  " : "  generate+drone  ");
    p = u2s(p, in_phase_ms / 1000u); wl_str(p, "s");
    oled_text(6, y, line, 15); y += 12;
    p = wl_str(line, "all: peak"); p = wl_pct(p, pr ? pr->peak_load : 0.0f);
    p = wl_str(p, "  misses "); p = u2s(p, pr ? pr->deadline_miss_count : 0u);
    p = wl_str(p, "  IRQ"); wl_pct(p, irq ? irq->peak_load : 0.0f);
    oled_text(6, y, line, 15); y += 12;
    p = wl_str(line, "voices "); p = u2s(p, (uint32_t)engine_active_voices());
    p = wl_str(p, "  batt mV "); u2s(p, (uint32_t)(bat_adc_read_volts() * 1000.0f + 0.5f));
    oled_text(6, y, line, 10); y += 12;
    oled_text(6, y, "power-cycle to exit", 9);
}
void diag_world_load_run(void) {
    const struct audio_profiler *pr = audio_profiler_state();
    engine_set_room(.5f); engine_set_nature(.6f); engine_set_master_volume(.6f);
    const uint32_t start = HAL_GetTick();
    uint32_t next_draw = 0, next_strike = 0, round = 1;
    uint32_t seen = pr ? pr->render_count : 0u, max_seen = pr ? pr->max_cycles : 0u;
    uint32_t miss_seen = pr ? pr->deadline_miss_count : 0u, clip_seen = pr ? pr->clip_count : 0u;
    int world = -1, phase = -1;
    for (;;) {
        uint32_t now = HAL_GetTick(), t = now - start;
        int w = (int)((t / 24000u) % (uint32_t)WORLD_COUNT), ph = (int)((t % 24000u) / 12000u);
        if (w != world || ph != phase) {
            if (w != world) {
                engine_set_generative(false, -1); wl_strike(false);
                if (world == WORLD_COUNT - 1 && w == 0) ++round;
                engine_set_world(w); engine_set_drone(true);
            }
            if (ph == 0) { wl_strike(false); engine_set_generative(true, -1); }
            else { engine_set_generative(false, -1); next_strike = now; }
            world = w; phase = ph;
        }
        if (phase == 1 && (int32_t)(now - next_strike) >= 0) {
            wl_strike(false); wl_strike(true); next_strike = now + 3000u;
        }
        engine_generative_tick(now);
        if (pr) {
            wl_stat_t *st = &wl_stat[world];
            if (pr->render_count != seen) {
                seen = pr->render_count;
                float load = pr->last_load;
                st->sum += load; ++st->n;
                if (load > st->peak) st->peak = load;
            }
            if (pr->max_cycles > max_seen) {
                max_seen = pr->max_cycles;
                float exact = pr->budget_cycles ? (float)max_seen / (float)pr->budget_cycles : 0.0f;
                if (exact > st->peak) st->peak = exact;
            }
            st->misses += pr->deadline_miss_count - miss_seen; miss_seen = pr->deadline_miss_count;
            st->clips += pr->clip_count - clip_seen; clip_seen = pr->clip_count;
        }
        if ((int32_t)(now - next_draw) >= 0) {
            wl_draw(world, phase, t % 12000u, round);
            oled_show();
            next_draw = now + 300u;
        }
    }
}
#endif
