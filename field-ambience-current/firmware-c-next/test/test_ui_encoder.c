/*
 * test_ui_encoder — one physical click must move exactly one step.
 *
 * This is the test that would have caught "way too fast": the previous decoder
 * counted falling edges on A, so contact bounce turned one detent into two or
 * three, and the wheel jumped. Bounce cannot be reproduced by hand and does not
 * show up in a preview, so it is simulated here.
 */
#include "../tools/ui_encoder.h"

#include <stdio.h>

static int failures;

#define CHECK(cond, ...) do {                            \
    if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); \
                   printf("\n"); ++failures; }            \
} while (0)

/* The Gray sequence one detent walks through, forwards. */
static const int SEQ[4][2] = { {0,0}, {0,1}, {1,1}, {1,0} };

static void feed(ui_enc_t *e, int idx) { ui_enc_update(e, SEQ[idx][0], SEQ[idx][1]); }

/* n clean detents. `pos` is carried by the caller: the decoder starts at 00
 * after ui_enc_init, so the walk has to start there too — a helper holding its
 * own static position would feed an illegal first transition after every
 * re-init and quietly test something else. */
static void turn(ui_enc_t *e, int *pos, int clicks, int dir)
{
    for (int c = 0; c < clicks; ++c)
        for (int s = 0; s < UI_ENC_STATES_PER_DETENT; ++s) {
            *pos = ((*pos + dir) % 4 + 4) % 4;
            feed(e, *pos);
        }
}

static void test_clean(void)
{
    ui_enc_t e;
    int pos = 0;
    ui_enc_init(&e, 0, 0);
    turn(&e, &pos, 10, +1);
    CHECK(ui_enc_take(&e) == 10, "10 clean clicks forward did not give 10 steps");

    pos = 0;
    ui_enc_init(&e, 0, 0);
    turn(&e, &pos, 7, -1);
    CHECK(ui_enc_take(&e) == -7, "7 clean clicks back did not give -7 steps");
}

/* A bouncing contact rattles between two adjacent states before settling. The
 * decoder must swallow all of it: forward and back cancel exactly. */
static void test_bounce(void)
{
    ui_enc_t e;
    ui_enc_init(&e, 0, 0);

    int pos = 0;
    for (int c = 0; c < 5; ++c) {
        for (int s = 0; s < UI_ENC_STATES_PER_DETENT; ++s) {
            int next = (pos + 1) % 4;
            /* rattle: next, back, next, back, next */
            for (int b = 0; b < 3; ++b) { feed(&e, next); feed(&e, pos); }
            feed(&e, next);
            pos = next;
        }
    }
    int32_t d = ui_enc_take(&e);
    CHECK(d == 5, "5 bouncing clicks produced %d steps — bounce is being "
                  "counted as movement", d);
}

/* Noise that never completes a detent must produce nothing at all, rather
 * than dribbling out steps in whichever direction it last twitched. */
static void test_jitter_is_silent(void)
{
    ui_enc_t e;
    ui_enc_init(&e, 0, 0);
    for (int i = 0; i < 200; ++i) { feed(&e, 1); feed(&e, 0); }
    CHECK(ui_enc_take(&e) == 0, "jitter between two states emitted steps");
}

/* A skipped sample (an interrupt missed while the panel was being written) is
 * an illegal transition. It must be dropped, not guessed at. */
static void test_illegal_transition(void)
{
    ui_enc_t e;
    ui_enc_init(&e, 0, 0);
    ui_enc_update(&e, 1, 1);            /* 00 -> 11: two states at once */
    CHECK(ui_enc_take(&e) == 0, "an illegal transition invented a step");
}

/* Turning back and forth around one position must return to zero, or slow
 * hunting for a value would drift. */
static void test_reversal_is_symmetric(void)
{
    ui_enc_t e;
    int pos = 0;
    ui_enc_init(&e, 0, 0);
    for (int i = 0; i < 20; ++i) { turn(&e, &pos, 1, +1); turn(&e, &pos, 1, -1); }
    CHECK(ui_enc_take(&e) == 0, "20 back-and-forth clicks did not cancel");
}

int main(void)
{
    test_clean();
    test_bounce();
    test_jitter_is_silent();
    test_illegal_transition();
    test_reversal_is_symmetric();

    if (failures) {
        printf("test_ui_encoder: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_ui_encoder: OK — one click is one step, bounce and jitter "
           "cancel, illegal transitions are dropped\n");
    return 0;
}
