/*
 * OLED preview renderer — WORLD model (r18.38).
 *
 * Builds the menu state on the host, asks it to draw into the OLED framebuffer,
 * and writes the framebuffer to a COLOR PPM image through the device's own
 * accent-tinted grey→RGB565 conversion (ADR-0015 step 1). Lets the user SEE
 * the UI — including the per-world colour tint — before flashing.
 *
 * The output is upscaled 4× per pixel so the 320×170 framebuffer becomes a
 * 1280×680 image — easier to look at.
 *
 * Usage:
 *   render_oled out_dir/
 * Produces a browse frame per parameter + a few edit frames.
 */

#include "menu.h"
#include "battery.h"
#include "oled.h"
#include "oled_color.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Engine setters are no-ops for the preview — the menu fires them but we just
 * want pixels. */
static void noop_world (int v)   { (void)v; }
static void noop_key   (int v)   { (void)v; }
static void noop_voice (int v)   { (void)v; }
static void noop_tuning(int v)   { (void)v; }
static void noop_space (float v) { (void)v; }
static void noop_shim  (float v) { (void)v; }
static void noop_atmos (float v) { (void)v; }
static void noop_motion(float v) { (void)v; }
static void noop_age   (float v) { (void)v; }
static void noop_echo  (float v) { (void)v; }
static void noop_blur  (float v) { (void)v; }

/* Write the framebuffer as a 4× upscaled COLOR PPM (P6, 8-bit RGB) using the
 * SAME accent-tinted grey→RGB565 conversion the device driver uses (ADR-0015
 * step 1) — so the preview shows the per-world tint exactly as it will appear
 * on the panel. RGB565 is expanded back to RGB888 for the image. */
static void write_ppm(const char *path) {
    oled_accent_settle();          /* no frame loop here — snap to the target */
    const int SCALE = 4;
    const int W = OLED_WIDTH * SCALE, H = OLED_HEIGHT * SCALE;
    const uint8_t *fb = oled_framebuffer();
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    static uint8_t row[320 * 4 * 3];
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        for (int x = 0; x < OLED_WIDTH; ++x) {
            int idx = (y * OLED_WIDTH + x) >> 1;
            uint8_t v4 = (x & 1) ? (fb[idx] & 0x0F) : (fb[idx] >> 4);
            uint16_t px = oled_grey565(v4);
            uint8_t r5 = (px >> 11) & 0x1F, g6 = (px >> 5) & 0x3F, b5 = px & 0x1F;
            uint8_t r8 = (uint8_t)((r5 << 3) | (r5 >> 2));   /* 5→8 bit */
            uint8_t g8 = (uint8_t)((g6 << 2) | (g6 >> 4));   /* 6→8 bit */
            uint8_t b8 = (uint8_t)((b5 << 3) | (b5 >> 2));   /* 5→8 bit */
            for (int s = 0; s < SCALE; ++s) {
                uint8_t *o = row + (size_t)(x * SCALE + s) * 3;
                o[0] = r8; o[1] = g8; o[2] = b8;
            }
        }
        for (int s = 0; s < SCALE; ++s) fwrite(row, 1, (size_t)W * 3, f);
    }
    fclose(f);
}

static void to_param(menu_param_t p) {
    if (menu_mode() != MENU_BROWSE) menu_push();
    int guard = 0;
    while (menu_current() != p && guard++ < MP_COUNT * 2) menu_rotate(+1);
}

int main(int argc, char **argv) {
    const char *dir = (argc > 1) ? argv[1] : ".";

    menu_callbacks_t cb = {
        .set_world = noop_world,   .set_key = noop_key, .set_tuning = noop_tuning, .set_voice = noop_voice,
        .set_space = noop_space,   .set_shimmer = noop_shim,
        .set_atmosphere = noop_atmos,
        .set_motion = noop_motion, .set_age = noop_age,
        .set_echo = noop_echo,     .set_blur = noop_blur,
    };
    menu_init(&cb);

    battery_set_pct(72);
    battery_set_usb_present(false);

    char path[512];

    /* One BROWSE frame per parameter. */
    static const char *NAMES[MP_COUNT] = {
        "01_world","02_key","03_tuning","04_voice","05_space","06_shimmer",
        "07_atmos","08_motion","09_age","10_echo","11_blur"
    };
    for (int p = 0; p < MP_COUNT; ++p) {
        to_param((menu_param_t)p);
        menu_render();
        snprintf(path, sizeof path, "%s/menu_browse_%s.ppm", dir, NAMES[p]);
        write_ppm(path);
    }

    /* All four world names in browse mode (long-name fit check). */
    to_param(MP_WORLD);
    for (int w = 0; w < MENU_WORLD_COUNT; ++w) {
        menu_push();                       /* enter edit */
        /* step world to index w */
        int guard = 0;
        while (menu_world_index() != w && guard++ < MENU_WORLD_COUNT * 2)
            menu_rotate(+1);
        menu_push();                       /* back to browse */
        menu_render();
        snprintf(path, sizeof path, "%s/menu_world_%d.ppm", dir, w);
        write_ppm(path);
    }

    /* EDIT-mode previews. */
    to_param(MP_WORLD); menu_push();
    menu_render(); snprintf(path, sizeof path, "%s/menu_edit_world.ppm", dir); write_ppm(path);
    menu_push();
    to_param(MP_SPACE); menu_push();
    menu_render(); snprintf(path, sizeof path, "%s/menu_edit_space.ppm", dir); write_ppm(path);
    menu_push();
    to_param(MP_AGE); menu_push(); menu_rotate(+20);    /* nudge age */
    menu_render(); snprintf(path, sizeof path, "%s/menu_edit_age.ppm", dir); write_ppm(path);
    menu_push();

    /* USB-present variant (charging). */
    to_param(MP_WORLD);
    battery_set_usb_present(true);
    menu_render(); snprintf(path, sizeof path, "%s/menu_world_usb.ppm", dir); write_ppm(path);
    battery_set_usb_present(false);

    printf("wrote previews to %s/\n", dir);
    return 0;
}
