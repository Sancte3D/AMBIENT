/* Actual Product HIGHLANDS, fixed 10ms Main schedule; no direct source score. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include "horn.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
enum { SR=44100,SECONDS=65,MAX_BLOCK=512,TICK=441 };
static FILE *trace;
static uint32_t at,last_ms,quiet_ms,reprise_ms,final_ms;
static int starts,max_sources,common_offs;
static const uint8_t owners[8]={6,7,15,6,7,6,7,15};
static const int pitches[8]={50,69,66,59,62,50,69,66};
static void hook(int on,uint8_t owner,float hz,float velocity) {
    fprintf(trace,"%u,%d,%u,%.7f,%.5f\n",at,on,owner,(double)hz,(double)velocity);
    if(on<=0) {if(on==0 && owner==15)++common_offs;return;}
    assert(starts<8 && owner==owners[starts]);
    assert((int)lroundf(69+12*log2f(hz/440))==pitches[starts]);
    if(starts==3 || starts==4)assert(!common_offs && (horn_active_sources()&(1u<<15)));
    if(starts==5) {
        reprise_ms=at*1000u/SR;assert(quiet_ms && reprise_ms-quiet_ms>=8000u);
    }
    if(++starts==8)last_ms=at*1000u/SR;
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
static void header(FILE *f) {
    uint32_t n=SR*SECONDS*4;
    fwrite("RIFF",1,4,f);u32(f,36+n);fwrite("WAVEfmt ",1,8,f);u32(f,16);
    u16(f,1);u16(f,2);u32(f,SR);u32(f,SR*4);u16(f,4);u16(f,16);
    fwrite("data",1,4,f);u32(f,n);
}
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    int block=atoi(argv[2]);bool room=!strcmp(argv[1],"room");
    if(block<1 || block>MAX_BLOCK || (!room && strcmp(argv[1],"dry")))return 2;
    FILE *out=fopen(argv[3],"wb");if(!out)return 2;header(out);
    char path[1024];if(snprintf(path,sizeof path,"%s.events.csv",argv[3])>=(int)sizeof path)return 2;
    trace=fopen(path,"w");if(!trace)return 2;fprintf(trace,"ack_frame,on,owner,hz,velocity\n");
    engine_init();engine_set_world(WORLD_HIGHLANDS);engine_set_note_hook(hook);engine_set_gen_seed(1234);
    engine_set_room(.24f);engine_set_nature(0);engine_set_fx_mode(room ? 1 : 0);
    engine_set_generative(true,-1);int16_t pcm[MAX_BLOCK*2];int peak=0;bool stopped=false;
    for(at=0;at<SR*SECONDS;) {
        if(at%TICK==0) {
            uint32_t ms=at*1000u/SR;engine_generative_tick(ms);
            if(last_ms && ms>=last_ms+7800u && !stopped) {engine_set_generative(false,-1);stopped=true;}
            int count=engine_active_voices();assert(count<=3);if(count>max_sources)max_sources=count;
            if(starts==5 && !count && !quiet_ms)quiet_ms=ms;
            if(stopped && !count && !final_ms)final_ms=ms;
        }
        uint32_t end=(at/TICK+1)*TICK;if(end>SR*SECONDS)end=SR*SECONDS;
        int n=(int)(end-at);if(n>block)n=block;engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {
            int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;u16(out,(uint16_t)pcm[i]);
        }
        at+=(uint32_t)n;
    }
    engine_generative_tick(SECONDS*1000);
    assert(stopped && starts==8 && max_sources==3 && engine_active_voices()==0 && common_offs==2 && final_ms);
    assert(engine_generative_episode_count()==1 && engine_generative_return_count()==1);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fprintf(stderr,"HIGHLANDS ENGINE mode=%s frames=%u starts=%d max_sources=%d final_sources=%d quiet_ms=%u reprise_ms=%u pause_ms=%u last_ms=%u final_ms=%u peak=%.8f\n",
        argv[1],at,starts,max_sources,engine_active_voices(),quiet_ms,reprise_ms,reprise_ms-quiet_ms,last_ms,final_ms,(double)peak/32768);
    fclose(trace);return fclose(out) ? 2 : 0;
}
