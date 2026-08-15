/*
 * test_ui_scene — the properties a drawing-as-interface has to hold.
 *
 * A scene cannot be checked for "looking right" in code, but three things that
 * would silently break it can:
 *
 *   1. it must actually respond to its parameters — a scene that renders the
 *      same pixels at 0 and at 100 is a picture, not an instrument;
 *   2. it must stay inside its own box, or it collides with the ring below and
 *      the one line of type above;
 *   3. it must never draw nothing, at any setting of any property.
 */
#include "../tools/ui_scene.h"
#include "oled.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond, ...) do {                            \
    if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); \
                   printf("\n"); ++failures; }            \
} while (0)

static const char *NAME[9] = { "World","Sound","Pitch","Harmony","Room",
                               "Time","Texture","Motion","FX" };

/* Scene box from ui_scene.c, plus a pixel of antialiasing headroom. */
#define BOX_X0 (40 - 2)
#define BOX_X1 (280 + 2)
#define BOX_Y0 (30 - 2)
#define BOX_Y1 (98 + 2)

static long render(const ui_scene_t *sc, int *x0, int *y0, int *x1, int *y1)
{
    uint16_t line[OLED_WIDTH];
    long lit = 0;
    *x0 = OLED_WIDTH; *y0 = OLED_HEIGHT; *x1 = -1; *y1 = -1;
    for (int y = 0; y < OLED_HEIGHT; ++y) {
        memset(line, 0, sizeof line);
        ui_scene_row(sc, y, line, 255);
        for (int x = 0; x < OLED_WIDTH; ++x)
            if (line[x]) {
                ++lit;
                if (x < *x0) *x0 = x;
                if (x > *x1) *x1 = x;
                if (y < *y0) *y0 = y;
                if (y > *y1) *y1 = y;
            }
    }
    return lit;
}

static void fill(ui_scene_t *sc, int g, float v)
{
    memset(sc, 0, sizeof *sc);
    sc->group = g;
    sc->n     = 3;
    sc->focus = 0;
    for (int i = 0; i < UI_SCENE_MAX_KNOBS; ++i) sc->v[i] = v;
}

static void test_draws_and_stays_in_box(void)
{
    static const float V[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
    for (int g = 0; g < 9; ++g)
        for (size_t k = 0; k < sizeof V / sizeof V[0]; ++k) {
            ui_scene_t sc;
            fill(&sc, g, V[k]);
            int x0, y0, x1, y1;
            long lit = render(&sc, &x0, &y0, &x1, &y1);

            CHECK(lit > 40, "%s at %.2f drew almost nothing (%ld px)",
                  NAME[g], (double)V[k], lit);
            if (lit == 0) continue;
            CHECK(x0 >= BOX_X0 && x1 <= BOX_X1,
                  "%s at %.2f escapes sideways: x %d..%d, box %d..%d",
                  NAME[g], (double)V[k], x0, x1, BOX_X0, BOX_X1);
            CHECK(y0 >= BOX_Y0 && y1 <= BOX_Y1,
                  "%s at %.2f escapes vertically: y %d..%d, box %d..%d — it "
                  "would run into the ring below or the label above",
                  NAME[g], (double)V[k], y0, y1, BOX_Y0, BOX_Y1);
        }
}

/* The whole claim of a scene is that the geometry carries the value. If two
 * very different settings render identical pixels, the drawing is decoration
 * and the parameter is invisible. */
static void test_responds_to_its_parameters(void)
{
    for (int g = 0; g < 9; ++g)
        for (int k = 0; k < UI_SCENE_MAX_KNOBS; ++k) {
            ui_scene_t lo, hi;
            fill(&lo, g, 0.5f);
            fill(&hi, g, 0.5f);
            lo.v[k] = 0.0f;
            hi.v[k] = 1.0f;

            uint16_t a[OLED_WIDTH], b[OLED_WIDTH];
            long diff = 0;
            for (int y = 0; y < OLED_HEIGHT; ++y) {
                memset(a, 0, sizeof a);
                memset(b, 0, sizeof b);
                ui_scene_row(&lo, y, a, 255);
                ui_scene_row(&hi, y, b, 255);
                for (int x = 0; x < OLED_WIDTH; ++x) if (a[x] != b[x]) ++diff;
            }
            /* Only knobs the scene actually uses — a group with one property
             * legitimately ignores the other two. */
            int used = (g == 1) ? 3 : (g == 0 || g == 7 || g == 8) ? 1 : 2;
            if (k >= used) continue;
            CHECK(diff > 30,
                  "%s knob %d changed only %ld px between 0 and 1 — the "
                  "drawing does not carry that parameter",
                  NAME[g], k, diff);
        }
}

int main(void)
{
    test_draws_and_stays_in_box();
    test_responds_to_its_parameters();

    if (failures) {
        printf("test_ui_scene: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_scene: OK — 9 scenes draw at every setting, stay in the "
           "box, and every property visibly moves its own geometry\n");
    return 0;
}
