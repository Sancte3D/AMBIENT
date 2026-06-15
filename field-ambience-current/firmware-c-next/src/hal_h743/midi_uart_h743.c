/*
 * midi_uart_h743.c — STM32H743 USART driver skeleton for TRS-MIDI Out.
 *
 * Replaces the RP2040 PIO-UART hack (src/hal_pico/midi_pio.c) with a native
 * STM32 USART. The H7 has hardware USARTs everywhere, so no PIO trickery
 * needed — just configure USART for 31250 baud, 8N1, TX-only.
 *
 * Pin map (SPEC §5.7 / r18.14):
 *   PD5 (pin 86) = USART2_TX (AF7) → 220 Ω → TRS-Tip   (MIDI data)
 *                                  +3V3 → 220 Ω → TRS-Ring
 *                                  GND → Sleeve
 *   Pegel 3.3 V — MMA-CA-033 (2020) explicitly allows 3.3 V for Type A.
 *   Kein Optokoppler nötig (gilt nur für MIDI IN; wir bauen nur OUT).
 *
 * Step 13.3 (TODO): STM32CubeH7 USART2 init at 31250 baud + DMA TX so
 * note-on/off bursts during fast chord changes don't block the menu refresh.
 * The existing src/midi.c ring buffer is hardware-independent; only the
 * TX-byte primitive (this file) changes.
 */

#include "midi.h"
#include <stdint.h>
#include <stdbool.h>

void midi_tx_init(void) {
    /* TODO(Step 13.3):
     *   1. Enable GPIOD + USART2 clocks. Configure PD5 = AF7, push-pull.
     *   2. Configure USART2: BRR for 31250 baud (PCLK/31250), 8 data bits,
     *      1 stop, no parity, TX only.
     *   3. Optional: enable DMA1 TX channel + interrupt. With < 320 bytes/s
     *      worst-case throughput (10 notes × 3 bytes × 10 Hz = 300 B/s),
     *      polled TX would also be fine, but DMA frees the audio IRQ from
     *      worrying about UART latency.
     */
}

void midi_tx_byte(uint8_t b) {
    /* TODO(Step 13.3): blocking write to USART2 TDR (wait TXE), or push to
     * DMA TX buffer. */
    (void)b;
}

bool midi_tx_busy(void) {
    /* TODO: return USART2 TC flag inverted (true if last byte still in flight). */
    return false;
}
