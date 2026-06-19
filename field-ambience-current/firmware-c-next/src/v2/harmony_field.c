/*
 * harmony_field.c — pentatonic-locked voice leading (ADR-0014 r2).
 *
 * Guarantee: every sounding pitch is a member of the active pentatonic scale
 * transposed to the current root. Pentatonic scales contain no minor-2nd and
 * no tritone, so no two voices can ever form a harsh interval — the field is
 * consonant by construction. "dissonance" macro only widens how far upper
 * voices wander among scale tones; it never introduces a clash.
 */

#include "v2/harmony_field.h"
#include "dsp.h"
#include <math.h>
#include <stddef.h>

/* Pentatonic scales (semitones from root). No semitone, no tritone. */
static const int MAJOR_PENT[HF_SCALE_LEN] = { 0, 2, 4, 7, 9 };
static const int MINOR_PENT[HF_SCALE_LEN] = { 0, 3, 5, 7, 10 };

/* Stacked voicing — base pentatonic step (degree) + octave per voice role.
 * Low voices anchor the chord; upper voices add air. */
static const int VOICE_DEGREE[HF_VOICE_COUNT] = { 0, 0, 3, 0, 1, 3, 4, 2 };
static const int VOICE_OCTAVE[HF_VOICE_COUNT] = { -2, -1, -1, 0, 0, 0, 1, 1 };

/* How many scale-steps each voice may wander from its anchor (gravity vs air). */
static const int VOICE_WANDER[HF_VOICE_COUNT] = { 0, 0, 1, 1, 2, 2, 3, 4 };

/* Asymmetric pan per role. */
static const float ROLE_PAN[HF_VOICE_COUNT] = {
    0.00f, -0.05f, 0.10f, -0.18f, 0.30f, -0.34f, 0.50f, -0.60f,
};

/* Small per-role detune (cents) — chorus warmth, not dissonance. */
static const float ROLE_DRIFT_CENTS[HF_VOICE_COUNT] = {
    -1.0f, 0.0f, +2.0f, -2.0f, +4.0f, -3.0f, +5.0f, -4.0f,
};

/* Base amplitude per role. */
static const float ROLE_BASE_AMP[HF_VOICE_COUNT] = {
    0.30f, 0.28f, 0.22f, 0.22f, 0.16f, 0.15f, 0.11f, 0.09f,
};

/* Retarget probability per role (Hz) — bass rarely, top often. */
static const float ROLE_RETARGET_HZ[HF_VOICE_COUNT] = {
    0.01f, 0.02f, 0.05f, 0.07f, 0.10f, 0.12f, 0.16f, 0.20f,
};

typedef struct {
    uint32_t  rng;
    int       root_midi;
    const int *scale;
    float     density;
    float     dissonance;       /* 0..1 → wander scale */
    float     motion;
    float     voice_target_count;
    hf_voice_t voices[HF_VOICE_COUNT];
} state_t;

static state_t S;

static uint32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x ? x : 0xDEADBEEFu;
    return *s;
}
static float urand(uint32_t *s) { return (float)xs32(s) * (1.0f / 4294967296.0f); }

static float midi_to_hz(int midi) {
    return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
}
static float midi_plus_cents(int midi, float cents) {
    return midi_to_hz(midi) * powf(2.0f, cents / 1200.0f);
}

/* pentatonic step (oct*5 + degree, may be negative) → midi note */
static int pos_to_midi(int pos) {
    int n = HF_SCALE_LEN;
    int oct = pos / n;
    int deg = pos % n;
    if (deg < 0) { deg += n; oct -= 1; }
    return S.root_midi + 12 * oct + S.scale[deg];
}

static int anchor_pos(int voice) {
    return VOICE_OCTAVE[voice] * HF_SCALE_LEN + VOICE_DEGREE[voice];
}

static void set_voice_pitch(int voice) {
    hf_voice_t *v = &S.voices[voice];
    v->midi_note = pos_to_midi(v->scale_pos);
    v->target_freq_hz = midi_plus_cents(v->midi_note, v->drift_cents);
}

