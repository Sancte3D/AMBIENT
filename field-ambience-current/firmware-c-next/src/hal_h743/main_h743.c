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
#include "leds.h"        /* controls/modifier state → PCA9685 16-ch PWM */
#include "menu.h"        /* menu state machine */

/* MCP23017 GPIO bit map (SPEC §7.2 / mcp23017_h743.c) — which expander bit is
 * which modifier button. Modifier push of EN1/2/4 also lands on GPB. */
#define MCP_BIT_SHIFT     0   /* GPB0 */
#define MCP_BIT_HOLD      1
#define MCP_BIT_DRONE     2
#define MCP_BIT_GENERATE  3
#define MCP_BIT_CLEAR     4

/* Menu → engine binding (ADR-0017 Phase 4). The menu state machine doesn't
 * know about engine.h; it calls the user-supplied callbacks. Wire them here
 * so a World/Space/Tone/Atmos/Drums change in the UI actually drives the
 * engine (Worlds module + Ambience + Tape are otherwise idle from the
 * user's perspective). Synchronous from menu_rotate/menu_push — never from
 * the audio thread. */
static void hal_set_world      (int   idx) { engine_set_world(idx); }
static void hal_set_space      (float v)   { engine_set_space(v); }
static void hal_set_tone       (float v)   { engine_set_brightness(800.0f + v * 4200.0f); }
static void hal_set_atmosphere (float v)   { engine_set_atmosphere(v); }
static void hal_set_drums      (int   on)  { (void)on;  /* Phase: drums.c lift */ }

int main(void) {
    /* TODO(Step 13.3): SystemClock_Config(); HAL_Init(); SysTick at 1 kHz. */

    oled_init();
    mcp_init();
    enc_init();
    /* midi_tx_init();  -- DEFERRED r18.30 (ADR-0004). J9 DNP für 5er-Run.
     * Hardware-Pfad PD5 → USART2 bleibt reserviert; bei Reaktivierung nur
     * diese Zeile einkommentieren + Buchse/220Ω auf PCB bestücken. */
    dsp_init();
    brain_init();
    engine_init();
    {
        /* ADR-0017 Phase 4: menu → engine wiring */
        menu_callbacks_t cb = {
            .set_world      = hal_set_world,
            .set_space      = hal_set_space,
            .set_tone       = hal_set_tone,
            .set_atmosphere = hal_set_atmosphere,
            .set_drums      = hal_set_drums,
        };
        menu_init(&cb);
    }
    controls_init();          /* hold-latch + modifier state (ADR-0008 r2) */
    params_init();            /* encoder param values → engine defaults */
    cells_init();             /* Hall velocity model */
    leds_init();              /* 16-ch PWM render */
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
                    menu_rotate(ev.value);              /* DISPLAY-Encoder: Menü-Navigation */
                } else {
                    params_encoder(ev.id, ev.value, now);   /* DRIVE/BRIGHT/VOLUME */
                }
            } else if (ev.kind == ENC_EVENT_PUSH && ev.value) {
                if (ev.id == PARAM_ENC_DISPLAY) menu_push(); /* DISPLAY-Push: Menü-Enter */
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

        /* --- 5. LED render @ ~60 Hz: state → 16 PCA9685 channels → I²C --- */
        {
            static uint32_t last_ms = 0;
            uint32_t now = 0;             /* TODO(13.3): HAL_GetTick() */
            uint16_t dt = (uint16_t)(now - last_ms);
            if (dt >= 16) {                /* 60 Hz tick */
                uint16_t pwm[LED_CH_COUNT];
                leds_render(now, dt, pwm);
                for (uint8_t ch = 0; ch < LED_CH_COUNT; ++ch)
                    pca_set_pwm(ch, 0, pwm[ch]);   /* on_count=0, off_count=duty */
                last_ms = now;
            }
        }

        /* --- 6. Menu refresh @ ~30 fps: menu_render() + oled_show() --- */
        /* TODO(13.3): rate-limit + framebuffer flush. */
    }
}
