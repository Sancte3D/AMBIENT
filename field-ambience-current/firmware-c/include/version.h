#ifndef FIELD_AMBIENCE_VERSION_H
#define FIELD_AMBIENCE_VERSION_H

/* This is the FROZEN HÖRTEST build (firmware-c/). Stable Step 12a + the
 * DEMO_AUTOPLAY switch in main.c. Use this folder to flash a Pico for the
 * listening test (see hoertest/HOERTEST.html). Active development of
 * Step 12b and beyond lives in the sibling folder firmware-c-next/. */
#define FAM_FW_VERSION_STR  "field-ambience native v0.9-dev step12a · hoertest"

/* Board target — sanity-checked at runtime against the SDK board macro. */
#define FAM_FW_BOARD_STR    "pico2 (RP2350)"

#endif
