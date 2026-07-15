/*
 * main_h743.c — STM32H743 product firmware entry point (r18.86, Step 13.3 —
 * the boot TODOs are now real code; vendor startup/linker/clock live in
 * this directory + vendor/).
 *
 * Boot sequence:
 *   1. I/D caches on, HAL_Init (SysTick 1 kHz), SystemClock_Config:
 *      HSE 8 MHz → PLL1 → 480 MHz core / 240 MHz AHB / 120 MHz APB;
 *      PLL3 → 11.289609 MHz SAI1 kernel (jitter-free 44.1 kHz).
 *   2. Init order below: LCD splash → I²C expander/LED drivers (dark) →
 *      encoders → DSP/brain/engine → menu/controls/params/leds/vu →
 *      audio (SAI1+DMA pump live, SPEC §8.3 /SHDN → /MUTE → XSMT).
 *   3. Main loop routes hardware events into the host-tested logic
 *      (controls.c / params.c / menu.c) — no gameplay logic lives here.
 *
 * r18.87 (User): VU-Meter komplett entfernt (U10 + vu.c + pca2-API) — nur
 * die Cell- und Modifier-LEDs auf U6 bleiben.
 *
 * Hold-latch logic (ADR-0008 r2) lives in controls.c (host-tested); this
 * loop only feeds it press/release edges from the MCP23017.
 */

#include "engine.h"
#include "v2/synth_host.h"
#include "brain.h"
#include "dsp.h"
#include "audio.h"
#include "diag.h"
#include "psram.h"
#include "encoders.h"
#include "mcp23017.h"
#include "oled.h"
#include "oled_color.h"  /* accent crossfade tick (world → screen tint) */
#include "overlay.h"     /* r19.21: transient knob-value overlay */
#include "knobs.h"       /* r19.21: encoder push short/long actions */
#include "scenes.h"      /* r19.22: parameter locks + 5 scene slots */
#include "bloom.h"       /* r19.23: chord-bloom cell mode */
#include <stdio.h>       /* snprintf (status overlay, control-rate only) */
#include "midi.h"
#include "controls.h"    /* hold-latch + modifier state machine (ADR-0008 r2) */
#include "params.h"      /* encoder → engine param bindings */
#include "leds.h"        /* controls/modifier state → PCA9685 16-ch PWM */
#include "menu.h"        /* menu state machine */
#include "battery.h"     /* r18.92: BAT_SENSE ADC -> % + USB glyph */
#include "h743_hal.h"    /* SystemClock_Config, enc_set_ext_push */

/* MCP23017 GPIO bit map (SPEC §7.2 / mcp23017_h743.c) — which expander bit is
 * which modifier button. Modifier push of EN1/2/4 also lands on GPB. */
/* Modifier bit positions are defined ONCE in mcp23017.h as MCP_BIT_MOD_*
 * (GPB0-4 = bits 8-12, because mcp_read_gpio packs (GPB<<8)|GPA). Do not
 * redefine them here — the old local 0-4 values read GPA, not GPB (bug). */

/* Cells sind seit r18.73 DIGITAL (MCP23017 GPA0-4) — feste Tap-Velocity wie
 * im Pico-HAL (CELL_VOICE_AMP 0.12 dort; controls.c skaliert identisch). */
#define CELL_TAP_AMP 0.12f

/* Jack-detect debounce (mechanical TRS contact bounce ≪ 50 ms). */
#define JACK_DEBOUNCE_MS 50u

/* Menu → engine binding (ADR-0017 Phase 4, r18.58 Reddit-macro pass).
 * Menu slots are now World/Space/Atmos/Motion/Age — Tone was dropped (it
 * duplicated the Brightness encoder) and Drums was dropped (adaptive drums
 * is its own can of worms; we ship sound, not timing). Synchronous from
 * menu_rotate/menu_push, never from the audio thread. */