static void seed_voice(int voice) {
    hf_voice_t *v = &S.voices[voice];
    v->pan = ROLE_PAN[voice];
    v->drift_cents = ROLE_DRIFT_CENTS[voice];
    v->age_s = 0.0f;
    v->scale_pos = anchor_pos(voice);
    set_voice_pitch(voice);
    v->freq_hz = v->target_freq_hz;
    v->target_amp = ROLE_BASE_AMP[voice];
    v->amp = 0.0f;     /* fade in */
    v->active = true;
}

/* Move a voice ±1 scale step (rarely ±2 for very mobile voices), staying
 * within its wander window around the anchor. Pentatonic → always consonant. */
static void retarget_voice(int voice) {
    hf_voice_t *v = &S.voices[voice];
    int wander = VOICE_WANDER[voice];
    /* dissonance widens the wander window for upper voices. */
    int extra = (int)(S.dissonance * (float)wander * 0.5f);
    int range = wander + extra;
    if (range <= 0) return;       /* bass/root never move */

    int anchor = anchor_pos(voice);
    int step = (urand(&S.rng) < 0.85f) ? 1 : 2;
    if (urand(&S.rng) < 0.5f) step = -step;
    int next = v->scale_pos + step;
    if (next > anchor + range) next = anchor + range;
    if (next < anchor - range) next = anchor - range;
    v->scale_pos = next;
    set_voice_pitch(voice);
    v->age_s = 0.0f;
}

static void update_active_set(void) {
    int target_active = (int)(S.voice_target_count + 0.5f);
    if (target_active < 3) target_active = 3;
    if (target_active > HF_VOICE_COUNT) target_active = HF_VOICE_COUNT;

    /* Bass/Root/Fifth always on — the gravity. */
    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        bool on = (i < 3) ? true : (i < target_active);
        S.voices[i].active = on;
        if (!on) {
            S.voices[i].target_amp = 0.0f;
        } else {
            S.voices[i].target_amp = ROLE_BASE_AMP[i] * (0.65f + 0.35f * S.density);
        }
    }
}

void hf_init(uint32_t seed, int center_midi) {
    S.rng = seed ? seed : 0xC0FFEE01u;
    S.root_midi = center_midi;
    S.scale = MINOR_PENT;       /* default dreamy minor pentatonic */
    S.density = 0.5f;
    S.dissonance = 0.2f;
    S.motion = 0.4f;
    S.voice_target_count = 6.0f;
    for (int i = 0; i < HF_VOICE_COUNT; ++i) seed_voice(i);
    update_active_set();
}

void hf_set_center(int center_midi) {
    S.root_midi = center_midi;
    for (int i = 0; i < HF_VOICE_COUNT; ++i) set_voice_pitch(i);
}

void hf_set_scale_minor(bool minor) {
    S.scale = minor ? MINOR_PENT : MAJOR_PENT;
    for (int i = 0; i < HF_VOICE_COUNT; ++i) set_voice_pitch(i);
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
    /* Slow voice-leading glide. */
    float coef_f = 1.0f - expf(-dt_s / 3.0f);   /* 3 s freq glide */
    float coef_a = 1.0f - expf(-dt_s / 2.0f);   /* 2 s amp glide  */

    for (int i = 0; i < HF_VOICE_COUNT; ++i) {
        hf_voice_t *v = &S.voices[i];
        v->age_s += dt_s;
        float p = ROLE_RETARGET_HZ[i] * (0.3f + 1.7f * S.motion) * dt_s;
        if (urand(&S.rng) < p) retarget_voice(i);
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
        retarget_voice(i);
    }
}

int hf_root_midi(void) { return S.root_midi; }
int hf_scale_len(void) { return HF_SCALE_LEN; }
int hf_scale_semitone(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= HF_SCALE_LEN) idx = HF_SCALE_LEN - 1;
    return S.scale[idx];
}
int hf_scale_pos_to_midi(int pos) { return pos_to_midi(pos); }
