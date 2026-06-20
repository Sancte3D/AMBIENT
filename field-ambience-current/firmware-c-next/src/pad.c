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
/* Amp envelope (seconds). Webapp cellOn uses attack 0.8 / release 3.0 — the
 * 1.5 s value from _makePadVoice defaults is for chord-spawn use, not cell
 * taps. Using the spawn value here made cells feel bloomy instead of tap-y. */
#define PAD_ATTACK_S   0.8f
#define PAD_RELEASE_S  3.0f

/* Per-side oscillator stack: 3 saws (×1, ×freqMul, ×0.5) + 2 squares. */
#define OSC_N 5

#define HAAS_LEN 1024                           /* > 0.014 s · 44.1 kHz ≈ 618 */

typedef enum { ENV_IDLE = 0, ENV_ATTACK, ENV_SUSTAIN, ENV_RELEASE } env_state_t;

typedef struct {
    float phase[OSC_N];     /* turns [0,1) */
    float inc[OSC_N];       /* per-sample increment = f·mul / SR (wow-modulated) */
    float base_inc[OSC_N];  /* nominal increment (no wow); inc = base_inc·wow_mul */

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

    /* per-voice tape-wow / vibrato LFO. Modulates osc increments at control
     * rate. Depth/rate come from the active profile. */
    float       wow_phase;
    float       wow_inc;    /* per-sample LFO phase increment */
    float       wow_depth;  /* fraction, e.g. 0.0017 ≈ ±3 cents */

    int         profile;    /* which profile this voice was born under */
} pad_voice_t;

/* ============================================================================
 * Pad voice PROFILES — four selectable timbres on the same saw-stack engine.
 * Switch via pad_set_profile(0..3). Each profile is a parameter set that
 * pad_note_on() reads at trigger time + the render loop reads each control
 * block (per-voice fields that need to survive a profile switch are baked
 * into pad_voice_t / pad_side_t at note_on time). ========================== */

typedef struct {
    const char *name;

    float attack_s;          /* amp attack */
    float release_s;         /* amp release */

    float cutoff_base;       /* LP base cutoff in Hz */
    float cutoff_mod;        /* LFO cutoff modulation depth in Hz */
    float q_mul;             /* SVF Q multiplier (1.0 = neutral; >1 ringy) */
    float fenv_amount;       /* filter envelope amount (0 = no sweep) */
    float fenv_attack_s;     /* filter env attack time */
    float lfo_rate_hz;       /* cutoff LFO rate (base; per-source variation added) */

    /* tape-wow / vibrato (per-voice LFO modulating osc increments) */
    float wow_depth;         /* 0 = none, 0.0017 = ±3 cents, 0.008 = ±14 cents */
    float wow_rate_hz;       /* 0.3–0.6 reads as tape-wow; 4–6 reads as vibrato */

    float detune_cents;      /* between-sides detune (base; per-source variation added) */

    /* oscillator stack mix (renormalised inside the render loop). The base
     * stack is 3 saws (×1, ×freqMul, ×0.5) + 2 squares (×1, ×freqMul);
     * these are the relative levels. */
    float saw1_w;            /* fundamental saws (both ×1 and ×freqMul) */
    float sub_w;             /* ×0.5 sub-saw weight */
    float pulse_w;           /* squares weight */

    /* reverb send override (-1 = leave engine default). Used to keep the
     * plucked-bell from drowning in wash; not yet wired through engine but
     * available if we want it later. */
    float send_amount;
} pad_profile_t;

#define PAD_PROFILE_COUNT 4

/* All four profiles. Hand-tuned, deliberately distinct characters; all are
 * warm/friendly, none should read as sacred/organ/cathedral. */
