/*
 * harmony_field.c — Engine V2 voice leading + consonance budget.
 *
 * Internal contract (ADR-0014 §Consonance Budget):
 *   - At least 2 stable intervals (root/fifth/octave/9/6/11) are always active.
 *   - Maximum 1 strong tension (small-2nd/tritone/major-7) at any time, and
 *     never in the bass.
 *   - Tensions fade in slowly (long amp glide).
 *   - Bass moves rarely; high partial moves often but with tiny amp.
 */

#include "v2/harmony_field.h"
#include "dsp.h"
#include <math.h>
#include <stddef.h>

/* Tuning drift in cents per voice role (ADR-0014 §Tuning and Drift). */
static const float ROLE_DRIFT_CENTS[HF_VOICE_COUNT] = {
    /* bass  */  -2.0f,
    /* root  */   0.0f,
    /* fifth */  +2.0f,
    /* third */  -1.0f,
    /* sixth */  +6.0f,
    /* ninth */  -4.0f,
    /* elev  */  +3.0f,
    /* high  */   0.0f, /* random-walk handled separately */
};

/* Default pan distribution — asymmetric so the field doesn't centre-pile. */
static const float ROLE_PAN[HF_VOICE_COUNT] = {
     0.00f,  /* bass: centre */
    -0.05f,
    +0.10f,
    -0.20f,
    +0.35f,
    -0.40f,
    +0.55f,
    -0.65f,
};

/* Base amplitudes per role (before density / target rolls). */
static const float ROLE_BASE_AMP[HF_VOICE_COUNT] = {
    0.32f, 0.28f, 0.26f, 0.18f, 0.16f, 0.14f, 0.10f, 0.08f,
};

/* Retarget probability per role (per second). */
static const float ROLE_RETARGET_HZ[HF_VOICE_COUNT] = {
    0.05f,  /* bass: rare */
    0.10f,
    0.10f,
    0.18f,
    0.22f,
    0.25f,
    0.30f,
    0.45f,
};

/* Octave the voice prefers (relative to center). Bass = below, high = above. */
static const int ROLE_OCTAVE[HF_VOICE_COUNT] = { -2, -1, 0, 0, 0, +1, +1, +2 };

typedef struct {
    uint32_t  rng;
    int       center_midi;
    float     density;
    float     dissonance;
    float     motion;
    float     voice_target_count;
    hf_voice_t voices[HF_VOICE_COUNT];
} state_t;

static state_t S;

/* xorshift32 PRNG */
static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x ? x : 0xDEADBEEFu;
    return *s;
}
static float urand(uint32_t *s) { return (float)xs32(s) * (1.0f / 4294967296.0f); }

/* Semitone offsets for "stable" chord tones (no harsh dissonances). */
static const int STABLE_OFFSETS[]  = { 0, 7, 12, 14, 9, 19, 21, 24 };  /* R 5 8va 9 6 12 13 octave2 */
#define N_STABLE (int)(sizeof STABLE_OFFSETS / sizeof STABLE_OFFSETS[0])

static const int COLOR_THIRD_OFFSETS[] = { 3, 4, 10 };  /* m3 / M3 / m7 */
#define N_COLOR_THIRD (int)(sizeof COLOR_THIRD_OFFSETS / sizeof COLOR_THIRD_OFFSETS[0])

/* Tensions used only when dissonance budget allows. */
static const int TENSION_OFFSETS[] = { 1, 6, 11 };
#define N_TENSION (int)(sizeof TENSION_OFFSETS / sizeof TENSION_OFFSETS[0])

/* Pick a fresh semitone offset for a role.
 * Tension allowed only on high voices, never bass. */
static int pick_offset_for_role(hf_voice_id_t role) {
    switch (role) {
        case HF_VOICE_BASS:
        case HF_VOICE_ROOT:
            /* Bass + Root: only Root or Octave (rarely). */
            return urand(&S.rng) < 0.85f ? 0 : 12;
        case HF_VOICE_FIFTH:
            /* Fifth or rarely fourth (sus). */
            return urand(&S.rng) < 0.9f ? 7 : 5;
        case HF_VOICE_THIRD:
            return COLOR_THIRD_OFFSETS[(int)(urand(&S.rng) * N_COLOR_THIRD)];
        case HF_VOICE_SIXTH:
            return urand(&S.rng) < 0.7f ? 9 : 14;   /* 6 or 9 */
        case HF_VOICE_NINTH:
            return urand(&S.rng) < 0.7f ? 14 : 9;   /* 9 or 6 */
        case HF_VOICE_ELEVENTH:
            /* Eleventh is the most exotic stable colour; sometimes upgrade
             * to tension if dissonance budget allows. */
            if (urand(&S.rng) < S.dissonance * 0.4f) {
                return TENSION_OFFSETS[(int)(urand(&S.rng) * N_TENSION)] + 12;
            }
            return urand(&S.rng) < 0.6f ? 17 : 21;
        case HF_VOICE_HIGH_PARTIAL:
            /* Free random over stable set. */
            return STABLE_OFFSETS[(int)(urand(&S.rng) * N_STABLE)];
        default:
            return 0;
    }
}

static float midi_to_hz(int midi) {
    return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}

/* Apply ±cents detune to a base midi frequency. */
static float midi_plus_cents(int midi, float cents) {
    return midi_to_hz(midi) * powf(2.0f, cents / 1200.0f);
}

