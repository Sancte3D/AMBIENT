/*
 * MIDI Out — Step 12b #6, PIO-UART hardware layer (DEVICE ONLY).
 *
 * Drains the midi.c software FIFO into a PIO state machine that transmits 8N1
 * UART at 31250 baud on GP21 → MIDI_TX → R220 → TRS Tip (SPEC §8 r15).
 *
 * NOTE: this file is part of the Pico SDK build only (it needs hardware/pio.h
 * + the generated midi_tx.pio.h). It is NOT compiled by the host unit tests —
 * the message logic it carries (midi.c) is what's host-tested. This layer is
 * a thin pump and has been verified by inspection against the SDK uart_tx
 * example, not by host tests.
 */

#include "midi.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "midi_tx.pio.h"          /* generated from src/midi_tx.pio by CMake */

#define MIDI_BAUD     31250
#define MIDI_TX_PIN   21          /* GP21 = MIDI_TX (SPEC §5 r15) */

static PIO  midi_pio = pio1;      /* PIO0 carries the I²S audio; use PIO1 */
static uint midi_sm;
static bool midi_hw_ready = false;

void midi_hw_init(void) {
    /* Claim a state machine + load the program on PIO1. */
    uint offset = pio_add_program(midi_pio, &midi_tx_program);
    midi_sm = (uint)pio_claim_unused_sm(midi_pio, true);
    midi_tx_program_init(midi_pio, midi_sm, offset, MIDI_TX_PIN, MIDI_BAUD);
    midi_hw_ready = true;
}

/* Move queued bytes from the software FIFO into the PIO TX FIFO while there is
 * room. Call frequently from the main loop — it never blocks: it stops as soon
 * as the PIO FIFO is full and leaves the rest in the software FIFO. */
void midi_hw_pump(void) {
    if (!midi_hw_ready) return;
    while (!pio_sm_is_tx_fifo_full(midi_pio, midi_sm)) {
        int b = midi_pop_byte();
        if (b < 0) break;                       /* nothing queued */
        /* The PIO program pulls a 32-bit word and shifts out 8 bits LSB-first;
         * place the byte in the low 8 bits. */
        pio_sm_put(midi_pio, midi_sm, (uint32_t)b);
    }
}
