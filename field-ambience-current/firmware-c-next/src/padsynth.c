/*
 * padsynth.c — spectral bed wavetable. See padsynth.h for the model notes.
 */

#include "padsynth.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR      ((float)DSP_SAMPLE_RATE_HZ)
#define N       PADSYNTH_N
#define NH      24                       /* harmonics drawn into the table */

/* Table + complex FFT scratch. Both land in this module's .bss → RAM_D1. */
static float s_table[N];
static float s_re[N];
static float s_im[N];
static int   s_ready = 0;

/* --- deterministic RNG (fixed seed per build → reproducible tests) ------ */
static uint32_t s_rng;
static inline float rnd01(void) {
    s_rng = s_rng * 1664525u + 1013904223u;
    return (float)(s_rng >> 8) / 16777216.0f;
}

/* --- iterative radix-2 complex FFT (in place, DIT, own implementation) ---
 * Textbook Cooley-Tukey. Twiddles from sinf/cosf at the start of each
 * butterfly group + no per-sample trig anywhere near the audio path —
 * generation runs at world change only. Inverse via conjugate trick. */
static void fft_complex(float *re, float *im, int n) {
    /* bit-reversal permutation */
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j |= bit;
        if (i < j) {
            float t;
            t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -6.28318530717959f / (float)len;
        float wr0 = cosf(ang), wi0 = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float wr = 1.0f, wi = 0.0f;
            for (int k = 0; k < len / 2; ++k) {
                int   a = i + k, b = i + k + len / 2;
                float xr = re[b] * wr - im[b] * wi;
                float xi = re[b] * wi + im[b] * wr;
                re[b] = re[a] - xr; im[b] = im[a] - xi;
                re[a] += xr;        im[a] += xi;
                /* twiddle rotation (recurrence — no trig per butterfly) */
                float nwr = wr * wr0 - wi * wi0;
                wi = wr * wi0 + wi * wr0;
                wr = nwr;
            }
        }
    }
}

static void ifft_complex(float *re, float *im, int n) {
    for (int i = 0; i < n; ++i) im[i] = -im[i];     /* conjugate            */
    fft_complex(re, im, n);
    float s = 1.0f / (float)n;
    for (int i = 0; i < n; ++i) {                    /* conjugate + scale    */
        re[i] *=  s;
        im[i] *= -s;
    }
}

/* --- per-world harmonic amplitude profiles (OUR timbre decisions) --------
 * The model gives us two levers per world: the harmonic rolloff (how dark)
 * and small emphasis patterns (what "material" the bed suggests). Values
 * are this instrument's tuning, matched to the worlds' identities. */
static float harmonic_amp(int world, int h) {
    float a = 1.0f / powf((float)h, world == 2 ? 2.2f :   /* Drive: darkest */
                                    world == 1 ? 1.9f :   /* Coast: glassy  */
                                    world == 3 ? 1.7f :   /* Hours: dusty   */
                                                 1.6f);   /* Tokyo: warm    */
    switch (world) {
        case 0:  /* Tokyo — warm, slight odd-harmonic glow (reedy dusk)    */
            if (h & 1) a *= 1.25f;
            break;
        case 1:  /* Coast — glassy: lifted 2nd/4th, thin mids              */
            if (h == 2 || h == 4) a *= 1.6f;
            if (h >= 5 && h <= 9) a *= 0.7f;
            break;
        case 2:  /* Drive — dark body, gentle 3rd for motion               */
            if (h == 3) a *= 1.3f;
            break;
        default: /* Hours — dusty: soft even emphasis, rounded top          */
            if (!(h & 1)) a *= 1.2f;
            if (h > 20) a *= 0.6f;
            break;
    }
    return a;
}

void padsynth_build(int world_idx, uint32_t seed) {
    if (world_idx < 0) world_idx = 0;
    if (world_idx > 3) world_idx = 3;
    s_rng = seed ? seed : (0xBED0000u + (uint32_t)world_idx);

    memset(s_re, 0, sizeof s_re);
    memset(s_im, 0, sizeof s_im);

    /* Draw each harmonic as a Gaussian bump of bins (the Nasca model).
     * r18.97 NOISE-CARPET FIX (user: "irgendwas rauscht hardcore
     * durchgehend"): the first tuning used 48 harmonics with bandwidth
     * growing at h^1.2 — the spacing between harmonics stays f0 while the
     * bands widen, so above h≈20 (~2 kHz) the Gaussians fully OVERLAP and
     * the spectrum degenerates into one continuous noise band. That WAS
     * the carpet, baked into the bed itself. Now: bandwidth grows only
     * LINEARLY (bands never merge below h≈58), 30 cents base, 24
     * harmonics, plus a soft Gaussian comb-off above h≈10 — the lower
     * partials keep the living ensemble breath, the top stays TONE, not
     * noise. */
    const float f0        = PADSYNTH_F0;
    const float bw_cents  = 30.0f;
    const float bw_scale  = 1.0f;
    const float bin_hz    = SR / (float)N;

    for (int h = 1; h <= NH; ++h) {
        float fh = f0 * (float)h;
        if (fh > SR * 0.42f) break;                    /* leave HF headroom */
        float amp   = harmonic_amp(world_idx, h)
                    * expf(-((float)h / 12.0f) * ((float)h / 12.0f));
        float bw_hz = (powf(2.0f, bw_cents / 1200.0f) - 1.0f)
                    * f0 * powf((float)h, bw_scale);
        if (bw_hz < bin_hz) bw_hz = bin_hz;            /* ≥ one bin wide    */
        float sigma = bw_hz / (2.354820f * bin_hz);    /* FWHM → σ in bins  */
        float centre = fh / bin_hz;
        int   lo = (int)(centre - sigma * 5.0f); if (lo < 1) lo = 1;
        int   hi = (int)(centre + sigma * 5.0f) + 1;
        if (hi > N / 2 - 1) hi = N / 2 - 1;
        for (int b = lo; b <= hi; ++b) {
            float d = ((float)b - centre) / sigma;
            float g = expf(-0.5f * d * d) * amp;
            /* random phase per contribution — THE ingredient that turns a
             * line spectrum into a living band */
            float ph = rnd01() * 6.28318530717959f;
            s_re[b] += g * cosf(ph);
            s_im[b] += g * sinf(ph);
        }
    }

    /* Hermitian symmetry → real time-domain signal after the IFFT. */
    for (int b = 1; b < N / 2; ++b) {
        s_re[N - b] =  s_re[b];
        s_im[N - b] = -s_im[b];
    }
    s_re[0] = s_im[0] = 0.0f;                          /* no DC             */
    s_im[N / 2] = 0.0f;

    ifft_complex(s_re, s_im, N);

    /* Normalise to a known RMS so pad voice levels stay world-independent
     * (target 0.22 ≈ the old saw-stack body after its filter). */
    double acc = 0.0;
    for (int i = 0; i < N; ++i) acc += (double)s_re[i] * s_re[i];
    float rms  = (float)sqrt(acc / (double)N);
    float gain = rms > 1e-9f ? 0.22f / rms : 0.0f;
    for (int i = 0; i < N; ++i) s_table[i] = s_re[i] * gain;

    s_ready = 1;
}

int padsynth_ready(void) { return s_ready; }

float padsynth_read(float phase) {
    int   i0 = (int)phase;
    float fr = phase - (float)i0;
    i0 &= (N - 1);
    int i1 = (i0 + 1) & (N - 1);
    return s_table[i0] + fr * (s_table[i1] - s_table[i0]);
}

const float *padsynth_table(void) { return s_table; }
