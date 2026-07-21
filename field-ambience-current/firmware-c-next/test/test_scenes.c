/*
 * Host test for r19.22 — parameter locks + 5 scene slots:
 *   - locks: locked params survive a world change, unlocked ones follow
 *   - lock toggle only works on lockable slots (Key + 7 macros)
 *   - scenes: save captures menu+params+gen-seed; recall restores them
 *   - recall does NOT touch the volume
 *   - persistence roundtrip through an injected RAM backend
 *   - scenes UI: open/cell-select/shift-save/idle-timeout, LED state
 *     sources (used/active) behave
 */
#include <stdio.h>
#include <string.h>
#include "scenes.h"
#include "menu.h"
#include "params.h"
#include "engine.h"
#include "brain.h"
#include "worlds.h"
#include "dsp.h"

static int checks = 0, fails = 0;
#define CHECK(cond, ...) do { ++checks; if (!(cond)) { ++fails; \
    fprintf(stderr, "FAIL %s:%d  ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

/* menu callbacks: route into the real engine (like the device main). */
static void cb_world(int w)        { engine_set_world(w); }
static void cb_key(int pc)         { engine_set_key(60 + pc); }
static void cb_space(float v)      { engine_set_reverb_size(v); }
static void cb_shimmer(float v)    { engine_set_shimmer(v); }
static void cb_atmos(float v)      { engine_set_atmosphere(v); }
static void cb_motion(float v)     { engine_set_motion(v); }
static void cb_age(float v)        { engine_set_age(v); }
static void cb_echo(float v)       { engine_set_echo(v); }
static void cb_blur(float v)       { engine_set_blur(v); }
static void cb_tuning(int t)       { engine_set_tuning(t); }
static void cb_voice(int v)        { engine_set_pad_voice(v); }
static void cb_synth(int sidx)     { engine_set_synth(sidx); }

/* RAM persistence backend. */
static unsigned char s_blob[512];
static unsigned      s_blob_len = 0;
static int           s_writes   = 0;
static bool ram_write(const void *b, unsigned len) {
    if (len > sizeof s_blob) return false;
    memcpy(s_blob, b, len); s_blob_len = len; ++s_writes; return true;
}
static bool ram_read(void *b, unsigned len) {
    if (s_blob_len == 0) return false;
    memcpy(b, s_blob, len < s_blob_len ? len : s_blob_len);
    return true;
}

static void menu_setup(void) {
    menu_callbacks_t m; memset(&m, 0, sizeof m);
    m.set_world = cb_world; m.set_key = cb_key; m.set_space = cb_space;
    m.set_shimmer = cb_shimmer; m.set_atmosphere = cb_atmos;
    m.set_motion = cb_motion; m.set_age = cb_age; m.set_echo = cb_echo;
    m.set_blur = cb_blur; m.set_tuning = cb_tuning; m.set_voice = cb_voice;
    m.set_synth = cb_synth;
    menu_init(&m);
}

/* Browse to a slot from MP_WORLD (menu starts in BROWSE on that slot). */
static void goto_slot(menu_param_t p) {
    menu_state_t st; menu_get_state(&st);       /* current values stay */
    while (menu_current() != p) menu_rotate(+1);
    (void)st;
}

int main(void) {
    printf("== scenes + parameter locks (r19.22) ==\n");
    dsp_init(); brain_init(); engine_init(); params_init();
    menu_setup();

    /* ---- 1. Locks: locked ECHO survives the world change ---- */
    goto_slot(MP_ECHO);
    menu_push();                                 /* EDIT */
    menu_rotate(+10);                            /* move echo off preset */
    int echo_set = menu_value_int(MP_ECHO);
    bool locked = menu_toggle_lock_current();
    CHECK(locked, "ECHO lock engages");
    CHECK(menu_param_locked(MP_ECHO), "lock bit visible");
    menu_push();                                 /* back to BROWSE */
    goto_slot(MP_WORLD);
    menu_push(); menu_rotate(+1); menu_push();   /* world +1 → preset load */
    CHECK(menu_value_int(MP_ECHO) == echo_set,
          "locked ECHO survived the world change (%d vs %d)",
          menu_value_int(MP_ECHO), echo_set);
    /* an UNlocked macro must have followed the new world preset */
    const world_t *w = worlds_get(menu_value_index(MP_WORLD));
    CHECK(menu_value_int(MP_SPACE) == w->space_pct,
          "unlocked SPACE follows the world preset");

    /* ---- 2. Lock rejected on non-lockable slot ---- */
    goto_slot(MP_TUNING);
    menu_push();
    CHECK(!menu_toggle_lock_current(), "TUNING is not lockable");
    menu_push();

    /* ---- 3. Scenes save/recall roundtrip ---- */
    scenes_init(ram_write, ram_read);
    CHECK(!scenes_used(0) && !scenes_used(4), "slots empty at boot");
    CHECK(scenes_active() == -1, "no active scene at boot");

    /* dial in a distinctive state */
    menu_state_t before;
    params_apply_scene(37, 240.0f);
    engine_set_gen_seed(0xABCD1234u);
    menu_get_state(&before);
    CHECK(scenes_save(2, 1000), "save to slot 2");
    CHECK(scenes_used(2), "slot 2 used");
    CHECK(scenes_active() == 2, "slot 2 active");
    CHECK(s_writes == 1, "save persisted through the backend");

    /* scramble everything */
    goto_slot(MP_WORLD);
    menu_push(); menu_rotate(+1); menu_push();
    params_apply_scene(80, -300.0f);
    engine_set_gen_seed(0x11111111u);
    int vol_before = params_volume_pct();

    CHECK(scenes_recall(2, 2000), "recall slot 2");
    menu_state_t after; menu_get_state(&after);
    CHECK(after.world == before.world && after.key_pc == before.key_pc &&
          after.space == before.space && after.echo == before.echo &&
          after.locks == before.locks,
          "menu state restored (world %d/%d)", after.world, before.world);
    CHECK(params_drive_pct() == 37, "drive restored (%d)", params_drive_pct());
    CHECK((int)params_bright_hz() == 240, "brightness restored (%d)",
          (int)params_bright_hz());
    CHECK(engine_gen_seed() == 0xABCD1234u, "generator seed restored");
    CHECK(params_volume_pct() == vol_before, "volume untouched by recall");

    /* ---- 4. Persistence: fresh init reloads from the backend ---- */
    scenes_init(ram_write, ram_read);
    CHECK(scenes_used(2), "slot 2 survives re-init (flash roundtrip)");
    CHECK(!scenes_used(0), "slot 0 still empty after re-init");
    CHECK(scenes_recall(2, 3000), "recall after re-init");
    CHECK(engine_gen_seed() == 0xABCD1234u, "seed survives the store");

    /* ---- 5. Scenes UI: open, save via SHIFT+cell, load via cell ---- */
    scenes_init(ram_write, ram_read);
    CHECK(!scenes_ui_active(), "UI closed at boot");
    scenes_ui_open(5000);
    CHECK(scenes_ui_active(), "UI open");
    scenes_ui_cell(0, true, 5100);               /* SHIFT+Cell1 = save   */
    CHECK(scenes_used(0), "SHIFT+cell saved slot 0");
    scenes_ui_cell(4, false, 5200);              /* empty slot: no-op    */
    CHECK(!scenes_used(4), "loading an empty slot is a no-op");
    scenes_ui_cell(0, false, 5300);              /* load slot 0          */
    CHECK(scenes_active() == 0, "cell load made slot 0 active");
    /* idle timeout closes the browser */
    scenes_ui_tick(5300 + SCENES_UI_IDLE_MS + 1);
    CHECK(!scenes_ui_active(), "UI closes after idle timeout");

    /* ---- 6. Scenes screen renders (framebuffer smoke) ---- */
    scenes_ui_open(20000);
    scenes_ui_render();
    scenes_ui_close();
    CHECK(1, "scenes render smoke");

    printf("\n%d checks, %d failures\n", checks, fails);
    printf("RESULT: %s\n", fails ? "FAIL" : "PASS");
    return fails ? 1 : 0;
}
