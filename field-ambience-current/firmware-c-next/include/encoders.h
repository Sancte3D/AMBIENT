#ifndef FAM_ENCODERS_H
#define FAM_ENCODERS_H

/*
 * 4× EC11 rotary-encoder bank — C port of firmware/encoders.py.
 *
 * Pin map per SPEC v0.6 §5 / firmware/config.py ENCODERS:
 *   EN1 "drive"   A=GP10 B=GP11 SW=GP12  id=1
 *   EN2 "bright"  A=GP13 B=GP14 SW=GP15  id=2
 *   EN3 "display" A=GP16 B=GP17 SW=GP18  id=3   (the menu encoder)
 *   EN4 "volume"  A=GP19 B=GP20 SW=GP21  id=4
 *
 * Hardware debounce (100 nF + 10 kΩ = 1 ms RC) lives on the PCB; firmware
 * adds a 4-state-quadrature decode + per-detent accumulation (EC11 produces
 * 4 quadrature transitions per mechanical click → we emit ±1 per detent).
 *
 * Sampling runs from a 1 kHz repeating timer (independent of the OLED /
 * MCP service loop). Events are queued into a small ring buffer; main
 * loop drains via enc_pop_event().
 *
 * The MicroPython ENCODERS table's `dir` field is honoured: +1 or -1 flips
 * the rotation sense to match how the panel is wired/mounted. Tweak in
 * the EncDef table inside encoders.c when bringing up real hardware.
 */

#include <stdbool.h>
#include <stdint.h>

#define ENC_COUNT 4

typedef enum {
    ENC_EVENT_NONE   = 0,
    ENC_EVENT_ROTATE = 1,   /* value field = signed +1 or -1 per detent */
    ENC_EVENT_PUSH   = 2,   /* value field = 1 down, 0 up (debounced) */
} enc_event_kind_t;

typedef struct {
    enc_event_kind_t kind;
    uint8_t          id;     /* 1..4 */
    int8_t           value;  /* delta for ROTATE, down/up for PUSH */
} enc_event_t;

/* Configure GPIOs (input + pull-up) for all 4 encoders and start the
 * 1 kHz polling timer. Call once during boot. */
void enc_init(void);

/* Pop one event from the ring buffer. Returns false if no event waiting.
 * Call from the main loop (no need to call from an IRQ). */
bool enc_pop_event(enc_event_t *out);

/* Current per-encoder accumulated detent count since boot (signed).
 * Convenient for the Step 4 OLED display; later steps replace this with
 * menu-state hooks. */
int32_t enc_position(uint8_t id);

/* Current debounced push-switch state (true = pressed). */
bool enc_pushed(uint8_t id);

#endif
