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
    g->last = -1; g->focus=2; g->time_spacing=1;
}
bool world_grammar_due(const world_grammar_t *g, uint32_t now) {
    return !g->timing_valid || (int32_t)(now-g->next_ms)>=0;
}
static void begin_figure(world_grammar_t *n,int maximum) {
    n->recalled=n->memory_len && between(n,0,3)==0;
    n->transpose=0;
    if(n->recalled) {
        n->length=n->memory_len;
        int lo=7,hi=0;
        for(int i=0;i<n->length;++i) {
            if(n->memory[i]<lo)lo=n->memory[i];
            if(n->memory[i]>hi)hi=n->memory[i];
        }
        int shift=(int)between(n,0,2)-1;
        if(lo+shift<0 || hi+shift>7)shift=0;
        n->transpose=(int8_t)shift;
        n->direction=n->memory[1]>n->memory[0];
    } else {
        n->length=(uint8_t)between(n,2,(uint32_t)maximum);
        n->direction=(uint8_t)between(n,0,1);
    }
}
world_offer_t world_grammar_propose(const world_grammar_t *g, uint32_t now, float activity) {
    world_offer_t o;
    memset(&o,0,sizeof o);
    o.next=*g;
    world_grammar_t *n=&o.next;
    if (!(activity>=0.0f && activity<=1.0f)) activity=0.5f;
    /* Activity changes time, never register, brightness or amplitude. */
    float spacing=1.35f-0.70f*activity; o.spacing=spacing;
    if (!n->episode_valid) {
        n->episode_until=now+between(n,45000,120000); n->episode_valid=1;
    }
    o.velocity=0.64f+(float)between(n,0,16)*0.01f;
    o.held_limit=1;
    if (n->world==WORLD_COAST) {
        o.held_limit=2;
        o.role=(uint8_t)(n->notes&1u);
        if (n->phase && (int32_t)(now-n->episode_until)>=0) {
            o.rest=true; o.gap_ms=between(n,4000,9000);
            n->episode_until=now+between(n,45000,120000); ++n->episodes;
            n->phase=0;
            n->focus=(uint8_t)bound((int)n->focus+(between(n,0,1) ? 2 : -2));
        } else {
            int step=(int)between(n,0,4)-2;
            o.index=(n->last<0 || !n->phase) ? n->focus : bound(n->last+step);
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
                begin_figure(n,3);
            }
            if (n->phase==0) {
                o.index=n->recalled ? n->memory[n->pos]+n->transpose :
                    n->pos ? bound(n->last+(n->direction ? 1 : -1)*(n->last<5 ? 2 : 1)) :
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
            if(n->pos+1<n->length) {
                if(n->phase) o.gap_ms=n->rhythm[n->pos];
                else if(n->recalled) o.gap_ms=n->memory_rhythm[n->pos];
            }
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
                begin_figure(n,4);
                o.index=n->recalled ? n->memory[0]+n->transpose :
                        n->last<0 ? 2 : bound(n->last+(int)between(n,0,2)-1);
            } else if(n->recalled) {
                o.index=n->memory[n->pos]+n->transpose;
            } else {
                int direction=n->direction ? 1 : -1;
                /* Arc: a related step, then a return; no pitch sweep. */
                if (n->pos==n->length-1) direction=-direction;
                int step=n->last<5 ? 2 : (int)between(n,1,2);
                int candidate=n->last+direction*step;
                if(candidate<0 || candidate>7) {
                    candidate=n->last-direction*step;
                    n->direction^=1u;
                }
                o.index=bound(candidate);
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
        if(g->world==WORLD_COAST && !g->phase) n.focus=(uint8_t)heard;
        if(g->world!=WORLD_COAST && g->phase==0) {
            n.motif[g->pos]=(int8_t)heard;
            if(g->pos) {
                uint32_t dt=now-g->heard_ms;
                if(g->time_spacing>0)dt=(uint32_t)((float)dt/g->time_spacing);
                if(dt<1400)dt=1400;
                if(dt>10000)dt=10000;
                n.rhythm[g->pos-1]=dt;
            }
            if(!g->pos && n.recalled) ++n.returns;
            if(n.phase==1) {
                memcpy(n.memory,n.motif,sizeof n.memory);
                memcpy(n.memory_rhythm,n.rhythm,sizeof n.memory_rhythm);
                n.memory_len=n.length;
            }
        }
        n.heard_ms=now; n.time_spacing=o->spacing;
        if (g->world==WORLD_WOODLAND && g->phase==1 && !g->pos) ++n.answers;
        n.last=(int8_t)heard; ++n.notes;
    }
    n.next_ms=now+o->gap_ms; n.timing_valid=1;
    *g=n;
}
