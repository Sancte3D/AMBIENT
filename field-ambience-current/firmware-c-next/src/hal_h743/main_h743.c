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
#include "cells.h"       /* Hall velocity model (ADR-0013) */
#include "controls.h"    /* hold-latch + modifier state machine (ADR-0008 r2) */
#include "params.h"      /* encoder → engine param bindings */

/* MCP23017 GPIO bit map (SPEC §7.2 / mcp23017_h743.c) — which expander bit is
 * which modifier button. Modifier push of EN1/2/4 also lands on GPB. */
#define MCP_BIT_SHIFT     0   /* GPB0 */
#define MCP_BIT_HOLD      1
#define MCP_BIT_DRONE     2
#define MCP_BIT_GENERATE  3
#define MCP_BIT_CLEAR     4

int main(void) {
    /* TODO(Step 13.3): SystemClock_Config(); HAL_Init(); SysTick at 1 kHz. */

    oled_init();
    mcp_init();
    enc_init();
    midi_tx_init();
    dsp_init();
    brain_init();
    engine_init();
    controls_init();          /* hold-latch + modifier state (ADR-0008 r2) */
    params_init();            /* encoder param values → engine defaults */
    cells_init();             /* Hall velocity model */
    audio_init();
    audio_set_renderer(engine_render);

    /* Main loop. All gameplay LOGIC is hardware-independent (controls.c /
     * params.c / cells.c, host-tested) — this loop only ROUTES hardware
     * events into it. The TODOs are the HAL reads, not logic. */
    uint16_t prev_gpio = 0xFFFF;          /* MCP pull-ups → idle high */
    for (;;) {
        /* --- 1. Encoder rotate/push → params + menu --- */
        enc_event_t ev;
        while (enc_pop_event(&ev)) {
            uint32_t now = 0; /* TODO(13.3): HAL_GetTick() */
            if (ev.kind == ENC_EVENT_ROTATE) {
                if (ev.id == PARAM_ENC_DISPLAY) {
                    /* TODO: menu_scroll(ev.value) — key/mode/setup navigation */
                } else {
                    params_encoder(ev.id, ev.value, now);   /* DRIVE/BRIGHT/VOLUME */
                }
            } else if (ev.kind == ENC_EVENT_PUSH && ev.value) {
                if (ev.id == PARAM_ENC_DISPLAY) { /* TODO: menu_enter() */ }
            }
        }

        /* --- 2. MCP23017 buttons → modifiers + cell taps --- */
        uint16_t gpio;
        if (mcp_read_gpio(&gpio)) {
            uint16_t fell = (uint16_t)(prev_gpio & ~gpio);   /* 1→0 = press */
            uint16_t rose = (uint16_t)(~prev_gpio & gpio);   /* 0→1 = release */
            if (fell & (1u<<MCP_BIT_SHIFT))    controls_modifier(MOD_SHIFT, true);
            if (fell & (1u<<MCP_BIT_HOLD))     controls_modifier(MOD_HOLD, true);
            if (fell & (1u<<MCP_BIT_DRONE))    controls_modifier(MOD_DRONE, true);
            if (fell & (1u<<MCP_BIT_GENERATE)) controls_modifier(MOD_GENERATE, true);
            if (fell & (1u<<MCP_BIT_CLEAR))    controls_modifier(MOD_CLEAR, true);
            (void)rose;
            prev_gpio = gpio;
        }

        /* --- 3. Cell Hall ADCs → velocity → controls --- */
        /* TODO(13.3): for each cell c: float pos = adc_read_norm(c);
         *   cell_event_t ce = cells_update(c, pos, HAL_GetTick());
         *   if (ce.kind == CELL_EVENT_PRESS)   controls_cell_press(c, ce.amp);
         *   if (ce.kind == CELL_EVENT_RELEASE) controls_cell_release(c);     */

        /* --- 4. Generative bar timer (SysTick-driven) → engine_generative_advance() --- */
        /* --- 5. LED render: pca_set_pwm() from controls_hold_base/shift + modifiers --- */
        /* --- 6. Menu refresh: oled_show() at ~30 fps --- */
    }
}