static const pad_profile_t PROFILES[PAD_PROFILE_COUNT] = {
    /* 0 — WarmChorus (Juno-106 inspired)
     *   short attack, sub-saw heavy, modest LP with subtle Q, light tape-wow
     *   acting as BBD-chorus simulacrum. The closest match to "warm 80s pad". */
    {   "WarmChorus",
        .attack_s        = 0.06f,
        .release_s       = 1.4f,
        .cutoff_base     = 850.0f,
        .cutoff_mod      = 350.0f,
        .q_mul           = 1.35f,
        .fenv_amount     = 0.30f,
        .fenv_attack_s   = 0.04f,
        .lfo_rate_hz     = 0.55f,
        .wow_depth       = 0.0022f,    /* ≈ ±3.8 cents */
        .wow_rate_hz     = 0.42f,
        .detune_cents    = 7.0f,
        .saw1_w          = 0.55f,
        .sub_w           = 1.10f,      /* sub heavy → octave-down warmth */
        .pulse_w         = 0.18f,
        .send_amount     = -1.0f,
    },
    /* 1 — CinematicWash (CS-80 / OB-Xa inspired)
     *   longer attack, full saw+pulse stack, audible vibrato (5 Hz), wider
     *   detune. Reads as filmic / spacious / "lived-in". */
    {   "CinematicWash",
        .attack_s        = 0.32f,
        .release_s       = 2.6f,
        .cutoff_base     = 1050.0f,
        .cutoff_mod      = 700.0f,
        .q_mul           = 1.10f,
        .fenv_amount     = 0.45f,
        .fenv_attack_s   = 0.20f,
        .lfo_rate_hz     = 0.08f,
        .wow_depth       = 0.0030f,    /* ≈ ±5 cents — gentle vibrato */
        .wow_rate_hz     = 4.6f,
        .detune_cents    = 11.0f,
        .saw1_w          = 0.85f,
        .sub_w           = 0.55f,
        .pulse_w         = 0.42f,
        .send_amount     = -1.0f,
    },
    /* 2 — TapeSoft (Boards of Canada / Wavestation inspired)
     *   dark LP, slow-deep tape-wow (clearly audible), moderate stack, no
     *   resonance. Reads as nostalgic / lo-fi / dreamy. */
    {   "TapeSoft",
        .attack_s        = 0.22f,
        .release_s       = 2.4f,
        .cutoff_base     = 620.0f,     /* dark */
        .cutoff_mod      = 260.0f,
        .q_mul           = 0.90f,      /* less resonant → softer */
        .fenv_amount     = 0.20f,
        .fenv_attack_s   = 0.18f,
        .lfo_rate_hz     = 0.04f,
        .wow_depth       = 0.0090f,    /* ≈ ±15 cents — audible wobble */
        .wow_rate_hz     = 0.55f,
        .detune_cents    = 13.0f,      /* lazy out-of-tune feel */
        .saw1_w          = 0.65f,
        .sub_w           = 0.70f,
        .pulse_w         = 0.32f,
        .send_amount     = -1.0f,
    },
    /* 3 — PluckedBell (subtractive bell, not FM)
     *   very fast attack, fenv sweeps the LP wide open then closes, high Q
     *   for ringy partials, almost no sustain — read as percussive mallet.
     *   NOT a DX7-FM impersonation; an honest subtractive plucked-bell. */
    {   "PluckedBell",
        .attack_s        = 0.005f,
        .release_s       = 1.8f,
        .cutoff_base     = 1400.0f,
        .cutoff_mod      = 1200.0f,
        .q_mul           = 2.20f,      /* ringy partials */
        .fenv_amount     = 0.85f,      /* strong filter-attack click */
        .fenv_attack_s   = 0.004f,     /* fast filter open */
        .lfo_rate_hz     = 0.06f,
        .wow_depth       = 0.0008f,    /* almost none — clean attack */
        .wow_rate_hz     = 0.30f,
        .detune_cents    = 5.0f,       /* tight detune so partials don't smear */
        .saw1_w          = 0.40f,
        .sub_w           = 0.30f,
        .pulse_w         = 0.85f,      /* pulse-heavy → glockig */
        .send_amount     = -1.0f,
    },
};

static int active_profile = 0;

#define PROF (&PROFILES[active_profile])

static pad_voice_t voices[PAD_MAX];
static int   ctl_phase;             /* shared control-rate counter */
static float bright_target;         /* brightness offset target (Hz) */
static float bright_cur;            /* smoothed brightness */
static float bright_coef;           /* per-control-block smoothing coef */

/* Oscillator layout per side: 0,1,2 = saws (×1, ×freqMul, ×0.5),
 * 3,4 = squares (×1, ×freqMul). Frequency multipliers below; the second-saw
 * and second-square multipliers are the per-side freqMul, filled in at setup. */
static const float OSC_MUL_BASE[OSC_N]  = { 1.0f, 1.0f, 0.5f, 1.0f, 1.0f };

/* voiceMix is a GLOBAL, smoothed timbre control (0 = warm/pure-saw … ~1.2 =
 * brass). It is applied at render time to ALL voices at once, so changing the
 * pad voice glides the whole stack into the new timbre together instead of
 * leaving old and new timbres ringing side by side. */
static float vmix_target;           /* requested voiceMix */
static float saww_cur, pulsew_cur;  /* smoothed saw / pulse weights */
static float vmix_coef;             /* per-control-block smoothing coef */

