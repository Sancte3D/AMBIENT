/*
 * gesture.c — Gesten-Schleife. Verhalten: siehe gesture.h.
 */

#include "gesture.h"

typedef struct {
    uint32_t t;         /* Offset seit Loop-Start (ms) */
    uint8_t  cell;
    uint8_t  pressed;   /* 1 = Druck, 0 = Loslassen */
} gev_t;

static gev_t           s_ev[GESTURE_MAX];
static int             s_count;
static gesture_state_t s_state;
static gesture_cell_fn s_on_cell;

static uint32_t s_rec_start;    /* REC: t0                       */
static uint32_t s_loop_len;     /* PLAY: Schleifenlaenge (ms)    */
static uint32_t s_play_base;    /* PLAY: now bei Loop-Beginn     */
static int      s_play_idx;     /* PLAY: naechstes Event         */
static uint8_t  s_play_down;    /* PLAY: Bitmaske gehaltener Zellen */

void gesture_init(gesture_cell_fn on_cell) {
    s_on_cell   = on_cell;
    s_count     = 0;
    s_state     = GESTURE_IDLE;
    s_rec_start = s_loop_len = s_play_base = 0;
    s_play_idx  = 0;
    s_play_down = 0;
}

gesture_state_t gesture_state(void) { return s_state; }
int             gesture_count(void) { return s_count; }

/* Alle noch von der Wiedergabe gehaltenen Zellen freigeben. */
static void release_play_voices(uint32_t now_ms) {
    for (uint8_t c = 0; c < 5; ++c)
        if (s_play_down & (1u << c)) {
            if (s_on_cell) s_on_cell(c, false, now_ms);
            s_play_down &= (uint8_t)~(1u << c);
        }
}

void gesture_clear(uint32_t now_ms) {
    release_play_voices(now_ms);
    s_count    = 0;
    s_state    = GESTURE_IDLE;
    s_play_idx = 0;
}

void gesture_record_cell(uint8_t cell, bool pressed, uint32_t now_ms) {
    if (s_state != GESTURE_REC || cell >= 5) return;
    if (s_count >= GESTURE_MAX) return;         /* Puffer voll → Rest kappen */
    s_ev[s_count].t       = now_ms - s_rec_start;
    s_ev[s_count].cell    = cell;
    s_ev[s_count].pressed = pressed ? 1 : 0;
    ++s_count;
}

void gesture_toggle(uint32_t now_ms) {
    switch (s_state) {
        case GESTURE_IDLE:
            s_count     = 0;
            s_rec_start = now_ms;
            s_state     = GESTURE_REC;
            break;
        case GESTURE_REC: {
            uint32_t len = now_ms - s_rec_start;
            if (len < GESTURE_MIN_LOOP) len = GESTURE_MIN_LOOP;
            if (s_count == 0) { s_state = GESTURE_IDLE; break; }  /* nichts */
            s_loop_len  = len;
            s_play_base = now_ms;
            s_play_idx  = 0;
            s_play_down = 0;
            s_state     = GESTURE_PLAY;
            break;
        }
        case GESTURE_PLAY:
        default:
            gesture_clear(now_ms);
            break;
    }
}

void gesture_tick(uint32_t now_ms) {
    if (s_state != GESTURE_PLAY || s_count == 0) return;

    uint32_t phase = now_ms - s_play_base;

    /* Loop-Ende erreicht: offene Stimmen freigeben, neu starten. */
    while (phase >= s_loop_len) {
        release_play_voices(now_ms);
        s_play_base += s_loop_len;
        s_play_idx   = 0;
        phase        = now_ms - s_play_base;
    }

    /* Alle bis zur aktuellen Phase faelligen Events feuern. */
    while (s_play_idx < s_count && s_ev[s_play_idx].t <= phase) {
        const gev_t *e = &s_ev[s_play_idx];
        if (e->cell < 5) {
            if (s_on_cell) s_on_cell(e->cell, e->pressed != 0, now_ms);
            if (e->pressed) s_play_down |=  (uint8_t)(1u << e->cell);
            else            s_play_down &= (uint8_t)~(1u << e->cell);
        }
        ++s_play_idx;
    }
}
