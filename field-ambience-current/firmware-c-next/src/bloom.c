/*
 * bloom.c — Chord-Bloom-Zellmodus. Verhalten: siehe bloom.h.
 */

#include "bloom.h"
#include "engine.h"
#include "brain.h"
#include "tuning.h"
#include "voicelead.h"
#include "rolepatch.h"

#define POOL      5          /* Chord-Sources 0..4 (= Base-Cell-Sources)  */
#define MAXV      4          /* r19.29: max 4 sustained chord voices       */
#define STEP_MS   100u       /* Grund-Abstand zwischen Akkordtoenen        */
#define JITTER_MS 70u        /* + 0..70 ms (deterministisch) pro Ton       */
#define ANCHOR    64         /* VOICE_CENTER — Register des ersten Akkords */

typedef struct {
    bool     armed;          /* wartet auf seinen Einsatz  */
    uint32_t due_ms;
    float    freq;
    float    amp;            /* r19.30: pad body amp, nach Rollen-Gain      */
    float    sparkle;        /* r19.30: >0 → zusaetzlich Glas-Bloom (EXT)    */
} onset_t;

static onset_t s_onset[POOL];
static int     s_prev_pitch[MAXV]; /* aktuell klingende Akkord-Tonhoehen      */
static int     s_prev_n;           /* Anzahl Toene des aktuell klingenden Akk. */
static bool    s_have_prev;
static int8_t  s_active_cell;      /* -1 = keiner                             */
static bool    s_hold;             /* Akkord gelatcht (HOLD beim Druck)       */
static uint32_t s_rng;

void bloom_init(void) {
    for (int i = 0; i < POOL; ++i) s_onset[i].armed = false;
    for (int i = 0; i < MAXV; ++i) s_prev_pitch[i] = 0;
    s_prev_n = 0;
    s_have_prev = false;
    s_active_cell = -1;
    s_hold = false;
    s_rng = 0x9E3779B9u;
}

static uint32_t jitter(void) {          /* 0..JITTER_MS, deterministisch */
    s_rng = s_rng * 1664525u + 1013904223u;
    return (s_rng >> 9) % (JITTER_MS + 1u);
}

/* r19.29 HARMONY: cell → world chord ROLE (brain_role_degree), then place the
 * new chord's pitch classes by MINIMAL-MOVEMENT voice leading (voicelead.c)
 * against the sounding chord — common tones stay put, only the voices that
 * must change move, and each keeps its own source so it glides, not re-strikes.
 * Capped at 4 sustained voices (clean level, no 20-voice pile-up). */
void bloom_press(uint8_t cell, float velocity_amp, bool hold, uint32_t now_ms) {
    if (cell >= POOL) return;

    int chord[BRAIN_MAX_CHORD];
    int cn = brain_chord(brain_role_degree((int)cell), chord, BRAIN_MAX_CHORD);
    if (cn <= 0) return;

    int m = cn > MAXV ? MAXV : cn;      /* keep root..(4th tone): the essentials */
    int pc[MAXV];
    for (int i = 0; i < m; ++i) pc[i] = ((chord[i] % 12) + 12) % 12;

    int voiced[MAXV];
    int n = voicelead(s_have_prev ? s_prev_pitch : 0, s_have_prev ? s_prev_n : 0,
                      pc, m, ANCHOR, voiced);

    /* Wurde der Akkord kleiner als der vorherige, die ueberzaehligen
     * Pool-Stimmen freigeben (Voice-Leading laesst die uebrigen gleiten). */
    for (int i = n; i < s_prev_n; ++i) {
        engine_note_off((uint8_t)i);
        s_onset[i].armed = false;
    }

    /* Gestaffelte Einsaetze: erster Ton sofort, dann STEP+Jitter pro Ton.
     * note_on auf einer schon klingenden Source re-pitcht sie (Ramp) →
     * die Stimmen gleiten ins neue Voicing statt hart neu zu triggern. */
    /* r19.30: each voice plays its chord ROLE, not the same pad patch — root
     * full, inner voices pulled back, the top voice softened + a glass shimmer
     * blooming over it (rolepatch.c). Turns "4 identical pad notes" into an
     * arranged chord. */
    uint32_t t = now_ms;
    for (int i = 0; i < n; ++i) {
        const role_patch_t *rp = rolepatch_get(rolepatch_role_for(i, n));
        s_onset[i].armed   = true;
        s_onset[i].due_ms  = t;
        s_onset[i].freq    = tuning_hz((float)voiced[i]);
        s_onset[i].amp     = velocity_amp * rp->gain;
        s_onset[i].sparkle = velocity_amp * rp->sparkle_gain;
        t += STEP_MS + jitter();
    }

    for (int i = 0; i < n; ++i) s_prev_pitch[i] = voiced[i];
    s_have_prev     = true;
    s_prev_n        = n;
    s_active_cell   = (int8_t)cell;
    s_hold          = hold;
    engine_set_user_presence(true);
}

void bloom_release(uint8_t cell, uint32_t now_ms) {
    (void)now_ms;
    engine_set_user_presence(false);       /* Finger ist physisch oben */
    if ((int)cell != s_active_cell) return;
    if (s_hold) return;                    /* gelatchter Akkord bleibt  */
    bloom_all_off();
}

void bloom_tick(uint32_t now_ms) {
    for (int i = 0; i < POOL; ++i) {
        if (s_onset[i].armed &&
            (int32_t)(now_ms - s_onset[i].due_ms) >= 0) {
            s_onset[i].armed = false;
            engine_note_on((uint8_t)i, s_onset[i].freq, s_onset[i].amp);
            if (s_onset[i].sparkle > 0.0f)      /* r19.30: extension shimmer */
                engine_sparkle_strike(s_onset[i].freq, s_onset[i].sparkle);
        }
    }
}

void bloom_all_off(void) {
    for (int i = 0; i < POOL; ++i) {
        s_onset[i].armed = false;
        engine_note_off((uint8_t)i);
    }
    s_prev_n      = 0;
    s_active_cell = -1;
    s_hold        = false;
    /* s_have_prev / s_prev_pitch bleiben: der naechste Akkord soll
     * weiterhin ins zuletzt gehoerte Register voice-leaden. */
}

int bloom_pending(void) {
    int n = 0;
    for (int i = 0; i < POOL; ++i) if (s_onset[i].armed) ++n;
    return n;
}

int bloom_active_cell(void) { return s_active_cell; }
int bloom_centroid(void) {
    if (s_prev_n <= 0) return ANCHOR;
    int sum = 0;
    for (int i = 0; i < s_prev_n; ++i) sum += s_prev_pitch[i];
    return sum / s_prev_n;
}

/* r19.29: read a sounding chord voice's pitch (for the display needles / tests);
 * -1 when that slot is silent. */
int bloom_voice_pitch(int i) {
    return (i >= 0 && i < s_prev_n) ? s_prev_pitch[i] : -1;
}
int bloom_voice_count(void) { return s_prev_n; }
