/*
 * pluck.c — Karplus-Strong plucked-string voices. See pluck.h.
 */

#include "pluck.h"
#include "shape.h"
#include "dsp.h"
#include <math.h>
#include <stdatomic.h>
#include <string.h>

#define SR        ((float)DSP_SAMPLE_RATE_HZ)
#define BUF_LEN   1024                 /* > SR/PLUCK_MIN_HZ = 735 @44.1k */
#define T60_S     3.2f                 /* nominal loss; filter shortens it */
#define VERB_SEND 0.50f                /* plucks bloom into the hall     */
#define STOP_FRAMES ((uint32_t)(0.020f * SR)) /* bounded 20 ms soft stop */
#define ENV_EPS   2.5e-4f              /* ≈ −72 dBFS → voice retires     */
#define EXCITATION_MODES 8            /* bounded, harmonic displacement */
#define DC_COEF 0.001424f             /* output-only DC tracker, ~10 Hz */

typedef struct {
    float buf[BUF_LEN];
    float N;              /* target period; read delay is N-1     */
    float damp;           /* smoothed loop-filter coefficient     */
    int   widx;           /* write index                          */
    float rho;            /* per-sample loop gain for the T60     */
    float y_prev;         /* averaging-lowpass memory             */
    float y_prev2;
    float env;            /* tracked peak envelope (for retiring) */
    float panL, panR;
    _Atomic int active;
    int owner;            /* 0..255 source; 256 = legacy one-shot */
    float stop_gain;
    float attack_phase, attack_step;
    float dc;
    volatile uint32_t stop_left;
} pluck_voice_t;

static pluck_voice_t v[PLUCK_VOICES];
static _Atomic float s_damp = 0.42f * (0.25f / 0.9f); /* FIR side weight: 0=bright */
static int      next_voice;
static uint32_t excitation_rng = 0x9E3779B9u;

static inline float excitation_variation(void) {
    excitation_rng = excitation_rng * 1664525u + 1013904223u;
    return (float)((int32_t)excitation_rng) * (1.0f / 2147483648.0f);
}

void pluck_init(void) {
    memset(v, 0, sizeof v);
    /* Slight opposing pans — equal-power at ±0.35. */
    for (int i = 0; i < PLUCK_VOICES; ++i) {
        float p = (i == 0) ? -0.35f : 0.35f;
        float a = (p + 1.0f) * 0.25f * 3.14159265f;
        v[i].panL = cosf(a);
        v[i].panR = sinf(a);
    }
    next_voice  = 0;
    excitation_rng = 0x9E3779B9u;
    s_damp      = 0.42f * (0.25f / 0.9f);
}

void pluck_set_damp(float damp) {
    if (!isfinite(damp)) return;
    if (damp < 0.0f) damp = 0.0f;
    if (damp > 0.9f) damp = 0.9f;
    /* Symmetric non-negative FIR: fixed one-sample phase delay and monotonic
     * loss. The old two-tap blend became brighter again above its midpoint. */
    s_damp = damp * (0.25f / 0.9f);
}

