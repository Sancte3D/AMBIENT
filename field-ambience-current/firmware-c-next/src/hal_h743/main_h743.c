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
 *   5. Main loop: drain enc_pop_event() → menu.c, drain MCP int → cells
 *      (digital, GPA0-4) + modifiers (GPB0-4) + jack-detect (GPA6)
 *      → hold-latch/modifier toggles, advance SysTick-driven
 *      generative bar timer if engine_set_generative(true,…).
 *
 * r18.85: VU-Meter ist angebunden — engine_render_peak() (Block-Peak des
 * finalen limitierten Outputs) → vu.c (host-getestete Ballistik: Instant-
 * Attack, 30 dB/s Release, 900 ms Peak-Hold, 8 Segmente −36…−0,5 dBFS) →
 * pca2_set_pwm() (U10 @ 0x41). Der I²C-Transport selbst ist Step 13.3.
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
#include "controls.h"    /* hold-latch + modifier state machine (ADR-0008 r2) */
#include "params.h"      /* encoder → engine param bindings */
#include "leds.h"        /* controls/modifier state → PCA9685 16-ch PWM */
#include "vu.h"          /* r18.85: engine peak → 8-seg VU (U10 @ 0x41) */
#include "menu.h"        /* menu state machine */

/* MCP23017 GPIO bit map (SPEC §7.2 / mcp23017_h743.c) — which expander bit is
 * which modifier button. Modifier push of EN1/2/4 also lands on GPB. */
/* Modifier bit positions are defined ONCE in mcp23017.h as MCP_BIT_MOD_*
 * (GPB0-4 = bits 8-12, because mcp_read_gpio packs (GPB<<8)|GPA). Do not
 * redefine them here — the old local 0-4 values read GPA, not GPB (bug). */

/* Cells sind seit r18.73 DIGITAL (MCP23017 GPA0-4) — feste Tap-Velocity wie
 * im Pico-HAL (CELL_VOICE_AMP 0.12 dort; controls.c skaliert identisch). */
#define CELL_TAP_AMP 0.12f

/* Menu → engine binding (ADR-0017 Phase 4, r18.58 Reddit-macro pass).
 * Menu slots are now World/Space/Atmos/Motion/Age — Tone was dropped (it
 * duplicated the Brightness encoder) and Drums was dropped (adaptive drums
 * is its own can of worms; we ship sound, not timing). Synchronous from
 * menu_rotate/menu_push, never from the audio thread. */
static void hal_set_world      (int   idx) { engine_set_world(idx); }
static void hal_set_space      (float v)   { engine_set_space(v); }
static void hal_set_atmosphere (float v)   { engine_set_atmosphere(v); }
static void hal_set_motion     (float v)   { engine_set_motion(v); }
static void hal_set_age        (float v)   { engine_set_age(v); }
static void hal_set_echo       (float v)   { engine_set_echo(v); }
static void hal_set_blur       (float v)   { engine_set_blur(v); }

