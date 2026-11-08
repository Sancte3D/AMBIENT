/* Heard memory, bounded proposals and real long-run ownership. */
#include "world_grammar.h"
#include "engine.h"
#include "engine_product.h"
#include "tuning.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {coast_phrase_t c;woodland_phrase_t w;highlands_phrase_t h;} phrases_t;
typedef struct {int midi,role;uint32_t hold,gap;float velocity;} intent_t;
static phrase_development_t *dev(phrases_t *p,int w) {return w==0 ? &p->c.development : w==1 ? &p->w.development : &p->h.development;}
static int phase(phrases_t *p,int w) {return w==0 ? p->c.phase : w==1 ? p->w.phase : p->h.phase;}
static uint32_t rng(phrases_t *p,int w) {return w==0 ? p->c.rng : w==1 ? p->w.rng : p->h.rng;}
static void restart(phrases_t *p,int w) {
    if(w==0)coast_phrase_restart(&p->c);else if(w==1)woodland_phrase_restart(&p->w);else highlands_phrase_restart(&p->h);
}
static intent_t hear(phrases_t *p,int w,int role,int key,int minor,uint32_t now) {
    intent_t i={0};phrases_t before=*p;
    if(w==0) {
        coast_offer_t a=coast_phrase_propose(&p->c,role,key,minor,.5f);
        assert(coast_phrase_pitch_allowed(a.midi,key,minor) && !memcmp(&before,p,sizeof *p));
        i=(intent_t){a.midi,a.role,a.hold_ms,a.gap_ms,a.velocity};
        assert(coast_phrase_heard(&p->c,&a,now));before=*p;assert(!coast_phrase_heard(&p->c,&a,now));
    } else if(w==1) {
        woodland_offer_t a=woodland_phrase_propose(&p->w,key,minor,.5f);
        assert(woodland_phrase_pitch_allowed(a.midi,key,minor) && !memcmp(&before,p,sizeof *p));
        i=(intent_t){a.midi,a.role,a.hold_ms,a.gap_ms,a.velocity};
        assert(woodland_phrase_heard(&p->w,&a,now));before=*p;assert(!woodland_phrase_heard(&p->w,&a,now));
    } else {
        highlands_offer_t a=highlands_phrase_propose(&p->h,key,minor,.5f);
        assert(highlands_phrase_pitch_allowed(a.midi,key,minor) && !memcmp(&before,p,sizeof *p));
        i=(intent_t){a.midi,a.role,a.hold_ms,a.gap_ms,a.velocity};
        assert(highlands_phrase_heard(&p->h,&a,now));before=*p;assert(!highlands_phrase_heard(&p->h,&a,now));
    }
    assert(!memcmp(&before,p,sizeof *p));return i;
}
static void pure(void) {
    unsigned spans=0;
    for(int w=0;w<3;++w)for(int key=0;key<12;++key)for(int minor=0;minor<2;++minor)for(uint32_t seed=1;seed<=32;++seed) {
        phrases_t p={0};coast_phrase_init(&p.c,seed);woodland_phrase_init(&p.w,seed);highlands_phrase_init(&p.h,seed);
        if(w==0)for(int r=0;r<3;++r)(void)hear(&p,w,r,key,minor,0);else (void)hear(&p,w,0,key,minor,0);
        uint32_t live=rng(&p,w);restart(&p,w);
        assert(!dev(&p,w)->completed && !dev(&p,w)->home_seed && !dev(&p,w)->recalls);
        assert(dev(&p,w)->kind==PHRASE_HOME && dev(&p,w)->phrase_seed==live);
        intent_t home[8];int seen=0,returns=0;
        for(int episode=0;episode<16;++episode) {
            int kind=dev(&p,w)->kind,at=0;seen|=1<<kind;if(episode<4)assert(kind==episode);
            int terminal=w==0 ? 4 : w==1 ? 6 : 8;
            while(phase(&p,w)<terminal) {
                int ph=phase(&p,w);if(w==2 && ph==5)assert(highlands_phrase_pause(&p.h,100,.5f));
                int roles=w==0 ? ph==0 ? 3 : ph==3 ? 1 : 2 : 1;
                for(int role=0;role<roles;++role) {
                    uint32_t completed=dev(&p,w)->completed;intent_t i=hear(&p,w,role,key,minor,100u+(uint32_t)at);
                    assert(i.hold<21000 && i.gap<21000);if(!episode)home[at]=i;
                    if(kind==PHRASE_RETURN) {
                        assert(i.midi==home[at].midi && i.role==home[at].role && i.velocity==home[at].velocity);
                        assert(i.hold==(uint32_t)((float)home[at].hold*.96f) && i.gap==(uint32_t)((float)home[at].gap*.96f));
                    }
                    if(phase(&p,w)<terminal)assert(dev(&p,w)->completed==completed);
                    ++at;
                }
            }
            assert(at==(w==1 ? 6 : 8));if(kind==PHRASE_RETURN)++returns;
            assert(dev(&p,w)->completed==(uint32_t)episode+1 && dev(&p,w)->recalls==(uint32_t)returns);
            assert(dev(&p,w)->home_seed==live && dev(&p,w)->span>=2 && dev(&p,w)->span<=4);spans|=1u<<dev(&p,w)->span;
            uint32_t progressed=rng(&p,w);restart(&p,w);assert(rng(&p,w)==progressed);
        }
        assert(seen==15 && returns>=3);
    }
    assert(spans==28);puts("PHRASE memory pure PASS: 2304 key/collection/seed cases, 16 phrases each, cancelled opening, exact recalled intent, all return spacings");fflush(stdout);
}
enum {SR=44100,BLOCK=512,TICK=441,SECONDS=600};
static uint32_t at,last_start,largest_gap;
static int seen,starts,active_world,key_pc,dark,peak,maximum;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    (void)velocity;if(on<=0)return;assert(owner==6 || owner==7 || owner==15);
    int midi=(int)lroundf(69+12*log2f(hz/440));
    assert(active_world==0 ? coast_phrase_pitch_allowed(midi,key_pc,dark) : active_world==1 ? woodland_phrase_pitch_allowed(midi,key_pc,dark) : highlands_phrase_pitch_allowed(midi,key_pc,dark));
    assert(fabsf(1200*log2f(hz/tuning_hz((float)midi)))<.01f);
    uint32_t now=(uint32_t)((uint64_t)at*1000/SR);if(starts && now-last_start>largest_gap)largest_gap=now-last_start;
    last_start=now;++starts;seen|=1<<engine_generative_phrase_kind();
}
static void actual(int w,int variant) {
    active_world=w;key_pc=variant==2 ? 9 : 2;dark=variant&1;
    engine_init();engine_set_world(w);engine_set_key_pc(key_pc);engine_set_mode(dark);engine_set_tuning(variant&1);
    engine_set_gen_seed(variant==0 ? 1234 : variant==1 ? 42 : 91267);engine_set_note_hook(hook);engine_set_room(.24f);engine_set_nature(0);
    engine_set_activity(variant==2 ? 0 : .5f);engine_set_attack(variant==2 ? 1 : .5f);engine_set_release(variant==2 ? 1 : .5f);
    engine_set_generative(true,-1);int16_t pcm[BLOCK*2];starts=seen=peak=maximum=0;last_start=largest_gap=0;
    for(at=0;at<SR*SECONDS;) {
        if(at%TICK==0) {
            engine_generative_tick((uint32_t)((uint64_t)at*1000/SR));int n=engine_active_voices();assert(n<=(w==1 ? 2 : 3));if(n>maximum)maximum=n;
        }
        uint32_t end=(at/TICK+1)*TICK;int n=(int)(end-at);if(n>BLOCK)n=BLOCK;engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;}at+=(uint32_t)n;
    }
    engine_generative_tick(SECONDS*1000);
    assert(seen==15 && engine_generative_memory_return_count()>=1 && largest_gap<90000);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0 && peak<16384);
    uint32_t completed=engine_generative_episode_count(),recalls=engine_generative_memory_return_count();engine_set_generative(false,-1);
    for(int ms=0;ms<20000;ms+=10) {engine_generative_tick(SECONDS*1000+(uint32_t)ms);engine_render(pcm,TICK);}
    assert(engine_active_voices()==0 && engine_generative_episode_count()==completed && engine_generative_memory_return_count()==recalls);
    printf("PHRASE actual world=%d case=%d starts=%d phrases=%u memory_returns=%u max_sources=%d largest_gap_ms=%u peak=%d PASS\n",w,variant,starts,completed,recalls,maximum,largest_gap,peak);fflush(stdout);
}
int main(void) {
    pure();for(int w=0;w<3;++w)for(int i=0;i<3;++i)actual(w,i);
    puts("PHRASE DEVELOPMENT PASS: 90 minutes actual PCM, four heard stages, 3 seeds, major/ET + minor/JI + low activity/max envelopes, bounded owners and retirement");return 0;
}
