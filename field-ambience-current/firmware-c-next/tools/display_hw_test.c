/*
 * display_hw_test — das Hardware-Gegenstück zur GitHub-Pages-Display-Sim.
 *
 * Die Sim (tools/display_sim.html, deployed via .github/workflows/pages.yml)
 * ist ein pixelgenauer JS-Port von src/menu.c. DIESES Programm macht das
 * Umgekehrte: es zeigt den ECHTEN menu.c-Renderer auf dem ECHTEN Panel —
 * Steckbrett-Aufbau mit Pico 2, bevor es eine Geräte-Platine gibt.
 *
 * ───────────────────────────────────────────────────────────────────────────
 * HARDWARE
 *   - Raspberry Pi Pico 2 (RP2350)
 *   - Adafruit 1.9" 320×170 IPS TFT, ST7789 (Adafruit #5394 / buyzero)
 *   - KY-040 Push-Encoder-Modul (Pins: GND, +, SW, DT, CLK)
 *   - 1× Tactile-Button (gegen GND, interner Pull-up)
 *
 * VERDRAHTUNG — Display (Pin-Namen wie auf dem Adafruit-Silk):
 *   Adafruit-Pin   Pico-GPIO   Pico-Pin (physisch)
 *   Vin            3V3 (OUT)   36          (Board hat eigenen Regler, 3-5 V ok)
 *   Gnd            GND         38 (o. a. GND)
 *   SCK            GP6         9           ← fest in src/lcd_st7789.c
 *   MOSI           GP7         10          ←        "
 *   TFT_CS         GP5         7           ←        "
 *   D/C            GP8         11          ←        "
 *   RST            GP9         12          ←        "
 *   Lite           offen lassen (Backlight dann an) — optional an 3V3
 *   MISO, SD_CS    unverbunden (SD-Slot wird nicht benutzt)
 *
 * VERDRAHTUNG — KY-040 Encoder:
 *   KY-040-Pin     Pico-GPIO   Pico-Pin (physisch)
 *   GND            GND         13
 *   +              3V3 (OUT)   36
 *   CLK            GP10        14
 *   DT             GP11        15
 *   SW             GP12        16
 *
 * VERDRAHTUNG — Tactile-Button:
 *   ein Bein an GP13 (Pico-Pin 17), das andere an GND (Pico-Pin 18).
 *
 * BUILD (Pico SDK 2.x, PICO_SDK_PATH gesetzt):
 *   cd firmware-c-next && mkdir -p build && cd build
 *   cmake .. -DPICO_BOARD=pico2
 *   make -j display_hw_test
 *   → build/display_hw_test.uf2 per BOOTSEL auf den Pico 2 ziehen.
 *
 * BEDIENUNG (Mapping zur Sim):
 *   Encoder drehen   = Display-Encoder der Sim → Menü browse / Wert editieren
 *   Encoder drücken  = Push der Sim           → Edit-Modus rein/raus
 *   Taster KURZ      = „Akku −10"-Knopf der Sim → 100→90→…→0→Lade-Glyph→100
 *   Taster LANG (1s) = Testbild weiterschalten:
 *                      MENU → RAMP → CHECKER → TYPE → MENU
 *
 * TESTBILDER (gibt es in der Sim nicht — Panel-Verifikation):
 *   RAMP    16 Graustufen-Balken + 1-px-Rahmen + Ecken-Ticks.
 *           Fehlt eine Rahmenkante oder ist Schwarz-Rand sichtbar →
 *           LCD_Y_OFFSET (35) bzw. MADCTL in lcd_st7789.c anpassen.
 *   CHECKER 1-px-Schachbrett — Pixelschärfe; Moiré/Schmieren → SPI zu
 *           schnell für die Kabel (LCD_SPI_HZ senken, s. CMakeLists).
 *   TYPE    Schrift-/Primitiven-Probe (Text 1×/2×/3×, Pille, Rundrechteck).
 *
 * TROUBLESHOOTING:
 *   Bild gespiegelt/kopfüber → LCD_MADCTL in lcd_st7789.c (0x60↔0xA0/0xC0/0x00)
 *   Menü dreht „falsch herum" → unten ENC_DIR auf -1 setzen (oder CLK/DT tauschen)
 *   Geister-Schritte beim Drehen → Kabel kürzen; Encoder-GND direkt zum Pico
 *   Display bleibt schwarz → erst Lite/Backlight prüfen (leuchtet das Panel?),
 *   dann USB-Konsole: dieses Programm loggt jeden Event über USB-CDC (115200).
 *
 * USB-Konsole: screen /dev/ttyACM0 115200  (oder minicom / PuTTY).
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

#include "oled.h"
#include "menu.h"
#include "battery.h"

/* --- bench pin map (display pins live in src/lcd_st7789.c) --------------- */
#define PIN_ENC_CLK 10
#define PIN_ENC_DT  11
#define PIN_ENC_SW  12
#define PIN_BTN     13

