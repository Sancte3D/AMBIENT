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

    /* 01 — main wheel, World selected */
    ui_wheel_init(&st);
    ui_wheel_settle(&st);
    SHOT("01_main_world");

    /* 02 — mid-rotation: the frame that proves the branches survive motion */
    ui_wheel_turn(&st, +1, 0);
    ui_wheel_tick(&st, 70);           /* part-way through the eased snap */
    SHOT("02_main_rotating");

    /* 03 — settled on Space */
    ui_wheel_init(&st);
    for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);
    ui_wheel_settle(&st);
    SHOT("03_main_space");

    /* 04 — press: the group wheel for Space */
    ui_wheel_press(&st, 0);
    ui_wheel_settle(&st);
    SHOT("04_group_space");

    /* 05 — rotate to Shimmer inside the group */
    ui_wheel_turn(&st, +1, 0);
    ui_wheel_settle(&st);
    SHOT("05_group_shimmer");

    /* 06 — press: the ring is the parameter */
    ui_wheel_press(&st, 0);
    SHOT("06_value_shimmer");

    /* 07 — turned up */
    for (int i = 0; i < 12; ++i) ui_wheel_turn(&st, +1, 0);
    SHOT("07_value_turned");

    /* 08 — a discrete parameter: the ring still carries position */
    ui_wheel_init(&st);
    ui_wheel_settle(&st);
    ui_wheel_press(&st, 0);          /* World has one member -> straight to
                                      * the value, no sub-wheel */
    ui_wheel_turn(&st, +2, 0);
    SHOT("08_value_world");

    /* 09 — a three-member group, to show branch count changing with depth */
    ui_wheel_init(&st);
    ui_wheel_turn(&st, +1, 0);
    ui_wheel_settle(&st);
    ui_wheel_press(&st, 0);
    ui_wheel_settle(&st);
    SHOT("09_group_synth");

    /* ---- motion strips ---------------------------------------------------
     * A still cannot show whether a move is smooth. These sample the SAME
     * tween code the device runs, at a real 16 ms frame interval, so the
     * spacing between frames IS the easing curve rather than a description of
     * one. Frames bunching up toward the end of a strip means the motion is
     * decelerating into the lock, which is what an ease-out looks like. */
    for (int seq = 0; seq < 3; ++seq) {
        ui_wheel_init(&st);
        ui_wheel_settle(&st);
        if (seq == 1) {                       /* descending into a group */
            for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);
            ui_wheel_settle(&st);
            ui_wheel_press(&st, 0);
        } else if (seq == 2) {                /* a value being turned */
            for (int i = 0; i < 4; ++i) ui_wheel_turn(&st, +1, 0);
            ui_wheel_settle(&st);
            ui_wheel_press(&st, 0);
            ui_wheel_settle(&st);
            ui_wheel_press(&st, 0);
            ui_wheel_settle(&st);
            for (int i = 0; i < 6; ++i) ui_wheel_turn(&st, +1, 0);
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
