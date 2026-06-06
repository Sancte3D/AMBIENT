/*
 * MIDI Out — Step 12b #6, hardware-independent core.
 *
 * Builds standard MIDI 1.0 channel-voice messages and queues their bytes in a
 * bounded ring buffer that the PIO-UART (midi_pio.c, on-device only) drains at
 * 31250 baud. All of this file is pure C and host-testable: enqueue a note_on,
 * pop the three bytes, check them.
 *
 * Status bytes (channel n = 0..15):
 *   Note On  : 0x90|n  note  velocity
 *   Note Off : 0x80|n  note  0
 *   Control  : 0xB0|n  controller  value
 * All values are masked to 7 bits (0..127) as the protocol requires — a stray
 * 8-bit value would otherwise look like a status byte and corrupt the stream.
 */

#include "midi.h"
#include <string.h>

#define FIFO_SIZE 512                  /* power of two; 170 queued 3-byte msgs */
#define FIFO_MASK (FIFO_SIZE - 1)

static uint8_t  fifo[FIFO_SIZE];
static volatile uint16_t head, tail;   /* head=write, tail=read (drained by ISR) */
static uint8_t  channel;
static bool     overflow;

void midi_init(void) {
    head = tail = 0;
    channel  = MIDI_DEFAULT_CHANNEL;
    overflow = false;
}

void midi_set_channel(uint8_t ch) { channel = ch & 0x0F; }

static int fifo_free(void) {
    return FIFO_SIZE - 1 - (int)((head - tail) & FIFO_MASK);
}

/* Enqueue a whole 3-byte message atomically: if all three don't fit, drop the
 * message entirely (never a partial message — a half message desyncs the
 * receiver) and latch the overflow flag. */
static void push3(uint8_t a, uint8_t b, uint8_t c) {
    if (fifo_free() < 3) { overflow = true; return; }
    fifo[head & FIFO_MASK] = a; head = (head + 1) & FIFO_MASK;
    fifo[head & FIFO_MASK] = b; head = (head + 1) & FIFO_MASK;
    fifo[head & FIFO_MASK] = c; head = (head + 1) & FIFO_MASK;
}

void midi_send_note_on(uint8_t note, uint8_t velocity) {
    push3((uint8_t)(0x90 | channel), note & 0x7F, velocity & 0x7F);
}

void midi_send_note_off(uint8_t note) {
    push3((uint8_t)(0x80 | channel), note & 0x7F, 0);
}

void midi_send_cc(uint8_t controller, uint8_t value) {
    push3((uint8_t)(0xB0 | channel), controller & 0x7F, value & 0x7F);
}

void midi_send_all_notes_off(void) {
    midi_send_cc(123, 0);              /* CC 123 = All Notes Off */
}

int midi_pop_byte(void) {
    if (tail == head) return -1;
    uint8_t b = fifo[tail & FIFO_MASK];
    tail = (tail + 1) & FIFO_MASK;
    return b;
}

int midi_pending(void) {
    return (int)((head - tail) & FIFO_MASK);
}

bool midi_overflowed(void) { return overflow; }