static void seed_voice(hf_voice_id_t role) {
    hf_voice_t *v = &S.voices[role];
    v->pan = ROLE_PAN[role];
    v->drift_cents = ROLE_DRIFT_CENTS[role];
    v->age_s = 0.0f;
    int off = pick_offset_for_role(role);
    int oct = ROLE_OCTAVE[role];
    v->midi_note = S.center_midi + off + 12 * oct;
    v->target_freq_hz = midi_plus_cents(v->midi_note, v->drift_cents);
    v->freq_hz = v->target_freq_hz;
    v->target_amp = ROLE_BASE_AMP[role];
    v->amp = 0.0f;       /* fade in */
    v->active = true;
}

static void retarget_voice(hf_voice_id_t role) {
    hf_voice_t *v = &S.voices[role];
    int off = pick_offset_for_role(role);
    int oct = ROLE_OCTAVE[role];
    int new_midi = S.center_midi + off + 12 * oct;
    /* Voice leading: maximum motion per retarget is one octave; if dice
     * rolled an octave-shifted target, only take a fifth/third of the way. */
    int delta = new_midi - v->midi_note;
    if (delta > 12)  delta = 12;
    if (delta < -12) delta = -12;
    v->midi_note += delta;
    v->target_freq_hz = midi_plus_cents(v->midi_note, v->drift_cents);
    v->age_s = 0.0f;
}

/* Roll active/inactive based on density + target count.
 * Bass + Root + Fifth always active (the gravity); Third onwards may drop. */
static void update_active_set(void) {
    int target_active = (int)(S.voice_target_count + 0.5f);
    if (target_active < 3) target_active = 3;
    if (target_active > HF_VOICE_COUNT) target_active = HF_VOICE_COUNT;

    /* Bass/Root/Fifth always active. */
    S.voices[HF_VOICE_BASS].active  = true;
    S.voices[HF_VOICE_ROOT].active  = true;
    S.voices[HF_VOICE_FIFTH].active = true;

    /* Third+ activated in priority order. */
    int remaining = target_active - 3;
    static const hf_voice_id_t order[] = {
        HF_VOICE_NINTH, HF_VOICE_SIXTH, HF_VOICE_THIRD,
        HF_VOICE_HIGH_PARTIAL, HF_VOICE_ELEVENTH,
    };
    int n = (int)(sizeof order / sizeof order[0]);
    for (int i = 0; i < n; ++i) {
        S.voices[order[i]].active = (i < remaining);
        if (!S.voices[order[i]].active) {
            S.voices[order[i]].target_amp = 0.0f;
        } else {
            S.voices[order[i]].target_amp =
                ROLE_BASE_AMP[order[i]] * (0.6f + 0.4f * S.density);
        }
    }

    /* Always-on voices scale by density too. */
    S.voices[HF_VOICE_BASS].target_amp  = ROLE_BASE_AMP[HF_VOICE_BASS]  * (0.7f + 0.3f * S.density);
    S.voices[HF_VOICE_ROOT].target_amp  = ROLE_BASE_AMP[HF_VOICE_ROOT]  * (0.7f + 0.3f * S.density);
    S.voices[HF_VOICE_FIFTH].target_amp = ROLE_BASE_AMP[HF_VOICE_FIFTH] * (0.7f + 0.3f * S.density);
}

void hf_init(uint32_t seed, int center_midi) {
    S.rng = seed ? seed : 0xC0FFEE01u;
    S.center_midi = center_midi;
    S.density = 0.5f;
    S.dissonance = 0.2f;
    S.motion = 0.4f;
    S.voice_target_count = 5.0f;
    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        seed_voice((hf_voice_id_t)i);
    }
    update_active_set();
}

void hf_set_center(int center_midi) {
    int delta = center_midi - S.center_midi;
    S.center_midi = center_midi;
    /* Slide all voices by the same interval — the field transposes, voice
     * leading inside the field is preserved. */
    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        S.voices[i].midi_note += delta;
        S.voices[i].target_freq_hz =
            midi_plus_cents(S.voices[i].midi_note, S.voices[i].drift_cents);
    }
}

void hf_set_density(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    S.density = v;
    update_active_set();
}

void hf_set_dissonance(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    S.dissonance = v;
}

void hf_set_motion(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    S.motion = v;
}

void hf_set_voice_target_count(float n) {
    if (n < 3.0f) n = 3.0f;
    if (n > (float)HF_VOICE_COUNT) n = (float)HF_VOICE_COUNT;
    S.voice_target_count = n;
    update_active_set();
}

void hf_advance(float dt_s) {
    if (dt_s < 0.0f) dt_s = 0.0f;

    /* Glide coefs:
     *   freq: tau ~2.5s (slow voice-leading)
     *   amp : tau ~1.8s
     */
    float coef_f = 1.0f - expf(-dt_s / 2.5f);
    float coef_a = 1.0f - expf(-dt_s / 1.8f);

    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        hf_voice_t *v = &S.voices[i];
        v->age_s += dt_s;

        /* Retarget dice — scaled by motion. */
        float p = ROLE_RETARGET_HZ[i] * (0.3f + 1.7f * S.motion) * dt_s;
        if (urand(&S.rng) < p) {
            retarget_voice((hf_voice_id_t)i);
        }

        v->freq_hz += coef_f * (v->target_freq_hz - v->freq_hz);
        v->amp     += coef_a * (v->target_amp     - v->amp);
    }
}

const hf_voice_t *hf_voice(int idx) {
    if (idx < 0 || idx >= HF_VOICE_COUNT) return NULL;
    return &S.voices[idx];
}

int hf_active_count(void) {
    int n = 0;
    for (int i = 0; i < HF_VOICE_COUNT; ++i) if (S.voices[i].active) ++n;
    return n;
}

void hf_new_field(uint32_t seed) {
    S.rng = seed ? seed : 0xC0FFEE02u;
    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        S.voices[i].amp = 0.0f;
        retarget_voice((hf_voice_id_t)i);
    }
}
