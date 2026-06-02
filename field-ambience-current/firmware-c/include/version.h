#ifndef FIELD_AMBIENCE_VERSION_H
#define FIELD_AMBIENCE_VERSION_H

/* Bump on each named milestone. Step 8 = famSubBass + famDeepBass (built per
 * the hörbarkeits-first order 9→11→10→8→12). All four audio layers are now
 * in the engine; only Step 12 (harmonic brain + USB-MIDI) remains. */
#define FAM_FW_VERSION_STR  "field-ambience native v0.9-dev step8"

/* Board target — sanity-checked at runtime against the SDK board macro. */
#define FAM_FW_BOARD_STR    "pico2 (RP2350)"

#endif
