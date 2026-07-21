/*
 * test_synth_device.c — r19.16 SYNTH mode through the DEVICE path.
 *
 * Registers the real synth_host as the engine's V2 backend (exactly like
 * main_h743 does) and drives everything through the public engine API:
 * engine_set_synth / engine_note_on / engine_render. Verifies:
 *   - without a backend, set_synth(>0) is a no-op (bench/host safety),
 *   - the ambient→V2 switch has NO discontinuity (max inter-sample step
 *     across the whole crossfade stays in normal-audio range),
 *   - every V2 core is playable via a cell note-on and stays bounded,
 *   - note_off decays, switch back to ambient restores the pad path.
 */
#include "engine.h"
#include "dsp.h"
#include "v2/synth_host.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0;
#define CHECK(c) do { assert(c); ++checks; } while (0)

#define BLK 512

static void be_select  (int id)              { synth_host_select((synth_id_t)id); }
static void be_note_on (int midi, float vel) { synth_host_note_on(midi, vel); }
static void be_note_off(void)                { synth_host_note_off(); }
static void be_panic   (void)                { synth_host_panic(); }
static void be_render  (int16_t *b, int n)   { synth_host_render(b, n); }
static const engine_synth_backend_t BE = {
    be_select, be_note_on, be_note_off, be_panic, be_render
};

static int peak_of(const int16_t *b, int frames) {
    int p = 0;
    for (int i = 0; i < frames * 2; ++i) {
        int a = b[i] < 0 ? -b[i] : b[i];
        if (a > p) p = a;
    }
    return p;
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    static int16_t buf[BLK * 2];

    dsp_init();
    engine_init();
    engine_set_world(0);

    /* 1) no backend registered → set_synth(>0) must be a no-op */
    engine_set_synth(3);
    CHECK(engine_synth() == 0);

    /* register the real V2 host, exactly like the product main */
    synth_host_init();
    engine_set_synth_backend(&BE);

    /* 2) ambient plays a pad note (baseline) */
    engine_note_on(0, 261.63f, 0.8f);
    int amb_peak = 0;
    for (int i = 0; i < 20; ++i) { engine_render(buf, BLK); }
    amb_peak = peak_of(buf, BLK);
    CHECK(amb_peak > 500);
    printf("  ambient pad peak = %d\n", amb_peak);

    /* 3) switch to FM GLASS while audio runs: the joined stream across the
     * crossfade must have no hard step (a raw swap would jump >20000). */
    engine_set_synth(2);
    CHECK(engine_synth() == 2);
    int16_t prevL = buf[(BLK - 1) * 2];
    int max_step = 0;
    for (int i = 0; i < 8; ++i) {                 /* covers the 15 ms fade */
        engine_render(buf, BLK);
        int step = abs((int)buf[0] - (int)prevL);
        for (int n = 1; n < BLK; ++n) {
            int s = abs((int)buf[2*n] - (int)buf[2*(n-1)]);
            if (s > step) step = s;
        }
        if (step > max_step) max_step = step;
        prevL = buf[(BLK - 1) * 2];
    }
    printf("  switch max inter-sample step = %d\n", max_step);
    CHECK(max_step < 12000);                      /* no click/pop swap      */

    /* 4) every V2 core is playable through the CELL path and bounded */
    static const char *names[6] =
        { "Acid", "FM Glass", "Mist", "Storm", "Orbit", "Bamboo" };
    for (int core = 1; core <= 6; ++core) {
        engine_set_synth(core);
        for (int i = 0; i < 6; ++i) engine_render(buf, BLK);   /* settle fade */
        engine_note_on(1, 220.0f, 0.9f);          /* cell source 1 → V2      */
        int pk = 0;
        for (int i = 0; i < 12; ++i) {
            engine_render(buf, BLK);
            int p = peak_of(buf, BLK);
            if (p > pk) pk = p;
        }
        printf("  core %-9s peak=%5d\n", names[core-1], pk);
        CHECK(pk > 300);                          /* audible                */
        CHECK(pk <= 32767);                       /* bounded                */
        engine_note_off(1);
        for (int i = 0; i < 30; ++i) engine_render(buf, BLK);
        int tail = peak_of(buf, BLK);
        CHECK(tail < pk);                         /* decays after release   */
    }

    /* 5) back to ambient: pad path works again */
    engine_set_synth(0);
    for (int i = 0; i < 8; ++i) engine_render(buf, BLK);       /* fade out  */
    CHECK(engine_synth() == 0);
    engine_note_on(0, 261.63f, 0.8f);
    int back = 0;
    for (int i = 0; i < 20; ++i) { engine_render(buf, BLK); }
    back = peak_of(buf, BLK);
    printf("  back-to-ambient pad peak = %d\n", back);
    CHECK(back > 500);

    printf("synth_device: %d checks, 0 failures\n", checks);
    return 0;
}