/* r18.88: after a world change, re-pitch latched voices so held notes
 * follow the new key instead of clashing with the new drone/bed. */
static void hal_set_world      (int   idx) { engine_set_world(idx);
                                             controls_refresh_held_pitches(); }
/* r18.98: KEY follows the same rule — future notes + drone re-key, held
 * latched notes re-pitch so they never clash with the new tonic. */
static void hal_set_key        (int   pc)  { engine_set_key_pc(pc);
                                             controls_refresh_held_pitches(); }
static void hal_set_voice      (int   idx) { engine_set_voice(idx); }
static void hal_set_tuning     (int   just){ engine_set_tuning(just); }
static void hal_set_space      (float v)   { engine_set_space(v); }
static void hal_set_shimmer    (float v)   { engine_set_shimmer(v); }
static void hal_set_atmosphere (float v)   { engine_set_atmosphere(v); }
static void hal_set_motion     (float v)   { engine_set_motion(v); }
static void hal_set_age        (float v)   { engine_set_age(v); }
static void hal_set_echo       (float v)   { engine_set_echo(v); }
static void hal_set_blur       (float v)   { engine_set_blur(v); }
static void hal_set_synth      (int   idx) { engine_set_synth(idx); }
static bool s_cell_bloom = false;   /* r19.23: 0 Note / 1 Bloom */
static void hal_set_cell(int mode) {
    bool bloom = (mode == 1);
    if (bloom != s_cell_bloom) {
        /* Modewechsel: die Stimmen des verlassenen Modus sauber beenden. */
        if (bloom) engine_all_off();     /* NOTE → BLOOM: evtl. Latches weg */
        else       bloom_all_off();      /* BLOOM → NOTE: Akkord weg        */
    }
    s_cell_bloom = bloom;
}

/* r19.16 — V2 sound-core backend: thin adapters so the engine keeps no link
 * dependency on src/v2 (host tests link engine.c without it). */
static void be_select  (int id)              { synth_host_select((synth_id_t)id); }
static void be_note_on (int midi, float vel) { synth_host_note_on(midi, vel); }
static void be_note_off(void)                { synth_host_note_off(); }
static void be_panic   (void)                { synth_host_panic(); }
static void be_render  (int16_t *b, int n)   { synth_host_render(b, n); }
static const engine_synth_backend_t s_v2_backend = {
    be_select, be_note_on, be_note_off, be_panic, be_render
};

/* Klinke drin → NUR den PAM8403 muten (AMP_nMUTE = PB15 LOW), Line-Out
 * bleibt live — NICHT audio_mute() rufen (das wuerde auch XSMT ziehen und
 * den Line-Out toeten). Design: ADR / v0.7. */
static bool s_jack_plugged = false;   /* r19.21: fuer das Status-Overlay */

