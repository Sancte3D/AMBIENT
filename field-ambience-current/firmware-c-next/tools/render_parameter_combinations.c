/* SD35: bounded reachable-control risks through the actual Product engine.
 * No Clear, mute or DSP substitute is used to end these sources. */
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "tuning.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
enum { SR=44100,TICK=441,BLOCK=512 };
typedef struct {float activity,attack,release,color,room,nature,volume;} controls_t;
static const controls_t profiles[]={
    {.5f,.5f,.5f,.5f,.5f,0,.6f}, /* normal reference */
    {1,0,0,1,1,1,1},             /* fast, short, bright, full sum */
    {1,1,1,1,1,1,1},             /* dense with longest articulation */
    {0,1,1,0,1,1,1},             /* slow, long, dark */
    {1,0,1,0,1,1,1},             /* fast onset, long release */
    {0,1,0,1,1,0,1},             /* slow attack, short release */
    {1,0,1,0,0,0,1},             /* dry dark, long release */
    {0,1,0,1,0,0,1}              /* dry bright, long attack */
};
static FILE *events,*changes;
static uint32_t at;
static int starts,offs,key,minor,world;
static bool generated;
static uint16_t mask(void) {return bowed_active_sources()|horn_active_sources()|pluck_active_sources();}
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(on==0 || on==1);
    assert(generated ? (owner==6 || owner==7 || owner==15) : owner<3);
    if(on) {
        int lo=generated ? 45 : 50,hi=generated ? (world==0 ? 80 : world==1 ? 68 : 75) : 69;
        float minimum=generated ? 105 : 140,maximum=generated ? (world==0 ? 850 : world==1 ? 470 : 650) : 470;
        assert(hz>=minimum && hz<=maximum && velocity>0 && velocity<=1);
        bool allowed=false;
        for(int midi=lo;midi<=hi;++midi) {
            int degree=(midi-key+120)%12;
            bool core=minor ? (degree==0 || degree==3 || degree==5 || degree==7 || degree==10) :
                (degree==0 || degree==2 || degree==4 || degree==7 || degree==9);
            if(core && fabsf(1200*log2f(hz/tuning_hz((float)midi)))<.01f)allowed=true;
        }
        assert(allowed);++starts;
    } else ++offs;
    fprintf(events,"%u,%d,%u,%.7f,%.8f,%d,%u\n",at,on,owner,(double)hz,(double)velocity,
        engine_generative_phrase_kind(),engine_generative_episode_count());
}
static void apply(controls_t c) {
    int before=starts,off_before=offs;
    engine_set_activity(c.activity);engine_set_attack(c.attack);engine_set_release(c.release);
    engine_set_color(c.color);engine_set_room(c.room);engine_set_fx_mode(c.room>0);
    engine_set_nature(c.nature);engine_set_master_volume(c.volume);
    assert(starts==before && offs==off_before);
    if(changes)fprintf(changes,"%u,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",at,
        (double)c.activity,(double)c.attack,(double)c.release,(double)c.color,
        (double)c.room,(double)c.nature,(double)c.volume);
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
static int close_checked(FILE *f) {
    int error=ferror(f);if(fflush(f))error=1;if(fsync(fileno(f)))error=1;if(fclose(f))error=1;return error;
}
static int integer(char *s,int lo,int hi) {
    char *end;long x=strtol(s,&end,10);assert(!*end && x>=lo && x<=hi);return (int)x;
}
int main(int argc,char **argv) {
    if(argc!=8 || (strcmp(argv[1],"gen") && strcmp(argv[1],"manual")))return 2;
    generated=!strcmp(argv[1],"gen");
    int family=integer(argv[2],0,generated ? 2 : 4),context=integer(argv[3],0,1);
    int profile=integer(argv[4],0,7),live=integer(argv[5],0,1),block=integer(argv[6],1,BLOCK);
    world=generated ? family : family==3 ? 1 : family==4 ? 0 : family;
    key=generated && context ? 9 : 2;minor=generated && context;
    uint32_t seed=context ? 42 : 1234;
    int seconds=generated ? 192 : 112,stop=generated ? 160 : 4;
    FILE *out=fopen(argv[7],"wb");if(!out)return 2;
    fwrite("RIFF",1,4,out);u32(out,36+(uint32_t)SR*seconds*4);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);
    fwrite("data",1,4,out);u32(out,(uint32_t)SR*seconds*4);
    char path[1024];
    assert(snprintf(path,sizeof path,"%s.events.csv",argv[7])<(int)sizeof path);
    events=fopen(path,"w");if(!events)return 2;
    fprintf(events,"ack_frame,on,owner,hz,velocity,kind,completed_phrases\n");
    assert(snprintf(path,sizeof path,"%s.state.csv",argv[7])<(int)sizeof path);
    FILE *state=fopen(path,"w");if(!state)return 2;
    fprintf(state,"frame,active_sources,kind,completed_phrases,memory_returns,admission_rejections,source_mask_before_tick,source_mask_after_tick,pluck_sources\n");
    engine_init();engine_set_world(world);engine_set_gen_seed(seed);
    engine_set_key_pc(key);engine_set_mode(minor);engine_set_tuning(generated && context);
    controls_t c=profiles[profile],settle=c;settle.nature=0;apply(settle);
    int16_t pcm[2*BLOCK];
    for(int frame=0;frame<SR;) {int n=SR-frame;if(n>block)n=block;
        engine_render(pcm,n);for(int i=0;i<2*n;++i)assert(!pcm[i]);frame+=n;}
    assert(snprintf(path,sizeof path,"%s.controls.csv",argv[7])<(int)sizeof path);
    changes=fopen(path,"w");if(!changes)return 2;
    fprintf(changes,"frame,activity,attack,release,color,room,nature,volume\n");
    engine_set_note_hook(hook);apply(c);if(generated)engine_set_generative(true,-1);
    int maximum=0,peak=0,max_step=0,previous[2]={0,0};bool rejected=false;
    uint32_t final_ms=0;bool stopped=false;
    for(at=0;at<(uint32_t)SR*seconds;) {
        if(at%TICK==0) {
            int tick=(int)(at/TICK);uint16_t before=mask();
            if(live && generated) {
                if(tick==1000 || tick==4000 || tick==8000 || tick==12000 || tick==13600)apply(profiles[tick==4000 ? 3 : 2]);
                if(tick==2000 || tick==10000) {controls_t quiet=profiles[6];quiet.volume=0;apply(quiet);}
                if(tick==2100 || tick==10200)apply(profiles[6]);
                if(tick>=3200 && tick<3400)apply(profiles[tick&1 ? 1 : 6]); /* 100 Hz target alternation */
                if(tick>=6400 && tick<=6500) {float x=(tick-6400)/100.0f;
                    controls_t ramp={x,x,1-x,x,x,x,1};apply(ramp);}
            }
            if(live && !generated) {
                if(tick==20 || tick==360)apply(profiles[2]);
                if(tick==40) {controls_t quiet=profiles[6];quiet.volume=0;apply(quiet);}
                if(tick==50)apply(profiles[6]);
                if(tick>=100 && tick<300)apply(profiles[tick&1 ? 1 : 6]);
                if(tick>=300 && tick<=350) {float x=(tick-300)/50.0f;
                    controls_t ramp={x,x,1-x,x,x,x,1};apply(ramp);}
            }
            if(tick==stop*100) {
                if(generated)engine_set_generative(false,-1);
                else {for(int owner=0;owner<3;++owner)engine_note_off((uint8_t)owner);engine_set_nature(0);}
                stopped=true;
            }
            engine_generative_tick((uint32_t)tick*10u);
            if(!generated && tick==0) {
                const int low[3]={50,57,62},high[3]={62,66,69};
                const int mixes[5][3]={{0,0,0},{1,1,1},{2,2,2},{1,1,0},{0,1,2}};
                int n=family==1 ? 2 : 3;
                for(int owner=0;owner<n;++owner) {
                    engine_set_world(mixes[family][owner]);
                    int midi=context ? high[owner] : low[owner];
                    if(family==1 && context && owner==1)midi=69;
                    assert(engine_try_note_on((uint8_t)owner,tuning_hz((float)midi),1));
                }
            }
            if(!generated && tick==10) {
                rejected=!engine_try_note_on(3,tuning_hz(context ? 57 : 66),1);assert(rejected);
            }
            int active=engine_active_voices();assert(active<=3 && pluck_active_count()<=2);
            if(generated)assert(active<=(world==1 ? 2 : 3));
            if(active>maximum)maximum=active;
            if(stopped && !active && !final_ms)final_ms=(uint32_t)tick*10u;
            fprintf(state,"%u,%d,%d,%u,%u,%u,%u,%u,%d\n",at,active,engine_generative_phrase_kind(),
                engine_generative_episode_count(),engine_generative_memory_return_count(),
                engine_admission_rejections(),before,mask(),pluck_active_count());
        }
        int n=(int)((at/TICK+1)*TICK-at);if(n>block)n=block;
        engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {
            int a=abs((int)pcm[i]);if(a>peak)peak=a;
            int ch=i&1,step=abs((int)pcm[i]-previous[ch]);previous[ch]=pcm[i];if(step>max_step)max_step=step;
            if(at>=(uint32_t)SR*(seconds-1))assert(!pcm[i]);
            u16(out,(uint16_t)pcm[i]);
        }
        at+=(uint32_t)n;
    }
    assert(stopped && final_ms && !engine_active_voices());
    assert(generated ? starts>=6 : (rejected && starts==(family==1 ? 2 : 3)));
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
    fprintf(stderr,"COMBINATIONS generated=%d family=%d context=%d profile=%d live=%d starts=%d offs=%d "
        "phrases=%u max_sources=%d final_ms=%u nonfinite=0 limited=0 peak=%.8f max_step=%d\n",
        generated,family,context,profile,live,starts,offs,engine_generative_episode_count(),maximum,
        final_ms,(double)peak/32768,max_step);
    int error=close_checked(state);if(close_checked(events))error=1;
    if(close_checked(changes))error=1;
    if(close_checked(out))error=1;
    return error ? 2 : 0;
}
