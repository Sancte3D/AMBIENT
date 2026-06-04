/*
 * Field Ambience — native C firmware.
 *
 *   - Step 1: USB CDC heartbeat + STATUS LED on GP26 @ 1 Hz
 *   - Step 2: SSD1322 OLED, banner
 *   - Step 3: I²C + MCP23017 + 10 switches + jack-detect
 *   - Step 4: 4× EC11 encoders (quadrature + push)
 *   - Step 5: I²S DMA + 440 Hz test sine, SPEC §8 pop-suppression
 *   - Step 7: polyphonic voice pool (sine + ASR per cell tap).
 *   - Step 9: famPadCore (detuned-saw pad) replaces the placeholder sine.
 *   - Step 11: famReverbMaster + engine mix-bus (pad → dry + reverb send).
 *   - Step 10: famTexture noise bed under everything (quiet idle ambience).
 *   - Step 8: famSubBass + famDeepBass under the lowest held cell.
 *   - Step 12a (here): harmonic brain. Cell→pitch is now real scale/mode/
 *     family harmony — each cell sounds the chord root of its scale degree
 *     (default C4 ionian, warm/add9), replacing the placeholder pentatonic.
 *     The menu, USB-MIDI and encoder→param bindings (Step 12b) are still to
 *     come; key/mode/vibe currently sit at their defaults.
 *
 * Engine-Port order is 9→11→10→8→12 (hörbarkeits-first, see
 * NATIVE_PORT_PLAN.md); Steps 8 and 10 are still pending.
 * (Step 6 was the schematic update that removed the Pi — no firmware change.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "version.h"
#include "oled.h"
#include "mcp23017.h"
#include "encoders.h"
#include "audio.h"
#include "dsp.h"
#include "engine.h"
#include "brain.h"

#define PIN_STATUS_LED  26

/* Cell→pitch now comes from the harmonic brain (Step 12a): each cell plays a
 * single pad voice on the chord ROOT of its scale degree, for the current
 * key/mode/vibe. Defaults: C4 ionian, warm/add9. */
#define CELL_VOICE_AMP 0.12f

/* ---------------------------------------------------------------------------
 * DEMO_AUTOPLAY — hear the full instrument with ONLY a Pico + a DAC.
 *
 * Set to 1 to make the firmware play a slow chord progression by itself, with
 * NO MCP23017 and NO buttons wired. The cells are triggered on a timer instead
 * of by hardware. Perfect for the breadboard listening test when the buttons
 * won't fit: you only solder the DAC and wire 5 lines + the XSMT/SCK/FMT
 * bridges. The DAC's XSMT must be tied high in hardware (jumper H3 → H, or the
 * XSMT pin → 3V3), since with no MCP the firmware can't release XSMT itself.
 *
 * Set back to 0 for the normal build (cells played by the real buttons).
 * ------------------------------------------------------------------------- */
#define DEMO_AUTOPLAY 0

/* Slow ambient progression over the scale degrees (cells 0..4). Each step is
 * held for DEMO_STEP_MS; the long pad release + reverb tail overlap the steps
 * so it never sounds choppy. A gentle I–IV–V–vi-ish wander. */
#define DEMO_STEP_MS 5000
static const uint8_t DEMO_SEQ[] = { 0, 3, 4, 2, 0, 4, 1, 3 };
#define DEMO_SEQ_LEN (sizeof DEMO_SEQ / sizeof DEMO_SEQ[0])

static void draw_banner_static(void) {
    oled_text( 72,  0, "FIELD AMBIENCE", 0x0F);
    oled_text( 88,  8, "V0.9 STEP 12A",  0x0A);
    oled_text(  0, 16, "TAP A CELL",     0x07);
}

