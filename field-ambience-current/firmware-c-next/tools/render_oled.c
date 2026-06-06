/*
 * OLED preview renderer — Step 12b #7.
 *
 * Builds the menu state on the host, asks it to draw into the OLED framebuffer,
 * and writes the framebuffer to a PGM image. Lets the user (and the design
 * reviewer) SEE the UI before flashing — same renderer the device runs.
 *
 * The output is upscaled 4× per pixel so the 256×64 framebuffer becomes a
 * 1024×256 image — easier to look at and to compare against the mockup.
 *
 * Usage:
 *   render_oled out_dir/
 * Produces:
 *   out_dir/menu_browse_<param>.pgm    one per slot in browse mode
 *   out_dir/menu_edit_<param>.pgm      one per slot in edit mode
 */

#include "menu.h"
#include "battery.h"
#include "oled.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Engine setters are no-ops for the preview — the menu fires them but we just
 * want pixels. */
static void noop_key (int v)         { (void)v; }
static void noop_mode(int v)         { (void)v; }
static void noop_vibe(int v)         { (void)v; }
static void noop_voi (int v)         { (void)v; }
static void noop_dro (bool b)        { (void)b; }
static void noop_gen (bool b, int p) { (void)b; (void)p; }
static void noop_t   (float v)       { (void)v; }
static void noop_b   (float v)       { (void)v; }
static void noop_s   (float v)       { (void)v; }
static void noop_m   (float v)       { (void)v; }
static void noop_v   (float v)       { (void)v; }

/* Write the framebuffer as a 4× upscaled PGM image (P5, 8-bit grey). */
static void write_pgm(const char *path) {
    const int SCALE = 4;
    const int W = OLED_WIDTH * SCALE, H = OLED_HEIGHT * SCALE;
    const uint8_t *fb = oled_framebuffer();
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return; }
    fprintf(f, "P5\n%d %d\n255\n", W, H);
    static uint8_t row[256 * 4];
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        for (int x = 0; x < OLED_WIDTH; ++x) {
            int idx = (y * OLED_WIDTH + x) >> 1;
            uint8_t v4 = (x & 1) ? (fb[idx] & 0x0F) : (fb[idx] >> 4);
            uint8_t v8 = (uint8_t)((v4 << 4) | v4);     /* 4-bit → 8-bit */
            for (int s = 0; s < SCALE; ++s) row[x * SCALE + s] = v8;
        }
        for (int s = 0; s < SCALE; ++s) fwrite(row, 1, (size_t)W, f);
    }
    fclose(f);
}

static void to_param(menu_param_t p) {
    /* navigation only works in browse mode; in edit mode rotate changes the
     * value, not the slot — so leave edit first. */
    if (menu_mode() != MENU_BROWSE) menu_push();
    int guard = 0;
    while (menu_current() != p && guard++ < MP_COUNT * 2) menu_rotate(+1);
}

int main(int argc, char **argv) {
    const char *dir = (argc > 1) ? argv[1] : ".";

    menu_callbacks_t cb = {
        noop_key, noop_mode, noop_vibe, noop_voi, noop_dro, noop_gen,
        noop_t, noop_b, noop_s, noop_m, noop_v
    };
    menu_init(&cb);

    /* Plausible battery state to match the mockup. */
    battery_set_pct(72);
    battery_set_usb_present(false);

    /* Pre-set a couple of values so the previews are not all defaults. */
    /* MODE → lydian */
    to_param(MP_MODE); menu_push(); menu_rotate(+3); menu_push();
    /* VIBE → bright */
    to_param(MP_VIBE); menu_push(); menu_rotate(+1); menu_push();
    /* TEXTURE → 35 */
    to_param(MP_TEXTURE); menu_push(); menu_rotate(+3); menu_push();
    /* DRONE on */
    to_param(MP_DRONE); menu_push(); menu_rotate(+1); menu_push();
    /* GENERATIVE → markov */
    to_param(MP_GENERATIVE); menu_push(); menu_rotate(+6); menu_push();

    /* Render one BROWSE frame per param. */
    static const char *NAMES[MP_COUNT] = {
        "01_key","02_mode","03_vibe","04_voice","05_drone","06_gen",
        "07_texture","08_bass","09_space","10_mood","11_vol"
    };
    char path[512];
    for (int p = 0; p < MP_COUNT; ++p) {
        to_param((menu_param_t)p);
        menu_render();
        snprintf(path, sizeof path, "%s/menu_browse_%s.pgm", dir, NAMES[p]);
        write_pgm(path);
    }

    /* Render EDIT-mode previews for a couple of slots. */
    to_param(MP_KEY); menu_push();
    menu_render(); snprintf(path, sizeof path, "%s/menu_edit_01_key.pgm", dir); write_pgm(path);
    menu_push();
    to_param(MP_MODE); menu_push();
    menu_render(); snprintf(path, sizeof path, "%s/menu_edit_02_mode.pgm", dir); write_pgm(path);
    menu_push();
    to_param(MP_VOLUME); menu_push();
    menu_render(); snprintf(path, sizeof path, "%s/menu_edit_11_vol.pgm", dir); write_pgm(path);

    /* USB-present variant (charging) on KEY. */
    to_param(MP_KEY);
    battery_set_usb_present(true);
    menu_render(); snprintf(path, sizeof path, "%s/menu_browse_01_key_usb.pgm", dir); write_pgm(path);

    printf("wrote previews to %s/\n", dir);
    return 0;
}
