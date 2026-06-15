/*
 * main_h743.c — STM32H743 product firmware entry point (skeleton).
 *
 * Boot sequence (Step 13.3 TODO):
 *   1. SystemClock_Config: HSE 8 MHz → PLL1 → 480 MHz core / 240 MHz AHB /
 *      120 MHz APB; PLL3 → 11.2896 MHz for SAI1 (jitter-free 44.1 kHz).
 *   2. Enable I/D cache (cm7_dcache_invalidate_by_addr around DMA buffers).
 *   3. SysTick at 1 kHz (for menu animation tick + encoder sample tick +
 *      hold-latch button debounce).
 *   4. Init power-up sequence in order:
 *        oled_init()       — LCD splash
 *        mcp_init()        — I²C bus, MCP23017 + PCA9685 (LEDs all dark)
 *        enc_init()        — 4 encoder timers + push GPIO
 *        midi_tx_init()    — USART2 ready, idle
 *        dsp_init()        — sine LUT
 *        brain_init()      — default C ionian / warm
 *        engine_init()     — reverb buffers, voice pools, generative state
 *        audio_init()      — SAI1+DMA pump live, /SHDN+/MUTE+XSMT sequence
 *        audio_set_renderer(engine_render);
 *   5. Main loop: drain enc_pop_event() → menu.c, drain MCP int → buttons
 *      → cell hold-latch + modifier toggles, advance SysTick-driven
 *      generative bar timer if engine_set_generative(true,…).
 *
 * Hold-latch logic (ADR-0008 r2 to implement here):
 *   Per cell: bool hold_base[5], hold_shift[5].
 *   Hold-modifier latched + cell tap → toggle hold_<shift?>[cell].
 *   On press: if hold_base toggled on → engine_note_on(cell, root);
 *             if hold_shift toggled on → engine_note_on(cell+9, root+12);
 *             if hold_base/shift toggled off → engine_note_off(...).
 *   Clear modifier → reset all 10 bits + engine_note_off(0..4) + (9..13).
 *
 * This file does not contain the actual STM32 startup vector table or
 * SystemInit — those live in vendor-provided files (CubeH7 + linker script)
 * that Step 13.3 will add.
 */

#include "engine.h"
#include "brain.h"
#include "dsp.h"
#include "audio.h"
#include "encoders.h"
#include "mcp23017.h"
#include "oled.h"
#include "midi.h"

int main(void) {
    /* TODO(Step 13.3): SystemClock_Config(); HAL_Init(); SysTick at 1 kHz. */

    oled_init();
    mcp_init();
    enc_init();
    midi_tx_init();
    dsp_init();
    brain_init();
    engine_init();
    audio_init();
    audio_set_renderer(engine_render);

    /* TODO: main loop. */
    for (;;) {
        /* drain encoder events → menu / param bindings */
        /* drain MCP IRQ → modifier toggles + hold-latch state machine */
        /* advance generative bar timer */
        /* refresh menu framebuffer + oled_show() */
    }
}
