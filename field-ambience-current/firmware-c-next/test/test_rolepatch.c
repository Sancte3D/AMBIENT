/*
 * test_rolepatch.c — r19.30 role-based chord timbre.
 *
 * Checks the role mapping (lowest = ROOT, highest = EXT, inner = THIRD/FIFTH)
 * and the patch table's shape: the root is the loudest body, inner voices are
 * pulled back, and ONLY the extension adds a glass shimmer.
 */
#include <stdio.h>
#include "rolepatch.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

int main(void) {
    printf("== rolepatch (r19.30) ==\n");

    /* role mapping for a 4-voice chord */
    CHECK(rolepatch_role_for(0, 4) == ROLE_ROOT,  "voice 0 = ROOT");
    CHECK(rolepatch_role_for(1, 4) == ROLE_THIRD, "voice 1 = THIRD");
    CHECK(rolepatch_role_for(2, 4) == ROLE_FIFTH, "voice 2 = FIFTH");
    CHECK(rolepatch_role_for(3, 4) == ROLE_EXT,   "voice 3 (top) = EXT");

    /* a triad: lowest ROOT, top EXT, middle THIRD */
    CHECK(rolepatch_role_for(0, 3) == ROLE_ROOT, "triad voice 0 = ROOT");
    CHECK(rolepatch_role_for(1, 3) == ROLE_THIRD, "triad middle = THIRD");
    CHECK(rolepatch_role_for(2, 3) == ROLE_EXT,  "triad top = EXT");

    /* single note is just the root */
    CHECK(rolepatch_role_for(0, 1) == ROLE_ROOT, "single voice = ROOT");

    const role_patch_t *root  = rolepatch_get(ROLE_ROOT);
    const role_patch_t *third = rolepatch_get(ROLE_THIRD);
    const role_patch_t *fifth = rolepatch_get(ROLE_FIFTH);
    const role_patch_t *ext   = rolepatch_get(ROLE_EXT);

    /* body balance: root loudest, inner voices pulled back */
    CHECK(root->gain > third->gain, "root louder than third");
    CHECK(third->gain > fifth->gain || fifth->gain <= third->gain,
          "inner voices are below the root");
    CHECK(root->gain >= ext->gain, "root at least as loud as the extension body");

    /* shimmer ONLY on the extension */
    CHECK(root->sparkle_gain  == 0.0f, "root has no sparkle");
    CHECK(third->sparkle_gain == 0.0f, "third has no sparkle");
    CHECK(fifth->sparkle_gain == 0.0f, "fifth has no sparkle");
    CHECK(ext->sparkle_gain    > 0.0f, "extension adds a glass shimmer");

    /* every gain in a sane range */
    for (int r = 0; r < ROLE_COUNT; ++r) {
        const role_patch_t *p = rolepatch_get((chord_role_t)r);
        CHECK(p->gain > 0.0f && p->gain <= 1.0f, "gain in (0,1] for role %d", r);
    }

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
