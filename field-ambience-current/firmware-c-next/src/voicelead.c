/*
 * voicelead.c — minimal-movement chord voicing. Behaviour: see voicelead.h.
 *
 * With ≤4 voices we can afford the exact answer: try every assignment of the
 * target pitch classes to the voice slots, place each in the octave nearest
 * that slot's previous pitch, and keep the assignment with the least total
 * movement. Common tones fall out for free (a slot whose previous pitch is the
 * target's pitch class costs zero) and each kept voice stays in its own slot,
 * so the engine can glide rather than re-trigger.
 */

#include "voicelead.h"

#define VL_MAX 4

/* Octave of pitch class `pc` closest to `target`, clamped to a sane band. */
static int octave_near(int pc, int target) {
    pc = ((pc % 12) + 12) % 12;
    int best = pc + 60, bestd = 1 << 30;
    for (int oct = 3; oct <= 7; ++oct) {
        int cand = pc + 12 * oct;
        if (cand < 36 || cand > 95) continue;
        int d = cand - target; if (d < 0) d = -d;
        if (d < bestd) { bestd = d; best = cand; }
    }
    return best;
}

/* All permutations of n≤4 indices, iteratively (Heap's algorithm is overkill;
 * n is tiny so we just index a static table). */
static const unsigned char PERM4[24][4] = {
    {0,1,2,3},{0,1,3,2},{0,2,1,3},{0,2,3,1},{0,3,1,2},{0,3,2,1},
    {1,0,2,3},{1,0,3,2},{1,2,0,3},{1,2,3,0},{1,3,0,2},{1,3,2,0},
    {2,0,1,3},{2,0,3,1},{2,1,0,3},{2,1,3,0},{2,3,0,1},{2,3,1,0},
    {3,0,1,2},{3,0,2,1},{3,1,0,2},{3,1,2,0},{3,2,0,1},{3,2,1,0},
};

int voicelead(const int *prev, int prev_n,
              const int *pc, int n, int anchor, int *out) {
    if (n > VL_MAX) n = VL_MAX;
    if (n <= 0) return 0;

    if (prev_n <= 0) {
        int base = anchor - ((anchor % 12) + 12) % 12;   /* octave floor */
        int last = -1000;
        for (int i = 0; i < n; ++i) {
            int p = (((pc[i] % 12) + 12) % 12) + base;
            while (p <= last) p += 12;
            out[i] = p; last = p;
        }
        return n;
    }

    /* Target register for slots beyond the previous chord's size. */
    int best_out[VL_MAX];
    int best_cost = 1 << 30;
    for (int pi = 0; pi < 24; ++pi) {
        int cand[VL_MAX], cost = 0, ok = 1;
        for (int slot = 0; slot < n; ++slot) {
            int t = PERM4[pi][slot];
            if (t >= n) { ok = 0; break; }         /* skip perms that index past n */
            int target = slot < prev_n ? prev[slot] : anchor;
            int p = octave_near(pc[t], target);
            cand[slot] = p;
            int d = p - target; if (d < 0) d = -d;
            cost += d;
        }
        if (ok && cost < best_cost) {
            best_cost = cost;
            for (int i = 0; i < n; ++i) best_out[i] = cand[i];
        }
    }
    for (int i = 0; i < n; ++i) out[i] = best_out[i];
    return n;
}
