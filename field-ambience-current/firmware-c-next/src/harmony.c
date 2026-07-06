/*
 * harmony.c — the harmonic safety core. See harmony.h for the model.
 */

#include "harmony.h"

/* --- the two pitch worlds (relative pitch classes) ----------------------- *
 * Major: core = major pentatonic, color = maj7 (high register only).
 * Minor: core = minor pentatonic, color = the 9th.
 * Neither core contains a semitone step or a tritone — the aggressive
 * clash classes are structurally out of the material. */
static const int CORE_MAJOR[5] = { 0, 2, 4, 7, 9 };
static const int CORE_MINOR[5] = { 0, 3, 5, 7, 10 };
#define COLOR_MAJOR 11
#define COLOR_MINOR 2

/* --- curated harmonic states (bass pc + 5 chord pcs, relative) ----------- *
 * Minor world (the user's D-world example, generalized):
 *   i9        D F A C E    bass 0
 *   bIII maj9 F A C E G    bass 3
 *   bVII 6/9  C D E G A    bass 10
 *   iv 6/9sus G A C D E    bass 5
 * Consecutive states share ≥ 4 pitch classes by construction.
 * Major world: all four states share ALL FIVE core pcs — only the bass
 * reinterprets them (C6/9 → Am11 → Gsus4/6/9 → Dsus2/4/7 shape). */
typedef struct { int bass; int pc[5]; } hstate_t;

static const hstate_t STATES_MINOR[4] = {
    { 0,  { 0, 3, 7, 10, 2 } },
    { 3,  { 3, 7, 10, 2, 5 } },
    { 10, { 10, 0, 2, 5, 7 } },
    { 5,  { 5, 7, 10, 0, 2 } },
};
static const hstate_t STATES_MAJOR[4] = {
    { 0, { 0, 2, 4, 7, 9 } },
    { 9, { 9, 0, 2, 4, 7 } },
    { 7, { 7, 9, 0, 2, 4 } },
    { 2, { 2, 4, 7, 9, 0 } },
};

/* --- register rules ------------------------------------------------------ */
#define REG_BASS_LO   38            /* bass lives 38..49 (D2..C#3)        */
#define REG_BASS_HI   49
#define REG_VOICE_LO  55            /* upper voices ~G3..G5, wide         */
#define REG_VOICE_HI  79
#define REG_MELODY_LO 62
#define REG_MELODY_HI 86
#define REG_COLOR_MIN 60            /* color pc only above C4             */
#define REG_SECONDS_MIN 60          /* 2nds rejected below C4             */

/* --- state --------------------------------------------------------------- */
static int      tonic_pc   = 2;     /* default D                          */
static int      is_minor   = 1;
static const hstate_t *table = STATES_MINOR;
static const int *core     = CORE_MINOR;
static int      color_pc   = COLOR_MINOR;

static int      st_idx;
static int      st_changes;
static int      st_common;          /* pcs kept on last mutation          */
static int      st_moved;           /* voices moved on last mutation      */
static int      voices[HARMONY_VOICES];
static uint32_t until_ms;
static int      timing_valid;
static uint32_t rng = 0x19A0C0DEu;

static float rnd01(void) {
    rng = rng * 1664525u + 1013904223u;
    return (float)(rng >> 8) / 16777216.0f;
}

static int wrap12(int v) { v %= 12; if (v < 0) v += 12; return v; }

/* interval class 0..6 (mod-12 distance folded) */
static int iclass(int a, int b) {
    int d = wrap12(a - b);
    return d > 6 ? 12 - d : d;
}

/* absolute pc of relative pc r in the current world */
static int abs_pc(int r) { return wrap12(tonic_pc + r); }

/* nearest MIDI note of pitch class `pc` to `around`, clamped to a band */
static int nearest_in_band(int pc, int around, int lo, int hi) {
    int base = around - wrap12(around - pc);          /* ≤ around           */
    int cand = (wrap12(around - pc) <= 6) ? base : base + 12;
    while (cand < lo) cand += 12;
    while (cand > hi) cand -= 12;
    if (cand < lo) cand = lo + wrap12(pc - lo);       /* band < octave safety */
    return cand;
}

/* --- voicing -------------------------------------------------------------
 * Initial voicing: spread the state's non-bass pcs wide across the voice
 * band (the bass pc itself lives in the bass register). On mutation:
 * voices whose pc survives KEEP THEIR PITCH; the others move to the
 * nearest new pc (minimal movement, no duplicate pcs). */
