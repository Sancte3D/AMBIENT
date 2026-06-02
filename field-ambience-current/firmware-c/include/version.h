#ifndef FIELD_AMBIENCE_VERSION_H
#define FIELD_AMBIENCE_VERSION_H

/* Bump on each named milestone. Step 10 = famTexture noise bed (built after
 * Step 11 per the hörbarkeits-first order 9→11→10→8→12, see
 * NATIVE_PORT_PLAN.md). */
#define FAM_FW_VERSION_STR  "field-ambience native v0.9-dev step10"

/* Board target — sanity-checked at runtime against the SDK board macro. */
#define FAM_FW_BOARD_STR    "pico2 (RP2350)"

#endif
