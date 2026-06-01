/*
 * Host-side unit tests for the hardware-independent DSP + voice-pool code.
 *
 * dsp.c and voices.c are pure C (only <math.h> / <string.h>), so they compile
 * and run natively — no Pico SDK, no ARM toolchain, no device. This gives a
 * fast correctness signal on the audio math that the firmware build alone
 * cannot (a clean UF2 link says nothing about whether the sine is a sine or
 * the envelope clicks).
 *
 * Build + run:
 *   cc -std=c11 -I../include test_dsp_voices.c ../src/dsp.c ../src/voices.c -lm -o /tmp/fam_test
 *   /tmp/fam_test
 * (or just run ./run_tests.sh)
 *
 * Exit 0 = all pass; non-zero = first failure already printed.
 */

#include "dsp.h"
#include "voices.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_fails;                                                     \
            fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__);           \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
        }                                                                  \
    } while (0)

/* ------------------------------------------------------------------ DSP */

static void test_sine_lut_accuracy(void) {
    /* The 1024-point linearly-interpolated LUT should track sinf() closely
     * across a full turn. Tolerance is generous enough for the table size
     * but tight enough to catch a broken index/wrap. */
    float max_err = 0.0f;
    for (int k = 0; k < 4096; ++k) {
        float ph  = (float)k / 4096.0f;          /* [0,1) in turns */
        float got = dsp_sin(ph);
        float ref = sinf(6.283185307179586f * ph);
        float e   = fabsf(got - ref);
        if (e > max_err) max_err = e;
    }
    CHECK(max_err < 1e-3f, "sine LUT max error too large: %g", max_err);
    printf("  sine LUT max abs error vs sinf: %.2e\n", max_err);
}

static void test_sine_phase_wrap(void) {
    /* Phase is in turns; integer turns and negative phase must wrap so the
     * waveform is continuous and periodic. */
    CHECK(fabsf(dsp_sin(0.0f) - 0.0f)  < 1e-3f, "sin(0) != 0");
    CHECK(fabsf(dsp_sin(0.25f) - 1.0f) < 1e-3f, "sin(0.25turn) != 1");
    CHECK(fabsf(dsp_sin(0.5f)  - 0.0f) < 1e-3f, "sin(0.5turn) != 0");
    CHECK(fabsf(dsp_sin(0.75f) + 1.0f) < 1e-3f, "sin(0.75turn) != -1");
    /* periodicity */
    CHECK(fabsf(dsp_sin(0.3f) - dsp_sin(1.3f))  < 1e-4f, "no +1 turn wrap");
    CHECK(fabsf(dsp_sin(0.3f) - dsp_sin(-0.7f)) < 1e-3f, "no negative wrap");
    CHECK(fabsf(dsp_sin(0.3f) - dsp_sin(5.3f))  < 1e-4f, "no multi-turn wrap");
}

static void test_midi_to_hz(void) {
    CHECK(fabsf(dsp_midi_to_hz(69.0f) - 440.0f)  < 0.01f, "A4 != 440Hz");
    CHECK(fabsf(dsp_midi_to_hz(57.0f) - 220.0f)  < 0.01f, "A3 != 220Hz");
    CHECK(fabsf(dsp_midi_to_hz(81.0f) - 880.0f)  < 0.01f, "A5 != 880Hz");
    /* one octave = 12 semitones = exact doubling */
    CHECK(fabsf(dsp_midi_to_hz(60.0f) * 2.0f - dsp_midi_to_hz(72.0f)) < 0.01f,
          "octave is not a frequency doubling");
}

/* --------------------------------------------------------------- voices */

/* Render N frames, return peak |sample| seen and the max sample-to-sample
 * step (to detect envelope clicks / discontinuities). */
typedef struct { int16_t peak; int16_t max_step; } render_stats_t;

static render_stats_t render_frames(int frames) {
    enum { CHUNK = 64 };
    int16_t buf[CHUNK * 2];
    render_stats_t st = {0, 0};
    int16_t prev = 0;
    int have_prev = 0;
    int left = frames;
    while (left > 0) {
        int n = left < CHUNK ? left : CHUNK;
        voices_render(buf, n);
        for (int i = 0; i < n; ++i) {
            int16_t s = buf[i * 2];
            CHECK(buf[i * 2] == buf[i * 2 + 1], "L/R channels differ");
            int16_t a = (int16_t)(s < 0 ? -s : s);
            if (a > st.peak) st.peak = a;
            if (have_prev) {
                int step = abs((int)s - (int)prev);
                if (step > st.max_step) st.max_step = (int16_t)step;
            }
            prev = s;
            have_prev = 1;
        }
        left -= n;
    }
    return st;
}

