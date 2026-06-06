/*
 * Host-side tests for the Step-12b #6 MIDI core (midi.c).
 *
 * Build via run_tests.sh, or:
 *   cc -std=c11 -I../include test_midi.c ../src/midi.c -o /tmp/midi_test
 */

#include "midi.h"

#include <stdio.h>
#include <stdlib.h>

static int g_checks = 0;
static int g_fails  = 0;

#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        ++g_checks;                                                        \
        if (!(cond)) {                                                     \
            ++g_fails;                                                     \
            fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__);           \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
        }                                                                  \
    } while (0)

static void expect3(int s, int d1, int d2, const char *what) {
    int a = midi_pop_byte(), b = midi_pop_byte(), c = midi_pop_byte();
    CHECK(a == s,  "%s status: got 0x%02X want 0x%02X", what, a, s);
    CHECK(b == d1, "%s data1: got %d want %d", what, b, d1);
    CHECK(c == d2, "%s data2: got %d want %d", what, c, d2);
}

static void test_note_on_off_bytes(void) {
    midi_init();
    midi_send_note_on(60, 100);
    midi_send_note_off(60);
    CHECK(midi_pending() == 6, "pending != 6 after two messages: %d", midi_pending());
    expect3(0x90, 60, 100, "note_on ch0");
    expect3(0x80, 60, 0,   "note_off ch0");
    CHECK(midi_pop_byte() == -1, "fifo not empty after draining");
    CHECK(midi_pending() == 0, "pending != 0 when empty");
}

static void test_channel(void) {
    midi_init();
    midi_set_channel(5);
    midi_send_note_on(64, 80);
    expect3(0x95, 64, 80, "note_on ch5");
    /* channel clamps to 0..15 */
    midi_set_channel(200);
    midi_send_cc(74, 33);
    expect3(0xB8, 74, 33, "cc ch (200 & 0x0F = 8)");
}

static void test_value_masking(void) {
    /* 8-bit values must be masked to 7 bits so they can't look like status. */
    midi_init();
    midi_send_note_on(200, 200);       /* 200 & 0x7F = 72 */
    expect3(0x90, 72, 72, "masked note_on");
    midi_send_cc(255, 255);            /* 255 & 0x7F = 127 */
    expect3(0xB0, 127, 127, "masked cc");
}

static void test_all_notes_off(void) {
    midi_init();
    midi_set_channel(3);
    midi_send_all_notes_off();
    expect3(0xB3, 123, 0, "all-notes-off");
}

static void test_fifo_order(void) {
    midi_init();
    for (int i = 0; i < 10; ++i) midi_send_note_on((uint8_t)(40 + i), 64);
    for (int i = 0; i < 10; ++i) {
        int a = midi_pop_byte(), b = midi_pop_byte(), c = midi_pop_byte();
        CHECK(a == 0x90 && b == 40 + i && c == 64, "fifo out of order at %d: %02X %d %d", i, a, b, c);
    }
}

static void test_overflow_drops_whole_messages_and_latches(void) {
    midi_init();
    CHECK(!midi_overflowed(), "overflow set before any drop");
    /* Flood far past the buffer. */
    int sent = 0;
    for (int i = 0; i < 10000; ++i) { midi_send_note_on(60, 100); ++sent; }
    CHECK(midi_overflowed(), "overflow not latched after flooding");

    /* Whatever is queued must be a whole number of 3-byte messages (no partial
     * message ever enqueued), and every status byte must be a note-on. */
    int bytes = midi_pending();
    CHECK(bytes % 3 == 0, "fifo holds a partial message: %d bytes", bytes);
    int msgs = 0, bad = 0;
    for (;;) {
        int a = midi_pop_byte();
        if (a < 0) break;
        int b = midi_pop_byte(), c = midi_pop_byte();
        if (a != 0x90 || b != 60 || c != 100) ++bad;
        ++msgs;
    }
    CHECK(bad == 0, "corrupted message under overflow (%d bad of %d)", bad, msgs);
    printf("  overflow: flooded %d, buffer held %d whole messages, 0 corrupt\n", sent, msgs);

    /* init clears the latch. */
    midi_init();
    CHECK(!midi_overflowed(), "overflow not cleared by midi_init");
}

int main(void) {
    printf("== midi (step12b #6) ==\n");
    test_note_on_off_bytes();
    test_channel();
    test_value_masking();
    test_all_notes_off();
    test_fifo_order();
    test_overflow_drops_whole_messages_and_latches();

    printf("\n%d checks, %d failures\n", g_checks, g_fails);
    if (g_fails) { printf("RESULT: FAIL\n"); return 1; }
    printf("RESULT: PASS\n");
    return 0;
}