void pad_init(void) {
    memset(voices, 0, sizeof voices);
    ctl_phase      = 0;
    bright_target  = 0.0f;
    bright_cur     = 0.0f;
    active_profile = 0;             /* default → WarmChorus */
    /* brightness glide ~80 ms, evaluated once per control block */
    bright_coef = 1.0f - expf(-(float)CTL_DECIMATE / (0.08f * SR));

    vmix_target = 0.0f;             /* warm */
    saww_cur    = 1.0f;             /* pure saw at boot */
    pulsew_cur  = 0.0f;
    /* timbre glide ~150 ms */
    vmix_coef = 1.0f - expf(-(float)CTL_DECIMATE / (0.15f * SR));
}

void pad_set_brightness(float hz) { bright_target = hz; }

/* Global pad-voice timbre — kept as the saw/pulse balance knob INSIDE the
 * active profile. Backward-compatible; tests still use this. The bigger
 * pad_set_profile() switches between four distinct timbre families. */
void pad_set_voice_mix(float vmix) { vmix_target = dsp_clampf(vmix, 0.0f, 1.5f); }

/* Select one of four character profiles. The switch takes effect on the next
 * pad_note_on (currently-ringing voices keep the profile they were born with
 * → no zipper). 0=WarmChorus, 1=CinematicWash, 2=TapeSoft, 3=PluckedBell. */
void pad_set_profile(int id) {
    if (id < 0) id = 0;
    if (id >= PAD_PROFILE_COUNT) id = PAD_PROFILE_COUNT - 1;
    active_profile = id;
}