static void voice_initial(void) {
    const hstate_t *s = &table[st_idx];
    /* four upper voices from the state's pcs (skip index 0 = bass pc) */
    static const int anchor[HARMONY_VOICES] = { 57, 62, 67, 74 };
    for (int v = 0; v < HARMONY_VOICES; ++v)
        voices[v] = nearest_in_band(abs_pc(s->pc[v + 1]), anchor[v],
                                    REG_VOICE_LO, REG_VOICE_HI);
}

static int pc_in_state(int apc, const hstate_t *s) {
    for (int i = 0; i < 5; ++i)
        if (abs_pc(s->pc[i]) == apc) return 1;
    return 0;
}

static void mutate_to(int next) {
    const hstate_t *from = &table[st_idx];
    const hstate_t *to   = &table[next];

    /* count common pitch classes (observability + the ≥3 contract) */
    int common = 0;
    for (int i = 0; i < 5; ++i)
        if (pc_in_state(abs_pc(from->pc[i]), to)) ++common;
    st_common = common;

    /* voices: keep survivors frozen, move the rest minimally */
    int moved = 0;
    for (int v = 0; v < HARMONY_VOICES; ++v) {
        int apc = wrap12(voices[v]);
        if (pc_in_state(apc, to)) continue;           /* common tone: freeze */
        /* nearest new pc not already voiced */
        int best = -1, bestd = 99;
        for (int i = 0; i < 5; ++i) {
            int npc = abs_pc(to->pc[i]);
            int dup = 0;
            for (int u = 0; u < HARMONY_VOICES; ++u)
                if (u != v && wrap12(voices[u]) == npc) dup = 1;
            if (dup) continue;
            int d = iclass(npc, apc);
            if (d < bestd) { bestd = d; best = npc; }
        }
        if (best >= 0) {
            int up   = voices[v] + wrap12(best - voices[v]);
            int down = up - 12;
            int cand = (up - voices[v] <= voices[v] - down) ? up : down;
            if (cand < REG_VOICE_LO) cand += 12;
            if (cand > REG_VOICE_HI) cand -= 12;
            voices[v] = cand;
            ++moved;
        }
    }
    st_moved = moved;
    st_idx   = next;
    ++st_changes;
}

/* --- public -------------------------------------------------------------- */

void harmony_init(void) {
    tonic_pc = 2; is_minor = 1;
    table = STATES_MINOR; core = CORE_MINOR; color_pc = COLOR_MINOR;
    st_idx = 0; st_changes = 0; st_common = 5; st_moved = 0;
    until_ms = 0; timing_valid = 0;
    rng = 0x19A0C0DEu;
    voice_initial();
}

void harmony_set_world(int tonic_midi, int minor) {
    tonic_pc = wrap12(tonic_midi);
    is_minor = minor ? 1 : 0;
    table    = is_minor ? STATES_MINOR : STATES_MAJOR;
    core     = is_minor ? CORE_MINOR   : CORE_MAJOR;
    color_pc = is_minor ? COLOR_MINOR  : COLOR_MAJOR;
    st_idx = 0; st_common = 5; st_moved = 0;
    timing_valid = 0;
    voice_initial();
}

void harmony_advance(void) {
    /* neighbor walk: mostly ±1 through the curated cycle, sometimes
     * across — every pair already satisfies the common-tone contract */
    int step = (rnd01() < 0.70f) ? 1 : 2;
    if (rnd01() < 0.5f) step = -step;
    mutate_to((st_idx + step + 4) & 3);
}

void harmony_tick(uint32_t now_ms) {
    if (!timing_valid) {
        until_ms = now_ms + 24000u + (uint32_t)(rnd01() * 24000.0f);
        timing_valid = 1;
        return;
    }
    if ((int32_t)(now_ms - until_ms) >= 0) {
        harmony_advance();
        until_ms = now_ms + 24000u + (uint32_t)(rnd01() * 24000.0f);
    }
}

int harmony_state_index(void)       { return st_idx; }
int harmony_state_changes(void)     { return st_changes; }
int harmony_last_common_tones(void) { return st_common; }
int harmony_last_moved_voices(void) { return st_moved; }

int harmony_bass_midi(void) {
    int pc = abs_pc(table[st_idx].bass);
    int m  = REG_BASS_LO + wrap12(pc - REG_BASS_LO);
    if (m > REG_BASS_HI) m -= 12;
    if (m < REG_BASS_LO) m += 12;   /* band is exactly one octave — safe */
    return m;
}

