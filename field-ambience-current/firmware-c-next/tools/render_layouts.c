/*
 * render_layouts — run the three candidate layouts through the SAME row
 * compositor the Pico bench uses and write what the panel would show.
 *
 * The point is that this is not a mockup. It links tools/ui_layouts.c and the
 * baked plate unchanged, so every pixel here is a pixel the device produces:
 * same Bitcount glyph data, same RGB565 plate, same blend arithmetic. If a
 * label is unreadable in these files it will be unreadable on the panel.
 *
 *   cc -O2 -std=c11 -Iinclude -Iassets -Itools -o /tmp/render_layouts \
 *       tools/render_layouts.c tools/ui_layouts.c assets/plate_plain.c \
 *       src/baked_font.c src/baked_font_data.c src/oled_draw.c \
 *       src/oled_color.c src/font_8x8.c
 *   /tmp/render_layouts <outdir>
 *   python3 tools/make_layout_sheet.py <outdir> <outdir>/LAYOUT_COMPARISON.png
 *
 * Writes one PPM per (layout, state). tools/make_layout_sheet.py turns them
 * into the side-by-side comparison and the 3x panel-scale views.
 */
#include "ui_layouts.h"
#include "oled.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_ppm(const char *path, const ui_state_t *st)
{
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%d %d\n255\n", OLED_WIDTH, OLED_HEIGHT);

    uint16_t line[OLED_WIDTH];
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        ui_compose_row(st, y, line);
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

    /* One continuous parameter and one discrete one per layout: the two cases
     * differ in widget AND in text length, which is where legibility breaks. */
    static const struct { const char *tag; int sel; int edit; }
    STATES[] = {
        { "browse_space", 4,  0 },   /* Space   — continuous, short value  */
        { "edit_world",   0,  1 },   /* World   — discrete, longest value  */
        { "browse_fx",    15, 0 },   /* FX      — discrete, 9 options      */
    };
    static const char *LAYOUT[UI_LAYOUT_COUNT] = { "a_focus", "b_context", "c_pages" };

    for (int l = 0; l < UI_LAYOUT_COUNT; ++l) {
        printf("%s\n", LAYOUT[l]);
        for (size_t s = 0; s < sizeof STATES / sizeof STATES[0]; ++s) {
            ui_state_t st;
            ui_init(&st, (ui_layout_t)l);
            st.sel  = (uint8_t)STATES[s].sel;
            st.edit = (uint8_t)STATES[s].edit;
            snprintf(path, sizeof path, "%s/%s_%s.ppm", dir, LAYOUT[l], STATES[s].tag);
            write_ppm(path, &st);
        }
    }
    return 0;
}
