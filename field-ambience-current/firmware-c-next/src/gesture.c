/*
 * gesture.c — Gesten-Schleife. Verhalten: siehe gesture.h.
 */

#include "gesture.h"
#include <math.h>

typedef struct {
    uint32_t t;         /* Offset seit Loop-Start (ms) */
    uint8_t  cell;
    uint8_t  pressed;   /* 1 = Druck, 0 = Loslassen */
    uint8_t upper,velocity;
} gev_t;

static gev_t           s_ev[GESTURE_MAX];
static int             s_count;
static gesture_state_t s_state;
static gesture_cell_fn s_on_cell;
static gesture_note_fn s_on_note;
static bool rec_upper;
static float rec_velocity;
static uint8_t recorded_upper;

static uint32_t s_rec_start;    /* REC: t0                       */
static uint32_t s_loop_len;     /* PLAY: Schleifenlaenge (ms)    */
static uint32_t s_play_base;    /* PLAY: now bei Loop-Beginn     */
static int      s_play_idx;     /* PLAY: naechstes Event         */
static uint16_t s_play_down;    /* PLAY: owned lower/upper cell lanes */

void gesture_init(gesture_cell_fn on_cell) {
    s_on_note=0;rec_upper=false;rec_velocity=1;recorded_upper=0;
    s_on_cell   = on_cell;
    s_count     = 0;
    s_state     = GESTURE_IDLE;
    s_rec_start = s_loop_len = s_play_base = 0;
    s_play_idx  = 0;
    s_play_down = 0;
}
void gesture_init_owned(gesture_note_fn on_note) { gesture_init(0);s_on_note=on_note; }
void gesture_set_record_context(bool upper,float velocity) {
    if(!isfinite(velocity) || velocity<=0)return;
    rec_upper=upper;rec_velocity=velocity>1 ? 1 : velocity;
}

gesture_state_t gesture_state(void) { return s_state; }
int             gesture_count(void) { return s_count; }

/* Alle noch von der Wiedergabe gehaltenen Zellen freigeben. */
static void release_play_voices(uint32_t now_ms) {
    if(s_on_note) {
        for(unsigned k=0;k<10;++k)if(s_play_down&(1u<<k))
            (void)s_on_note((uint8_t)(k%5),k>=5,false,0,now_ms);
        s_play_down=0;return;
    }
    for (uint8_t c = 0; c < 5; ++c)
        if (s_play_down & (1u << c)) {
            if (s_on_cell) s_on_cell(c, false, now_ms);
            s_play_down &= (uint8_t)~(1u << c);
        }
}
void gesture_stop(uint32_t now_ms) {
    release_play_voices(now_ms);s_state=GESTURE_IDLE;s_play_idx=0;
}
void gesture_play(uint32_t now_ms) {
    if(!s_count || !s_loop_len)return;
    release_play_voices(now_ms);s_play_base=now_ms;s_play_idx=0;s_state=GESTURE_PLAY;
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
    if(pressed) {
        if(rec_upper)recorded_upper|=(uint8_t)(1u<<cell);
        else recorded_upper&=(uint8_t)~(1u<<cell);
    }
    s_ev[s_count].upper=(recorded_upper>>cell)&1u;
    s_ev[s_count].velocity=(uint8_t)(rec_velocity*127+.5f);
    ++s_count;
}

void gesture_toggle(uint32_t now_ms) {
    switch (s_state) {
        case GESTURE_IDLE:
            if(s_on_note && s_count && s_loop_len) {gesture_play(now_ms);break;}
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
            if(s_on_note)gesture_stop(now_ms);else gesture_clear(now_ms);
            break;
    }
}
static void owned_event(const gev_t *e,uint32_t now) {
    unsigned lane=e->cell+5u*e->upper;uint16_t bit=(uint16_t)(1u<<lane);
    if(e->pressed) {
        /* An open note across the wrap is a tie, never another attack. */
        if(!(s_play_down&bit) && s_on_note(e->cell,e->upper,true,(float)e->velocity/127,now))s_play_down|=bit;
    } else if(s_play_down&bit) {
        (void)s_on_note(e->cell,e->upper,false,0,now);s_play_down&=(uint16_t)~bit;
    }
}

void gesture_tick(uint32_t now_ms) {
    if (s_state != GESTURE_PLAY || s_count == 0) return;

    uint32_t phase = now_ms - s_play_base;
    if(s_on_note) {
        if(phase>=s_loop_len) {
            uint32_t cycles=phase/s_loop_len;
            /* At most one ending and one current cycle, <=256 events. Very
             * late Main service skips old note-ons instead of bursting them. */
            if(cycles==1)while(s_play_idx<s_count)owned_event(&s_ev[s_play_idx++],now_ms);
            else release_play_voices(now_ms);
            s_play_base+=cycles*s_loop_len;s_play_idx=0;phase=now_ms-s_play_base;
        }
        while(s_play_idx<s_count && s_ev[s_play_idx].t<=phase)owned_event(&s_ev[s_play_idx++],now_ms);
        return;
    }

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
