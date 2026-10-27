/* Actual H743 input function + real Product UI and audio. Only hardware
 * queues/tick injected. Wake gestures compare against no-input controls. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "brain.h"
#include "cells.h"
#include "controls.h"
#include "cell_router.h"
#include "params.h"
#include "menu.h"
#include "leds.h"
#include "scenes.h"
#include "knobs.h"
#include "overlay.h"
#include "bloom.h"
#include "landscape.h"
#include "gesture.h"
#include "encoders.h"
#include "mcp23017.h"
#include "ui_idle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define CELL_TAP_AMP 0.12f
enum { CELL_NOTE, CELL_HARMONY, CELL_LAND, FRAMES = 12 * 44100 };
static int s_cell_mode;
static uint32_t s_field_seed, clock_ms;
static cell_router_t s_cell_router;
static uint16_t gpio, prev_gpio;
static uint8_t jack_pending;
static uint32_t jack_edge;
static bool pending, dirty;
static enc_event_t queue[32];
static bool enc_down[ENC_COUNT];
static unsigned qin, qout, display_short, display_long, write_calls;
static unsigned ons, offs;
static int16_t expected[FRAMES * 2], actual[FRAMES * 2];

static uint32_t HAL_GetTick(void) { return clock_ms; }
bool enc_pop_event(enc_event_t *out) {
    if (qout == qin) return false;
    *out = queue[qout++]; return true;
}
static void enqueue(unsigned id, enc_event_kind_t kind, int value) {
    assert(qin < 32); queue[qin++] = (enc_event_t){kind, (uint8_t)id, (int8_t)value};
}
bool enc_pushed(uint8_t id) { return enc_down[id - 1]; }
void enc_set_ext_push(uint8_t id, bool down) {
    if (enc_down[id - 1] != down) {
        enc_down[id - 1] = down; enqueue(id, ENC_EVENT_PUSH, down);
    }
}
bool mcp_irq_pending(void) { return pending; }
uint16_t mcp_service(void) { pending = false; return gpio; }
static void inputs(uint16_t down) { gpio = (uint16_t)~down; pending = true; }
static void short_cb(void) { ++display_short; menu_push(); }
static void long_cb(void) { ++display_long; scenes_ui_open(clock_ms); }
static bool store(const void *p, unsigned n) { (void)p; (void)n; ++write_calls; return false; }
static bool load(void *p, unsigned n) { (void)p; (void)n; return false; }
static void hook(int on, uint8_t owner, float hz, float velocity) {
    (void)owner; (void)hz; (void)velocity;
    if (on > 0) ++ons;
    if (on == 0) ++offs;
}

#include "ui_h743_routing.inc"

static void reset(int world, uint32_t epoch) {
    dsp_init(); brain_init(); engine_init(); engine_boot_mute();
    menu_callbacks_t cb = {.set_world = engine_set_world, .set_key = engine_set_key_pc,
        .set_tuning = engine_set_tuning, .set_attack = engine_set_attack,
        .set_release = engine_set_release, .set_space = engine_set_space,
        .set_atmosphere = engine_set_atmosphere};
    menu_init(&cb); controls_init(); params_init(); overlay_init(); bloom_init();
    landscape_init(NULL);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    gesture_init_owned(engine_gesture_note);
#else
    gesture_init(route_cell);
#endif
    scenes_init(store, load);
    knobs_callbacks_t kb = {.display_push = short_cb, .display_long = long_cb};
    knobs_init(&kb); leds_init(); s_backlight_pct = 70;
    leds_set_backlight(LED_PWM_MAX * 70 / 100);
    memset(&s_cell_router, 0, sizeof s_cell_router); s_cell_mode = CELL_NOTE;
    s_field_seed = 0x1234; qin = qout = 0; pending = dirty = false;
    memset(enc_down, 0, sizeof enc_down);
    gpio = prev_gpio = UINT16_MAX; jack_pending = 0xff; jack_edge = 0;
    display_short = display_long = write_calls = ons = offs = 0;
    engine_set_world(world); engine_generative_new_field(42);
    controls_modifier(MOD_GENERATE, true); engine_set_note_hook(hook);
    init_human_inputs(epoch - (UI_IDLE_MS - 1000), gpio);
}

static void poll(uint32_t ms) {
    clock_ms = ms;
    poll_human_inputs(ms, &prev_gpio, &jack_pending, &jack_edge, &dirty);
    if (qin == qout) qin = qout = 0;
}

static void run(int world, int wake, bool idle, uint32_t epoch, int cap, int16_t *pcm) {
    reset(world, epoch);
    for (unsigned t = 0; t < 12000; t += 10) {
        uint32_t now = epoch + t;
        if (!idle) s_ui_idle.last_ms = now; /* untouched awake control */
        if (idle && t == 2000) {
            if (wake < 10) {
                unsigned key = wake < 5 ? (unsigned)wake : 8u + (unsigned)wake - 5u;
                inputs((uint16_t)(1u << key));
            } else if (wake < 14) enqueue((unsigned)wake - 9, ENC_EVENT_PUSH, 1);
            else if (wake < 18) enqueue((unsigned)wake - 13, ENC_EVENT_ROTATE, 1);
            else if (wake == 18) {
                /* All simultaneous MCP presses and queued encoder events
                 * are wake-only, not Save/Clear/Generate/menu/parameter work. */
                inputs(0x1f1f); enqueue(4, ENC_EVENT_ROTATE, 1);
                enqueue(3, ENC_EVENT_PUSH, 1);
            } else {
                inputs(1u << MCP_BIT_ENC4_SW);
                if (wake == 20) enqueue(4, ENC_EVENT_ROTATE, 1);
            }
        }
        if (idle && t == 2600) {
            inputs(0);
            if (wake >= 10 && wake < 14) enqueue((unsigned)wake - 9, ENC_EVENT_PUSH, 0);
            if (wake == 18) enqueue(3, ENC_EVENT_PUSH, 0);
        }
        /* Identical later human actions still work on both runs. */
        if (t == 4000) enqueue(4, ENC_EVENT_ROTATE, 1);
        if (t == 6000) inputs(1u << MCP_BIT_MOD_GENERATE);
        if (t == 6010) inputs(0);
        if (t == 8000) inputs(1u << MCP_BIT_CELL1);
        if (t == 8500) inputs(0);
        poll(now);
        if (t == 1500 && idle) assert(s_ui_idle.quiet);
        if (t == 2020 && idle) assert(!s_ui_idle.quiet && dirty);
        if (t == 3000) {
            assert(!s_ui_idle.down && !s_ui_idle.suppressed);
            assert(!display_short && !display_long && !write_calls);
            assert(params_volume_pct() == 30 && !params_muted());
            assert(controls_modifier_active(MOD_GENERATE));
            assert(!controls_modifier_active(MOD_SHIFT));
            assert(!controls_modifier_active(MOD_HOLD));
            assert(s_backlight_pct == 70);
        }
        engine_generative_tick(now);
        uint16_t pwm[LED_CH_COUNT]; leds_render(now, 10, pwm);
        if (idle && t == 1500) for (unsigned c = 0; c < LED_CH_COUNT; ++c) assert(pwm[c] == 0);
        if (idle && t == 3000) assert(pwm[15] == LED_PWM_MAX * 70 / 100);
        unsigned done = 0;
        while (done < 441) {
            unsigned n = 441 - done;
            if (n > (unsigned)cap) n = (unsigned)cap;
            engine_render(pcm + ((t / 10) * 441 + done) * 2, (int)n);
            done += n;
        }
    }
    assert(params_volume_pct() == 31);
    assert(!controls_modifier_active(MOD_GENERATE));
    assert(ons > 0 && offs > 0);
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}

