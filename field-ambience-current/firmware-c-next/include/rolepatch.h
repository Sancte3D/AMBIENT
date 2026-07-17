#ifndef FAM_ROLEPATCH_H
#define FAM_ROLEPATCH_H

/*
 * rolepatch.c — role-based chord voicing timbre (r19.30).
 *
 * The insight from HiChord/Orchid (clean-room, from the public editors — no
 * firmware/preset/sample used): a chord sounds ARRANGED, not like "four
 * pressed synth keys", when each note plays a different role with its own
 * balance and colour. Until now every HARMONY voice went through the same pad
 * patch. This module assigns each voiced note a ROLE by its position in the
 * chord and returns how that role should sound, reusing the engines we already
 * have (pad body + glass shimmer):
 *
 *   ROOT   lowest  — full warm body
 *   THIRD  inner   — quieter, darker (sits under the root)
 *   FIFTH  inner   — pulled back, wide
 *   EXT    highest — quieter body PLUS a glass shimmer that blooms into the
 *                    hall (the "magic" top the FM extensions give)
 *
 * v1 wires `gain` (scales the sustained pad voice) and `sparkle_gain` (>0 also
 * fires a glass bloom for that voice). The remaining chat fields — per-role
 * cutoff, pan, envelope, reverb send, and a control-rate mod matrix — are
 * reserved for r19.32+ so this stays a small, reuse-only step. Pure data +
 * integer role mapping; host-testable.
 */

typedef enum { ROLE_ROOT = 0, ROLE_THIRD, ROLE_FIFTH, ROLE_EXT, ROLE_COUNT } chord_role_t;

typedef struct {
    float gain;          /* scales the sustained pad voice for this role     */
    float sparkle_gain;  /* >0: also bloom a glass tone (extension shimmer)   */
    /* reserved (r19.32+): cutoff, pan, attack, release, reverb_send, mod ... */
} role_patch_t;

/* The role of voice `i` in an n-note voicing: lowest = ROOT, highest = EXT,
 * the ones between = THIRD then FIFTH. */
chord_role_t rolepatch_role_for(int i, int n);

/* The patch for a role. */
const role_patch_t *rolepatch_get(chord_role_t role);

#endif