int harmony_fifth_midi(void) { return harmony_bass_midi() + 7; }

int harmony_voices(int *out, int max) {
    int n = max < HARMONY_VOICES ? max : HARMONY_VOICES;
    for (int i = 0; i < n; ++i) out[i] = voices[i];
    return n;
}

int harmony_in_world(int midi) {
    int pc = wrap12(midi - tonic_pc);
    for (int i = 0; i < 5; ++i)
        if (core[i] == pc) return 1;
    if (pc == color_pc) return midi >= REG_COLOR_MIN;
    return 0;
}

int harmony_collision_ok(int midi, const int *sustained, int n_sus) {
    for (int i = 0; i < n_sus; ++i) {
        int ic = iclass(midi, sustained[i]);
        if (ic == 1 || ic == 6) return 0;
        if (ic == 2 && (midi < REG_SECONDS_MIN ||
                        sustained[i] < REG_SECONDS_MIN)) return 0;
    }
    return 1;
}

/* melody candidates around `from`, best first, per the interval table:
 * repeat · scale neighbors · fourth/fifth · sixth/octave · color */
static int build_candidates(int from, float p_color, int *cand) {
    int n = 0;
    float r = rnd01();

    /* the melodic universe: core pcs in the melody band + the color pc
     * above REG_COLOR_MIN. Collect the scale as sorted midi around from. */
    int scale[40]; int ns = 0;
    for (int m = REG_MELODY_LO; m <= REG_MELODY_HI && ns < 40; ++m)
        if (harmony_in_world(m)) scale[ns++] = m;
    if (ns == 0) return 0;

    /* index of `from` (or nearest) in the scale */
    int fi = 0, fd = 999;
    for (int i = 0; i < ns; ++i) {
        int d = scale[i] > from ? scale[i] - from : from - scale[i];
        if (d < fd) { fd = d; fi = i; }
    }

    /* ordered preference by the dice roll (quality gate comes AFTER —
     * every candidate is appended so a rejected head falls through) */
    int up = rnd01() < 0.5f ? 1 : -1;
    if (r < 0.30f) {                       /* repeat */
        cand[n++] = scale[fi];
    } else if (r < 0.65f) {                /* neighbor step */
        if (fi + up >= 0 && fi + up < ns)  cand[n++] = scale[fi + up];
        if (fi - up >= 0 && fi - up < ns)  cand[n++] = scale[fi - up];
    } else if (r < 0.85f) {                /* fourth / fifth */
        for (int i = 0; i < ns; ++i) {
            int d = scale[i] > from ? scale[i] - from : from - scale[i];
            if (d == 5 || d == 7) cand[n++] = scale[i];
            if (n >= 4) break;
        }
    } else if (r < 0.95f) {                /* sixth / octave */
        for (int i = 0; i < ns; ++i) {
            int d = scale[i] > from ? scale[i] - from : from - scale[i];
            if (d == 9 || d == 12) cand[n++] = scale[i];
            if (n >= 4) break;
        }
    }
    if (rnd01() < p_color) {               /* color note, high register */
        for (int m = REG_MELODY_HI; m >= REG_COLOR_MIN; --m)
            if (wrap12(m - tonic_pc) == color_pc &&
                m >= REG_MELODY_LO) { cand[n++] = m; break; }
    }
    /* fallback ladder: everything near `from`, closest first */
    for (int d = 0; d <= 12 && n < 24; ++d) {
        if (fi + d < ns)  cand[n++] = scale[fi + d];
        if (d > 0 && fi - d >= 0) cand[n++] = scale[fi - d];
    }
    return n;
}

int harmony_melody_next(int last_midi, const int *sustained, int n_sus,
                        float p_color) {
    int from = last_midi;
    if (from < REG_MELODY_LO || from > REG_MELODY_HI) {
        /* phrase opening: start near the top harmony voice */
        from = voices[HARMONY_VOICES - 1];
        while (from < REG_MELODY_LO) from += 12;
        while (from > REG_MELODY_HI) from -= 12;
    }
    int cand[24];
    int n = build_candidates(from, p_color, cand);
    for (int i = 0; i < n; ++i) {
        if (!harmony_in_world(cand[i])) continue;
        if (harmony_collision_ok(cand[i], sustained, n_sus))
            return cand[i];                /* first safe = next best      */
    }
    return -1;                             /* nothing safe → silence      */
}
