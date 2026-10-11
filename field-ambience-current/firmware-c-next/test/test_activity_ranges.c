/* Activity must change time without rewriting held intent or heard memory. */
#include "world_grammar.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct { coast_phrase_t c; woodland_phrase_t w; highlands_phrase_t h; } phrases_t;
typedef struct { int midi,role; uint32_t hold,gap; float velocity; } intent_t;
typedef union { coast_offer_t c; woodland_offer_t w; highlands_offer_t h; } offer_t;
static int phase(const phrases_t *p,int w) { return w==0 ? p->c.phase : w==1 ? p->w.phase : p->h.phase; }
static phrase_development_t *development(phrases_t *p,int w) {
    return w==0 ? &p->c.development : w==1 ? &p->w.development : &p->h.development;
}
static intent_t propose(const phrases_t *p,int w,int role,int key,int minor,float activity,offer_t *o) {
    if(w==0) {
        o->c=coast_phrase_propose(&p->c,role,key,minor,activity);
        return (intent_t){o->c.midi,o->c.role,o->c.hold_ms,o->c.gap_ms,o->c.velocity};
    }
    if(w==1) {
        o->w=woodland_phrase_propose(&p->w,key,minor,activity);
        return (intent_t){o->w.midi,o->w.role,o->w.hold_ms,o->w.gap_ms,o->w.velocity};
    }
    o->h=highlands_phrase_propose(&p->h,key,minor,activity);
    return (intent_t){o->h.midi,o->h.role,o->h.hold_ms,o->h.gap_ms,o->h.velocity};
}
static void heard(phrases_t *p,int w,const offer_t *o,uint32_t now) {
    assert(w==0 ? coast_phrase_heard(&p->c,&o->c,now) : w==1 ?
        woodland_phrase_heard(&p->w,&o->w,now) : highlands_phrase_heard(&p->h,&o->h,now));
}
int main(void) {
    unsigned cases=0,offers=0,changed[3]={0},pauses=0;
    for(int w=0;w<3;++w)for(int key=0;key<12;++key)for(int minor=0;minor<2;++minor)for(uint32_t seed=1;seed<=8;++seed) {
        phrases_t p={0};coast_phrase_init(&p.c,seed);woodland_phrase_init(&p.w,seed);highlands_phrase_init(&p.h,seed);
        unsigned seen=0;uint32_t now=0xFFFF0000u;
        for(int episode=0;episode<8;++episode) {
            seen|=1u<<development(&p,w)->kind;
            while(phase(&p,w)<(w==0 ? 4 : w==1 ? 6 : 8)) {
                int ph=phase(&p,w);
                if(w==2 && ph==5) {
                    highlands_phrase_t low=p.h,mid=p.h,high=p.h;
                    assert(highlands_phrase_pause(&low,now,0));
                    assert(highlands_phrase_pause(&mid,now,.5f));
                    assert(highlands_phrase_pause(&high,now,1));
                    uint32_t a=low.next_ms-now,b=mid.next_ms-now,c=high.next_ms-now;
                    assert(a>b && b>=c && c>=8000u);
                    assert(!highlands_phrase_due(&high,high.next_ms-1));
                    assert(highlands_phrase_due(&high,high.next_ms));
                    p.h=mid;now=mid.next_ms;++pauses;
                }
                int roles=w==0 ? ph==0 ? 3 : ph==3 ? 1 : 2 : 1;
                for(int role=0;role<roles;++role) {
                    phrases_t before=p;offer_t o[3];intent_t a[3];
                    for(int i=0;i<3;++i)a[i]=propose(&p,w,role,key,minor,(float)i/2,&o[i]);
                    assert(!memcmp(&p,&before,sizeof p));
                    for(int i=0;i<3;++i) {
                        assert(a[i].midi==a[1].midi && a[i].role==a[1].role);
                        assert(a[i].hold==a[1].hold && a[i].velocity==a[1].velocity);
                        assert(a[i].midi>=45 && a[i].hold<21000u && a[i].gap<30000u);
                    }
                    assert(a[0].gap>=a[1].gap && a[1].gap>=a[2].gap);
                    changed[w]+=a[0].gap>a[2].gap;
                    heard(&p,w,&o[1],now);++offers;
                }
                now+=30000u;
            }
            assert(development(&p,w)->completed==(uint32_t)episode+1);
            if(w==0)coast_phrase_restart(&p.c);else if(w==1)woodland_phrase_restart(&p.w);else highlands_phrase_restart(&p.h);
        }
        assert(seen==15 && development(&p,w)->recalls>=1);++cases;
    }
    for(int w=0;w<3;++w)assert(changed[w]);
    printf("ACTIVITY PURE PASS: %u key/collection/seed cases, %u heard intents across eight phrases, "
        "min/default/max preserve pitch/role/velocity/hold and memory; %u post-retirement pause intents >=8s, timer wrap\n",cases,offers,pauses);
    return 0;
}
