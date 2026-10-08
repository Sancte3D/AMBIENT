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

bool coast_phrase_pitch_allowed(int midi, int key, bool minor) {
    return midi>=45 && midi<=80 && core(midi,key,minor);
}

bool woodland_phrase_pitch_allowed(int midi,int key,bool minor) {
    return midi>=45 && midi<=68 && core(midi,key,minor);
}
void woodland_phrase_init(woodland_phrase_t *g,uint32_t seed) {
    memset(g,0,sizeof *g);g->rng=seed ? seed : 0xA6B13E7Du;
    g->variant=(uint8_t)(g->rng&1u);
}
void woodland_phrase_restart(woodland_phrase_t *g) {
    g->phase=g->timing_valid=0;g->variant=(uint8_t)((g->rng>>8)&1u);
}
bool woodland_phrase_due(const woodland_phrase_t *g,uint32_t now) {
    return !g->timing_valid || (int32_t)(now-g->next_ms)>=0;
}
woodland_offer_t woodland_phrase_propose(const woodland_phrase_t *g,
                                       int key,bool minor,float activity) {
    woodland_offer_t o;memset(&o,0,sizeof o);o.midi=-1;o.phase=g->phase;
    if(g->phase>=6)return o;
    int center=60+wrap12(key);if(center>68)center-=12;
    int third=minor ? -9 : -8,sixth=minor ? -2 : -3;
    int pitch[6]={-12,third,sixth,0,third,g->variant ? sixth : -5};
    static const uint32_t holds[6]={11000,18000,10000,8000,9000,6000};
    static const uint32_t gaps[6]={2000,12000,9000,4000,7000,0};
    uint32_t h=g->rng*1664525u+1013904223u;
    o.role=g->phase&1u;o.midi=center+pitch[g->phase];
    o.velocity=(o.role ? .48f : .58f)+(float)((h>>8)%5u)*.01f;
    o.hold_ms=holds[g->phase]+(h%501u);
    if(!(activity>=0 && activity<=1))activity=.5f;
    o.gap_ms=(uint32_t)((float)gaps[g->phase]*(1.35f-.70f*activity));
    if(o.gap_ms<1400)o.gap_ms=1400;
    if(g->phase==5)o.gap_ms=o.hold_ms+5000u+(h%4001u);
    return o;
}
bool woodland_phrase_heard(woodland_phrase_t *g,const woodland_offer_t *o,uint32_t now) {
    if(g->phase>=6 || o->phase!=g->phase || o->role!=(g->phase&1u) || o->midi<0)return false;
    if(g->phase==0 && g->episodes)++g->returns;
    if(g->phase==2)++g->answers;
    g->rng=g->rng*1664525u+1013904223u;++g->notes;
    if(++g->phase==6)++g->episodes;
    g->next_ms=now+o->gap_ms;g->timing_valid=1;return true;
}
bool highlands_phrase_pitch_allowed(int midi,int key,bool minor) {
    return midi>=45 && midi<=75 && core(midi,key,minor);
}
void highlands_phrase_init(highlands_phrase_t *g,uint32_t seed) {
    memset(g,0,sizeof *g);g->rng=seed ? seed : 0xA6B13E7Du;
    g->variant=(uint8_t)(g->rng&1u);
}
void highlands_phrase_restart(highlands_phrase_t *g) {
    g->phase=g->timing_valid=g->rest_valid=0;
    g->variant=(uint8_t)((g->rng>>8)&1u);
}
bool highlands_phrase_due(const highlands_phrase_t *g,uint32_t now) {
    return !g->timing_valid || (int32_t)(now-g->next_ms)>=0;
}
highlands_offer_t highlands_phrase_propose(const highlands_phrase_t *g,
                                         int key,bool minor,float activity) {
    highlands_offer_t o;memset(&o,0,sizeof o);o.midi=-1;o.phase=g->phase;
    if(g->phase>=8 || (g->phase==5 && !g->rest_valid))return o;
    int center=60+wrap12(key);if(center>68)center-=12;
    int pitch[8]={-12,7,minor ? 3 : 4,minor || g->variant ? -5 : -3,
                  0,-12,7,minor ? 3 : 4};
    static const uint8_t roles[8]={0,1,2,0,1,0,1,2};
    static const uint32_t holds[8]={6000,7000,0,7000,7000,7500,7200,7200};
    static const uint32_t gaps[8]={800,800,8400,1000,0,1000,1000,0};
    uint32_t h=g->rng*1664525u+1013904223u;
    o.role=roles[g->phase];o.midi=center+pitch[g->phase];
    static const float accents[8]={.55f,.41f,.45f,.48f,.50f,.52f,.38f,.44f};
    o.velocity=accents[g->phase]+(float)((h>>8)%5u)*.01f;
    o.hold_ms=holds[g->phase] ? holds[g->phase]+h%251u : 0;
    if(!(activity>=0 && activity<=1))activity=.5f;
    o.gap_ms=(uint32_t)((float)gaps[g->phase]*(1.35f-.70f*activity));
    if(gaps[g->phase] && o.gap_ms<500)o.gap_ms=500;
    return o;
}
bool highlands_phrase_heard(highlands_phrase_t *g,const highlands_offer_t *o,uint32_t now) {
    static const uint8_t roles[8]={0,1,2,0,1,0,1,2};
    if(g->phase>=8 || o->phase!=g->phase || o->role!=roles[g->phase] || o->midi<0 ||
       (g->phase==5 && !g->rest_valid))return false;
    if(g->phase==5)++g->returns;
    g->rng=g->rng*1664525u+1013904223u;++g->notes;
    if(++g->phase==8)++g->episodes;
    if(g->phase==5 || g->phase==8)g->rest_valid=0;
    g->next_ms=now+o->gap_ms;g->timing_valid=1;return true;
}
bool highlands_phrase_pause(highlands_phrase_t *g,uint32_t now,float activity) {
    if((g->phase!=5 && g->phase!=8) || g->rest_valid)return false;
    if(!(activity>=0 && activity<=1))activity=.5f;
    uint32_t rest=(uint32_t)((float)(12000u+g->rng%2001u)*(1.35f-.70f*activity));
    if(rest<8000)rest=8000;
    g->next_ms=now+rest;g->timing_valid=g->rest_valid=1;return true;
}
void coast_phrase_init(coast_phrase_t *g, uint32_t seed) {
    memset(g,0,sizeof *g);
    g->rng=seed ? seed : 0xA6B13E7Du;
    g->variant=(uint8_t)(g->rng&1u);
}
void coast_phrase_restart(coast_phrase_t *g) {
    g->phase=g->heard_mask=g->timing_valid=0;
    g->variant=(uint8_t)((g->rng>>8)&1u);
}
bool coast_phrase_due(const coast_phrase_t *g, uint32_t now) {
    return !g->timing_valid || (int32_t)(now-g->next_ms)>=0;
}
coast_offer_t coast_phrase_propose(const coast_phrase_t *g,int role,
                                  int key,bool minor,float activity) {
    coast_offer_t o;
    memset(&o,0,sizeof o);
    o.phase=g->phase; o.role=(uint8_t)role; o.midi=-1;
    if(role<0 || role>2 || g->phase>3 ||
       (role==2 && g->phase!=0) || (g->phase==3 && role!=0)) return o;
    int center=60+wrap12(key); if(center>68)center-=12;
    int lower[4]={-12,-5,minor ? -2 : -3,0};
    int upper[3]={12,g->variant ? 7 : (minor ? 10 : 9),
                       g->variant ? (minor ? 5 : 2) : (minor ? 3 : 4)};
    o.midi=center+(role==2 ? (minor ? -9 : -8) :
                          role==0 ? lower[g->phase] : upper[g->phase]);
    uint32_t h=g->rng^(uint32_t)(role*0x9E3779B9u);
    h=h*1664525u+1013904223u;
    o.velocity=(role==0 ? .55f : role==1 ? .44f : .32f)+(float)((h>>8)%6u)*.01f;
    o.hold_ms=role==2 ? 0u : g->phase==3 ? 5000u+(h%501u) : 2800u+(h%301u);
    if(!(activity>=0 && activity<=1))activity=.5f;
    float spacing=1.35f-.70f*activity;
    o.gap_ms=(uint32_t)((float)(5800u+g->rng%401u)*spacing);
    if(g->phase==3)o.gap_ms=o.hold_ms+4000u+g->rng%4001u;
    return o;
}
bool coast_phrase_heard(coast_phrase_t *g,const coast_offer_t *o,uint32_t now) {
    if(o->phase!=g->phase || o->midi<0 || g->phase>3 || o->role>2) return false;
    uint8_t required=g->phase==0 ? 7u : g->phase==3 ? 1u : 3u;
    uint8_t bit=(uint8_t)(1u<<o->role);
    if(!(required&bit) || (g->heard_mask&bit))return false;
    if(!g->heard_mask) {
        g->phase_ms=now;
        if(g->phase==0 && g->episodes)++g->returns;
    }
    g->heard_mask|=bit; ++g->notes;
    if(g->heard_mask==required) {
        g->next_ms=(g->phase==3 ? now : g->phase_ms)+o->gap_ms;
        if(g->phase==3)++g->episodes;
        ++g->phase; g->heard_mask=0; g->timing_valid=1;
        g->rng=g->rng*1664525u+1013904223u;
    }
    return true;
}
