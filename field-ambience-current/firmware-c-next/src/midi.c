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
#include <math.h>

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

/* --- pure freq/amp → MIDI conversion (host-tested; used by the engine tap) - */

/* Nearest equal-tempered MIDI note for a frequency (A4=69=440 Hz), clamped to
 * the valid 0..127 range. 0 Hz / silence maps to -1 (no note). */
int midi_note_from_hz(float hz) {
    if (hz <= 0.0f) return -1;
    int n = (int)lrintf(69.0f + 12.0f * log2f(hz / 440.0f));
    if (n < 0)   n = 0;
    if (n > 127) n = 127;
    return n;
}

/* Linear amp (0..1) → MIDI velocity 1..127. A sounding note is never velocity
 * 0 (that would read as note-off), so the floor is 1. */
uint8_t midi_vel_from_amp(float amp) {
    if (amp <= 0.0f) return 1;
    int v = (int)lrintf(amp * 127.0f);
    if (v < 1)   v = 1;
    if (v > 127) v = 127;
    return (uint8_t)v;
}