static bool start_note(int owner, float freq_hz, float amp) {
    if (!isfinite(freq_hz) || !isfinite(amp) || freq_hz <= 0.0f ||
        freq_hz >= SR * 0.5f || amp <= 0.0f) return false;
    if (freq_hz < PLUCK_MIN_HZ) freq_hz = PLUCK_MIN_HZ;
    if (amp > 1.0f) amp = 1.0f;
    /* Never reset a ringing delay line. Releases occupy their slot too.
     * A busy source or full pool declines the event without changing RNG. */
    if (owner != 256)
        for (int k = 0; k < PLUCK_VOICES; ++k)
            if (v[k].active && v[k].owner == owner) return false;
    int i = -1;
    for (int k = 0; k < PLUCK_VOICES; ++k) {
        int candidate = (next_voice + k) % PLUCK_VOICES;
        if (!v[candidate].active) { i = candidate; break; }
    }
    if (i < 0) return false;
    next_voice = (i + 1) % PLUCK_VOICES;

    pluck_voice_t *p = &v[i];
    p->N   = SR / freq_hz;
    if (p->N > (float)(BUF_LEN - 4)) p->N = (float)(BUF_LEN - 4);
    p->damp = s_damp;
    p->rho = powf(0.001f, 1.0f / (freq_hz * T60_S * shape_release_scale())); /* r19.60 */
    p->widx   = 0;
    p->y_prev = p->y_prev2 = 0.0f;
    p->env    = amp;
    p->owner = owner;
    p->stop_gain = 1.0f;
    p->stop_left = 0;
    float attack_s = dsp_clampf(0.008f * shape_attack_scale(), 0.004f, 0.032f);
    p->attack_phase = 0.0f;
    p->attack_step = 1.0f / (attack_s * SR);
    p->dc = 0.0f;

    /* A plucked displacement, rather than a randomly missing fundamental.
     * Eight harmonic modes approximate a triangular string displacement:
     * sin(pi * mode * position) / mode^2. Slight position variation changes
     * articulation without adding a noise floor. This bounded work happens
     * at note-on; the existing delay/filter still produces the natural ring.
     * dsp_init() has prepared the sine LUT, so no per-sample transcendental
     * or extra source buffer is needed. Modes above 0.45 SR are omitted. */
    float position = 0.24f + 0.015f * excitation_variation();
    float coefficients[EXCITATION_MODES];
    int modes = 0;
    for (int m = 1; m <= EXCITATION_MODES &&
                    (m == 1 || m * freq_hz < SR * 0.45f); ++m) {
        coefficients[m - 1] = dsp_sin(0.5f * m * position) / (float)(m * m);
        modes = m;
    }
    int n = (int)p->N + 1;
    float mean = 0.0f;
    for (int k = 0; k < BUF_LEN; ++k) {
        float x = 0.0f;
        if (k < n) {
            float phase = (float)k / p->N;
            for (int m = 1; m <= modes; ++m)
                x += coefficients[m - 1] * dsp_sin(m * phase);
            mean += x;
        }
        p->buf[k] = x;
    }
    /* Finite/fractional cycles can otherwise seed a slowly decaying DC mode.
     * Centre the excitation and bound its peak before publishing the voice.
     * Convex interpolation/filtering and rho <= 1 preserve this peak bound. */
    mean /= (float)n;
    float peak = 0.0f;
    for (int k = 0; k < n; ++k) {
        p->buf[k] -= mean;
        float a = fabsf(p->buf[k]);
        if (a > peak) peak = a;
    }
    float gain = peak > 1.0e-8f ? amp / peak : 0.0f;
    for (int k = 0; k < n; ++k) p->buf[k] *= gain;
    /* Start past the burst so the read head reaches fresh displacement
     * before the write head overwrites it. */
    p->widx = n;
    /* The audio IRQ must never see a half-filled excitation buffer. */
    __asm__ volatile("" ::: "memory");
    p->active = 1;
    return true;
}

bool pluck_note_on(uint8_t source, float freq_hz, float amp) {
    /* Owned starts must not silently change the recorded pitch. */
    if (freq_hz < PLUCK_MIN_HZ || freq_hz > PLUCK_MAX_HZ) return false;
    return start_note((int)source, freq_hz, amp);
}
void pluck_note(float freq_hz, float amp) {
    (void)start_note(256, freq_hz, amp);
}
static void release_voice(pluck_voice_t *p) {
#ifdef FAM_SOUND_PRODUCT
    /* Product audio owner: a preparation cancelled before its first sample
     * is not a 20 ms audible strike, MIDI event or heard score item. */
    if (p->active && p->attack_phase==0.0f) { p->active=0; return; }
#endif
    if (p->active && !p->stop_left) p->stop_left = STOP_FRAMES;
}
void pluck_note_off(uint8_t source) {
    for (int i = 0; i < PLUCK_VOICES; ++i)
        if (v[i].owner == (int)source) release_voice(&v[i]);
}
void pluck_all_off(void) {
    for (int i = 0; i < PLUCK_VOICES; ++i) release_voice(&v[i]);
}

