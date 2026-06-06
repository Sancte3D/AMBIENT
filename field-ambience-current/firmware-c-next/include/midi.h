#ifndef FAM_MIDI_H
#define FAM_MIDI_H

/*
 * MIDI Out — Step 12b #6.
 *
 * Layered so the message logic stays fully host-testable:
 *
 *   ┌─────────────────────────────────────────┐
 *   │ midi_send_note_on / note_off / cc       │  ← high-level (called by
 *   │ midi_set_channel                        │     engine + main loop)
 *   ├─────────────────────────────────────────┤
 *   │ midi_pop_byte (FIFO)                    │  ← drained by the PIO-UART
 *   ├─────────────────────────────────────────┤
 *   │ midi_hw_init / midi_hw_pump             │  ← Pico SDK + PIO program
 *   └─────────────────────────────────────────┘  ← stub here, ON-DEVICE only
 *
 * The top two layers are pure C with a static ring buffer — fully host-
 * testable: emit a note_on, drain three bytes (0x90+ch, note, vel),
 * verify them. The PIO-UART layer is in midi_pio.c and is only compiled
 * inside the Pico SDK build (CMake guard). PIO program: 31250 Baud 8N1
 * on GP21 → MIDI_TX → R220 → TRS Tip (SPEC §8 r15).
 */

#include <stdint.h>
#include <stdbool.h>

#define MIDI_DEFAULT_CHANNEL 0          /* MIDI channels are 0..15 internally,
                                           displayed 1..16 in UIs */

void midi_init(void);

/* Set the output channel for all subsequent messages (0..15, clamped). */
void midi_set_channel(uint8_t channel);

/* High-level senders. Each enqueues a 3-byte message into the FIFO. The FIFO
 * is bounded — overflow drops the message silently rather than blocking the
 * audio thread, which is the correct behaviour for a live performance MIDI
 * tap (a missed note is much better than an audio glitch). */
void midi_send_note_on(uint8_t note, uint8_t velocity);
void midi_send_note_off(uint8_t note);
void midi_send_cc(uint8_t controller, uint8_t value);

/* All-notes-off on the current channel (CC 123). Useful on stop/panic. */
void midi_send_all_notes_off(void);

/* FIFO drain — call from the PIO-UART pump (or the host test). Returns the
 * next byte 0..255 or -1 when the FIFO is empty. */
int midi_pop_byte(void);

/* For diagnostics + tests: how many bytes are queued for transmission. */
int midi_pending(void);

/* Were any messages dropped because the FIFO was full? (Latches; cleared
 * by midi_init.) Lets the UI flag a problem rather than silently glitch. */
bool midi_overflowed(void);

#endif
