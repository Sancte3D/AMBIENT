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

/* Table + FFT scratch, this module's .bss → RAM_D1.
 *
 * r19.17 D1-RECLAIM: the output is REAL, and the spectrum we build is
 * Hermitian by construction — so the classic real-IFFT trick applies: an
 * N-point real inverse transform computed with one N/2-point COMPLEX IFFT
 * (even/odd interleave, textbook Cooley-Tukey identity; same math, same
 * table up to float rounding). Scratch drops from 2×N floats (128 KB) to
 * 2×(N/2+1) (64 KB).
 *
 * r19.40 realtime-safety: the final pass writes all N floats of s_table in a
 * loop while the audio ISR reads it — so during that (sub-ms) window a read
 * could straddle the write frontier and return a torn value (old+new mixed →
 * a loud spectral pop). A true fix is a double buffer + pointer swap, but the
 * RAM budget has no home for a second 64 KB table (D1 tight, D2 96 % full with
 * echo+blur, D3 high-latency, DTCM holds pad+stack). So instead the reader is
 * GATED: while s_publishing is set (only around the final copy) it returns a
 * clean 0.0f instead of a half-written sample. Same tiny window as before, but
 * a bounded silence rather than torn data — and the world-change re-bloom
 * crossfade masks it. (Not a hard atomicity guarantee against a flag flip mid-
 * read; that needs the double buffer. It removes the torn-garbage case.) */
#define NHALF (N / 2)
static float s_table[N];
static float s_xr[NHALF + 1];   /* X[0..N/2] half-spectrum, then Z (in place) */
static float s_xi[NHALF + 1];
static int   s_ready = 0;
static volatile int s_publishing = 0;   /* 1 only during the final s_table copy */

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

    memset(s_xr, 0, sizeof s_xr);
    memset(s_xi, 0, sizeof s_xi);

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
            s_xr[b] += g * cosf(ph);
            s_xi[b] += g * sinf(ph);
        }
    }

    /* Half-spectrum boundary bins (the mirror X[N-b] = conj(X[b]) is now
     * IMPLICIT in the even/odd recombination below — never materialised). */
    s_xr[0] = s_xi[0] = 0.0f;                          /* no DC             */
    s_xr[NHALF] = s_xi[NHALF] = 0.0f;                  /* Nyquist           */

    /* Real IFFT via one N/2-point complex IFFT (even/odd split):
     *   E[k] = (X[k] + X[k+N/2]) / 2,   O[k] = (X[k] - X[k+N/2])/2 · W^{+k}
     *   Z[k] = E[k] + j·O[k],  z = IDFT_{N/2}(Z),  x[2m]+j·x[2m+1] = z[m]
     * with X[k+N/2] = conj(X[N/2-k]) from Hermitian symmetry. Processed in
     * conjugate PAIRS (k, N/2-k) so Z overwrites X in place. */
    {
        const float w = 6.28318530717959f / (float)N;
        /* k = 0 (uses X[0] and X[N/2], both real slots) */
        {
            float e_r = 0.5f * (s_xr[0] + s_xr[NHALF]);
            float e_i = 0.5f * (s_xi[0] + s_xi[NHALF]);
            float o_r = 0.5f * (s_xr[0] - s_xr[NHALF]);
            float o_i = 0.5f * (s_xi[0] - s_xi[NHALF]);
            s_xr[0] = e_r - o_i;                       /* Z = E + jO        */
            s_xi[0] = e_i + o_r;
        }
        for (int k = 1; k <= NHALF / 2; ++k) {
            int   km = NHALF - k;                      /* the pair partner  */
            float ar = s_xr[k],  ai = s_xi[k];         /* X[k]              */
            float br = s_xr[km], bi = s_xi[km];        /* X[N/2-k]          */
            float tw_r = cosf(w * (float)k), tw_i = sinf(w * (float)k);

            /* Z[k]: X[k+N/2] = conj(X[N/2-k]) = (br, -bi) */
            float e_r = 0.5f * (ar + br), e_i = 0.5f * (ai - bi);
            float d_r = 0.5f * (ar - br), d_i = 0.5f * (ai + bi);
            float o_r = d_r * tw_r - d_i * tw_i;       /* D · W^{+k}        */
            float o_i = d_r * tw_i + d_i * tw_r;
            float zk_r = e_r - o_i, zk_i = e_i + o_r;  /* E + jO            */

            float zm_r = zk_r, zm_i = zk_i;
            if (k != km) {
                /* Z[N/2-k]: X[(N/2-k)+N/2] = conj(X[k]) = (ar, -ai);
                 * twiddle W^{+(N/2-k)} = -conj(W^{+k}) = (-tw_r, tw_i) */
                float E_r = 0.5f * (br + ar), E_i = 0.5f * (bi - ai);
                float D_r = 0.5f * (br - ar), D_i = 0.5f * (bi + ai);
                float O_r = -(D_r * tw_r) - D_i * tw_i;
                float O_i =  D_r * tw_i  - D_i * tw_r;
                zm_r = E_r - O_i; zm_i = E_i + O_r;
            }
            s_xr[k]  = zk_r; s_xi[k]  = zk_i;
            s_xr[km] = zm_r; s_xi[km] = zm_i;
        }
    }

    ifft_complex(s_xr, s_xi, NHALF);   /* z[m]: re = x[2m], im = x[2m+1]   */

    /* Normalise to a known RMS so pad voice levels stay world-independent
     * (target 0.22 ≈ the old saw-stack body after its filter). Σx² over the
     * full table = Σ(zre²+zim²) over the half-length z. */
    double acc = 0.0;
    for (int m = 0; m < NHALF; ++m)
        acc += (double)s_xr[m] * s_xr[m] + (double)s_xi[m] * s_xi[m];
    float rms  = (float)sqrt(acc / (double)N);
    float gain = rms > 1e-9f ? 0.22f / rms : 0.0f;
    /* Gate the reader for the duration of the (non-atomic) table copy so the
     * audio ISR never reads a half-written s_table. See the header note. */
    s_publishing = 1;
    __asm__ volatile("" ::: "memory");
    for (int m = 0; m < NHALF; ++m) {                  /* single final pass */
        s_table[2 * m]     = s_xr[m] * gain;
        s_table[2 * m + 1] = s_xi[m] * gain;
    }
    __asm__ volatile("" ::: "memory");
    s_publishing = 0;

    s_ready = 1;
}

int padsynth_ready(void) { return s_ready; }

float padsynth_read(float phase) {
    if (s_publishing) return 0.0f;   /* table mid-rewrite: clean 0, never torn */
    int   i0 = (int)phase;
    float fr = phase - (float)i0;
    i0 &= (N - 1);
    int i1 = (i0 + 1) & (N - 1);
    return s_table[i0] + fr * (s_table[i1] - s_table[i0]);
}

const float *padsynth_table(void) { return s_table; }