int main(void) {
    /* TODO(Step 13.3): SystemClock_Config(); HAL_Init(); SysTick at 1 kHz. */

    oled_init();
    mcp_init();
    enc_init();
    /* midi_tx_init();  -- Firmware-seitig DEFERRED r18.30 (ADR-0004).
     * r18.82-Doku-Fix: die HARDWARE ist komplett bestueckt (J10 PJ-320D +
     * 2x 220R seit r18.67; frueher stand hier faelschlich "J9 DNP" — J9 ist
     * der Akku-JST). Bei Reaktivierung nur diese Zeile einkommentieren. */
    dsp_init();
    brain_init();
    engine_init();
    {
        /* ADR-0017 Phase 4 + r18.58 Reddit-macro menu */
        menu_callbacks_t cb = {
            .set_world      = hal_set_world,
            .set_space      = hal_set_space,
            .set_atmosphere = hal_set_atmosphere,
            .set_motion     = hal_set_motion,
            .set_age        = hal_set_age,
            .set_echo       = hal_set_echo,
            .set_blur       = hal_set_blur,
        };
        menu_init(&cb);
    }
    controls_init();          /* hold-latch + modifier state (ADR-0008 r2) */
    params_init();            /* encoder param values → engine defaults */
    leds_init();              /* 16-ch PWM render */
    vu_init();                /* r18.85: 8-seg VU meter (U10) */
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

        /* --- 2. MCP23017 → cells + modifiers + jack-detect ---
         * r18.82: Der alte Abschnitt 3 ("Cell Hall ADCs → velocity") war seit
         * r18.73 stale — die Cells sind DIGITALE Switches am MCP23017
         * (GPA0-4, aktiv-LOW mit internem Pull-up), keine Hall-ADCs mehr
         * (adc_read_norm/cells_update existieren fuer die Cells nicht mehr;
         * PC0/PC1/PA4/PB0/PB1 sind NC-Reserven). Gleicher Edge-Pfad wie die
         * bench-erprobte Pico-Implementierung (src/hal_pico/main_pico.c),
         * hier durch die host-getestete controls.c-Statemachine geroutet. */
        uint16_t gpio;
        if (mcp_read_gpio(&gpio)) {
            uint16_t fell = (uint16_t)(prev_gpio & ~gpio);   /* 1→0 = press */
            uint16_t rose = (uint16_t)(~prev_gpio & gpio);   /* 0→1 = release */

            /* Cells GPA0-4: digital an/aus → feste Velocity (wie Pico-HAL). */
            for (uint8_t c = 0; c < 5; ++c) {
                uint16_t m = (uint16_t)(1u << (MCP_BIT_CELL1 + c));
                if (fell & m) controls_cell_press(c, CELL_TAP_AMP);
                if (rose & m) controls_cell_release(c);
            }

            /* Modifier GPB0-4 (bits 8-12). */
            if (fell & (1u<<MCP_BIT_MOD_SHIFT))    controls_modifier(MOD_SHIFT, true);
            if (fell & (1u<<MCP_BIT_MOD_HOLD))     controls_modifier(MOD_HOLD, true);
            if (fell & (1u<<MCP_BIT_MOD_DRONE))    controls_modifier(MOD_DRONE, true);
            if (fell & (1u<<MCP_BIT_MOD_GENERATE)) controls_modifier(MOD_GENERATE, true);
            if (fell & (1u<<MCP_BIT_MOD_CLEAR))    controls_modifier(MOD_CLEAR, true);

            /* Jack-Detect GPA6 (r18.82-Hardware: PJ-320D-Switch oeffnet beim
             * Einstecken → mit R_DET 10k/C_DET 1µF + MCP-Pull-up liest der
             * Pin unplugged ≈ 0,3 V = LOW, plugged = HIGH). Design (ADR/v0.7):
             * Klinke drin → NUR den PAM8403 muten (AMP_nMUTE = PB15 LOW),
             * Line-Out bleibt live — NICHT audio_mute() rufen (das wuerde
             * auch XSMT ziehen und den Line-Out toeten). */
            {
                uint16_t jm = (uint16_t)(1u << MCP_BIT_JACK);
                if ((fell | rose) & jm) {
                    bool plugged = (gpio & jm) != 0;
                    (void)plugged;
                    /* TODO(13.3): PB15 (AMP_nMUTE) = plugged ? LOW : HIGH
                     * (+ ~50 ms Debounce ueber den 1-kHz-SysTick). */
                }
            }

            prev_gpio = gpio;
        }

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
                /* r18.85: VU meter — engine block peak → 8 segments → U10 */
                uint16_t vu[VU_CH_COUNT];
                vu_update(engine_render_peak(), now);
                vu_render(vu);
                for (uint8_t ch = 0; ch < VU_CH_COUNT; ++ch)
                    pca2_set_pwm(ch, 0, vu[ch]);
                last_ms = now;
            }
        }

        /* --- 6. Menu refresh @ ~30 fps: menu_render() + oled_show() --- */
        /* TODO(13.3): rate-limit + framebuffer flush. */
    }
}
