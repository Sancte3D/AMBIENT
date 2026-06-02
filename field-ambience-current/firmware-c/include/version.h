#ifndef FIELD_AMBIENCE_VERSION_H
#define FIELD_AMBIENCE_VERSION_H

/* Bump on each named milestone. Step 11 = famReverbMaster (Freeverb-style)
 * + engine mix-bus. Engine-Port-Reihenfolge ist 9→11→10→8→12 (hörbarkeits-
 * first), siehe NATIVE_PORT_PLAN.md. */
#define FAM_FW_VERSION_STR  "field-ambience native v0.9-dev step11"

/* Board target — sanity-checked at runtime against the SDK board macro. */
#define FAM_FW_BOARD_STR    "pico2 (RP2350)"

#endif
