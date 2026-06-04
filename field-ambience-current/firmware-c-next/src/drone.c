/*
 * Drone — Step 12b #2. One sustained detuned-saw voice on the key root,
 * with live portamento so it follows key changes instead of clashing.
 *
 * Structure mirrors a single famPadCore voice (2 detuned sides × 3 saws →
 * resonant SVF swept by LFO + filter-env → Haas → amp env) but adds a glided
 * fundamental, like the bass. Control-rate work (filter coeffs, LFO, filter
 * env, pitch glide, oscillator increments) runs every CTL_DECIMATE samples;
 * the oscillators + SVF + Haas run per sample.
 *
 * Webapp drone params (spawnDrone): amp 0.05, attack 6 s, release 4 s,
 * detune 4 cents, cutoffBase 650, cutoffMod 150, cutoffRate 0.03 Hz,
 * lpRes 0.15 (→ Q 1.8), fenvAttack 8 s, fenvDecay 6 s, fenvSustain 0.6,
 * fenvAmount 0.1, verbSend 0.45.
 */

#include "drone.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

#define SR            ((float)DSP_SAMPLE_RATE_HZ)
#define CTL_DECIMATE  16
#define DSP_PI        3.14159265358979f

#define DRONE_AMP        0.05f
#define ATTACK_S         6.0f
#define RELEASE_S        (4.0f / 3.0f)     /* setTargetAtTime tau */
#define DETUNE_CENTS     4.0f
#define CUTOFF_BASE      650.0f
#define CUTOFF_MOD       150.0f
#define CUTOFF_RATE      0.03f
#define LP_Q             (0.15f * 12.0f)
#define FENV_ATTACK_S    8.0f
#define FENV_DECAY_S     6.0f
#define FENV_SUSTAIN     0.6f
#define FENV_AMOUNT      0.1f
#define VERB_SEND        0.45f
#define GLIDE_S          0.5f              /* portamento time constant */
#define SILENCE_EPS      1.0e-4f
#define HAAS_LEN         1024              /* > 0.012 s · 44.1 kHz */

#define OSC_N 3                            /* 3 saws per side: ×1, ×freqMul, ×0.5 */

typedef enum { ENV_IDLE = 0, ENV_ATTACK, ENV_SUSTAIN, ENV_RELEASE } env_state_t;

typedef struct {
    float phase[OSC_N];
    float inc[OSC_N];
    float freqMul;        /* second-saw detune multiplier (1.003 / 0.997) */
    int   sign;           /* −1 / +1 detune direction */
    dsp_svf_t svf;
    float lfoPhase, lfoInc;
    float panL, panR;
    float dline[HAAS_LEN];
    int   dpos, ddelay;
} drone_side_t;

static drone_side_t side[2];

static float freq_cur, freq_tgt;     /* root frequency (Hz), glided */
static float glide_coef;             /* per-control-block one-pole */

static float env;                    /* amp env, peaks at DRONE_AMP */
static float atkInc, relCoef;
static env_state_t state;

static float fenv;                   /* filter env 0..1 (control rate) */
static int   fenvStage;
static float fenvAtkInc, fenvDecCoef;

static int   ctl_phase;

static void update_incs(void);       /* defined below */

static float ctl_coef(float tau_s) {
    if (tau_s <= 0.0f) return 1.0f;
    return 1.0f - expf(-(float)CTL_DECIMATE / (tau_s * SR));
}

static void pan_gains(float p, float *gl, float *gr) {
    p = dsp_clampf(p, -1.0f, 1.0f);
    float a = (p + 1.0f) * 0.25f * DSP_PI;
    *gl = cosf(a); *gr = sinf(a);
}

void drone_init(void) {
    memset(side, 0, sizeof side);
    for (int s = 0; s < 2; ++s) {
        side[s].sign    = (s == 0) ? -1 : +1;
        side[s].freqMul = (s == 0) ? 1.003f : 0.997f;
        dsp_svf_reset(&side[s].svf);
        dsp_svf_set(&side[s].svf, CUTOFF_BASE, LP_Q);
        side[s].lfoInc  = CUTOFF_RATE * (s == 0 ? 1.0f : 1.13f) / SR;
        side[s].lfoPhase = (s == 0) ? 0.0f : 0.37f;
        side[s].ddelay = (int)((s == 0 ? 0.008f : 0.012f) * SR);
        pan_gains(s == 0 ? -0.2f : 0.2f, &side[s].panL, &side[s].panR);
    }
    freq_cur = freq_tgt = dsp_midi_to_hz(60.0f);   /* C4 until set */
    glide_coef = ctl_coef(GLIDE_S);
    env = 0.0f;
    atkInc  = DRONE_AMP / (ATTACK_S * SR);
    relCoef = dsp_smooth_coef(RELEASE_S);
    state = ENV_IDLE;
    fenv = 0.0001f; fenvStage = 0;
    fenvAtkInc  = (1.0f / (FENV_ATTACK_S * SR)) * (float)CTL_DECIMATE;
    fenvDecCoef = ctl_coef(FENV_DECAY_S / 3.0f);
    ctl_phase = 0;
}

