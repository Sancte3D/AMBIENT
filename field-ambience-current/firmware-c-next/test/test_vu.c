/*
 * Host test for the VU meter module (vu.c) — segment thresholds, attack,
 * wall-clock decay, peak hold, boundary interpolation, engine tap.
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "vu.h"
#include "engine.h"
#include "brain.h"
#include "dsp.h"
#include "audio.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static int lit_segments(const uint16_t pwm[VU_CH_COUNT]) {
    int n = 0;
    for (int i = 0; i < VU_CH_COUNT; ++i) if (pwm[i] > 0) ++n;
    return n;
}

static float db_to_lin(float db) { return powf(10.0f, db / 20.0f); }

int main(void) {
    printf("== vu (engine peak → 8-segment meter) ==\n");
    uint16_t pwm[VU_CH_COUNT];
    uint32_t now = 1000;

    /* ---- 1. boots dark ---- */
    vu_init();
    vu_render(pwm);
    CHECK(lit_segments(pwm) == 0, "boots dark (%d lit)", lit_segments(pwm));
    CHECK(vu_peak_segment() == -1, "no held peak at boot");

    /* ---- 2. segment thresholds at -17 dBFS: SEG_DB[3] = -18 is the highest
     *         threshold at/below -17 → segs 0..2 full, seg 3 is the boundary
     *         segment AND carries the peak dot (lifted to full), 4..7 dark */
    vu_init();
    vu_update(db_to_lin(-17.0f), now);
    vu_render(pwm);
    for (int i = 0; i < 3; ++i)
        CHECK(pwm[i] == VU_DUTY_WHITE, "seg %d full at -17 dB (%d)", i, pwm[i]);
    CHECK(vu_peak_segment() == 3, "peak segment = 3 at -17 dB (%d)", vu_peak_segment());
    CHECK(pwm[3] == VU_DUTY_WHITE, "peak dot lifts boundary seg 3 to full");
    for (int i = 4; i < VU_CH_COUNT; ++i)
        CHECK(pwm[i] == 0, "seg %d dark at -17 dB", i);

    /* ---- 2b. boundary interpolation, observable below a held dot: hit
     *          -0.2 dB (dot = seg 7), decay ~496 ms to ≈ -15 dB → seg 3 is
     *          the boundary (-18..-12) and must sit strictly between 0 and
     *          full while the dot still holds seg 7 ---- */
    vu_init(); now = 3000;
    vu_update(db_to_lin(-0.2f), now);
    for (int t = 0; t < 31; ++t) { now += 16; vu_update(0.0f, now); }
    CHECK(vu_level_db() > -18.0f && vu_level_db() < -12.0f,
          "level in seg-3 span after 496 ms (%f)", vu_level_db());
    vu_render(pwm);
    CHECK(pwm[3] > 0 && pwm[3] < VU_DUTY_WHITE,
          "seg 3 interpolated (%d)", pwm[3]);
    CHECK(pwm[7] == VU_DUTY_WHITE, "dot still holds seg 7 during decay");
    for (int i = 0; i < 3; ++i)
        CHECK(pwm[i] == VU_DUTY_WHITE, "seg %d full under bar (%d)", i, pwm[i]);
    for (int i = 4; i < 7; ++i)
        CHECK(pwm[i] == 0, "seg %d dark between bar and dot", i);

    /* ---- 3. attack is instant, decay is ~30 dB/s ---- */
    vu_init(); now = 5000;
    vu_update(db_to_lin(-6.0f), now);
    CHECK(fabsf(vu_level_db() - (-6.0f)) < 0.01f, "instant attack to -6 dB (%f)", vu_level_db());
    /* 1 second of silence at 60 Hz ticks → level should drop ~30 dB */
    for (int t = 0; t < 60; ++t) { now += 16; vu_update(0.0f, now); }
    float dropped = -6.0f - vu_level_db();
    CHECK(dropped > 25.0f && dropped < 33.0f,
          "decay ~30 dB in 960 ms (dropped %.1f)", dropped);

    /* ---- 4. floor clamp ---- */
    for (int t = 0; t < 120; ++t) { now += 16; vu_update(0.0f, now); }
    CHECK(vu_level_db() == VU_FLOOR_DB, "level clamps at floor (%f)", vu_level_db());
    vu_render(pwm);
    CHECK(lit_segments(pwm) == 0, "dark at floor");

    /* ---- 5. peak hold: top segment stays lit through decay, then falls --- */
    vu_init(); now = 9000;
    vu_update(db_to_lin(-0.2f), now);           /* everything lit, peak = seg 7 */
    vu_render(pwm);
    CHECK(vu_peak_segment() == 7, "peak = seg 7 after near-FS hit");
    CHECK(pwm[7] == VU_DUTY_WHITE, "seg 7 full");
    /* 500 ms silence: bar has decayed ~15 dB but the hold keeps seg 7 lit */
    for (int t = 0; t < 31; ++t) { now += 16; vu_update(0.0f, now); }
    vu_render(pwm);
    CHECK(vu_level_db() < -10.0f, "bar decayed below -10 dB (%f)", vu_level_db());
    CHECK(pwm[7] == VU_DUTY_WHITE, "peak dot still held at 500 ms");
    /* past VU_PEAK_HOLD_MS: the dot falls onto the live bar */
    for (int t = 0; t < 31; ++t) { now += 16; vu_update(0.0f, now); }
    vu_render(pwm);
    CHECK(vu_peak_segment() < 7, "peak released after hold (%d)", vu_peak_segment());
    CHECK(pwm[7] == 0, "seg 7 dark after hold release");

    /* ---- 6. over-FS input clamps to 0 dB, no overflow ---- */
    vu_init(); now = 20000;
    vu_update(3.0f, now);
    CHECK(vu_level_db() <= 0.0f, "over-FS clamps to 0 dB (%f)", vu_level_db());
    vu_render(pwm);
    for (int i = 0; i < VU_CH_COUNT; ++i)
        CHECK(pwm[i] <= VU_PWM_MAX, "pwm bounded seg %d", i);
    CHECK(lit_segments(pwm) == VU_CH_COUNT, "all 8 lit at FS");

    /* ---- 7. engine tap: silence → ~0 peak; a held note → peak > 0;
     *         values always in [0,1] ---- */
    dsp_init(); brain_init(); engine_init();
    static int16_t buf[AUDIO_BUFFER_FRAMES * 2];
    engine_render(buf, AUDIO_BUFFER_FRAMES);
    CHECK(engine_render_peak() >= 0.0f && engine_render_peak() <= 1.0f,
          "tap bounded on silence (%f)", engine_render_peak());
    float idle_peak = engine_render_peak();
    engine_note_on(0, 220.0f, 0.5f);
    float max_peak = 0.0f;
    for (int b = 0; b < 40; ++b) {              /* ~0.46 s — pad has attack */
        engine_render(buf, AUDIO_BUFFER_FRAMES);
        if (engine_render_peak() > max_peak) max_peak = engine_render_peak();
    }
    CHECK(max_peak > idle_peak + 0.01f,
          "held note raises tap (idle %f → %f)", idle_peak, max_peak);
    CHECK(max_peak <= 1.0f, "tap stays <= 1.0 (%f)", max_peak);

    /* ---- 8. tap drives the meter end-to-end ---- */
    vu_init(); now = 30000;
    vu_update(max_peak, now);
    vu_render(pwm);
    CHECK(lit_segments(pwm) > 0, "engine peak lights the meter");

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