uint16_t pluck_active_sources(void) {
    uint16_t mask=0;
    for(int i=0;i<PLUCK_VOICES;++i) if(v[i].active && v[i].owner>=0 && v[i].owner<16)
        mask|=(uint16_t)(1u<<v[i].owner);
    return mask;
}
int pluck_active_count(void) {
    int c = 0;
    for (int i = 0; i < PLUCK_VOICES; ++i) c += v[i].active ? 1 : 0;
    return c;
}

void pluck_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames) {
    float damping=atomic_load_explicit(&s_damp,memory_order_relaxed);
    for (int i = 0; i < PLUCK_VOICES; ++i) {
        pluck_voice_t *p = &v[i];
        if (!p->active) continue;

        float env_track = p->env;
        for (int n = 0; n < frames; ++n) {
            /* Symmetric [a, 1-2a, a] damping has exactly one sample of phase
             * delay throughout its usable band, independently of a. The read
             * head therefore stays fixed when BRIGHTNESS changes. Only loss
             * follows the 80 ms smoother. Interpolation's small residual is
             * covered by rendered-pitch tests. */
            p->damp += (damping - p->damp) * (1.0f / (0.080f * SR));
            float rpos = (float)p->widx - (p->N - 1.0f);
            if (rpos < 0.0f) rpos += (float)BUF_LEN;
            int   r0 = (int)rpos;
            float fr = rpos - (float)r0;
            int   r1 = r0 + 1; if (r1 >= BUF_LEN) r1 = 0;
            float y  = p->buf[r0] + fr * (p->buf[r1] - p->buf[r0]);

            /* Damped feedback: averaging lowpass + T60 loop gain. */
            float fb = p->rho * (p->damp * (y + p->y_prev2) +
                                (1.0f - 2.0f * p->damp) * p->y_prev);
            p->y_prev2 = p->y_prev;
            p->y_prev = y;
            p->buf[p->widx] = fb;
            if (++p->widx >= BUF_LEN) p->widx = 0;

            /* Output and send share the ramp; the loop itself is untouched.
             * First release sample keeps its gain, last reaches silence.
             * Repeated stop requests cannot restart/prolong the release. */
            float attack = 1.0f;
            if (p->attack_phase < 1.0f) {
                float t = p->attack_phase;
                attack = t * t * (3.0f - 2.0f * t);
                p->attack_phase += p->attack_step;
                if (p->attack_phase > 1.0f) p->attack_phase = 1.0f;
            }
            /* Fractional startup/filter state can retain a tiny zero-frequency
             * mode, especially after high notes lose their audible modes.
             * Remove it outside the feedback: loop pitch/decay are unchanged. */
            p->dc += DC_COEF * (y - p->dc);
            float audible = y - p->dc;
            float output = audible * attack * p->stop_gain;
            if (p->stop_left) {
                --p->stop_left;
                p->stop_gain = (float)p->stop_left / (float)STOP_FRAMES;
            }
            dry_L[n]  += output * p->panL;
            dry_R[n]  += output * p->panR;
            send_L[n] += output * p->panL * VERB_SEND;
            send_R[n] += output * p->panR * VERB_SEND;

            /* cheap peak tracker: instant up, slow down */
            float a = audible < 0.0f ? -audible : audible;
            env_track = a > env_track ? a : env_track * 0.99995f;
            if (p->stop_gain == 0.0f) { p->active = 0; break; }
        }
        p->env = env_track;
        if (p->env < ENV_EPS) p->active = 0;
    }
}