/* Flip to -1 if rotating clockwise moves the menu the wrong way. */
#define ENC_DIR (+1)

/* Long-press threshold for the tactile button (scene switch). */
#define LONG_PRESS_MS  800
/* Debounce: a level must be stable this long before it counts. */
#define DEBOUNCE_MS    20

/* --- 1 kHz sampler state (timer IRQ writes, main loop consumes) ---------- */
static volatile int  ev_detents;   /* accumulated encoder detents, signed   */
static volatile bool ev_push;      /* encoder switch short-press            */
static volatile bool ev_short;     /* tactile short-press                   */
static volatile bool ev_long;      /* tactile long-press                    */

/* Quadrature transition table: index = (prev<<2)|cur, two bits per line.
 * Invalid transitions (bounce) contribute 0, which is what makes the
 * accumulator approach robust on a noisy mechanical encoder. */
static const int8_t QDELTA[16] = {
     0, +1, -1,  0,
    -1,  0,  0, +1,
    +1,  0,  0, -1,
     0, -1, +1,  0,
};

typedef struct {
    uint     pin;
    bool     stable;       /* debounced level (true = released, pull-up)   */
    uint16_t flip_ms;      /* how long the raw level disagreed with stable */
    uint32_t held_ms;      /* time since debounced press                   */
    bool     long_fired;   /* long event already emitted for this hold     */
} button_t;

static button_t btn_sw  = { .pin = PIN_ENC_SW, .stable = true };
static button_t btn_tac = { .pin = PIN_BTN,    .stable = true };

/* Debounce + short/long classification, called at 1 kHz per button.
 * Short fires on release (before the threshold); long fires the moment the
 * threshold is reached, so the user gets feedback while still holding. */
static void button_tick(button_t *b, volatile bool *short_ev, volatile bool *long_ev) {
    bool raw = gpio_get(b->pin);            /* high = released (pull-up)    */
    if (raw != b->stable) {
        if (++b->flip_ms >= DEBOUNCE_MS) {  /* debounced edge               */
            b->stable  = raw;
            b->flip_ms = 0;
            if (!raw) {                     /* press                        */
                b->held_ms    = 0;
                b->long_fired = false;
            } else if (!b->long_fired && short_ev) {  /* release = short    */
                *short_ev = true;
            }
        }
    } else {
        b->flip_ms = 0;
    }
    if (!b->stable && !b->long_fired) {
        if (++b->held_ms >= LONG_PRESS_MS && long_ev) {
            b->long_fired = true;
            *long_ev = true;
        }
    }
}

