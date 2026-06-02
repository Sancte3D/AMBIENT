/*
 * famPadCore — Step 9. Ported from the Web-Audio `_makePadVoice`.
 *
 * Faithful to the reference structure: two detuned sides, each a 3-saw +
 * 2-square stack into a resonant lowpass swept by LFO + filter-ADSR +
 * brightness, then Haas-delayed and opposing-panned for width, summed under
 * one bloom/decay amp envelope. Default voiceMix = 0 → pure saw character
 * (the squares are weighted in only when the voice param is raised later).
 *
 * Audio-rate work per sample: 10 polyBLEP oscillators + 2 SVF + 2 Haas reads
 * per voice. Filter cutoff is recomputed at control rate (every CTL_DECIMATE
 * samples) — the LFO and envelopes are far slower than audio rate, so this is
 * inaudible and keeps the tanf out of the inner loop.
 */

#include "pad.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define DSP_PI       3.14159265358979f
#define SR           ((float)DSP_SAMPLE_RATE_HZ)
#define CTL_DECIMATE 16                         /* control-rate = SR/16 ≈ 2.76 kHz */

/* Amp envelope (seconds). Slow bloom + long tail per the Sound Constitution. */
#define PAD_ATTACK_S   1.6f
#define PAD_RELEASE_S  3.0f

/* Per-side oscillator stack: 3 saws (×1, ×freqMul, ×0.5) + 2 squares. */
#define OSC_N 5

#define HAAS_LEN 1024                           /* > 0.014 s · 44.1 kHz ≈ 618 */

typedef enum { ENV_IDLE = 0, ENV_ATTACK, ENV_SUSTAIN, ENV_RELEASE } env_state_t;

typedef struct {
    float phase[OSC_N];     /* turns [0,1) */
    float inc[OSC_N];       /* per-sample increment = f·mul / SR */
    float gain[OSC_N];      /* saw / pulse weights, baked */

    dsp_svf_t svf;
    float cutoffBase, cutoffMod, fenvAmount;
    float lfoPhase, lfoInc;

    /* filter envelope (control rate): attack 0→1, then decay→sustain */
    float fenv, fenvAtkInc, fenvDecCoef, fenvSustain;
    int   fenvStage;        /* 0 attack, 1 decayed */

    float dline[HAAS_LEN];  /* Haas micro-delay */
    int   dpos, ddelay;

    float panL, panR;       /* equal-power pan, baked */
} pad_side_t;

typedef struct {
    bool        used;
    uint8_t     source;
    pad_side_t  side[2];

    float       env;        /* current amp level, peaks at `amp` */
    float       amp;        /* peak target */
    float       atkInc;     /* per-sample linear attack step */
    float       relCoef;    /* per-sample exponential release coef */
    env_state_t state;
} pad_voice_t;

static pad_voice_t voices[PAD_MAX];
static int   ctl_phase;             /* shared control-rate counter */
static float bright_target;         /* brightness offset target (Hz) */
static float bright_cur;            /* smoothed brightness */
static float bright_coef;           /* per-control-block smoothing coef */

/* Frequency multipliers + base gains for the 5 oscillators (voiceMix = 0). */
static const float OSC_MUL_BASE[OSC_N]  = { 1.0f, 1.0f, 0.5f, 1.0f, 1.0f };
static const float OSC_SAW_GAIN[OSC_N]  = { 1.0f, 1.0f, 0.5f, 0.0f, 0.0f };

void pad_init(void) {
    memset(voices, 0, sizeof voices);
    ctl_phase     = 0;
    bright_target = 0.0f;
    bright_cur    = 0.0f;
    /* brightness glide ~80 ms, evaluated once per control block */
    bright_coef = 1.0f - expf(-(float)CTL_DECIMATE / (0.08f * SR));
}

void pad_set_brightness(float hz) { bright_target = hz; }

/* Equal-power pan: p in [-1,1] → (gl,gr). */
static void pan_gains(float p, float *gl, float *gr) {
    p = dsp_clampf(p, -1.0f, 1.0f);
    float a = (p + 1.0f) * 0.25f * DSP_PI;      /* 0 … π/2 */
    *gl = cosf(a);
    *gr = sinf(a);
}

/* Smoothing coefficient for an exponential approach over `tau_s`, evaluated
 * once per control block of CTL_DECIMATE samples. */
static float ctl_coef(float tau_s) {
    if (tau_s <= 0.0f) return 1.0f;
    return 1.0f - expf(-(float)CTL_DECIMATE / (tau_s * SR));
}

static int find_source(uint8_t source) {
    for (int i = 0; i < PAD_MAX; ++i)
        if (voices[i].used && voices[i].source == source) return i;
    return -1;
}

