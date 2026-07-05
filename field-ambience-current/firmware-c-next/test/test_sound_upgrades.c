/*
 * Host test for the r18.89 sound-engine expansion:
 *   - dsp noise primitives: pink (1/f), dust (random impulses), crackle
 *     (logistic chaos) — bounded, reproducible, statistically sane
 *   - dsp drive shaper: transparent-ish at tiny gain, odd+even harmonics at
 *     high gain, makeup keeps small-signal level, never NaN/inf
 *   - pluck.c Karplus-Strong: pitch accuracy (zero-crossing count), ~T60
 *     decay, self-retiring voices, silent when idle
 *   - engine master DRIVE stage: 0 = transparent, 1 = bounded + louder
 *     harmonics but no runaway (makeup)
 *   - tape vinyl crackle: off by default, audible at age=1, bounded
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "dsp.h"
#include "pluck.h"
#include "tape.h"
#include "reverb.h"
#include "engine.h"
#include "brain.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define SR 44100

int main(void) {
    printf("== r18.89 sound upgrades (noise/drive/pluck/crackle) ==\n");
    dsp_init();

    /* ---- 1. pink noise ---- */
    {
        dsp_pink_t p; dsp_pink_seed(&p, 0xABCD1234u);
        double sum = 0, sumsq = 0, dsumsq = 0; float prev = 0, peak = 0;
        for (int i = 0; i < SR * 10; ++i) {
            float y = dsp_pink(&p);
            float a = fabsf(y);
            if (a > peak) peak = a;
            sum += y; sumsq += (double)y * y;
            float d = y - prev; dsumsq += (double)d * d; prev = y;
            CHECK(isfinite(y), "pink finite");
            if (fails) break;
        }
        double mean = sum / (SR * 10.0);
        double rms  = sqrt(sumsq / (SR * 10.0));
        double drms = sqrt(dsumsq / (SR * 10.0));
        CHECK(fabs(mean) < 0.02, "pink ~zero-mean (%f)", mean);
        CHECK(peak < 1.5f, "pink bounded (peak %f)", peak);
        CHECK(rms > 0.05 && rms < 0.5, "pink has body (rms %f)", rms);
        /* 1/f slope proxy: successive-difference RMS ≪ signal RMS (white
         * would have drms ≈ rms·√2; pink concentrates energy low). */
        CHECK(drms < rms * 0.9, "pink is low-heavy vs white (drms/rms %.2f)",
              drms / rms);
    }

    /* ---- 2. dust ---- */
    {
        dsp_dust_t d; dsp_dust_seed(&d, 0x1337BEEFu);
        dsp_dust_set_density(&d, 8.0f);
        int hits = 0;
        for (int i = 0; i < SR * 20; ++i) {
            float y = dsp_dust(&d);
            CHECK(y >= 0.0f && y <= 1.0f, "dust in [0,1] (%f)", y);
            if (y > 0.0f) ++hits;
            if (fails > 5) break;
        }
        /* 8/s over 20 s = 160 expected; LCG variance is small at n=160 */
        CHECK(hits > 80 && hits < 320, "dust density ~8/s (got %d in 20 s)", hits);
        dsp_dust_set_density(&d, 0.0f);
        int quiet = 1;
        for (int i = 0; i < SR; ++i) if (dsp_dust(&d) != 0.0f) quiet = 0;
        CHECK(quiet, "dust density 0 = silence");
    }

    /* ---- 3. crackle ---- */
    {
        dsp_crackle_t c; dsp_crackle_init(&c, 1.85f);
        float peak = 0; int nonzero = 0;
        for (int i = 0; i < SR * 5; ++i) {
            float y = dsp_crackle(&c);
            CHECK(isfinite(y), "crackle finite");
            float a = fabsf(y);
            if (a > peak) peak = a;
            if (a > 1e-6f) ++nonzero;
            if (fails > 5) break;
        }
        CHECK(peak <= 2.0f, "crackle bounded (peak %f)", peak);
        CHECK(nonzero > SR, "crackle actually crackles (%d nonzero)", nonzero);
    }

    /* ---- 4. drive shaper ---- */
    {
        /* tiny gain ≈ transparent */
        float e = 0;
        for (float x = -0.8f; x <= 0.8f; x += 0.05f) {
            float y = dsp_drive_shape(x, 1.0f, 0.0f) * dsp_drive_makeup(1.0f, 0.0f);
            float d = fabsf(y - tanhf(x));
            if (d > e) e = d;
        }
        CHECK(e < 1e-6f, "g=1,bias=0 == plain tanh (err %g)", e);
        /* silence stays silence even with bias */
        CHECK(fabsf(dsp_drive_shape(0.0f, 4.5f, 0.3f)) < 1e-7f,
              "drive(0) == 0 (bias re-centred)");
        /* makeup keeps small-signal level ~unity */
        float y = dsp_drive_shape(0.01f, 4.5f, 0.28f) * dsp_drive_makeup(4.5f, 0.28f);
        CHECK(fabsf(y / 0.01f - 1.0f) < 0.05f,
              "small-signal ≈ unity with makeup (gain %f)", y / 0.01f);
        /* hard input never exceeds tanh bounds */
        float m = dsp_drive_shape(10.0f, 4.5f, 0.28f);
        CHECK(m <= 2.0f && isfinite(m), "drive bounded on abuse (%f)", m);
    }

    /* ---- 5. pluck: pitch + decay + retire ---- */
    {
        pluck_init();
        CHECK(pluck_active_count() == 0, "plucks idle at boot");
        enum { N = 256 };
        float dl[N], dr[N], sl[N], sr[N];
        /* silence when idle */
        for (int i = 0; i < N; ++i) dl[i] = dr[i] = sl[i] = sr[i] = 0;
        pluck_render_mix(dl, dr, sl, sr, N);
        float acc = 0; for (int i = 0; i < N; ++i) acc += fabsf(dl[i]) + fabsf(dr[i]);
        CHECK(acc == 0.0f, "idle plucks add nothing");

        const float F = 440.0f;
        pluck_note(F, 0.5f);
        CHECK(pluck_active_count() == 1, "one voice after note");
        /* Capture 0.6 s; pitch-check via AUTOCORRELATION at the expected
         * period (a KS string is harmonic-rich, so zero-crossing counting
         * measures the spectral centroid, not the fundamental — every
         * partial IS periodic in N, so r(N)/r(0) must be high, and a
         * detuned loop would not correlate at this exact lag). */
        enum { CAP = 26460 };                        /* 0.6 s */
        static float cap[CAP];
        int w = 0; float pk1 = 0;
        while (w < CAP) {
            for (int i = 0; i < N; ++i) dl[i] = dr[i] = sl[i] = sr[i] = 0;
            pluck_render_mix(dl, dr, sl, sr, N);
            for (int i = 0; i < N && w < CAP; ++i) {
                cap[w++] = dl[i];
                float a = fabsf(dl[i]); if (a > pk1) pk1 = a;
            }
        }
        CHECK(pk1 > 0.05f, "pluck audible (peak %f)", pk1);
        {
            int   start = SR / 8;                    /* skip the burst attack */
            int   lag   = (int)(SR / F + 0.5f);      /* 100 samples */
            double r0 = 0, rl = 0, rh = 0;           /* rh = half-period lag */
            for (int t = start; t < CAP - lag; ++t) {
                r0 += (double)cap[t] * cap[t];
                rl += (double)cap[t] * cap[t + lag];
                rh += (double)cap[t] * cap[t + lag / 2];
            }
            CHECK(r0 > 0, "signal energy present");
            CHECK(rl / r0 > 0.6, "periodic at SR/440 (r=%.2f) — pitch correct",
                  rl / r0);
            CHECK(rl > rh, "fundamental period dominates the half-period");
        }

        /* decay: after ~2×T60 the voice must have retired itself */
        for (int b = 0; b < (SR * 7) / N; ++b) {
            for (int i = 0; i < N; ++i) dl[i] = dr[i] = sl[i] = sr[i] = 0;
            pluck_render_mix(dl, dr, sl, sr, N);
        }
        CHECK(pluck_active_count() == 0, "pluck retired after the ring (%d)",
              pluck_active_count());
    }

    /* ---- 6. engine master drive: transparent at 0, bounded at 1 ---- */
    {
        brain_init(); engine_init();
        int16_t buf[512];
        engine_note_on(0, dsp_midi_to_hz(57.0f), 0.25f);
        /* settle attack */
        for (int b = 0; b < 500; ++b) engine_render(buf, 256);
        long p0 = 0;
        for (int b = 0; b < 200; ++b) {
            engine_render(buf, 256);
            for (int i = 0; i < 512; ++i) { long a = buf[i] < 0 ? -buf[i] : buf[i]; if (a > p0) p0 = a; }
        }
        engine_set_drive(1.0f);
        for (int b = 0; b < 200; ++b) engine_render(buf, 256);   /* smooth in */
        long p1 = 0;
        for (int b = 0; b < 200; ++b) {
            engine_render(buf, 256);
            for (int i = 0; i < 512; ++i) { long a = buf[i] < 0 ? -buf[i] : buf[i]; if (a > p1) p1 = a; }
        }
        CHECK(p0 > 500, "reference tone audible (peak %ld)", p0);
        CHECK(p1 <= 32767 && p1 > 200, "full drive bounded + alive (peak %ld)", p1);
        /* makeup: full drive must not be a >4x jump or a collapse */
        CHECK(p1 < p0 * 4 && p1 > p0 / 4,
              "drive is colour, not a volume switch (%ld -> %ld)", p0, p1);
    }

    /* ---- 7. tape vinyl crackle ---- */
    {
        tape_init();
        enum { N = 256 };
        float L[N], R[N];
        for (int i = 0; i < N; ++i) L[i] = R[i] = 0;
        tape_crackle_render_add(L, R, N);
        float acc = 0; for (int i = 0; i < N; ++i) acc += fabsf(L[i]);
        CHECK(acc == 0.0f, "crackle off by default");
        tape_set_crackle(1.0f);
        float pk = 0; int nz = 0;
        for (int b = 0; b < (SR * 10) / N; ++b) {
            for (int i = 0; i < N; ++i) L[i] = R[i] = 0;
            tape_crackle_render_add(L, R, N);
            for (int i = 0; i < N; ++i) {
                float a = fabsf(L[i]);
                if (a > pk) pk = a;
                if (a > 1e-5f) ++nz;
                CHECK(isfinite(L[i]) && isfinite(R[i]), "crackle finite");
                if (fails > 20) break;
            }
        }
        CHECK(pk > 0.001f && pk < 0.30f,
              "age=1 crackle present but quiet (peak %f)", pk);
        CHECK(nz > SR / 10, "ticks ring through the resonator (%d nz)", nz);
    }

    /* ---- 8. r18.91 coherence audit: registers + hall ----
     * "Alles muss aufeinander abgestimmt sein": (a) the bass sub layer
     * (lowest/4) must stay ABOVE the master DC blocker corner (~35 Hz →
     * floor 28 Hz with margin) for EVERY world x degree x vibe — no
     * energy wasted below the system's own highpass, no forbidden
     * sub-bass mud; (b) melody register (56..96, +12 played) must stay
     * above the pluck delay-line floor; (c) the HALL tail must grow
     * MONOTONICALLY with SPACE and never blow up. */
    {
        for (int w = 0; w < 4; ++w) {
            engine_init(); engine_set_world(w);
            for (int vibe = 0; vibe < 4; ++vibe) {
                brain_set_vibe(vibe);
                for (int deg = 1; deg <= 7; ++deg) {
                    int root = brain_cell_root(deg - 1);
                    float sub = dsp_midi_to_hz((float)root) * 0.25f;
                    CHECK(sub >= 28.0f,
                          "sub floor: world %d vibe %d deg %d root %d -> %.1f Hz",
                          w, vibe, deg, root, sub);
                    CHECK(root + 12 >= 56 && dsp_midi_to_hz(56.0f) > PLUCK_MIN_HZ,
                          "melody register above pluck floor (root %d)", root);
                }
            }
        }
        /* hall monotonicity: burst -> tail RMS at 2 s for three sizes */
        double t2[3]; float sizes[3] = { 0.3f, 0.6f, 0.9f };
        for (int k = 0; k < 3; ++k) {
            reverb_init(); reverb_set(sizes[k], 0.3f); reverb_set_drive(0.15f);
            enum { N = 256 };
            static float il[N], ir[N], ol[N], or_[N];
            for (int b = 0; b < 200; ++b) {           /* settle smoothing */
                for (int i = 0; i < N; ++i) il[i] = ir[i] = 0;
                reverb_render(il, ir, ol, or_, N);
            }
            unsigned r = 777;
            double acc = 0; long cnt = 0;
            for (int b = 0; b < (SR * 4) / N; ++b) {
                for (int i = 0; i < N; ++i) {
                    if (b * N + i < SR / 2) {
                        r = r * 1664525u + 1013904223u;
                        il[i] = ((int)r) / 2147483648.0f * 0.3f;
                        r = r * 1664525u + 1013904223u;
                        ir[i] = ((int)r) / 2147483648.0f * 0.3f;
                    } else il[i] = ir[i] = 0;
                }
                reverb_render(il, ir, ol, or_, N);
                int t0 = b * N;
                for (int i = 0; i < N; ++i) {
                    int t = t0 + i;
                    CHECK(isfinite(ol[i]) && fabsf(ol[i]) < 4.0f, "hall bounded");
                    if (t >= 2 * SR && t < 3 * SR) {
                        acc += (double)ol[i] * ol[i] + (double)or_[i] * or_[i];
                        ++cnt;
                    }
                }
                if (fails > 40) break;
            }
            t2[k] = sqrt(acc / (double)(cnt ? cnt : 1));
        }
        CHECK(t2[0] < t2[1] && t2[1] < t2[2],
              "hall tail grows with SPACE (%.5f %.5f %.5f)", t2[0], t2[1], t2[2]);
        CHECK(t2[2] > 1e-4, "big hall actually sustains at 2 s (%.6f)", t2[2]);
    }

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
