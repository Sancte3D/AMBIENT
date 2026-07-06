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
#include "glass.h"
#include "shimmer.h"
#include "tape.h"
#include "reverb.h"
#include "padsynth.h"
#include "body.h"
#include "engine.h"
#include "brain.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

#define SR 44100
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

    /* ---- 9. r18.92 noise-floor regression (user: "Grundrauschen zu
     * praesent") ---- idle floor with engine defaults must stay a whisper
     * (hiss ducks in silence), and even the full demo macro setting
     * (texture 0.2 + age 0.45 + atmos 0.35) must stay under -44 dBFS. */
    {
        int16_t b[512];
        double acc; long cnt;
        engine_init();
        for (int i = 0; i < 300; ++i) engine_render(b, 256);
        acc = 0; cnt = 0;
        for (int i = 0; i < 400; ++i) {
            engine_render(b, 256);
            for (int k = 0; k < 512; ++k) { acc += (double)b[k] * b[k]; ++cnt; }
        }
        double floor_db = 20.0 * log10(sqrt(acc / cnt) / 32767.0 + 1e-12);
        CHECK(floor_db < -60.0, "idle default floor is a whisper (%.1f dBFS)",
              floor_db);
        engine_set_texture(0.20f); engine_set_age(0.45f);
        engine_set_atmosphere(0.35f);
        for (int i = 0; i < 300; ++i) engine_render(b, 256);
        acc = 0; cnt = 0;
        for (int i = 0; i < 400; ++i) {
            engine_render(b, 256);
            for (int k = 0; k < 512; ++k) { acc += (double)b[k] * b[k]; ++cnt; }
        }
        floor_db = 20.0 * log10(sqrt(acc / cnt) / 32767.0 + 1e-12);
        CHECK(floor_db < -44.0 && floor_db > -70.0,
              "demo-macro floor sits behind the music (%.1f dBFS)", floor_db);
    }

    /* ---- 10. r18.93 PADsynth bed table ----
     * (a) the table is deterministic, finite, non-silent, normalized;
     * (b) it is PERIODIC by construction — the seam between the last and
     *     first sample must look like any other adjacent-sample step;
     * (c) the energy sits AT the harmonics of PADSYNTH_F0: Goertzel power
     *     on-harmonic must dominate between-harmonics by an order of
     *     magnitude (that's the whole point of the spectral model). */
    {
        padsynth_build(0, 0);
        CHECK(padsynth_ready(), "table built");
        const float *t = padsynth_table();
        double acc = 0; float pk = 0;
        float max_step = 0;
        for (int i = 0; i < PADSYNTH_N; ++i) {
            CHECK(isfinite(t[i]), "table finite");
            acc += (double)t[i] * t[i];
            float a = fabsf(t[i]); if (a > pk) pk = a;
            float d = fabsf(t[(i + 1) & (PADSYNTH_N - 1)] - t[i]);
            if (d > max_step) max_step = d;
            if (fails > 5) break;
        }
        float rms = (float)sqrt(acc / PADSYNTH_N);
        CHECK(fabsf(rms - 0.22f) < 0.01f, "normalized rms (%f)", rms);
        CHECK(pk < 1.5f, "table bounded (peak %f)", pk);
        float seam = fabsf(t[0] - t[PADSYNTH_N - 1]);
        CHECK(seam <= max_step + 1e-6f,
              "loop seam is an ordinary step (%g vs max %g)", seam, max_step);

        /* Goertzel power at harmonic k of the table fundamental
         * (bin = k * F0 / (SR/N)) vs halfway between harmonics. */
        double on = 0, off = 0;
        for (int k = 1; k <= 6; ++k) {
            double won = 2.0 * M_PI * (PADSYNTH_F0 * k) / 44100.0;
            double wof = 2.0 * M_PI * (PADSYNTH_F0 * (k + 0.5f)) / 44100.0;
            double s0 = 0, s1 = 0, q0 = 0, q1 = 0;
            for (int i = 0; i < PADSYNTH_N; ++i) {
                double v0 = t[i] + 2.0 * cos(won) * s0 - s1;
                s1 = s0; s0 = v0;
                double v1 = t[i] + 2.0 * cos(wof) * q0 - q1;
                q1 = q0; q0 = v1;
            }
            on  += s0 * s0 + s1 * s1 - 2.0 * cos(won) * s0 * s1;
            off += q0 * q0 + q1 * q1 - 2.0 * cos(wof) * q0 * q1;
        }
        CHECK(on > off * 10.0,
              "energy sits on the harmonics (on/off %.1f)", on / (off + 1e-9));

        /* worlds differ: same seed, different profile → different tables */
        float t0_probe = t[1234];
        padsynth_build(2, 0);
        CHECK(fabsf(padsynth_table()[1234] - t0_probe) > 1e-9f,
              "world profiles produce distinct tables");
        padsynth_build(0, 0);
        CHECK(fabsf(padsynth_table()[1234] - t0_probe) < 1e-9f,
              "same world+seed reproduces bit-identically");
    }

    /* ---- 11. r18.94 modal body ----
     * (a) amount 0 = bit-exact bypass; (b) an impulse RINGS: the body adds
     * a decaying tail where the dry input has none; (c) the ring sits AT
     * the material's mode frequencies (Goertzel on-mode vs between-modes);
     * (d) worlds are distinct materials; (e) noise abuse stays bounded. */
    {
        enum { BN = 256 };
        float L[BN], R[BN];

        body_init(); body_set_amount(0.0f);
        for (int i = 0; i < BN; ++i) { L[i] = (i == 0) ? 1.0f : 0.0f; R[i] = L[i]; }
        body_process(L, R, BN);
        CHECK(L[0] == 1.0f && L[1] == 0.0f && L[100] == 0.0f,
              "amount 0 is a bit-exact bypass");

        /* impulse response capture, world 0, default wet */
        enum { IRN = 44100 };
        static float ir[IRN];
        body_init();
        for (int b = 0; b < IRN / BN; ++b) {
            for (int i = 0; i < BN; ++i) { L[i] = (b == 0 && i == 0) ? 1.0f : 0.0f; R[i] = L[i]; }
            body_process(L, R, BN);
            for (int i = 0; i < BN; ++i) ir[b * BN + i] = L[i];
        }
        double e_early = 0, e_late = 0;
        for (int i = SR * 3 / 100; i < SR * 30 / 100; ++i) e_early += (double)ir[i] * ir[i];
        for (int i = SR * 30 / 100; i < SR * 60 / 100; ++i) e_late  += (double)ir[i] * ir[i];
        CHECK(e_early > 1e-7, "body rings after the impulse (%.3g)", e_early);
        CHECK(e_late < e_early, "ring decays (%.3g -> %.3g)", e_early, e_late);
        int finite_ok = 1;
        for (int i = 0; i < IRN; ++i) if (!isfinite(ir[i])) finite_ok = 0;
        CHECK(finite_ok, "impulse response finite");

        /* Goertzel: world-0 wood mode at 180 Hz vs between-modes 220 Hz */
        {
            double won = 2.0 * M_PI * 180.0 / 44100.0;
            double wof = 2.0 * M_PI * 220.0 / 44100.0;
            double s0 = 0, s1 = 0, q0 = 0, q1 = 0;
            /* skip the dry impulse itself (flat spectrum — it feeds both
             * bins equally and washes the ratio out); measure the RING. */
            for (int i = 200; i < IRN; ++i) {
                double v0 = ir[i] + 2.0 * cos(won) * s0 - s1; s1 = s0; s0 = v0;
                double v1 = ir[i] + 2.0 * cos(wof) * q0 - q1; q1 = q0; q0 = v1;
            }
            double on  = s0 * s0 + s1 * s1 - 2.0 * cos(won) * s0 * s1;
            double off = q0 * q0 + q1 * q1 - 2.0 * cos(wof) * q0 * q1;
            CHECK(on > off * 5.0, "ring sits on the wood modes (on/off %.1f)",
                  on / (off + 1e-12));
        }

        /* worlds are distinct materials: compare early IR samples */
        static float ir1[2048];
        body_init(); body_set_world(1);
        for (int b = 0; b < 8; ++b) {
            for (int i = 0; i < BN; ++i) { L[i] = (b == 0 && i == 0) ? 1.0f : 0.0f; R[i] = L[i]; }
            body_process(L, R, BN);
            for (int i = 0; i < BN; ++i) ir1[b * BN + i] = L[i];
        }
        double dsum = 0;
        for (int i = 0; i < 2048; ++i) dsum += fabs((double)ir1[i] - ir[i]);
        CHECK(dsum > 1e-3, "materials differ across worlds (%g)", dsum);

        /* stability under noise abuse */
        body_init();
        unsigned rr = 99; float pk = 0;
        for (int b = 0; b < (SR * 5) / BN; ++b) {
            for (int i = 0; i < BN; ++i) {
                rr = rr * 1664525u + 1013904223u;
                L[i] = R[i] = ((int)rr) / 2147483648.0f;
            }
            body_process(L, R, BN);
            for (int i = 0; i < BN; ++i) {
                float a = fabsf(L[i]); if (a > pk) pk = a;
            }
        }
        CHECK(isfinite(pk) && pk < 8.0f, "bounded under 5 s full-scale noise (%f)", pk);
    }

    /* ---- 12. r18.98 FM glass voice + engine VOICE dispatch + KEY pc ----
     * (a) a strike sounds, is bounded, and SELF-DECAYS to silence;
     * (b) the FM spectrum is INHARMONIC while the strike is bright: the
     *     first upper sideband (1 + ratio-1 = 2.5307 f) carries real energy
     *     in the first 200 ms and nearly none in the late ring (the index
     *     envelope is the strike — that asymmetry IS the glass);
     * (c) engine_set_voice routes cell presses: 2 → glass, 1 → string,
     *     0 → neither (reference sound untouched);
     * (d) engine_set_key_pc anchors pitch classes into MIDI 54..65 and
     *     round-trips all four world tonics. */
    {
        enum { GN = 512 };
        static float gL[GN], gR[GN], gsL[GN], gsR[GN];
        static float early[8820], late[8820];   /* 200 ms windows */
        glass_init();
        glass_note(220.0f, 0.25f);
        CHECK(glass_active_count() == 1, "glass voice active after strike");
        float pk = 0; long n_early = 0, n_late = 0; long pos = 0;
        for (int b = 0; b < (5 * 44100) / GN; ++b) {
            for (int i = 0; i < GN; ++i) gL[i] = gR[i] = gsL[i] = gsR[i] = 0.0f;
            glass_render_mix(gL, gR, gsL, gsR, GN);
            for (int i = 0; i < GN; ++i) {
                float a = fabsf(gL[i]); if (a > pk) pk = a;
                if (pos < 8820)                          early[n_early++] = gL[i];
                if (pos >= 44100 && pos < 44100 + 8820)  late [n_late++]  = gL[i];
                ++pos;
            }
        }
        CHECK(pk > 0.02f && pk < 1.0f, "glass strike audible + bounded (%f)", pk);
        CHECK(glass_active_count() == 0, "glass self-decayed inside 5 s (%d)",
              glass_active_count());

        /* Goertzel at f and at the 2.5307 f sideband, early vs late */
        double e_f = 0, e_sb = 0, l_f = 0, l_sb = 0;
        {
            double wf  = 2.0 * M_PI * 220.0 / 44100.0;
            double wsb = 2.0 * M_PI * (220.0 * 2.5307) / 44100.0;
            double s0, s1, v;
            #define GOE(buf, cnt, w, out) do { s0 = s1 = 0; \
                for (long i = 0; i < (cnt); ++i) { \
                    v = (buf)[i] + 2.0 * cos(w) * s0 - s1; s1 = s0; s0 = v; } \
                out = s0 * s0 + s1 * s1 - 2.0 * cos(w) * s0 * s1; } while (0)
            GOE(early, n_early, wf,  e_f);
            GOE(early, n_early, wsb, e_sb);
            GOE(late,  n_late,  wf,  l_f);
            GOE(late,  n_late,  wsb, l_sb);
            #undef GOE
        }
        CHECK(e_sb > e_f * 0.15,
              "strike is inharmonic-bright (sb/f %.3f)", e_sb / (e_f + 1e-12));
        CHECK(l_sb < l_f * 0.05,
              "ring is nearly pure (sb/f %.4f)", l_sb / (l_f + 1e-12));

        /* (c) engine dispatch */
        engine_init();
        engine_set_voice(2);
        engine_note_on(0, 220.0f, 0.12f);
        CHECK(glass_active_count() > 0, "VOICE=GLASS strikes on a cell press");
        CHECK(pluck_active_count() == 0, "no string when GLASS is chosen");
        engine_note_off(0);
        { int16_t b[512]; for (int i = 0; i < 1200; ++i) engine_render(b, 256); }
        engine_set_voice(1);
        engine_note_on(1, 330.0f, 0.12f);
        CHECK(pluck_active_count() > 0, "VOICE=STRING strikes on a cell press");
        engine_note_off(1);
        { int16_t b[512]; for (int i = 0; i < 1200; ++i) engine_render(b, 256); }
        engine_set_voice(0);
        int g0 = glass_active_count(), p0 = pluck_active_count();
        engine_note_on(2, 440.0f, 0.12f);
        CHECK(glass_active_count() == g0 && pluck_active_count() == p0,
              "VOICE=PAD leaves the reference sound untouched");
        engine_note_off(2);

        /* (d) KEY pitch-class anchor: all four world tonics round-trip */
        engine_set_key_pc(9);  CHECK(brain_get_key() == 57, "pc 9 -> A3 57 (%d)",  brain_get_key());
        engine_set_key_pc(2);  CHECK(brain_get_key() == 62, "pc 2 -> D4 62 (%d)",  brain_get_key());
        engine_set_key_pc(6);  CHECK(brain_get_key() == 54, "pc 6 -> F#3 54 (%d)", brain_get_key());
        engine_set_key_pc(0);  CHECK(brain_get_key() == 60, "pc 0 -> C4 60 (%d)",  brain_get_key());
        engine_set_key_pc(-3); CHECK(brain_get_key() == 57, "pc wraps below (%d)", brain_get_key());
    }

    /* ---- 13. r18.99 SHIMMER + WOW/FLUTTER + ENO LOOPS ----
     * (a) shimmer module: feed a pure 220 Hz sine, the return must be
     *     dominated by 440 Hz (one octave up) — Goertzel 440 vs 220;
     *     amount 0 adds NOTHING (bit-exact bypass);
     * (b) tape wow: depth 0 = bit-exact pass-through; depth 1 modulates
     *     the pitch of a 1 kHz sine (zero-crossing period variance > 0)
     *     while staying bounded;
     * (c) Eno loops: in autoplay the pad pool grows beyond the single bed
     *     voice (loops join one by one) and never exceeds bed + 3;
     *     disabling generative releases them all. */
    {
        enum { SB = 256 };
        static float sL[SB], sR[SB], oL[SB], oR[SB];
        static float ret[44100];
        shimmer_init();
        shimmer_set_amount(1.0f);
        long pos = 0; float phase = 0.0f;
        for (int b = 0; b < 44100 / SB; ++b) {
            for (int i = 0; i < SB; ++i) {
                sL[i] = sR[i] = sinf(2.0f * (float)M_PI * phase) * 0.3f;
                phase += 220.0f / 44100.0f; if (phase >= 1.0f) phase -= 1.0f;
                oL[i] = oR[i] = 0.0f;
            }
            shimmer_feed_add(oL, oR, SB);       /* read previous content */
            shimmer_capture(sL, sR, SB);        /* then feed the sine    */
            for (int i = 0; i < SB && pos < 44100; ++i) ret[pos++] = oL[i];
        }
        double e220 = 0, e440 = 0;
        {
            double w220 = 2.0 * M_PI * 220.0 / 44100.0;
            double w440 = 2.0 * M_PI * 440.0 / 44100.0;
            double s0 = 0, s1 = 0, v;
            for (long i = 22050; i < 44100; ++i) {           /* settled half */
                v = ret[i] + 2.0 * cos(w220) * s0 - s1; s1 = s0; s0 = v; }
            e220 = s0 * s0 + s1 * s1 - 2.0 * cos(w220) * s0 * s1;
            s0 = s1 = 0;
            for (long i = 22050; i < 44100; ++i) {
                v = ret[i] + 2.0 * cos(w440) * s0 - s1; s1 = s0; s0 = v; }
            e440 = s0 * s0 + s1 * s1 - 2.0 * cos(w440) * s0 * s1;
        }
        CHECK(e440 > e220 * 3.0,
              "shimmer return is the octave UP (440/220 = %.2f)",
              e440 / (e220 + 1e-12));

        shimmer_init();
        shimmer_set_amount(0.0f);
        for (int i = 0; i < SB; ++i) { oL[i] = 0.123f; oR[i] = -0.5f; }
        shimmer_feed_add(oL, oR, SB);
        CHECK(oL[0] == 0.123f && oR[7] == -0.5f, "shimmer 0 is bit-exact off");

        /* (b) wow */
        tape_init();
        tape_set_wow_depth(0.0f);
        static float wL[8192], wR[8192], refL[8192];
        float ph2 = 0.0f;
        for (int i = 0; i < 8192; ++i) {
            wL[i] = wR[i] = sinf(2.0f * (float)M_PI * ph2) * 0.4f;
            refL[i] = wL[i];
            ph2 += 1000.0f / 44100.0f; if (ph2 >= 1.0f) ph2 -= 1.0f;
        }
        tape_wow_process(wL, wR, 8192);
        int same = 1;
        for (int i = 0; i < 8192; ++i) if (wL[i] != refL[i]) { same = 0; break; }
        CHECK(same, "wow depth 0 is bit-exact pass-through");

        tape_init();
        tape_set_wow_depth(1.0f);
        /* run ~3 s of sine through in blocks, collect zero-cross periods */
        double periods[4096]; int np = 0;
        float prev = 0.0f; long idx = 0; long last_up = -1;
        ph2 = 0.0f;
        float pkw = 0.0f;
        for (int b = 0; b < (3 * 44100) / SB; ++b) {
            for (int i = 0; i < SB; ++i) {
                wL[i] = wR[i] = sinf(2.0f * (float)M_PI * ph2) * 0.4f;
                ph2 += 1000.0f / 44100.0f; if (ph2 >= 1.0f) ph2 -= 1.0f;
            }
            tape_wow_process(wL, wR, SB);
            for (int i = 0; i < SB; ++i) {
                float a = fabsf(wL[i]); if (a > pkw) pkw = a;
                if (prev <= 0.0f && wL[i] > 0.0f && idx > 44100 / 2) {
                    if (last_up >= 0 && np < 4096)
                        periods[np++] = (double)(idx - last_up);
                    last_up = idx;
                }
                prev = wL[i]; ++idx;
            }
        }
        CHECK(pkw < 0.5f, "wow bounded (peak %f)", pkw);
        double mean = 0; for (int i = 0; i < np; ++i) mean += periods[i];
        mean /= (np > 0 ? np : 1);
        double var = 0; for (int i = 0; i < np; ++i)
            var += (periods[i] - mean) * (periods[i] - mean);
        var /= (np > 0 ? np : 1);
        CHECK(np > 1000 && var > 0.01,
              "wow modulates pitch (n=%d, period var %.4f)", np, var);

        /* (c) Eno loops join the bed */
        engine_init();
        engine_set_generative(true, -1);
        uint32_t t = 42000;
        engine_generative_tick(t);
        int16_t eb[512];
        int maxv = 0;
        for (int step = 0; step < 4000; ++step) {            /* 64 s */
            t += 16;
            engine_generative_tick(t);
            int v = engine_active_voices();
            if (v > maxv) maxv = v;
            if ((step & 255) == 0)
                for (int k = 0; k < 30; ++k) engine_render(eb, 256);
        }
        CHECK(maxv >= 2, "Eno loops joined the bed (max voices %d)", maxv);
        CHECK(maxv <= 4, "never more than bed + 3 loops (max %d)", maxv);
        engine_set_generative(false, -1);
        for (int k = 0; k < 2500; ++k) engine_render(eb, 256);   /* ~14.5 s */
        CHECK(engine_active_voices() == 0,
              "loops released on disable (%d)", engine_active_voices());
    }

    printf("%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
