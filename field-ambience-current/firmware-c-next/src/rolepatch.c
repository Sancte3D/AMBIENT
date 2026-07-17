/*
 * rolepatch.c — role-based chord timbre. Behaviour: see rolepatch.h.
 */

#include "rolepatch.h"

/* v1 patch table. gain shapes the sustained pad body per role; sparkle_gain
 * adds a glass bloom (only the extension gets it → shimmer stays off the bass
 * and root, exactly where a bright top helps and a dark bottom must stay
 * clean). Tuned by ear against the four-voice HARMONY chord. */
static const role_patch_t TABLE[ROLE_COUNT] = {
    /* ROOT  */ { 1.00f, 0.00f },   /* full warm body                     */
    /* THIRD */ { 0.68f, 0.00f },   /* quieter, darker under the root      */
    /* FIFTH */ { 0.58f, 0.00f },   /* pulled back                         */
    /* EXT   */ { 0.52f, 0.42f },   /* soft body + a glass shimmer on top  */
};

chord_role_t rolepatch_role_for(int i, int n) {
    if (n <= 0) return ROLE_ROOT;
    if (i <= 0)      return ROLE_ROOT;
    if (i >= n - 1)  return ROLE_EXT;      /* highest voice = extension    */
    if (i == 1)      return ROLE_THIRD;
    return ROLE_FIFTH;                      /* any inner voice above the 3rd */
}

const role_patch_t *rolepatch_get(chord_role_t role) {
    if (role < 0 || role >= ROLE_COUNT) role = ROLE_ROOT;
    return &TABLE[role];
}
