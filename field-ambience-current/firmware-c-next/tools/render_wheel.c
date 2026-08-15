/*
 * render_wheel — the radial navigation system, rendered through the SAME row
 * compositor the Pico bench uses. Not a mockup: identical geometry, identical
 * Bitcount glyph data, identical blend arithmetic, so anything unreadable here
 * is unreadable on the panel.
 *
 *   cc -O2 -std=c11 -Iinclude -Itools -o /tmp/render_wheel \
 *       tools/render_wheel.c tools/ui_wheel.c tools/ui_draw.c \
 *       src/baked_font.c src/baked_font_data.c src/oled_draw.c \
 *       src/oled_color.c src/font_8x8.c -lm
 *   /tmp/render_wheel <outdir>
 *
 * Writes the full rotate -> press -> rotate -> press flow plus a mid-rotation
 * frame, which is the one that shows whether the branches read while moving.
 */
#include "ui_wheel.h"
#include "oled.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_ppm(const char *path, const wheel_state_t *st)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", OLED_WIDTH, OLED_HEIGHT);

    uint16_t line[OLED_WIDTH];
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        ui_wheel_compose_row(st, y, line);
        for (int x = 0; x < OLED_WIDTH; ++x) {
            int r5 = (line[x] >> 11) & 0x1F;
            int g6 = (line[x] >> 5)  & 0x3F;
            int b5 =  line[x]        & 0x1F;
            unsigned char px[3] = {
                (unsigned char)((r5 << 3) | (r5 >> 2)),
                (unsigned char)((g6 << 2) | (g6 >> 4)),
                (unsigned char)((b5 << 3) | (b5 >> 2)),
            };
            fwrite(px, 1, 3, f);
        }
    }
    fclose(f);
    printf("  %s\n", path);
}

int main(int argc, char **argv)
{
    const char *dir = (argc > 1) ? argv[1] : ".";
    char path[512];
    wheel_state_t st;

#define SHOT(tag) do { snprintf(path, sizeof path, "%s/%s.ppm", dir, tag); \
                       write_ppm(path, &st); } while (0)

    /* 01 — the wheel itself */
    ui_wheel_init(&st);
    ui_wheel_settle(&st);
    SHOT("01_wheel_world");

    /* 02 — mid-rotation: the frame that proves the branches survive motion */
    ui_wheel_turn(&st, +1, 0);
    ui_wheel_tick(&st, 70);           /* part-way through the eased snap */
    SHOT("02_wheel_rotating");

    /* 02b — settled four groups along, still on the wheel. Pairs with
     * 07_scene_room to show the same group before and after the press. */
    ui_wheel_init(&st);
    for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);
    ui_wheel_settle(&st);
    SHOT("02b_wheel_room");

    /* 03..11 — every scene, settled, at its seeded values. One per group:
     * this is the sheet that shows whether nine drawings actually read as nine
     * different places rather than nine variations of the same widget. */
    static const char *SCENE_TAG[WHEEL_GROUPS] = {
        "03_scene_world", "04_scene_sound", "05_scene_pitch",
        "06_scene_harmony", "07_scene_room", "08_scene_time",
        "09_scene_texture", "10_scene_motion", "11_scene_fx"
    };
    for (int g = 0; g < WHEEL_GROUPS; ++g) {
        ui_wheel_init(&st);
        for (int i = 0; i < g; ++i) ui_wheel_turn(&st, +1, 0);
        ui_wheel_settle(&st);
        ui_wheel_press(&st, 0);
        ui_wheel_settle(&st);
        snprintf(path, sizeof path, "%s/%s.ppm", dir, SCENE_TAG[g]);
        write_ppm(path, &st);
    }

    /* 12/13 — the same scene at two very different settings, which is the
     * whole claim: the geometry carries the value, not a number beside it. */
    ui_wheel_init(&st);
    for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);   /* Room */
    ui_wheel_settle(&st);
    ui_wheel_press(&st, 0);
    for (int i = 0; i < 40; ++i) ui_wheel_turn(&st, -1, 0);
    ui_wheel_settle(&st);
    SHOT("12_room_small");
    for (int i = 0; i < 60; ++i) ui_wheel_turn(&st, +1, 0);
    ui_wheel_settle(&st);
    SHOT("13_room_large");

    /* ---- motion strips ---------------------------------------------------
     * A still cannot show whether a move is smooth. These sample the SAME
     * tween code the device runs, at a real 16 ms frame interval, so the
     * spacing between frames IS the easing curve rather than a description of
     * one. Frames bunching up toward the end of a strip means the motion is
     * decelerating into the lock, which is what an ease-out looks like. */
    for (int seq = 0; seq < 3; ++seq) {
        ui_wheel_init(&st);
        ui_wheel_settle(&st);
        if (seq == 1) {                       /* opening a scene */
            for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);
            ui_wheel_settle(&st);
            ui_wheel_press(&st, 0);
        } else if (seq == 2) {                /* a property being turned */
            for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);
            ui_wheel_settle(&st);
            ui_wheel_press(&st, 0);
            ui_wheel_settle(&st);
            for (int i = 0; i < 8; ++i) ui_wheel_turn(&st, +1, 0);
        } else {                              /* one wheel step */
            ui_wheel_turn(&st, +1, 0);
        }

        for (int f = 0; f < 18; ++f) {
            snprintf(path, sizeof path, "%s/anim%d_%02d.ppm", dir, seq, f);
            write_ppm(path, &st);
            ui_wheel_tick(&st, 16);
        }
    }

#undef SHOT
    return 0;
}
