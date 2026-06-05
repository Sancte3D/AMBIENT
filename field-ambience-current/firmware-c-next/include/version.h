#ifndef FIELD_AMBIENCE_VERSION_H
#define FIELD_AMBIENCE_VERSION_H

/* This is the ACTIVE DEVELOPMENT build (firmware-c-next/). Step 12b lands
 * here piece by piece. The sibling folder firmware-c/ is the frozen hörtest
 * snapshot; if anything breaks here, fall back to that one. */
#define FAM_FW_VERSION_STR  "field-ambience native v0.9-dev step12b-wip"

/* Board target — sanity-checked at runtime against the SDK board macro. */
#define FAM_FW_BOARD_STR    "pico2 (RP2350)"

#endif
