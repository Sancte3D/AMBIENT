#include "world_grammar.h"
#include <string.h>

static uint32_t draw(world_grammar_t *g) {
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng >> 8;
}
static uint32_t between(world_grammar_t *g, uint32_t lo, uint32_t hi) {
    return lo + draw(g) % (hi - lo + 1u);
}
static int bound(int i) { return i < 0 ? 0 : (i > 7 ? 7 : i); }
static int wrap12(int i) { i %= 12; return i < 0 ? i + 12 : i; }
static bool core(int midi, int key, bool minor) {
    static const uint8_t major[5] = {0, 2, 4, 7, 9};
    static const uint8_t dark[5] = {0, 3, 5, 7, 10};
    const uint8_t *p = minor ? dark : major;
    int pc = wrap12(midi - key);
    for (int i=0; i<5; ++i) if (pc == p[i]) return true;
    return false;
}
int world_pitch_count(int key, bool minor) {
    int n=0;
    for (int m=50; m<=69; ++m) n += core(m, key, minor);
    return n;
}
int world_pitch_midi(int index, int key, bool minor) {
    int n=world_pitch_count(key, minor);
    if (index < 0) index=0;
    if (index >= n) index=n-1;
    for (int m=50; m<=69; ++m) if (core(m,key,minor) && index-- == 0) return m;
    return 50;
}
void world_grammar_init(world_grammar_t *g, int world, uint32_t seed) {
    memset(g,0,sizeof *g);
    g->world = world >= 0 && world < CORE_WORLD_COUNT ? world : WORLD_COAST;
    g->rng = seed ? seed : 0xA6B13E7Du;
    g->last = -1;
}
bool world_grammar_due(const world_grammar_t *g, uint32_t now) {
    return !g->timing_valid || (int32_t)(now-g->next_ms)>=0;
}
world_offer_t world_grammar_propose(const world_grammar_t *g, uint32_t now, float activity) {
    world_offer_t o;
    memset(&o,0,sizeof o);
    o.next=*g;
    world_grammar_t *n=&o.next;
    if (!(activity>=0.0f && activity<=1.0f)) activity=0.5f;
    /* Activity changes time, never register, brightness or amplitude. */
    float spacing=1.35f-0.70f*activity;
    if (!n->episode_until) n->episode_until=now+between(n,45000,120000);
    o.velocity=0.64f+(float)between(n,0,16)*0.01f;
    o.held_limit=1;
    if (n->world==WORLD_COAST) {
        o.held_limit=2;
        o.role=(uint8_t)(n->notes&1u);
        if (n->phase && (int32_t)(now-n->episode_until)>=0) {
            o.rest=true; o.gap_ms=between(n,4000,9000);
            n->episode_until=now+between(n,45000,120000); ++n->episodes;
            n->phase=0;
        } else {
            int step=(int)between(n,0,4)-2;
            o.index=n->last<0 ? 2 : bound(n->last+step);
            /* A common tone can connect independent envelopes; avoid an
             * unbroken repeated root by changing after a zero step. */
            if (o.index==n->last && (n->notes%3u)==2u) o.index=bound(o.index+1);
            o.hold_ms=between(n,6000,14000);
            o.gap_ms=between(n,4000,10000);
            n->phase=1;
        }
    } else if (n->world==WORLD_WOODLAND) {
        o.held_limit=2;
        o.role=n->phase;
        if (n->phase==3) {
            o.rest=true; o.gap_ms=between(n,6000,16000);
            n->phase=n->pos=0;
            if ((int32_t)(now-n->episode_until)>=0) {
                ++n->episodes; n->episode_until=now+between(n,45000,120000);
            }
        } else {
            if (!n->pos && !n->phase) {
                n->length=(uint8_t)between(n,2,3);
                n->direction=(uint8_t)between(n,0,1);
            }
            if (n->phase==0) {
                o.index=n->pos ? bound(n->last+(n->direction ? 1 : -1)*(n->last<5 ? 2 : 1)) :
                                 (int)between(n,2,5);
            } else {
                o.index=n->motif[n->pos];
                /* Answer recalls the heard contour. Variation changes only
                 * the final degree, by one scale step, then leaves space. */
                if (n->phase==2 && n->pos==n->length-1)
                    o.index=bound(o.index+(n->direction ? -1 : 1));
            }
            o.hold_ms=0; /* natural source decay, never a hidden sustain */
            o.gap_ms=between(n,2000,6000);
            ++n->pos;
            if (n->pos==n->length) {
                n->pos=0; ++n->phase;
                o.gap_ms += n->phase<3 ? between(n,3000,6000) : 0;
            }
        }
    } else {
        o.role=0;
        if (n->phase) {
            o.rest=true; o.gap_ms=between(n,8000,20000);
            n->phase=n->pos=0;
            if ((int32_t)(now-n->episode_until)>=0) {
                ++n->episodes; n->episode_until=now+between(n,45000,120000);
            }
        } else {
            if (!n->pos) {
                n->length=(uint8_t)between(n,2,4);
                n->direction=(uint8_t)between(n,0,1);
                o.index=n->last<0 ? 2 : bound(n->last+(int)between(n,0,2)-1);
            } else {
                int direction=n->direction ? 1 : -1;
                /* Arc: a related step, then a return; no pitch sweep. */
                if (n->pos==n->length-1) direction=-direction;
                o.index=bound(n->last+direction*(int)between(n,1,2));
            }
            o.hold_ms=between(n,1500,5000);
            o.gap_ms=o.hold_ms+between(n,900,2400);
            if (++n->pos==n->length) n->phase=1;
        }
    }
    o.gap_ms=(uint32_t)((float)o.gap_ms*spacing);
    if (o.gap_ms<1400) o.gap_ms=1400;
    return o;
}
void world_grammar_commit(world_grammar_t *g, const world_offer_t *o,
                          int heard, uint32_t now) {
    /* All grammar state commits together only after actual admission. */
    world_grammar_t n=o->next;
    if (!o->rest) {
        if (g->world==WORLD_WOODLAND && g->phase==0)
            n.motif[g->pos]=(int8_t)heard;
        if (g->world==WORLD_WOODLAND && g->phase==1 && !g->pos) ++n.answers;
        n.last=(int8_t)heard; ++n.notes;
    }
    n.next_ms=now+o->gap_ms; n.timing_valid=1;
    *g=n;
}