static void apply_amp_mute_for_jack(bool plugged) {
    s_jack_plugged = plugged;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15,
                      plugged ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/* r19.22: Scenes-Flash-Backend (scenes_flash_h743.c, Bank-2-Sektor). */
bool scenes_flash_write(const void *blob, unsigned len);
bool scenes_flash_read(void *blob, unsigned len);

/* r19.22: DISPLAY kurz/lang sind kontextabhaengig —
 *   Scenes-Browser offen: kurz = schliessen
 *   Menue BROWSE:         kurz = menu_push, lang = Scenes oeffnen
 *   Menue EDIT:           kurz = menu_push, lang = Parameter-Lock toggeln */
static void display_short_dispatch(void) {
    if (scenes_ui_active()) { scenes_ui_close(); return; }
    menu_push();
}
static void display_long_dispatch(void) {
    uint32_t now = HAL_GetTick();
    if (scenes_ui_active()) { scenes_ui_close(); return; }
    if (menu_mode() == MENU_EDIT) {
        bool locked = menu_toggle_lock_current();
        overlay_show(menu_current_label(), locked ? "LOCKED" : "UNLOCKED",
                     now, 0);
    } else {
        scenes_ui_open(now);
    }
}

/* r19.21: VOLUME lang gedrueckt → Batterie/USB + Ausgangsstatus. */
static void knob_status_lines(char *l1, unsigned n1, char *l2, unsigned n2) {
    snprintf(l1, n1, battery_usb_present() ? "BAT %d%% +USB" : "BAT %d%%",
             battery_pct());
    snprintf(l2, n2, "%s", s_jack_plugged ? "PHONES" : "SPEAKERS");
}

/* --- MIDI note tap (r19.14) ---------------------------------------------
 * Mirror the PLAYED cell notes (base sources 0..4, shift 9..13) to MIDI out.
 * The generative bed/drone/sparkle/melody sources are intentionally NOT
 * forwarded — MIDI out represents what the PLAYER performs, not the autoplay.
 * Runs at control rate (engine note path, not the audio ISR), so the FIFO
 * enqueue is safe. One note tracked per source for a correct note-off. */
static int8_t s_midi_src_note[16];
static bool midi_src_is_played(uint8_t s) { return s <= 4 || (s >= 9 && s <= 13); }

static void midi_note_tap(int on, uint8_t source, float freq_hz, float amp) {
    if (on < 0) {                                  /* engine_all_off */
        midi_send_all_notes_off();
        for (int i = 0; i < 16; ++i) s_midi_src_note[i] = -1;
        return;
    }
    if (source >= 16 || !midi_src_is_played(source)) return;
    if (on) {
        int note = midi_note_from_hz(freq_hz);
        if (note < 0) return;
        if (s_midi_src_note[source] >= 0)          /* retrigger: off the old */
            midi_send_note_off((uint8_t)s_midi_src_note[source]);
        midi_send_note_on((uint8_t)note, midi_vel_from_amp(amp));
        s_midi_src_note[source] = (int8_t)note;
    } else if (s_midi_src_note[source] >= 0) {
        midi_send_note_off((uint8_t)s_midi_src_note[source]);
        s_midi_src_note[source] = -1;
    }
}

/* DTCM/D2 DSP-buffer regions (stm32h743_flash.ld): the startup file zeroes
 * only the main .bss, so these must be zeroed here before ANY engine code
 * runs — C globals rely on zero-init. */
extern uint32_t _sdtcm_bss, _edtcm_bss, _sd2_bss, _ed2_bss;

int main(void) {
    /* Zero the out-of-.bss DSP regions first (see linker script). */
    for (uint32_t *p = &_sdtcm_bss; p < &_edtcm_bss; ++p) *p = 0u;
    for (uint32_t *p = &_sd2_bss;   p < &_ed2_bss;   ++p) *p = 0u;

    /* Denormal protection (r18.90): flush-to-zero on the M7 FPU. Reverb,
     * echo and pluck feedback tails decay THROUGH the denormal range; on
     * ARMv7-M denormal arithmetic takes a slow support path — a classic
     * embedded-audio CPU spike exactly when the instrument goes quiet.
     * FZ in FPSCR covers thread mode; FPDSCR seeds the FPSCR on exception
     * entry (the audio render runs in the DMA IRQ). Host tests are by
     * definition unaffected (this is device-only code). */
    __set_FPSCR(__get_FPSCR() | (1u << 24));            /* FZ, thread   */
    FPU->FPDSCR |= (1u << 24);                          /* FZ, handlers */

    /* Cortex-M7 caches next (the 480 MHz core is crippled without them),
     * then the HAL time base, then the real clock tree. */
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();                       /* SysTick 1 kHz + NVIC grouping */
    SystemClock_Config();             /* 480 MHz core, PLL3 → SAI 44.1 kHz */

    oled_init();
    mcp_init();
    enc_init();
    /* MIDI Out activated (r19.14): message core + USART2/PD5 driver both
     * complete since r18.86; the note tap below mirrors played cells to J10
     * (PJ-320D TRS Type A + 2x 220R). ⚠ On real hardware, verify the TRS-A
     * vs -B wiring against the receiver before relying on it (BRING_UP). */
    midi_init();
    midi_hw_init();
    dsp_init();
    brain_init();
    engine_init();
    engine_boot_mute();       /* r19.20 SPEC boot: silent until params_init
                               * sets the 30 % target → ~350 ms fade-in */
    for (int i = 0; i < 16; ++i) s_midi_src_note[i] = -1;
    engine_set_note_hook(midi_note_tap);   /* played cells → MIDI out */
    synth_host_init();
    engine_set_synth_backend(&s_v2_backend);   /* r19.16: SYNTH menu slot live */
    {
        /* ADR-0017 Phase 4 + r18.58 Reddit-macro menu */
        menu_callbacks_t cb = {
            .set_world      = hal_set_world,
            .set_key        = hal_set_key,
            .set_tuning     = hal_set_tuning,
            .set_voice      = hal_set_voice,
            .set_space      = hal_set_space,
            .set_shimmer    = hal_set_shimmer,
            .set_atmosphere = hal_set_atmosphere,
            .set_motion     = hal_set_motion,
            .set_age        = hal_set_age,
            .set_echo       = hal_set_echo,
            .set_blur       = hal_set_blur,
            .set_synth      = hal_set_synth,
            .set_cell       = hal_set_cell,
        };
        menu_init(&cb);
    }
    controls_init();          /* hold-latch + modifier state (ADR-0008 r2) */
    bloom_init();             /* r19.23: chord-bloom cell mode */
    params_init();            /* encoder param values → engine defaults */
    overlay_init();
    {   /* r19.21: Encoder-Push-Belegung (DISPLAY kurz = Menue wie bisher;
         * DISPLAY lang bleibt fuer Scenes reserviert). */
        knobs_callbacks_t kcb = { 0 };
        kcb.display_push = display_short_dispatch;   /* r19.22: kontextabhaengig */
        kcb.display_long = display_long_dispatch;    /* r19.22: Lock / Scenes    */
        kcb.status_lines = knob_status_lines;
        knobs_init(&kcb);
    }
    {   /* r19.22: Scenes aus dem Flash laden (Bank-2-Sektor, dual-bank —
         * Audio laeuft beim Speichern weiter). */
        scenes_init(scenes_flash_write, scenes_flash_read);
    }
    leds_init();              /* 16-ch PWM render */
    leds_set_backlight((uint16_t)(LED_PWM_MAX * 70 / 100));   /* 70 % boot */
    bat_adc_init();           /* r18.92: BAT_SENSE (PA3) — battery UI */
    audio_init();
    audio_set_renderer(engine_render);

    /* --- Bring-up diagnostics (r19.14): hold CELL1 at power-on to enter a
     * live readout (profiler load/WCET/misses/clips, battery, voices) + a
     * QSPI-PSRAM self-test on CELL2. A bench instrument, not a play mode — it
     * never returns (power-cycle to exit). Drives a heavy scene so the load%
     * is meaningful. CELL bits are active-low (MCP pull-ups). */
    if (!(mcp_state() & (1u << MCP_BIT_CELL1))) {
        engine_set_world(0);
        engine_set_drone(true);
        engine_set_generative(true, 0);
        engine_set_reverb_size(1.0f); engine_set_echo(1.0f);
        engine_set_blur(1.0f);        engine_set_shimmer(1.0f);
        int psram_pass = 0, psram_ran = 0;
        for (;;) {
            uint32_t now = HAL_GetTick();
            engine_generative_tick(now);
            if (!psram_ran && !(mcp_state() & (1u << MCP_BIT_CELL2))) {
                psram_ran  = 1;
                psram_pass = (psram_init() && psram_selftest()) ? 1 : 0;
            }
            diag_draw(psram_pass, psram_ran);
            oled_show();
            HAL_Delay(120);
        }
    }

    /* Initial jack state (mcp_init primed the cache): plugged at boot →
     * speaker amp stays muted from the first un-mute onwards. */
    uint16_t prev_gpio = mcp_state();
    apply_amp_mute_for_jack((prev_gpio & (1u << MCP_BIT_JACK)) != 0);

    /* Jack-detect debounce state (0xFF = nothing pending). */
    uint8_t  jack_pending  = 0xFF;
    uint32_t jack_edge_ms  = 0;

    /* UI frame pacing. */
    uint32_t last_frame_ms = 0;
    bool     ui_dirty      = true;     /* draw the first frame immediately */

    /* Main loop. All gameplay LOGIC is hardware-independent (controls.c /
     * params.c / menu.c, host-tested) — this loop only ROUTES hardware
     * events into it. */
    for (;;) {
        uint32_t now = HAL_GetTick();

        /* --- 1. Encoder rotate/push → params + menu ---
         * r18.92: SHIFT + DISPLAY = backlight (SPEC transient overlay rule
         * — a hardware modifier, not a menu row). 5 %/detent, 10..100 %. */
        static int backlight_pct = 70;
        enc_event_t ev;
        while (enc_pop_event(&ev)) {
            if (ev.kind == ENC_EVENT_ROTATE) {
                if (ev.id == PARAM_ENC_DISPLAY) {
                    if (controls_modifier_active(MOD_SHIFT)) {
                        backlight_pct += ev.value * 5;
                        if (backlight_pct < 10)  backlight_pct = 10;
                        if (backlight_pct > 100) backlight_pct = 100;
                        leds_set_backlight((uint16_t)(LED_PWM_MAX *
                                                      backlight_pct / 100));
                    } else {
                        menu_rotate(ev.value);          /* DISPLAY: Menü   */
                        ui_dirty = true;
                    }
                } else {
                    /* r19.21: DRIVE/BRIGHT/VOLUME → params + Overlay */
                    knobs_rotate(ev.id, ev.value, now);
                }
            } else if (ev.kind == ENC_EVENT_PUSH) {
                /* r19.21: BEIDE Flanken in die Kurz/Lang-Klassifikation.
                 * DISPLAY-kurz ruft menu_push ueber den Callback (feuert
                 * jetzt auf der Loslass-Flanke statt auf Druck — noetig
                 * fuer die Lang-Druck-Unterscheidung). */
                knobs_push(ev.id, ev.value != 0, now);
                if (ev.id == PARAM_ENC_DISPLAY && !ev.value)
                    ui_dirty = true;            /* Menue evtl. veraendert */
            }
        }
        knobs_tick(now);                        /* Lang-Druck-Schwelle */
        scenes_ui_tick(now);                    /* Scenes-Idle-Timeout  */
        bloom_tick(now);                        /* r19.23: Akkord-Einsaetze */

        /* --- 2. MCP23017 → cells + modifiers + EN4-push + jack-detect ---
         * INT-driven: INTA (PC13, EXTI) latches s_irq_pending; mcp_service()
         * reads both ports (auto-clears the MCP interrupt). No polling —
         * the I²C bus stays free for the 60 Hz LED/VU writes.
         * r18.82: Cells sind DIGITALE Switches (GPA0-4, aktiv-LOW mit
         * Pull-up), keine Hall-ADCs mehr. Gleicher Edge-Pfad wie die
         * bench-erprobte Pico-Implementierung, geroutet durch die host-
         * getestete controls.c-Statemachine. */
        if (mcp_irq_pending()) {
            uint16_t gpio = mcp_service();
            uint16_t fell = (uint16_t)(prev_gpio & ~gpio);   /* 1→0 = press */
            uint16_t rose = (uint16_t)(~prev_gpio & gpio);   /* 0→1 = release */

            /* Cells GPA0-4: digital an/aus → feste Velocity (wie Pico-HAL).
             * r19.22: solange der Scenes-Browser offen ist, WAEHLEN die
             * Cells Slots (SHIFT+Cell speichert) statt Noten zu spielen. */
            for (uint8_t c = 0; c < 5; ++c) {
                uint16_t m = (uint16_t)(1u << (MCP_BIT_CELL1 + c));
                if (scenes_ui_active()) {
                    if (fell & m)
                        scenes_ui_cell(c, controls_modifier_active(MOD_SHIFT),
                                       now);
                } else if (s_cell_bloom) {
                    /* r19.23: BLOOM — Cell triggert den Akkord der Stufe. */
                    if (fell & m)
                        bloom_press(c, CELL_TAP_AMP,
                                    controls_modifier_active(MOD_HOLD), now);
                    if (rose & m) bloom_release(c, now);
                } else {
                    if (fell & m) controls_cell_press(c, CELL_TAP_AMP);
                    if (rose & m) controls_cell_release(c);
                }
            }

            /* Modifier GPB0-4 (bits 8-12). r19.20: BOTH edges reach
             * controls.c — SHIFT is momentary now (needs its release), and
             * feeding releases uniformly keeps the toggles honest too
             * (they only act on pressed=true). CLEAR press also drives the
             * confirm-flash that existed since r18.64 but was never wired. */
            if (fell & (1u<<MCP_BIT_MOD_SHIFT))    controls_modifier(MOD_SHIFT, true);
            if (rose & (1u<<MCP_BIT_MOD_SHIFT))    controls_modifier(MOD_SHIFT, false);
            if (fell & (1u<<MCP_BIT_MOD_HOLD))     controls_modifier(MOD_HOLD, true);
            if (rose & (1u<<MCP_BIT_MOD_HOLD))     controls_modifier(MOD_HOLD, false);
            if (fell & (1u<<MCP_BIT_MOD_DRONE))    controls_modifier(MOD_DRONE, true);
            if (rose & (1u<<MCP_BIT_MOD_DRONE))    controls_modifier(MOD_DRONE, false);
            if (fell & (1u<<MCP_BIT_MOD_GENERATE)) controls_modifier(MOD_GENERATE, true);
            if (rose & (1u<<MCP_BIT_MOD_GENERATE)) controls_modifier(MOD_GENERATE, false);
            if (fell & (1u<<MCP_BIT_MOD_CLEAR))  { controls_modifier(MOD_CLEAR, true);
                                                   bloom_all_off();  /* r19.23 */
                                                   leds_clear_flash(now); }
            if (rose & (1u<<MCP_BIT_MOD_CLEAR))    controls_modifier(MOD_CLEAR, false);

            /* EN4 (volume) push lives on GPB5 — feed the level into the
             * encoder layer so enc_pushed(4)/PUSH events stay uniform. */
            {
                uint16_t em = (uint16_t)(1u << MCP_BIT_ENC4_SW);
                if ((fell | rose) & em)
                    enc_set_ext_push(4, (gpio & em) == 0);   /* active-low */
            }

            /* Jack-Detect GPA6 (r18.82-Hardware: PJ-320D-Switch oeffnet beim
             * Einstecken → mit R_DET 10k/C_DET 1µF + MCP-Pull-up liest der
             * Pin unplugged ≈ 0,3 V = LOW, plugged = HIGH). Edge → arm the
             * debounce; applied below once stable for JACK_DEBOUNCE_MS. */
            {
                uint16_t jm = (uint16_t)(1u << MCP_BIT_JACK);
                if ((fell | rose) & jm) {
                    jack_pending = (gpio & jm) ? 1 : 0;
                    jack_edge_ms = now;
                }
            }

            prev_gpio = gpio;
        }

        /* --- 3. Jack debounce settle → PAM8403 /MUTE --- */
        if (jack_pending != 0xFF &&
            (uint32_t)(now - jack_edge_ms) >= JACK_DEBOUNCE_MS) {
            /* Still at the level that armed the debounce? (A bounce mid-
             * window re-armed it above, so reaching here means stable.) */
            apply_amp_mute_for_jack(jack_pending == 1);
            jack_pending = 0xFF;
        }

        /* --- 4. Generative autoplay (r18.88) — self-gating: silent until
         * MOD_GENERATE latches the bed on (controls.c), immediate first
         * note, humanized bars + chord-tone sparkles, live playing always
         * overrides. All timing inside engine_generative_tick(). */
        engine_generative_tick(now);

        /* --- 5. LED render @ ~60 Hz: state → PCA9685 (U6) → I²C --- */
        {
            static uint32_t last_ms = 0;
            uint16_t dt = (uint16_t)(now - last_ms);
            if (dt >= 16) {                /* 60 Hz tick */
                uint16_t pwm[LED_CH_COUNT];
                leds_render(now, dt, pwm);
                pca_set_all_pwm(pwm);      /* one auto-inc I²C burst, not 16 */
                last_ms = now;
            }
        }

        /* --- 6. Menu refresh, rate-limited to ~30 fps ---
         * Redraw only when the menu was touched or the accent crossfade is
         * still moving. oled_show_async() kicks a background SPI-DMA row
         * pipeline and returns immediately, so the ~29 ms panel write no
         * longer stalls the main loop (inputs/generative stay responsive).
         * The !oled_flush_busy() guard means we never rewrite the framebuffer
         * while the DMA is still reading it (r19.12). */
        {   /* r19.21: Overlay-Zustandswechsel erzwingen ein Redraw —
             * neues show() (Generation) oder Ablauf (aktiv→inaktiv). */
            static uint32_t ov_gen_seen = 0;
            static bool     ov_was      = false;
            static bool     sc_was      = false;
            bool ov_now = overlay_active(now);
            bool sc_now = scenes_ui_active();
            if (overlay_gen() != ov_gen_seen) { ov_gen_seen = overlay_gen(); ui_dirty = true; }
            if (ov_now != ov_was)             { ov_was = ov_now;             ui_dirty = true; }
            /* r19.22: Scenes-Screen lebt (LED-/Slot-Zustand) — waehrend er
             * offen ist einfach jedes 30-fps-Fenster neu zeichnen. */
            if (sc_now || sc_now != sc_was)   { sc_was = sc_now;             ui_dirty = true; }
        }
        if (ui_dirty && !oled_flush_busy() &&
            (uint32_t)(now - last_frame_ms) >= 33) {
            bool accent_moving = oled_accent_tick(now) != 0;
            if (scenes_ui_active())       scenes_ui_render();
            else if (overlay_active(now)) overlay_render();
            else                          menu_render();
            oled_show_async();
            ui_dirty      = accent_moving;
            last_frame_ms = now;
        }

        /* --- 7. Battery + USB-power poll @ 1 Hz (r18.92): BAT_SENSE ADC →
         * piecewise LiPo curve → menu battery glyph; GPA7 = VBUS detect. */
        {
            static uint32_t last_bat_ms = 0;
            if ((uint32_t)(now - last_bat_ms) >= 1000u) {
                last_bat_ms = now;
                float v = bat_adc_read_volts();
                int pct = battery_pct_from_voltage(v);
                bool usb = (mcp_state() & (1u << MCP_BIT_VBUS)) != 0;
                if (pct != battery_pct() || usb != battery_usb_present()) {
                    battery_set_pct(pct);
                    battery_set_usb_present(usb);
                    ui_dirty = true;                 /* refresh the glyph */
                }
            }
        }

        /* --- 8. MIDI Out pump (no-op until midi_hw_init() is re-enabled,
         * ADR-0004). Never blocks. --- */
        midi_hw_pump();
    }
}
