/* SCN7 product catalogue. Shared user parameters survive World changes.
 * HIGHLANDS is a source candidate, not a hearing-approved product promise. */
#include "worlds.h"
#include "world_grammar.h"
_Static_assert(WORLD_COUNT == CORE_WORLD_COUNT, "Catalogue/score disagree");
static const world_t worlds[WORLD_COUNT] = {
    { .name="COAST", .subtitle="tones pass across open water",
      .accent_r=154,.accent_g=205,.accent_b=211,
      .space_pct=50,.atmos_pct=0,.motion_pct=50,.key_midi=50 },
    { .name="WOODLAND", .subtitle="small figures answer",
      .accent_r=157,.accent_g=193,.accent_b=157,
      .space_pct=50,.atmos_pct=0,.motion_pct=50,.key_midi=50 },
    { .name="HIGHLANDS", .subtitle="fragments leave open space",
      .accent_r=196,.accent_g=186,.accent_b=164,
      .space_pct=50,.atmos_pct=0,.motion_pct=50,.key_midi=50 }
};
const world_t *worlds_get(int i) {
    if(i<0)i=0;
    if(i>=WORLD_COUNT)i=WORLD_COUNT-1;
    return &worlds[i];
}
int worlds_count(void) { return WORLD_COUNT; }
/* Legacy query is never the product scheduler. */
const world_phrase_t *worlds_phrase(int i) {
    static const world_phrase_t p[WORLD_COUNT]={{6,14,4,10,35},{2,3,6,16,30},{2,4,8,20,20}};
    if(i<0)i=0;
    if(i>=WORLD_COUNT)i=WORLD_COUNT-1;
    return &p[i];
}
