/*
 * Harmonic brain — Step 12a. Port of the webapp's harmony functions.
 *
 * chordAtDegree(root, mode, degree, family):
 *   intervals = SCALES[mode] (7 scale steps in semitones)
 *   for each family step (scaleStepOffset [, chromaticSemis]):
 *     idx  = (degree-1) + scaleStepOffset
 *     note = root + intervals[idx mod 7] + 12*floor(idx/7) [+ chromaticSemis]
 *
 * voiceCentered(chord): shift the whole chord by whole octaves so its mean
 * sits near MIDI 64, keeping every voicing in a consistent register.
 */

#include "brain.h"

/* 6 church modes, semitone intervals over the octave. */
static const int8_t SCALES[BRAIN_MODE_COUNT][7] = {
    { 0, 2, 4, 5, 7, 9, 11 },   /* ionian     */
    { 0, 2, 3, 5, 7, 9, 10 },   /* dorian     */
    { 0, 1, 3, 5, 7, 8, 10 },   /* phrygian   */
    { 0, 2, 4, 6, 7, 9, 11 },   /* lydian     */
    { 0, 2, 4, 5, 7, 9, 10 },   /* mixolydian */
    { 0, 2, 3, 5, 7, 8, 10 },   /* aeolian    */
};

/* A chord-family step: a scale-step offset plus an optional chromatic
 * alteration in semitones (the webapp's [step, semi] form). */
typedef struct { int8_t step; int8_t semi; } cstep_t;

/* vibe → chord family: warm=add9, bright=maj7, deep=min11, floating=sus2. */
static const cstep_t FAM_ADD9 [4] = { {0,0},{2,0},{4,0},{8,0} };
static const cstep_t FAM_MAJ7 [4] = { {0,0},{2,0},{4,0},{6,0} };
static const cstep_t FAM_MIN11[6] = { {0,0},{2,0},{4,0},{6,0},{8,0},{10,0} };
static const cstep_t FAM_SUS2 [3] = { {0,0},{1,0},{4,0} };

static const cstep_t *VIBE_FAMILY[BRAIN_VIBE_COUNT] = {
    FAM_ADD9, FAM_MAJ7, FAM_MIN11, FAM_SUS2
};
static const int VIBE_FAMILY_N[BRAIN_VIBE_COUNT] = { 4, 4, 6, 3 };

#define VOICE_CENTER 64

static int s_key  = 60;     /* C4 */
static int s_mode = 0;      /* ionian */
static int s_vibe = 0;      /* warm → add9 */

void brain_init(void) {
    s_key = 60; s_mode = 0; s_vibe = 0;
}

static int clampi(int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); }

/* Floor-division and floor-mod for n=7 (idx can be ≥ 7 for high family steps;
 * it is never negative here, but be correct anyway). */
static int fmod7(int a) { int m = a % 7; return m < 0 ? m + 7 : m; }
static int fdiv7(int a) { return (a - fmod7(a)) / 7; }

void brain_set_key(int tonic_midi) { s_key  = clampi(tonic_midi, 0, 127); }
void brain_set_mode(int mode_idx)  { s_mode = clampi(mode_idx, 0, BRAIN_MODE_COUNT - 1); }
void brain_set_vibe(int vibe_idx)  { s_vibe = clampi(vibe_idx, 0, BRAIN_VIBE_COUNT - 1); }

int brain_get_mode(void) { return s_mode; }
int brain_get_vibe(void) { return s_vibe; }
int brain_get_key(void)  { return s_key; }

/* Unvoiced chord at a degree (1-indexed) for the current key/mode/vibe. */
static int chord_at_degree(int degree, int *out, int max) {
    const int8_t *iv  = SCALES[s_mode];
    const cstep_t *fam = VIBE_FAMILY[s_vibe];
    int n = VIBE_FAMILY_N[s_vibe];
    int d = degree - 1;

    int count = 0;
    for (int i = 0; i < n && count < max; ++i) {
        int idx  = d + fam[i].step;
        int note = s_key + iv[fmod7(idx)] + 12 * fdiv7(idx) + fam[i].semi;
        out[count++] = note;
    }
    return count;
}

/* Shift the chord by whole octaves so its mean is near VOICE_CENTER. */
static void voice_centered(int *chord, int n) {
    if (n <= 0) return;
    int sum = 0;
    for (int i = 0; i < n; ++i) sum += chord[i];
    float mean = (float)sum / (float)n;
    /* round((CENTER - mean)/12) * 12 */
    float steps = (VOICE_CENTER - mean) / 12.0f;
    int shift = (int)(steps < 0 ? steps - 0.5f : steps + 0.5f) * 12;
    for (int i = 0; i < n; ++i) chord[i] += shift;
}

int brain_chord(int degree, int *out_midi, int max) {
    int n = chord_at_degree(degree, out_midi, max);
    voice_centered(out_midi, n);
    return n;
}

int brain_cell_root(int cell) {
    int chord[BRAIN_MAX_CHORD];
    int n = brain_chord(cell + 1, chord, BRAIN_MAX_CHORD);
    if (n <= 0) return s_key;
    int lo = chord[0];
    for (int i = 1; i < n; ++i) if (chord[i] < lo) lo = chord[i];
    return lo;
}
