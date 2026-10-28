/*
 * gesture.c — Gesten-Schleife. Verhalten: siehe gesture.h.
 */

#include "gesture.h"
#include <math.h>
#include <string.h>

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

static void archive_u32(uint8_t *p,uint32_t v) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));
}
static uint32_t archive_read(const uint8_t *p) {
    uint32_t v=0;for(unsigned i=0;i<4;++i)v|=(uint32_t)p[i]<<(8*i);return v;
}
static uint32_t archive_hash(const uint8_t *p,unsigned n) {
    uint32_t h=2166136261u;for(unsigned i=0;i<n;++i)h=(h^p[i])*16777619u;return h;
}
unsigned gesture_export(void *bytes,unsigned capacity) {
    unsigned length=16u+8u*(unsigned)s_count;
    if(!bytes || capacity<length || s_state==GESTURE_REC)return 0;
    uint8_t *p=bytes;memcpy(p,"GLP1",4);archive_u32(p+4,(uint32_t)s_count);
    archive_u32(p+8,s_count ? s_loop_len : GESTURE_MIN_LOOP);
    for(int i=0;i<s_count;++i) {
        uint8_t *e=p+12+8*i;archive_u32(e,s_ev[i].t);
        e[4]=s_ev[i].cell;e[5]=s_ev[i].pressed;e[6]=s_ev[i].upper;e[7]=s_ev[i].velocity;
    }
    archive_u32(p+length-4,archive_hash(p,length-4));return length;
}
unsigned gesture_archive_empty(void *bytes,unsigned capacity) {
    if(!bytes || capacity<16)return 0;
    uint8_t *p=bytes;memcpy(p,"GLP1",4);archive_u32(p+4,0);
    archive_u32(p+8,GESTURE_MIN_LOOP);archive_u32(p+12,archive_hash(p,12));return 16;
}
bool gesture_archive_valid(const void *bytes,unsigned length) {
    if(!bytes || length<16 || length>GESTURE_ARCHIVE_MAX)return false;
    const uint8_t *p=bytes;unsigned n=archive_read(p+4);uint32_t duration=archive_read(p+8);
    if(memcmp(p,"GLP1",4) || n>GESTURE_MAX || length!=16u+8u*n ||
       duration<GESTURE_MIN_LOOP || archive_read(p+length-4)!=archive_hash(p,length-4))return false;
    uint32_t last=0;
    for(unsigned i=0;i<n;++i) {
        const uint8_t *e=p+12+8*i;uint32_t t=archive_read(e);
        if(t<last || t>duration || e[4]>=5 || e[5]>1 || e[6]>1 || e[7]>127 ||
           (e[5] && !e[7]))return false;
        last=t;
    }
    return true;
}
bool gesture_import(const void *bytes,unsigned length,uint32_t now_ms) {
    if(!gesture_archive_valid(bytes,length))return false;
    const uint8_t *p=bytes;unsigned n=archive_read(p+4);uint32_t duration=archive_read(p+8);
    /* Validate the entire record before releasing any playback owner. */
    gesture_clear(now_ms);s_loop_len=duration;
    for(unsigned i=0;i<n;++i) {
        const uint8_t *e=p+12+8*i;
        s_ev[i]=(gev_t){archive_read(e),e[4],e[5],e[6],e[7]};
    }
    s_count=(int)n;recorded_upper=0;return true;
}

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
    if(s_on_note && s_state==GESTURE_REC && s_count) {
        /* A Mute/stop may arrive before the next Engine clock update. Keep
         * the partial take valid even if that cached clock is slightly old. */
        uint32_t length=now_ms-s_rec_start;
        if((int32_t)length<0)length=0;
        uint32_t last=s_ev[s_count-1].t;
        if(length<=last)length=last+1u;
        s_loop_len=length<GESTURE_MIN_LOOP ? GESTURE_MIN_LOOP : length;
    }
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