void drone_set_root_midi(int midi) {
    if (midi < 0)   midi = 0;
    if (midi > 127) midi = 127;
    freq_tgt = dsp_midi_to_hz((float)midi);
    if (state == ENV_IDLE) freq_cur = freq_tgt;    /* snap while silent */
}

void drone_enable(bool on) {
    if (on) {
        if (state == ENV_IDLE) {
            freq_cur = freq_tgt;
            env = 0.0001f;
            fenv = 0.0001f; fenvStage = 0;
            ctl_phase = 0;             /* force control_update on first sample */
            update_incs();             /* incs fresh immediately */
        }
        state = ENV_ATTACK;
    } else {
        if (state != ENV_IDLE) state = ENV_RELEASE;
    }
}

bool drone_active(void) { return state != ENV_IDLE; }

/* Recompute per-side oscillator increments from the (glided) root. */
static void update_incs(void) {
    static const float MUL0 = 1.0f, MUL2 = 0.5f;
    for (int s = 0; s < 2; ++s) {
        float f = freq_cur * powf(2.0f, (side[s].sign * DETUNE_CENTS) / 1200.0f);
        side[s].inc[0] = f * MUL0 / SR;
        side[s].inc[1] = f * side[s].freqMul / SR;
        side[s].inc[2] = f * MUL2 / SR;
    }
}

static void control_update(void) {
    /* pitch glide */
    freq_cur += glide_coef * (freq_tgt - freq_cur);
    update_incs();

    /* filter envelope */
    if (fenvStage == 0) {
        fenv += fenvAtkInc;
        if (fenv >= 1.0f) { fenv = 1.0f; fenvStage = 1; }
    } else {
        fenv += fenvDecCoef * (FENV_SUSTAIN - fenv);
    }

    for (int s = 0; s < 2; ++s) {
        side[s].lfoPhase += side[s].lfoInc * (float)CTL_DECIMATE;
        if (side[s].lfoPhase >= 1.0f) side[s].lfoPhase -= 1.0f;
        float lfo = dsp_sin(side[s].lfoPhase);
        float cutoff = CUTOFF_BASE
                     + lfo  * CUTOFF_MOD * 0.5f
                     + fenv * CUTOFF_MOD * FENV_AMOUNT;
        dsp_svf_set(&side[s].svf, dsp_clampf(cutoff, 80.0f, 8000.0f), LP_Q);
    }
}

void drone_render_mix(float *dry_L, float *dry_R,
                      float *send_L, float *send_R, int frames) {
    if (state == ENV_IDLE) return;

    for (int n = 0; n < frames; ++n) {
        if (ctl_phase == 0) control_update();

        /* amp envelope (per sample, click-free) */
        switch (state) {
            case ENV_ATTACK:
                env += atkInc;
                if (env >= DRONE_AMP) { env = DRONE_AMP; state = ENV_SUSTAIN; }
                break;
            case ENV_RELEASE:
                env -= relCoef * env;
                if (env <= SILENCE_EPS) { env = 0.0f; state = ENV_IDLE; }
                break;
            default: break;
        }
        if (state == ENV_IDLE) break;

        float L = 0.0f, R = 0.0f;
        for (int s = 0; s < 2; ++s) {
            drone_side_t *sd = &side[s];
            float sig = dsp_poly_saw(sd->phase[0], sd->inc[0])
                      + dsp_poly_saw(sd->phase[1], sd->inc[1])
                      + 0.5f * dsp_poly_saw(sd->phase[2], sd->inc[2]);
            sig *= 0.4f;
            for (int k = 0; k < OSC_N; ++k) {
                sd->phase[k] += sd->inc[k];
                if (sd->phase[k] >= 1.0f) sd->phase[k] -= 1.0f;
            }
            float lp = dsp_svf_lp(&sd->svf, sig);

            sd->dline[sd->dpos] = lp;
            int rd = sd->dpos - sd->ddelay;
            if (rd < 0) rd += HAAS_LEN;
            float delayed = sd->dline[rd];
            if (++sd->dpos >= HAAS_LEN) sd->dpos = 0;

            L += delayed * sd->panL;
            R += delayed * sd->panR;
        }

        if (++ctl_phase >= CTL_DECIMATE) ctl_phase = 0;

        float oL = L * env, oR = R * env;
        dry_L[n]  += oL;            dry_R[n]  += oR;
        send_L[n] += oL * VERB_SEND; send_R[n] += oR * VERB_SEND;
    }
}
