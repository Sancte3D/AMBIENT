/*
 * EC11 encoder bank — C port of firmware/encoders.py.
 *
 * Sampled at 1 kHz from a hardware alarm timer; that is plenty for the
 * RC-debounced signals (1 ms RC + 4× quadrature per detent → worst-case
 * detent rate well under 100 Hz).
 */

#include "encoders.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"

/* --- Encoder static config. Matches firmware/config.py ENCODERS table.
 * If a knob feels backwards on real hardware, flip its dir field.
 *
 * r18.14 (ADR-0012): the four knobs are no longer identical parts —
 *   - display = EC11E with detents + push switch. 1 detent = 1 step:
 *     substeps = 4 for a full-cycle part (15 det / 15 PPR) or 2 for a
 *     half-step part (30 det / 15 PPR — the "two clicks per percent" bug
 *     class). Tune after the exact EC11E variant is sourced.
 *   - drive / bright / volume = EC11E183440C, smooth, NO detents, NO push.
 *     substeps = 2 → 36 ticks/rev (18 PPR), ~1 % per 10° slow turn; the
 *     menu's velocity-acceleration layer scales fast spins. has_sw = false
 *     skips switch polling (pin floats high via pull-up — wired but
 *     unpopulated on the PCB). */
typedef struct {
    uint8_t id;
    uint8_t pin_a, pin_b, pin_sw;
    int8_t  dir;          /* +1 or -1 */
    int8_t  substeps;     /* quadrature transitions per emitted step */
    bool    has_sw;       /* push switch populated? */
} enc_def_t;

static const enc_def_t defs[ENC_COUNT] = {
    { 1, 10, 11, 12, +1, 2, false },  /* drive   — smooth, no push */
    { 2, 13, 14, 15, +1, 2, false },  /* bright  — smooth, no push */
    { 3, 16, 17, 18, +1, 4, true  },  /* display — detent + push   */
    { 4, 19, 20, 21, +1, 2, false },  /* volume  — smooth, no push */
};

/* Per-encoder runtime state. */
typedef struct {
    uint8_t prev_ab;       /* last sampled (A<<1)|B */
    int8_t  accum;         /* signed sub-step accumulator */
    int32_t position;      /* cumulative detents since boot */
    /* push-switch debounce */
    uint8_t sw_stable;     /* current debounced level (1 = released) */
    uint8_t sw_count;      /* counted bounces */
    bool    pushed;        /* convenience cache for enc_pushed() */
} enc_state_t;

static volatile enc_state_t states[ENC_COUNT];

/* Quadrature transition table indexed by (prev_AB << 2) | curr_AB.
 * Valid CW transitions → +1, CCW → −1, invalid/no-change → 0.
 * Identical to the Python _QUAD table — same numbering, same maths. */
static const int8_t QUAD_TBL[16] = {
    0, -1, +1,  0,
   +1,  0,  0, -1,
   -1,  0,  0, +1,
    0, +1, -1,  0,
};

/* --- Tiny lock-free SP/SP ring of events for main-loop consumption. --- */
#define EVT_RING 32   /* power-of-two */
static volatile enc_event_t evt_ring[EVT_RING];
static volatile uint8_t     evt_head = 0; /* writer (timer ISR) */
static volatile uint8_t     evt_tail = 0; /* reader (main loop) */

static inline void evt_push(enc_event_kind_t k, uint8_t id, int8_t v) {
    uint8_t next = (uint8_t)((evt_head + 1) & (EVT_RING - 1));
    if (next == evt_tail) return;            /* full → drop oldest writer-side */
    evt_ring[evt_head].kind  = k;
    evt_ring[evt_head].id    = id;
    evt_ring[evt_head].value = v;
    evt_head = next;
}

bool enc_pop_event(enc_event_t *out) {
    if (evt_tail == evt_head) return false;
    *out = evt_ring[evt_tail];
    evt_tail = (uint8_t)((evt_tail + 1) & (EVT_RING - 1));
    return true;
}

/* --- 1 kHz service: poll all 4 encoders + their push switches. --- */
static bool service_timer(repeating_timer_t *t) {
    (void)t;
    for (int i = 0; i < ENC_COUNT; ++i) {
        const enc_def_t *d = &defs[i];
        volatile enc_state_t *s = &states[i];

        /* Rotation. EC11 + pull-up: idle reads as 1, detents pass through
         * Gray-code transitions on A and B. */
        uint8_t a = (uint8_t)gpio_get(d->pin_a);
        uint8_t b = (uint8_t)gpio_get(d->pin_b);
        uint8_t ab = (uint8_t)((a << 1) | b);
        if (ab != s->prev_ab) {
            uint8_t idx = (uint8_t)((s->prev_ab << 2) | ab);
            s->accum = (int8_t)(s->accum + QUAD_TBL[idx]);
            s->prev_ab = ab;
            if (s->accum >= d->substeps) {
                s->accum = 0;
                int8_t step = d->dir;
                s->position += step;
                evt_push(ENC_EVENT_ROTATE, d->id, step);
            } else if (s->accum <= -d->substeps) {
                s->accum = 0;
                int8_t step = (int8_t)-d->dir;
                s->position += step;
                evt_push(ENC_EVENT_ROTATE, d->id, step);
            }
        }

        /* Push switch (active-low). 3-of-N consecutive samples confirm a
         * level change — same heuristic as the .py driver. Encoders without
         * a populated switch (ADR-0012) skip polling entirely. */
        if (!d->has_sw) continue;
        uint8_t raw = (uint8_t)gpio_get(d->pin_sw);
        if (raw == s->sw_stable) {
            s->sw_count = 0;
        } else {
            s->sw_count++;
            if (s->sw_count >= 3) {
                s->sw_stable = raw;
                s->sw_count  = 0;
                s->pushed    = (raw == 0);
                evt_push(ENC_EVENT_PUSH, d->id, (int8_t)(raw == 0 ? 1 : 0));
            }
        }
    }
    return true;
}

static repeating_timer_t s_timer;

void enc_init(void) {
    for (int i = 0; i < ENC_COUNT; ++i) {
        const enc_def_t *d = &defs[i];
        gpio_init(d->pin_a);  gpio_set_dir(d->pin_a,  GPIO_IN); gpio_pull_up(d->pin_a);
        gpio_init(d->pin_b);  gpio_set_dir(d->pin_b,  GPIO_IN); gpio_pull_up(d->pin_b);
        gpio_init(d->pin_sw); gpio_set_dir(d->pin_sw, GPIO_IN); gpio_pull_up(d->pin_sw);

        volatile enc_state_t *s = &states[i];
        uint8_t a = (uint8_t)gpio_get(d->pin_a);
        uint8_t b = (uint8_t)gpio_get(d->pin_b);
        s->prev_ab   = (uint8_t)((a << 1) | b);
        s->accum     = 0;
        s->position  = 0;
        s->sw_stable = (uint8_t)gpio_get(d->pin_sw);
        s->sw_count  = 0;
        s->pushed    = (s->sw_stable == 0);
    }
    evt_head = evt_tail = 0;
    /* Negative period = "fire every N µs regardless of how long the callback
     * took". 1 ms = 1 kHz. */
    add_repeating_timer_us(-1000, service_timer, NULL, &s_timer);
}

int32_t enc_position(uint8_t id) {
    for (int i = 0; i < ENC_COUNT; ++i) {
        if (defs[i].id == id) return states[i].position;
    }
    return 0;
}

bool enc_pushed(uint8_t id) {
    for (int i = 0; i < ENC_COUNT; ++i) {
        if (defs[i].id == id) return states[i].pushed;
    }
    return false;
}
