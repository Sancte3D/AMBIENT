/*
 * bloom.c — Chord-Bloom-Zellmodus. Verhalten: siehe bloom.h.
 */

#include "bloom.h"
#include "engine.h"
#include "brain.h"
#include "tuning.h"

#define POOL      5          /* Chord-Sources 0..4 (= Base-Cell-Sources) */
#define STEP_MS   100u       /* Grund-Abstand zwischen Akkordtoenen      */
#define JITTER_MS 70u        /* + 0..70 ms (deterministisch) pro Ton      */

typedef struct {
    bool     armed;          /* wartet auf seinen Einsatz  */
    uint32_t due_ms;
    float    freq;
    float    amp;
} onset_t;

static onset_t s_onset[POOL];
static int     s_prev_n;      /* Anzahl Toene des aktuell klingenden Akkords */
static int     s_prev_centroid;
static bool    s_have_prev;
static int8_t  s_active_cell; /* -1 = keiner                                */
static bool    s_hold;        /* Akkord gelatcht (HOLD beim Druck)          */
static uint32_t s_rng;

void bloom_init(void) {
    for (int i = 0; i < POOL; ++i) s_onset[i].armed = false;
    s_prev_n = 0;
    s_prev_centroid = 0;
    s_have_prev = false;
    s_active_cell = -1;
    s_hold = false;
    s_rng = 0x9E3779B9u;
}

static uint32_t jitter(void) {          /* 0..JITTER_MS, deterministisch */
    s_rng = s_rng * 1664525u + 1013904223u;
    return (s_rng >> 9) % (JITTER_MS + 1u);
}

/* Voice-Leading: brain_chord zentriert um VOICE_CENTER. Den ganzen neuen
 * Akkord um ganze Oktaven verschieben, so dass sein Schwerpunkt dem des
 * vorherigen Akkords am naechsten liegt — kuerzester Weg auf Block-Ebene,
 * ohne Stimmkreuzungen (die brain-interne Voicing bleibt erhalten). */
static void voice_lead(int *chord, int n) {
    if (n <= 0 || !s_have_prev) return;
    int sum = 0;
    for (int i = 0; i < n; ++i) sum += chord[i];
    int centroid = sum / n;
    /* naechste Oktav-Verschiebung k*12, die |centroid + k*12 - prev| minimiert */
    int k = (s_prev_centroid - centroid);
    k = (k >= 0) ? (k + 6) / 12 : -(( -k + 6) / 12);
    int shift = k * 12;
    for (int i = 0; i < n; ++i) chord[i] += shift;
}

void bloom_press(uint8_t cell, float velocity_amp, bool hold, uint32_t now_ms) {
    if (cell >= POOL) return;

    int chord[BRAIN_MAX_CHORD];
    int n = brain_chord((int)cell + 1, chord, BRAIN_MAX_CHORD);
    if (n <= 0) return;
    if (n > POOL) n = POOL;              /* Voice-Budget: max 5 Toene */

    voice_lead(chord, n);

    /* Wurde der Akkord kleiner als der vorherige, die ueberzaehligen
     * Pool-Stimmen freigeben (Voice-Leading laesst die uebrigen gleiten). */
    for (int i = n; i < s_prev_n; ++i) {
        engine_note_off((uint8_t)i);
        s_onset[i].armed = false;
    }

    /* Gestaffelte Einsaetze: erster Ton sofort, dann STEP+Jitter pro Ton.
     * note_on auf einer schon klingenden Source re-pitcht sie (Ramp) →
     * die Stimmen gleiten ins neue Voicing statt hart neu zu triggern. */
    uint32_t t = now_ms;
    for (int i = 0; i < n; ++i) {
        s_onset[i].armed  = true;
        s_onset[i].due_ms = t;
        s_onset[i].freq   = tuning_hz((float)chord[i]);
        s_onset[i].amp    = velocity_amp;
        t += STEP_MS + jitter();
    }

    int sum = 0;
    for (int i = 0; i < n; ++i) sum += chord[i];
    s_prev_centroid = sum / n;
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
    /* s_have_prev / s_prev_centroid bleiben: der naechste Akkord soll
     * weiterhin ins zuletzt gehoerte Register voice-leaden. */
}

int bloom_pending(void) {
    int n = 0;
    for (int i = 0; i < POOL; ++i) if (s_onset[i].armed) ++n;
    return n;
}

int bloom_active_cell(void) { return s_active_cell; }
int bloom_centroid(void)    { return s_prev_centroid; }