static bool sampler_1khz(struct repeating_timer *t) {
    (void)t;

    /* Encoder: accumulate quadrature steps, count one detent per full
     * 4-transition cycle, latched at the rest position (CLK=DT=high on the
     * KY-040 detents). */
    static uint8_t q_prev = 3;
    static int     q_acc  = 0;
    uint8_t cur = (uint8_t)((gpio_get(PIN_ENC_CLK) << 1) | gpio_get(PIN_ENC_DT));
    q_acc += QDELTA[(q_prev << 2) | cur];
    q_prev = cur;
    if (cur == 3) {
        if (q_acc >= 4)       ev_detents += ENC_DIR;
        else if (q_acc <= -4) ev_detents -= ENC_DIR;
        q_acc = 0;
    }

    button_tick(&btn_sw,  &ev_push, NULL);      /* encoder push: short only */
    button_tick(&btn_tac, &ev_short, &ev_long); /* tactile: short + long    */
    return true;
}

/* --- battery demo (mirrors the sim's "Akku −10" / charge-source buttons) - */
static void battery_demo_step(void) {
    if (battery_usb_present()) {
        battery_set_usb_present(false);
        battery_set_pct(100);
    } else if (battery_pct() > 0) {
        battery_set_pct(battery_pct() - 10);
    } else {
        battery_set_usb_present(true);
    }
}

/* --- test scenes ---------------------------------------------------------
 * NOTE on labels: the 8x8 font (src/font_8x8.c) deliberately ships a partial
 * glyph set — no 'Q', no 'Z'. Keep scene labels inside 0-9 A-P R-Y . - + # % / *.
 */
typedef enum { SCENE_MENU = 0, SCENE_RAMP, SCENE_CHECKER, SCENE_TYPE, SCENE_COUNT } scene_t;

static const char *SCENE_NAME[SCENE_COUNT] = { "MENU", "RAMP", "CHECKER", "TYPE" };

static void render_frame_and_corners(void) {
    /* 1-px frame on the outermost rows/columns: if any edge is missing or a
     * black border shows, LCD_X/Y_OFFSET or MADCTL in lcd_st7789.c is wrong. */
    oled_rect_fill(0, 0, OLED_WIDTH, 1, 15);
    oled_rect_fill(0, OLED_HEIGHT - 1, OLED_WIDTH, 1, 15);
    oled_rect_fill(0, 0, 1, OLED_HEIGHT, 15);
    oled_rect_fill(OLED_WIDTH - 1, 0, 1, OLED_HEIGHT, 15);
    /* corner ticks pointing inwards (asymmetric on purpose: also catches
     * mirrored MADCTL) */
    oled_rect_fill(1, 1, 12, 3, 15);              /* top-left: long horiz    */
    oled_rect_fill(1, 1, 3, 12, 15);
    oled_rect_fill(OLED_WIDTH - 7, 1, 6, 3, 15);  /* top-right: short        */
    oled_rect_fill(1, OLED_HEIGHT - 4, 6, 3, 15); /* bottom-left: short      */
}

static void render_ramp(void) {
    oled_fill(0);
    for (int i = 0; i < 16; ++i) {
        int x0 = (OLED_WIDTH * i) / 16;
        int x1 = (OLED_WIDTH * (i + 1)) / 16;
        oled_rect_fill(x0, 24, x1 - x0, 104, (uint8_t)i);
    }
    render_frame_and_corners();
    oled_text(8, 8,   "RAMP 0-15", 12);
    oled_text(8, 150, "FRAME - PANEL EDGE - OFFSET OK", 8);
}

static void render_checker(void) {
    /* 1-px checkerboard: sharpness + SPI-integrity check. Smearing or noise
     * here usually means the SPI clock is too fast for the jumper wires. */
    for (int y = 0; y < OLED_HEIGHT; ++y)
        for (int x = 0; x < OLED_WIDTH; ++x)
            oled_pixel(x, y, ((x ^ y) & 1) ? 15 : 0);
    oled_rect_fill(6, OLED_HEIGHT - 18, 96, 12, 0);
    oled_text(8, OLED_HEIGHT - 16, "CHECKER 1PX", 12);
}