/* --- Step 3 button display (preserved, compact form on bottom rows) --- */
static void draw_buttons_state(uint16_t v) {
    /* Labels (Step 3 style, compacted onto y=48). */
    oled_text(0, 48, "C", 0x06);
    const int cell_x[5] = { 16, 32, 48, 64, 80 };
    for (int i = 0; i < 5; ++i) {
        bool pressed = !(v & (1u << (MCP_BIT_CELL1 + i)));
        char ch[2] = { pressed ? 'O' : '.', 0 };
        oled_text(cell_x[i], 48, " ", 0);
        oled_text(cell_x[i], 48, ch, pressed ? 0x0F : 0x05);
    }
    oled_text(96, 48, "M", 0x06);
    const int mod_x[5] = { 112, 128, 144, 160, 176 };
    for (int i = 0; i < 5; ++i) {
        bool pressed = !(v & (1u << (MCP_BIT_MOD_SHIFT + i)));
        char ch[2] = { pressed ? 'O' : '.', 0 };
        oled_text(mod_x[i], 48, " ", 0);
        oled_text(mod_x[i], 48, ch, pressed ? 0x0F : 0x05);
    }
    bool jack_in = (v & (1u << MCP_BIT_JACK)) != 0;
    oled_text(200, 48, " ", 0);
    oled_text(200, 48, "J", jack_in ? 0x0F : 0x05);
}

/* --- Step 4 encoder display --- */

/* Render a signed position into a fixed-width 5-char field: e.g. "+0123".
 * Range clamped to ±9999; values outside print as "+9999" / "-9999". */
static void format_signed(int32_t v, char out[6]) {
    bool neg = (v < 0);
    long n = neg ? -(long)v : (long)v;
    if (n > 9999) n = 9999;
    out[0] = neg ? '-' : '+';
    out[1] = (char)('0' + (n / 1000) % 10);
    out[2] = (char)('0' + (n / 100)  % 10);
    out[3] = (char)('0' + (n / 10)   % 10);
    out[4] = (char)('0' + (n)        % 10);
    out[5] = 0;
}

static void draw_encoders_state(void) {
    const char *labels[ENC_COUNT] = { "DRIV", "BRIT", "DISP", "VOL " };
    const int   col_x[ENC_COUNT]  = { 0, 64, 128, 192 };
    char buf[6];

    /* Row 24: labels + push-state dot */
    for (int i = 0; i < ENC_COUNT; ++i) {
        oled_text(col_x[i], 24, "        ", 0);   /* clear column band */
        oled_text(col_x[i], 24, labels[i], 0x07);
        bool sw = enc_pushed((uint8_t)(i + 1));
        oled_text(col_x[i] + 40, 24, sw ? "O" : ".", sw ? 0x0F : 0x05);
    }
    /* Row 32: signed position */
    for (int i = 0; i < ENC_COUNT; ++i) {
        oled_text(col_x[i], 32, "        ", 0);
        format_signed(enc_position((uint8_t)(i + 1)), buf);
        oled_text(col_x[i], 32, buf, 0x0F);
    }
}