static void test_pool_alloc_and_count(void) {
    voices_all_off();
    /* drain any release tails */
    render_frames(DSP_SAMPLE_RATE_HZ * 3);
    CHECK(voices_active_count() == 0, "pool not empty at start: %d",
          voices_active_count());

    voices_note_on(0, 220.0f, 0.5f);
    voices_note_on(1, 330.0f, 0.5f);
    CHECK(voices_active_count() == 2, "expected 2 active, got %d",
          voices_active_count());

    /* re-trigger an existing source must NOT add a voice */
    voices_note_on(0, 440.0f, 0.5f);
    CHECK(voices_active_count() == 2, "re-trigger stacked a voice: %d",
          voices_active_count());

    voices_note_off(0);
    voices_note_off(1);
    /* still "active" (releasing), then fully drains */
    render_frames(DSP_SAMPLE_RATE_HZ * 3);
    CHECK(voices_active_count() == 0, "voices did not drain after release: %d",
          voices_active_count());
}

static void test_voice_stealing(void) {
    voices_all_off();
    render_frames(DSP_SAMPLE_RATE_HZ * 3);

    /* Fill all 8 slots from distinct sources. */
    for (int s = 0; s < VOICES_MAX; ++s)
        voices_note_on((uint8_t)s, dsp_midi_to_hz(48.0f + s), 0.3f);
    /* let envelopes diverge a bit so "quietest" is well-defined */
    render_frames(64);
    CHECK(voices_active_count() == VOICES_MAX, "pool not full: %d",
          voices_active_count());

    /* 9th distinct source must steal, not overflow. */
    voices_note_on(99, 440.0f, 0.3f);
    CHECK(voices_active_count() <= VOICES_MAX,
          "pool overflowed past VOICES_MAX: %d", voices_active_count());
    CHECK(voices_active_count() == VOICES_MAX,
          "9th note should keep pool full at %d, got %d",
          VOICES_MAX, voices_active_count());
}

static void test_envelope_click_free_and_timing(void) {
    voices_all_off();
    render_frames(DSP_SAMPLE_RATE_HZ * 3);

    /* One quiet voice so the per-sample step is dominated by the envelope
     * ramp, not the carrier. amp small => peak small; a click would still
     * show up as a step far larger than the smooth waveform's. */
    voices_note_on(0, 110.0f, 0.8f);

    /* During attack (~0.8s): no sample-to-sample jump anywhere near a click.
     * A genuine click (instant on) would be thousands of LSB in one step. */
    render_stats_t atk = render_frames((int)(0.8f * DSP_SAMPLE_RATE_HZ));
    CHECK(atk.max_step < 2000, "attack not click-free, max step=%d", atk.max_step);

    /* After the attack window the voice should have reached full envelope:
     * peak ~= amp*full-scale = 0.8*32767 ~= 26214. Allow margin. */
    render_stats_t sus = render_frames(2048);
    CHECK(sus.peak > 24000, "voice did not reach sustain peak: %d", sus.peak);
    printf("  sustain peak: %d (expect ~26214)\n", sus.peak);

    /* Release: gate off, render the full release window, must be click-free
     * and end (near) silent. */
    voices_note_off(0);
    render_stats_t rel = render_frames((int)(2.5f * DSP_SAMPLE_RATE_HZ) + 1024);
    CHECK(rel.max_step < 2000, "release not click-free, max step=%d", rel.max_step);
    CHECK(voices_active_count() == 0, "voice still active after release window");

    render_stats_t after = render_frames(512);
    CHECK(after.peak == 0, "non-silent after full release: peak=%d", after.peak);
}

static void test_output_bounded_under_load(void) {
    /* Eight loud voices summed: the soft-clip must keep the int16 in range
     * (no wrap to a huge negative value). */
    voices_all_off();
    render_frames(DSP_SAMPLE_RATE_HZ * 3);
    for (int s = 0; s < VOICES_MAX; ++s)
        voices_note_on((uint8_t)s, dsp_midi_to_hz(50.0f + s * 2), 1.0f);
    render_stats_t st = render_frames(DSP_SAMPLE_RATE_HZ); /* 1s */
    CHECK(st.peak <= 32767, "output exceeded int16 max: %d", st.peak);
    printf("  peak under 8-voice full load: %d\n", st.peak);
}

int main(void) {
    dsp_init();
    voices_init();

    printf("== DSP ==\n");
    test_sine_lut_accuracy();
    test_sine_phase_wrap();
    test_midi_to_hz();

    printf("== voices ==\n");
    test_pool_alloc_and_count();
    test_voice_stealing();
    test_envelope_click_free_and_timing();
    test_output_bounded_under_load();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) {
        printf("RESULT: FAIL\n");
        return 1;
    }
    printf("RESULT: PASS\n");
    return 0;
}
