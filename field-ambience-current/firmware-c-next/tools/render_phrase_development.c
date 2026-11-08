/* Ten-minute actual engine development; fixed Main grid independent of block. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
enum {SR=44100,TICK=441,MAX_BLOCK=512,SECONDS=600};
static FILE *trace;static uint32_t at;static int starts,seen,maximum;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    fprintf(trace,"%u,%d,%u,%.7f,%.5f,%d,%u\n",at,on,owner,(double)hz,(double)velocity,
        engine_generative_phrase_kind(),engine_generative_episode_count());
    if(on>0) {++starts;seen|=1<<engine_generative_phrase_kind();}
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
int main(int argc,char **argv) {
    if(argc!=5)return 2;
    int world=atoi(argv[1]),block=atoi(argv[2]);uint32_t seed=(uint32_t)strtoul(argv[3],NULL,10);
    if(world<0 || world>2 || block<1 || block>MAX_BLOCK)return 2;
    FILE *out=fopen(argv[4],"wb");if(!out)return 2;
    fwrite("RIFF",1,4,out);u32(out,36+SR*SECONDS*4);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);fwrite("data",1,4,out);u32(out,SR*SECONDS*4);
    char path[1024];if(snprintf(path,sizeof path,"%s.events.csv",argv[4])>=(int)sizeof path)return 2;
    trace=fopen(path,"w");if(!trace)return 2;fprintf(trace,"ack_frame,on,owner,hz,velocity,kind,completed_phrases\n");
    engine_init();engine_set_world(world);engine_set_gen_seed(seed);engine_set_note_hook(hook);
    engine_set_room(.24f);engine_set_nature(0);engine_set_generative(true,-1);
    int16_t pcm[MAX_BLOCK*2];int peak=0;
    for(at=0;at<SR*SECONDS;) {
        if(at%TICK==0) {
            uint32_t ms=(uint32_t)((uint64_t)at*1000/SR);engine_generative_tick(ms);
            if(ms==590000)engine_set_generative(false,-1);
            int n=engine_active_voices();assert(n<=(world==1 ? 2 : 3));if(n>maximum)maximum=n;
        }
        uint32_t end=(at/TICK+1)*TICK;int n=(int)(end-at);if(n>block)n=block;engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;u16(out,(uint16_t)pcm[i]);}
        at+=(uint32_t)n;
    }
    engine_generative_tick(SECONDS*1000);
    assert(seen==15 && engine_generative_memory_return_count()>=1 && engine_active_voices()==0);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fprintf(stderr,"world=%d seed=%u starts=%d phrases=%u memory_returns=%u max_sources=%d final_sources=0 peak=%.8f\n",
        world,seed,starts,engine_generative_episode_count(),engine_generative_memory_return_count(),maximum,(double)peak/32768);
    if(fclose(trace))return 2;
    return fclose(out) ? 2 : 0;
}
