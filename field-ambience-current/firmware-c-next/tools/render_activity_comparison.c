/* SD33: Activity boundaries and live changes through the actual Product core. */
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

enum { SR=44100,TICK=441,MAX_BLOCK=512,SECONDS=360,STOP=328 };
static FILE *events;
static uint32_t at;
static int starts,seen;
static float activity;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(on==0 || on==1);
    assert(owner==6 || owner==7 || owner==15);
    fprintf(events,"%u,%d,%u,%.7f,%.5f,%d,%u,%.2f\n",at,on,owner,(double)hz,
        (double)velocity,engine_generative_phrase_kind(),engine_generative_episode_count(),(double)activity);
    if(on>0) { ++starts;seen|=1<<engine_generative_phrase_kind(); }
}
static void u16(FILE *f,uint16_t x) { fputc(x&255,f);fputc(x>>8,f); }
static void u32(FILE *f,uint32_t x) { u16(f,x&65535);u16(f,x>>16); }
static int close_checked(FILE *f) {
    int error=ferror(f);
    if(fflush(f))error=1;
    if(fsync(fileno(f)))error=1;
    if(fclose(f))error=1;
    return error;
}
static int chunk(uint32_t frame,uint32_t end,int block) {
    uint32_t next=(frame/TICK+1)*TICK;if(next>end)next=end;
    int n=(int)(next-frame);return n>block ? block : n;
}
int main(int argc,char **argv) {
    if(argc!=7)return 2;
    char *end;
    long world=strtol(argv[1],&end,10);if(*end || world<0 || world>2)return 2;
    errno=0;unsigned long value=strtoul(argv[2],&end,10);
    if(errno || *end || value>UINT32_MAX || !value)return 2;
    uint32_t seed=(uint32_t)value;
    activity=strtof(argv[3],&end);if(*end || !isfinite(activity) || activity<0 || activity>1)return 2;
    long live=strtol(argv[4],&end,10);if(*end || live<0 || live>1)return 2;
    long block=strtol(argv[5],&end,10);if(*end || block<1 || block>MAX_BLOCK)return 2;
    FILE *out=fopen(argv[6],"wb");if(!out)return 2;
    fwrite("RIFF",1,4,out);u32(out,36+(uint32_t)SR*SECONDS*4);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);
    fwrite("data",1,4,out);u32(out,(uint32_t)SR*SECONDS*4);
    char path[1024];
    if(snprintf(path,sizeof path,"%s.events.csv",argv[6])>=(int)sizeof path)return 2;
    events=fopen(path,"w");if(!events)return 2;
    fprintf(events,"ack_frame,on,owner,hz,velocity,kind,completed_phrases,activity\n");
    if(snprintf(path,sizeof path,"%s.state.csv",argv[6])>=(int)sizeof path)return 2;
    FILE *state=fopen(path,"w");if(!state)return 2;
    fprintf(state,"frame,active_sources,kind,completed_phrases,memory_returns,admission_rejections,activity\n");
    engine_init();engine_set_world((int)world);engine_set_gen_seed(seed);
    engine_set_activity(activity);engine_set_color(.5f);engine_set_attack(.5f);engine_set_release(.5f);
    engine_set_master_volume(.6f);engine_set_room(.5f);engine_set_fx_mode(1);engine_set_nature(0);
    int16_t pcm[MAX_BLOCK*2];
    for(uint32_t frame=0;frame<SR;) {
        int n=chunk(frame,SR,(int)block);engine_render(pcm,n);
        for(int i=0;i<2*n;++i)assert(pcm[i]==0);
        frame+=(uint32_t)n;
    }
    engine_set_note_hook(hook);engine_set_generative(true,-1);
    int maximum=0,peak=0;uint32_t final_ms=0;bool stopped=false;
    static const uint32_t change_ms[6]={10000,32000,64000,96000,128000,160000};
    static const float targets[6]={0,1,.5f,0,1,.5f};
    for(at=0;at<(uint32_t)SR*SECONDS;) {
        if(at%TICK==0) {
            uint32_t ms=(uint32_t)((uint64_t)at*1000/SR);
            if(live)for(int i=0;i<6;++i)if(ms==change_ms[i]) {
                int before=starts;activity=targets[i];engine_set_activity(activity);assert(starts==before);
            }
            if(ms==STOP*1000u) { engine_set_generative(false,-1);stopped=true; }
            engine_generative_tick(ms);
            int count=engine_active_voices();assert(count<=(world==1 ? 2 : 3));
            if(count>maximum)maximum=count;
            if(stopped && !count && !final_ms)final_ms=ms;
            fprintf(state,"%u,%d,%d,%u,%u,%u,%.2f\n",at,count,engine_generative_phrase_kind(),
                engine_generative_episode_count(),engine_generative_memory_return_count(),
                engine_admission_rejections(),(double)activity);
        }
        int n=chunk(at,(uint32_t)SR*SECONDS,(int)block);engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {
            int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;
            if(at>=(uint32_t)SR*(SECONDS-1))assert(pcm[i]==0);
            u16(out,(uint16_t)pcm[i]);
        }
        at+=(uint32_t)n;
    }
    engine_generative_tick(SECONDS*1000u);
    assert(stopped && final_ms && engine_active_voices()==0 && seen==15);
    assert(engine_generative_episode_count()>=4 && engine_generative_memory_return_count()>=1);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0 && peak<16384);
    fprintf(stderr,"ACTIVITY world=%ld seed=%u initial=%.2f live=%ld frames=%u starts=%d "
        "phrases=%u memory_returns=%u max_sources=%d final_ms=%u nonfinite=0 limited=0 peak=%.8f\n",world,seed,
        (double)strtof(argv[3],NULL),live,at,starts,engine_generative_episode_count(),
        engine_generative_memory_return_count(),maximum,final_ms,(double)peak/32768);
    int error=close_checked(state);if(close_checked(events))error=1;if(close_checked(out))error=1;
    return error ? 2 : 0;
}