int main(void) {
    stdio_init_all();

    gpio_init(PIN_STATUS_LED);
    gpio_set_dir(PIN_STATUS_LED, GPIO_OUT);

    oled_init();
    oled_fill(0);
    draw_banner_static();
    oled_show();

    bool mcp_ok = mcp_init();
    if (!mcp_ok) {
        oled_text(0, 48, "MCP23017 NOT FOUND", 0x0F);
        printf("ERROR: MCP23017 did not ACK at 0x%02X\n", MCP_I2C_ADDR);
    } else {
        draw_buttons_state(mcp_state());
    }

    enc_init();
    draw_encoders_state();
    oled_show();

    /* Step 11: bring up DSP + engine (pad pool + Freeverb-style reverb) and
     * register engine_render as the audio block renderer BEFORE audio_init(),
     * so the power-up sequence streams silence (no voices active yet) and
     * stays pop-free. */
    dsp_init();
    brain_init();
    engine_init();
    audio_set_renderer(engine_render);

    if (!mcp_ok) {
        printf("WARN: audio start without MCP — PCM XSMT cannot be released\n");
    }
    audio_init();
    /* Raise the famTexture bed only AFTER the SPEC §8 un-mute sequence has
     * finished. It starts at 0 and glides up over ~2 s, so the power-up stays
     * pop-free while the device settles into a quiet ambient bed at idle. */
    engine_set_texture(0.20f);
#if DEMO_AUTOPLAY
    printf("audio: I2S pump live — DEMO_AUTOPLAY on, no MCP/buttons needed\n");
#else
    printf("audio: I2S pump live, engine ready (pad+reverb+texture+bass) — tap a cell\n");
#endif

    uint32_t hb_count = 0;
    absolute_time_t next_blink_at = make_timeout_time_ms(500);
    absolute_time_t next_hb_at    = make_timeout_time_ms(2000);
    bool led_on = false;
    uint16_t prev_mcp = mcp_ok ? mcp_state() : 0xFFFF;

#if DEMO_AUTOPLAY
    /* DEMO_AUTOPLAY sequencer state. */
    absolute_time_t next_demo_at = make_timeout_time_ms(2500);  /* let texture bloom first */
    int demo_step = 0;
    int demo_prev_cell = -1;
#endif

    while (true) {
        bool dirty = false;

#if DEMO_AUTOPLAY
        if (absolute_time_diff_us(get_absolute_time(), next_demo_at) <= 0) {
            if (demo_prev_cell >= 0) engine_note_off((uint8_t)demo_prev_cell);
            int cell = DEMO_SEQ[demo_step];
            int midi = brain_cell_root(cell);
            engine_note_on((uint8_t)cell, dsp_midi_to_hz((float)midi), CELL_VOICE_AMP);
            printf("DEMO step %d -> cell %d (degree %d, midi %d)\n",
                   demo_step, cell + 1, cell + 1, midi);
            demo_prev_cell = cell;
            demo_step = (demo_step + 1) % (int)DEMO_SEQ_LEN;
            next_demo_at = make_timeout_time_ms(DEMO_STEP_MS);
            dirty = true;
        }
#endif

        if (mcp_ok && mcp_irq_pending()) {
            uint16_t v = mcp_service();

            /* Cell press/release edges → voice note on/off. Bit low = pressed
             * (active-low). Compare against the previous state to find edges. */
            for (int c = 0; c < 5; ++c) {
                uint16_t mask = (uint16_t)(1u << (MCP_BIT_CELL1 + c));
                bool now_pressed  = !(v & mask);
                bool was_pressed  = !(prev_mcp & mask);
                if (now_pressed && !was_pressed) {
                    int midi = brain_cell_root(c);
                    engine_note_on((uint8_t)c, dsp_midi_to_hz((float)midi), CELL_VOICE_AMP);
                    printf("cell %d ON  (degree %d, midi %d)\n", c + 1, c + 1, midi);
                } else if (!now_pressed && was_pressed) {
                    engine_note_off((uint8_t)c);
                    printf("cell %d OFF\n", c + 1);
                }
            }
            prev_mcp = v;

            draw_buttons_state(v);
            dirty = true;
        }

        enc_event_t e;
        while (enc_pop_event(&e)) {
            if (e.kind == ENC_EVENT_ROTATE) {
                printf("ENC %u rotate %+d  pos=%ld\n",
                       e.id, e.value,
                       (long)enc_position(e.id));
            } else if (e.kind == ENC_EVENT_PUSH) {
                printf("ENC %u push   %s\n",
                       e.id, e.value ? "DOWN" : "UP");
            }
            dirty = true;
        }

        if (dirty) {
            /* Live voice count next to the 'TAP A CELL' banner line. */
            char vc[2] = { (char)('0' + (engine_active_voices() & 7)), 0 };
            oled_text(176, 16, "VOX ", 0x07);
            oled_text(208, 16, vc, 0x0F);
            draw_encoders_state();
            oled_show();
        }

        if (absolute_time_diff_us(get_absolute_time(), next_blink_at) <= 0) {
            led_on = !led_on;
            gpio_put(PIN_STATUS_LED, led_on);
            next_blink_at = make_timeout_time_ms(500);
        }
        if (absolute_time_diff_us(get_absolute_time(), next_hb_at) <= 0) {
            printf("%s — %s — heartbeat %lu\n",
                   FAM_FW_VERSION_STR, FAM_FW_BOARD_STR,
                   (unsigned long)hb_count++);
            next_hb_at = make_timeout_time_ms(2000);
        }

        sleep_ms(20);   /* keep encoder/MCP latency under a frame; 1 kHz sampling runs on the timer */
    }
}