/* Actual MCP/H743 path: physical up is honored even under the Scene UI. */
static void expression_route(int world,bool scene,bool hardware,int16_t *pcm) {
    reset(world,100);controls_modifier(MOD_GENERATE,true);
    controls_modifier(MOD_HOLD,true);
    for(unsigned t=0;t<6000;t+=10) {
        uint32_t now=100+t;s_ui_idle.last_ms=now;
        if(t==0) {
            if(hardware)inputs(1u<<MCP_BIT_CELL1);
            else {controls_cell_press(0,CELL_TAP_AMP);controls_cell_physical(0,true);}
        }
        if(scene && t==4000)scenes_ui_open(now);
        if(t==4500) {
            if(hardware)inputs(0);
            else if(scene)controls_cell_physical(0,false);
            else controls_cell_release(0);
        }
        poll(now);engine_generative_tick(now);
        engine_render(pcm+2*(t/10)*441,441);
    }
    assert(controls_hold_base(0));
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}

#ifdef FAM_SOUND_FINISH_CANDIDATE
static void nature_menu(void) {
    for(int world=0;world<3;++world) {
        reset(world,100);engine_set_generative(false,-1);engine_set_master_volume(.75f);
        for(int i=0;menu_current()!=MP_ATMOS && i<MP_COUNT;++i)menu_rotate(1);
        assert(menu_current()==MP_ATMOS && !strcmp(menu_current_label(),"Nature"));
        assert(menu_value_int(MP_ATMOS)==0);menu_push();menu_rotate(65);
        assert(menu_value_int(MP_ATMOS)==65 && !strcmp(menu_current_value_text(),"65%"));
        int16_t pcm[1024];unsigned audible=0;
        for(int f=0;f<10*44100;f+=512) {
            engine_render(pcm,512);
            for(int i=0;i<1024;++i)if(pcm[i])++audible;
        }
        assert(audible && !engine_active_voices() && !ons);
        menu_rotate(-65);assert(menu_value_int(MP_ATMOS)==0);
        for(int f=0;f<31*44100;f+=512)engine_render(pcm,512);
        for(int i=0;i<1024;++i)assert(!pcm[i]);
        assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
    }
    puts("H743 Nature menu: existing 0/65/0% editor, actual Engine audio without voices, all Worlds and settled silence pass");
}
#endif
int main(void) {
#ifdef FAM_SOUND_FINISH_CANDIDATE
    nature_menu();
#endif
    /* Real boot initializer must prime the indirect EN4 level before its
     * snapshot. Its queued down and later up are both consumed, no Mute. */
    reset(0, 100);
    gpio = prev_gpio = (uint16_t)~(1u << MCP_BIT_ENC4_SW);
    init_human_inputs(100, gpio);
    assert(s_ui_idle.down == (1u << 19) && s_ui_idle.suppressed == (1u << 19));
    poll(100 + UI_IDLE_MS);
    assert(!s_ui_idle.quiet && !params_muted());
    inputs(0); poll(110 + UI_IDLE_MS); poll(120 + UI_IDLE_MS);
    assert(!s_ui_idle.down && !s_ui_idle.suppressed && !params_muted());
    poll(119 + 2 * UI_IDLE_MS); assert(!s_ui_idle.quiet);
    poll(120 + 2 * UI_IDLE_MS); assert(s_ui_idle.quiet);
    for(int world=0;world<3;++world) for(int scene=0;scene<2;++scene) {
        expression_route(world,scene,false,expected);
        expression_route(world,scene,true,actual);
        assert(!memcmp(expected,actual,6*44100*2*sizeof *actual));
    }
    puts("H743 physical unfold: all Worlds, normal and Scene-overlay release match explicit physical controls");
    unsigned cases = 0;
    for (int world = 0; world < 3; ++world) for (int partition = 0; partition < 2; ++partition) {
        int cap = partition ? 512 : 64;
        uint32_t epoch = partition ? UINT32_MAX - 2500 : 100;
        run(world, 0, false, epoch, cap, expected);
        unsigned saved_ons = ons, saved_offs = offs;
        for (int wake = 0; wake < 21; ++wake) {
            run(world, wake, true, epoch, cap, actual);
            assert(memcmp(expected, actual, sizeof actual) == 0);
            assert(ons == saved_ons && offs == saved_offs);
            ++cases;
        }
    }
    printf("H743 UI: %u whole 12-second PCM comparisons, 21 wake routes, all Worlds, 64/512 caps and Main wrap pass\n", cases);
}
