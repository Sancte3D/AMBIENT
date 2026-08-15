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

/* THE COLOUR RULE, checked rather than asserted in a comment.
 *
 * A scene may contain exactly two kinds of ink: the dim structural grey, and
 * the colours of the encoders. If a stroke is coloured, a knob moves it; if it
 * is grey, no knob does. That is the entire labelling system — the value has no
 * name written next to it, only a colour that matches the knob under the hand —
 * so a stray fourth tone is not a cosmetic slip, it is a control the user
 * cannot attribute to anything.
 *
 * Everything is blended over black at coverage a, so a lit pixel is the source
 * colour scaled by a/255. The check is therefore on DIRECTION, not on value:
 * antialiased edges are dim versions of a legal colour, and a mixture of two
 * legal colours is not. */
#define N_INK (1 + UI_SCENE_MAX_KNOBS)
#define SLACK 14.0f      /* 5-6-5 costs 8 in r/b and 4 in g; this clears both */

static void ink(int c, float out[3])
{
    static const float DIM_GREY[3] = { 42.0f, 42.0f, 42.0f };
    if (c == 0) { out[0] = DIM_GREY[0]; out[1] = DIM_GREY[1]; out[2] = DIM_GREY[2]; }
    else for (int j = 0; j < 3; ++j) out[j] = (float)UI_KNOB_RGB[c - 1][j];
}

/* One ink at some coverage: the pixel is that colour scaled down. */
static int is_single_ink(const float px[3])
{
    for (int c = 0; c < N_INK; ++c) {
        float col[3];
        ink(c, col);
        int big = 0;
        for (int j = 1; j < 3; ++j) if (col[j] > col[big]) big = j;
        float s = px[big] / col[big];
        if (s > 1.05f) continue;
        int ok = 1;
        for (int j = 0; j < 3; ++j) {
            float d = px[j] - s * col[j];
            if (d < -SLACK || d > SLACK) ok = 0;
        }
        if (ok) return 1;
    }
    return 0;
}

/* Where two legal strokes cross, the pixel is one ink blended over the other:
 * px = w1*C1 + w2*C2 with w >= 0 and w1 + w2 <= 1, which is what sequential
 * alpha compositing over black produces. That is a legitimate mixture, so it
 * has to be allowed — but only between two colours that are themselves legal.
 * A purple pixel where red crosses blue is the drawing working; a purple pixel
 * on its own is a fourth ink. */
static int is_ink_pair(const float px[3])
{
    for (int a = 0; a < N_INK; ++a)
        for (int b = a + 1; b < N_INK; ++b) {
            float ca[3], cb[3];
            ink(a, ca);
            ink(b, cb);
            for (int i = 0; i <= 32; ++i) {
                float w1 = i / 32.0f;
                for (int j = 0; j + i <= 32; ++j) {
                    float w2 = j / 32.0f;
                    int ok = 1;
                    for (int k = 0; k < 3; ++k) {
                        float d = px[k] - w1 * ca[k] - w2 * cb[k];
                        if (d < -SLACK || d > SLACK) { ok = 0; break; }
                    }
                    if (ok) return 1;
                }
            }
        }
    return 0;
}

static int is_legal_ink(int r, int g, int b)
{
    const float px[3] = { (float)r, (float)g, (float)b };
    return is_single_ink(px) || is_ink_pair(px);
}

static void test_only_grey_and_encoder_colours(void)
{
    static const float V[] = { 0.0f, 0.33f, 0.66f, 1.0f };
    for (int g = 0; g < 9; ++g)
        for (size_t v = 0; v < sizeof V / sizeof V[0]; ++v) {
            ui_scene_t sc;
            fill(&sc, g, V[v]);
            int bad = 0, bad_x = -1, bad_y = -1, br = 0, bg = 0, bb = 0;
            uint16_t line[OLED_WIDTH];
            for (int y = 0; y < OLED_HEIGHT; ++y) {
                memset(line, 0, sizeof line);
                ui_scene_row(&sc, y, line, 255);
                for (int x = 0; x < OLED_WIDTH; ++x) {
                    if (!line[x]) continue;
                    int r5 = (line[x] >> 11) & 0x1F, g6 = (line[x] >> 5) & 0x3F,
                        b5 = line[x] & 0x1F;
                    int r = (r5 << 3) | (r5 >> 2), gg = (g6 << 2) | (g6 >> 4),
                        b = (b5 << 3) | (b5 >> 2);
                    if (!is_legal_ink(r, gg, b)) {
                        if (!bad) { bad_x = x; bad_y = y; br = r; bg = gg; bb = b; }
                        ++bad;
                    }
                }
            }
            CHECK(bad == 0,
                  "%s at %.2f drew %d pixel(s) that are neither the structural "
                  "grey nor an encoder colour — first at (%d,%d) = (%d,%d,%d). "
                  "Colour IS the label here; an unattributable tone is an "
                  "unattributable control",
                  NAME[g], (double)V[v], bad, bad_x, bad_y, br, bg, bb);
        }
}

int main(void)
{
    test_draws_and_stays_in_box();
    test_responds_to_its_parameters();
    test_only_grey_and_encoder_colours();

    if (failures) {
        printf("test_ui_scene: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_scene: OK — 9 scenes draw at every setting, stay in the "
           "box, every property visibly moves its own geometry, and nothing "
           "is drawn in a tone outside grey + the three encoder colours\n");
    return 0;
}