static void render_type(void) {
    oled_fill(0);
    oled_text(8, 8, "TYPE TEST", 12);
    oled_text(8, 26, "GREY 4 8 12 15", 4);
    oled_text(72, 26, "8", 8);
    oled_text(88, 26, "12", 12);
    oled_text(112, 26, "15", 15);
    oled_text_smooth(8, 44, "SMOOTH 2X", 15, 2);
    oled_text_smooth(8, 70, "SMOOTH 3X", 15, 3);
    /* the menu's own primitives: pill + AA rounded rect */
    oled_pill(8, 108, 80, 10, 15);
    oled_pill(96, 108, 24, 10, 6);
    oled_rrect_stroke(140, 100, 160, 56, 16, 2, 12);
    oled_text(152, 122, "RRECT R16", 10);
    render_frame_and_corners();
}

static void render_scene(scene_t s) {
    switch (s) {
        case SCENE_MENU:    menu_render();    break;
        case SCENE_RAMP:    render_ramp();    break;
        case SCENE_CHECKER: render_checker(); break;
        case SCENE_TYPE:    render_type();    break;
        default: break;
    }
    oled_show();
}

int main(void) {
    stdio_init_all();

    /* inputs: all active-low against GND, internal pull-ups. The KY-040
     * module has its own pull-ups on CLK/DT to '+'; the internal ones are
     * harmless in parallel and cover SW (module-dependent). */
    const uint pins[] = { PIN_ENC_CLK, PIN_ENC_DT, PIN_ENC_SW, PIN_BTN };
    for (unsigned i = 0; i < sizeof pins / sizeof pins[0]; ++i) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_up(pins[i]);
    }

    oled_init();
    menu_init(NULL);                 /* engine callbacks: none on the bench */
    battery_set_usb_present(false);
    battery_set_pct(100);

    scene_t scene = SCENE_MENU;
    render_scene(scene);

    struct repeating_timer timer;
    add_repeating_timer_us(-1000, sampler_1khz, NULL, &timer);

    printf("display_hw_test up — scene=%s\n", SCENE_NAME[scene]);

    uint32_t last_beat = to_ms_since_boot(get_absolute_time());
    while (true) {
        /* drain sampler events atomically */
        uint32_t irq = save_and_disable_interrupts();
        int  detents = ev_detents;  ev_detents = 0;
        bool push    = ev_push;     ev_push    = false;
        bool tshort  = ev_short;    ev_short   = false;
        bool tlong   = ev_long;     ev_long    = false;
        restore_interrupts(irq);

        bool dirty = false;

        if (tlong) {
            scene = (scene_t)((scene + 1) % SCENE_COUNT);
            printf("scene -> %s\n", SCENE_NAME[scene]);
            dirty = true;
        }
        if (tshort) {
            battery_demo_step();
            printf("battery: %d%% usb=%d\n", battery_pct(), battery_usb_present());
            dirty = true;            /* visible in the MENU scene only      */
        }
        if (detents != 0) {
            if (scene == SCENE_MENU) {
                menu_rotate(detents);
                printf("rotate %+d -> %s = %s\n", detents,
                       menu_current_label(), menu_current_value_text());
                dirty = true;
            } else {
                printf("rotate %+d (ignored in %s)\n", detents, SCENE_NAME[scene]);
            }
        }
        if (push) {
            if (scene == SCENE_MENU) {
                menu_push();
                printf("push -> mode=%s\n",
                       menu_mode() == MENU_EDIT ? "EDIT" : "BROWSE");
                dirty = true;
            } else {
                printf("push (ignored in %s)\n", SCENE_NAME[scene]);
            }
        }

        if (dirty) render_scene(scene);

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_beat >= 5000) {
            last_beat = now;
            printf("alive — scene=%s bat=%d%%\n", SCENE_NAME[scene], battery_pct());
        }

        sleep_ms(5);                 /* ~200 Hz UI poll, plenty for a menu  */
    }
}