/* Pick a slot: idle first, else steal the quietest (lowest env). */
static int alloc_slot(void) {
    int best = -1; float best_env = 1e9f;
    for (int i = 0; i < PAD_MAX; ++i) {
        if (!voices[i].used) return i;
        if (voices[i].env < best_env) { best_env = voices[i].env; best = i; }
    }
    return best;
}

static void side_setup(pad_side_t *s, float base_freq, int sign, float voice_pan,
                       float detune_cents, float freq_mul, float cutoff_base,
                       float cutoff_rate, float fenv_attack_s, bool keep_phase) {
    float f = base_freq * powf(2.0f, (sign * detune_cents) / 1200.0f);

    const float mul[OSC_N] = { OSC_MUL_BASE[0], freq_mul, OSC_MUL_BASE[2],
                               OSC_MUL_BASE[3], freq_mul };
    for (int k = 0; k < OSC_N; ++k) {
        if (!keep_phase) s->phase[k] = 0.0f;
        s->inc[k]  = f * mul[k] / SR;
        s->gain[k] = OSC_SAW_GAIN[k];           /* voiceMix = 0 → squares silent */
    }

    if (!keep_phase) dsp_svf_reset(&s->svf);
    s->cutoffBase  = cutoff_base;
    s->cutoffMod   = 500.0f;
    s->fenvAmount  = 0.5f;

    s->lfoInc   = cutoff_rate * (sign > 0 ? 1.13f : 1.0f) / SR;
    if (!keep_phase) s->lfoPhase = (sign > 0) ? 0.37f : 0.0f;

    s->fenv        = 0.0001f;
    s->fenvStage   = 0;
    s->fenvAtkInc  = (1.0f / (fenv_attack_s * SR)) * (float)CTL_DECIMATE;
    s->fenvDecCoef = ctl_coef(4.5f / 3.0f);     /* fenvDecay ≈ 4.5 s */
    s->fenvSustain = 0.65f;

    if (!keep_phase) { memset(s->dline, 0, sizeof s->dline); s->dpos = 0; }
    /* Haas: +side 14 ms, −side 8 ms. */
    s->ddelay = (int)((sign > 0 ? 0.014f : 0.008f) * SR);
    if (s->ddelay >= HAAS_LEN) s->ddelay = HAAS_LEN - 1;

    pan_gains(dsp_clampf(voice_pan + sign * 0.25f, -1.0f, 1.0f), &s->panL, &s->panR);
}

void pad_note_on(uint8_t source, float freq_hz, float amp) {
    int i = find_source(source);
    bool keep_phase = (i >= 0);                 /* re-trigger: glide, don't click */
    if (i < 0) i = alloc_slot();
    if (i < 0) return;
    pad_voice_t *v = &voices[i];

    /* Deterministic per-source variation (no rand(): keeps tests reproducible
     * while still detuning each voice differently). */
    float detune = 7.0f + (float)(source & 3);          /* 7 … 10 cents   */
    float cbase  = 1100.0f + (float)(source % 5) * 40.0f;
    float crate  = 0.05f + (float)(source % 4) * 0.015f;
    float fatk   = 2.5f + (float)(source % 3) * 0.5f;

    v->used   = true;
    v->source = source;
    v->amp    = dsp_clampf(amp, 0.0f, 1.0f);
    if (!keep_phase) v->env = 0.0001f;
    v->atkInc = v->amp / (PAD_ATTACK_S * SR);
    v->relCoef = dsp_smooth_coef(PAD_RELEASE_S / 3.0f);
    v->state  = ENV_ATTACK;

    /* side 0 = −detune, freqMul 1.003; side 1 = +detune, freqMul 0.997 */
    side_setup(&v->side[0], freq_hz, -1, 0.0f, detune, 1.003f, cbase, crate, fatk, keep_phase);
    side_setup(&v->side[1], freq_hz, +1, 0.0f, detune, 0.997f, cbase, crate, fatk, keep_phase);
}

void pad_note_off(uint8_t source) {
    int i = find_source(source);
    if (i < 0) return;
    if (voices[i].state != ENV_IDLE) voices[i].state = ENV_RELEASE;
}

void pad_all_off(void) {
    for (int i = 0; i < PAD_MAX; ++i)
        if (voices[i].used && voices[i].state != ENV_IDLE)
            voices[i].state = ENV_RELEASE;
}

/* Control-rate update for one side: advance LFO + filter env, recompute the
 * lowpass cutoff. Called every CTL_DECIMATE samples. */
static void side_control(pad_side_t *s) {
    /* LFO */
    s->lfoPhase += s->lfoInc * (float)CTL_DECIMATE;
    if (s->lfoPhase >= 1.0f) s->lfoPhase -= 1.0f;
    float lfo = dsp_sin(s->lfoPhase);

    /* filter envelope */
    if (s->fenvStage == 0) {
        s->fenv += s->fenvAtkInc;
        if (s->fenv >= 1.0f) { s->fenv = 1.0f; s->fenvStage = 1; }
    } else {
        s->fenv += s->fenvDecCoef * (s->fenvSustain - s->fenv);
    }

    float cutoff = s->cutoffBase
                 + lfo  * s->cutoffMod * 0.5f
                 + s->fenv * s->cutoffMod * s->fenvAmount
                 + bright_cur;
    dsp_svf_set(&s->svf, dsp_clampf(cutoff, 80.0f, 8000.0f), 0.18f * 12.0f);
}