int pad_get_profile(void) { return active_profile; }
const char *pad_profile_name(int id) {
    if (id < 0 || id >= PAD_PROFILE_COUNT) return "?";
    return PROFILES[id].name;
}

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
                       float cutoff_rate, float fenv_attack_s, bool keep_phase,
                       const pad_profile_t *p) {
    float f = base_freq * powf(2.0f, (sign * detune_cents) / 1200.0f);

    const float mul[OSC_N] = { OSC_MUL_BASE[0], freq_mul, OSC_MUL_BASE[2],
                               OSC_MUL_BASE[3], freq_mul };
    for (int k = 0; k < OSC_N; ++k) {
        if (!keep_phase) s->phase[k] = 0.0f;
        s->base_inc[k] = f * mul[k] / SR;
        s->inc[k]      = s->base_inc[k];        /* wow scales this each ctl block */
    }

    if (!keep_phase) dsp_svf_reset(&s->svf);
    s->cutoffBase  = cutoff_base;
    s->cutoffMod   = p->cutoff_mod;
    s->fenvAmount  = p->fenv_amount;

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

    const pad_profile_t *p = PROF;

    /* Deterministic per-source variation (no rand(): keeps tests reproducible
     * while still detuning each voice differently). The profile sets the
     * BASE for each parameter; we add a small per-source offset for ensemble
     * decorrelation. */
    float detune = p->detune_cents + (float)(source & 3);            /* base + 0..3c */
    float cbase  = p->cutoff_base + (float)(source % 5) * 30.0f;     /* base + 0..120 Hz */
    float crate  = p->lfo_rate_hz + (float)(source % 4) * 0.012f;
    float fatk   = p->fenv_attack_s + (float)(source % 3) * 0.05f;

    v->used   = true;
    v->source = source;
    v->profile = active_profile;
    v->amp    = dsp_clampf(amp, 0.0f, 1.0f);
    if (!keep_phase) v->env = 0.0001f;
    v->atkInc  = v->amp / (p->attack_s * SR);
    v->relCoef = dsp_smooth_coef(p->release_s / 3.0f);
    v->state   = ENV_ATTACK;

    /* tape-wow / vibrato per voice. Phase deterministic per source so the
     * ensemble decorrelates without rand(); rate also varies slightly so
     * voices don't lock-step. */
    if (!keep_phase) {
        v->wow_phase = (float)(source & 7) * 0.125f;
        v->wow_inc   = (p->wow_rate_hz + 0.013f * (float)(source % 10)) / SR;
        v->wow_depth = p->wow_depth;
    }

    /* Webapp cellOn pans cells across the stereo field: pan = (degree-4)·0.15
     * → Cell 1 left (−0.45), Cell 5 right (+0.15). For source IDs ≥ 5
     * (chord-spawn, generative bed) keep centred. */
    float voice_pan = (source < 5) ? ((float)source - 3.0f) * 0.15f : 0.0f;

    /* side 0 = −detune, freqMul 1.003; side 1 = +detune, freqMul 0.997 */
    side_setup(&v->side[0], freq_hz, -1, voice_pan, detune, 1.003f, cbase, crate, fatk, keep_phase, p);
    side_setup(&v->side[1], freq_hz, +1, voice_pan, detune, 0.997f, cbase, crate, fatk, keep_phase, p);
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

/* Control-rate update for one side: apply wow to osc increments, advance
 * LFO + filter env, recompute the lowpass cutoff. Called every CTL_DECIMATE
 * samples. wow_mul comes from the parent voice (per-voice wow → both sides
 * wobble coherently; voices wobble independently → ensemble chorus). Q comes
 * from the active profile (profile-of-the-voice — read via voice->profile
 * by the caller, then passed in as q_mul). */
static void side_control(pad_side_t *s, float wow_mul, float q_mul) {
    /* apply wow to osc increments (tape-wow / vibrato — kills static orgel) */
    for (int k = 0; k < OSC_N; ++k) s->inc[k] = s->base_inc[k] * wow_mul;

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
    dsp_svf_set(&s->svf, dsp_clampf(cutoff, 80.0f, 8000.0f), 0.18f * 12.0f * q_mul);
}

/* Raw stereo float render — no master soft-clip, no int16 conversion. The
 * two public render APIs (pad_render and pad_render_mix) both delegate
 * here; the master tanh moved out to the engine in Step 11 so the dry pad
 * can be mixed cleanly with the reverb wet bus. Overwrites outL/outR. */
static void render_block_float(float *outL, float *outR, int frames) {
    for (int n = 0; n < frames; ++n) {
        bool ctl = (ctl_phase == 0);
        if (ctl) {
            bright_cur += bright_coef * (bright_target - bright_cur);
            /* Smooth the global timbre weights toward the requested voiceMix.
             * sawWeight = 1 - mix*0.5, pulseWeight = mix*0.5 (webapp). */
            float saww_tgt   = dsp_clampf(1.0f - vmix_target * 0.5f, 0.0f, 1.0f);
            float pulsew_tgt = dsp_clampf(vmix_target * 0.5f,        0.0f, 1.0f);
            saww_cur   += vmix_coef * (saww_tgt   - saww_cur);
            pulsew_cur += vmix_coef * (pulsew_tgt - pulsew_cur);
        }

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

            /* per-voice tape-wow / vibrato: advance phase + compute multiplier
             * once per control block (16 samples). Profile-controlled depth +
             * rate. wow_mul = 1 + sin(phase) * depth. */
            float wow_mul = 1.0f;
            if (ctl) {
                v->wow_phase += v->wow_inc * (float)CTL_DECIMATE;
                if (v->wow_phase >= 1.0f) v->wow_phase -= 1.0f;
                wow_mul = 1.0f + dsp_sin(v->wow_phase) * v->wow_depth;
            }

            /* Per-voice profile (the voice was born under its own profile;
             * re-triggering an existing source keeps its original profile so
             * a live profile switch never zippers a ringing voice). */
            const pad_profile_t *vp = &PROFILES[v->profile];

            float vL = 0.0f, vR = 0.0f;
            for (int sd = 0; sd < 2; ++sd) {
                pad_side_t *s = &v->side[sd];
                if (ctl) side_control(s, wow_mul, vp->q_mul);

                /* Oscillator stack mix from the voice's profile.
                 *   saws[0,1] = fundamental, saws[2] = sub-octave (×0.5)
                 *   pulses[3,4] = squares (×1, ×freqMul)
                 * Profile weights (saw1_w, sub_w, pulse_w) re-balance the
                 * stack so each profile has a recognisable timbral identity
                 * instead of every voice being the same 5-osc soup. */
                float saw_fund = dsp_poly_saw(s->phase[0], s->inc[0])
                               + dsp_poly_saw(s->phase[1], s->inc[1]);
                float saw_sub  = dsp_poly_saw(s->phase[2], s->inc[2]);
                float pulses   = dsp_poly_square(s->phase[3], s->inc[3])
                               + dsp_poly_square(s->phase[4], s->inc[4]);

                float saws = vp->saw1_w * saw_fund + vp->sub_w * saw_sub;
                /* `saww_cur` / `pulsew_cur` are still the global saw↔pulse
                 * macro crossfade (pad_set_voice_mix). It multiplies the
                 * profile pulse weight, so the macro acts as an extra slider
                 * INSIDE each profile rather than replacing it. */
                float sig = (saws * saww_cur
                            + pulses * vp->pulse_w * (0.5f + pulsew_cur))
                            * 0.32f;

                for (int k = 0; k < OSC_N; ++k) {
                    s->phase[k] += s->inc[k];
                    if (s->phase[k] >= 1.0f) s->phase[k] -= 1.0f;
                }

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