/* Raw stereo float render — no master soft-clip, no int16 conversion. The
 * two public render APIs (pad_render and pad_render_mix) both delegate
 * here; the master tanh moved out to the engine in Step 11 so the dry pad
 * can be mixed cleanly with the reverb wet bus. Overwrites outL/outR. */
static void render_block_float(float *outL, float *outR, int frames) {
    for (int n = 0; n < frames; ++n) {
        bool ctl = (ctl_phase == 0);
        if (ctl) bright_cur += bright_coef * (bright_target - bright_cur);

        float L = 0.0f, R = 0.0f;

        for (int i = 0; i < PAD_MAX; ++i) {
            pad_voice_t *v = &voices[i];
            if (!v->used) continue;

            /* amp envelope, per sample (click-free) */
            switch (v->state) {
                case ENV_ATTACK:
                    v->env += v->atkInc;
                    if (v->env >= v->amp) { v->env = v->amp; v->state = ENV_SUSTAIN; }
                    break;
                case ENV_RELEASE:
                    v->env -= v->relCoef * v->env;
                    if (v->env <= 1.0e-5f) {
                        v->env = 0.0f; v->state = ENV_IDLE; v->used = false;
                    }
                    break;
                default: break;
            }
            if (!v->used) continue;

            float vL = 0.0f, vR = 0.0f;
            for (int sd = 0; sd < 2; ++sd) {
                pad_side_t *s = &v->side[sd];
                if (ctl) side_control(s);

                /* oscillator stack */
                float sig = 0.0f;
                for (int k = 0; k < OSC_N; ++k) {
                    float g = s->gain[k];
                    if (g != 0.0f) {
                        sig += g * dsp_poly_saw(s->phase[k], s->inc[k]);
                    }
                    s->phase[k] += s->inc[k];
                    if (s->phase[k] >= 1.0f) s->phase[k] -= 1.0f;
                }
                sig *= 0.4f;

                float lp = dsp_svf_lp(&s->svf, sig);

                /* Haas micro-delay */
                s->dline[s->dpos] = lp;
                int rd = s->dpos - s->ddelay;
                if (rd < 0) rd += HAAS_LEN;
                float delayed = s->dline[rd];
                if (++s->dpos >= HAAS_LEN) s->dpos = 0;

                vL += delayed * s->panL;
                vR += delayed * s->panR;
            }

            L += vL * v->env;
            R += vR * v->env;
        }

        if (++ctl_phase >= CTL_DECIMATE) ctl_phase = 0;

        outL[n] = L;
        outR[n] = R;
    }
}

/* Backward-compatible standalone path: render → tanh master → int16. Used
 * by the Step-9 host tests (`test_pad.c`) and as a fallback renderer. */
void pad_render(int16_t *buf, int frames) {
    enum { CH = 64 };
    float L[CH], R[CH];
    int left = frames, out_idx = 0;
    while (left > 0) {
        int n = left < CH ? left : CH;
        render_block_float(L, R, n);
        for (int i = 0; i < n; ++i) {
            float l = tanhf(L[i] * 0.9f);
            float r = tanhf(R[i] * 0.9f);
            buf[(out_idx + i) * 2 + 0] = (int16_t)(l * 32767.0f);
            buf[(out_idx + i) * 2 + 1] = (int16_t)(r * 32767.0f);
        }
        out_idx += n; left -= n;
    }
}

/* Step 11 mix-bus path: ADDS into dry + scaled into reverb-send buffers.
 * No master clip here — that's the engine's job after dry+wet are summed. */
void pad_render_mix(float *dry_L, float *dry_R,
                    float *send_L, float *send_R,
                    int frames, float send_amount) {
    enum { CH = 64 };
    float L[CH], R[CH];
    int left = frames, out_idx = 0;
    while (left > 0) {
        int n = left < CH ? left : CH;
        render_block_float(L, R, n);
        for (int i = 0; i < n; ++i) {
            dry_L[out_idx + i]  += L[i];
            dry_R[out_idx + i]  += R[i];
            send_L[out_idx + i] += L[i] * send_amount;
            send_R[out_idx + i] += R[i] * send_amount;
        }
        out_idx += n; left -= n;
    }
}

int pad_active_count(void) {
    int c = 0;
    for (int i = 0; i < PAD_MAX; ++i)
        if (voices[i].used && voices[i].state != ENV_IDLE) ++c;
    return c;
}
