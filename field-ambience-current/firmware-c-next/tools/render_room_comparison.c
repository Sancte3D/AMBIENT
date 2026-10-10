/* SD19: actual Product performance or explicitly isolated shared-room impulse.
 * No direct source score, modified envelopes, or substitute reverb. */
#include "engine.h"
#include "engine_product.h"
#include "ambient_room.h"
#include "dsp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum { SR=44100, SECONDS=32, STOP=16, MAX_BLOCK=512, TICK=441 };
static FILE *trace;
static uint32_t at;
static int starts;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    fprintf(trace,"%u,%d,%u,%.7f,%.5f\n",at,on,owner,(double)hz,(double)velocity);
    if(on>0)++starts;
}
static void u16(FILE *f,uint16_t x) { fputc(x&255,f);fputc(x>>8,f); }
static void u32(FILE *f,uint32_t x) { u16(f,x&65535);u16(f,x>>16); }
static void header(FILE *f,bool floating) {
    uint32_t bytes=floating ? 8u : 4u,n=SR*SECONDS*bytes;
    fwrite("RIFF",1,4,f);u32(f,36+n);fwrite("WAVEfmt ",1,8,f);u32(f,16);
    u16(f,floating ? 3 : 1);u16(f,2);u32(f,SR);u32(f,SR*bytes);
    u16(f,(uint16_t)bytes);u16(f,floating ? 32 : 16);
    fwrite("data",1,4,f);u32(f,n);
}
static void float_sample(FILE *f,float x) {
    uint32_t bits;assert(sizeof x==sizeof bits && isfinite(x));
    memcpy(&bits,&x,sizeof bits);u32(f,bits);
}
static int chunk(uint32_t frame,uint32_t end,int block) {
    uint32_t next=(frame/TICK+1)*TICK;
    if(next>end)next=end;
    int n=(int)(next-frame);return n>block ? block : n;
}
static void impulse(FILE *out,float amount,int block) {
    float l[MAX_BLOCK],r[MAX_BLOCK],sl[MAX_BLOCK],sr[MAX_BLOCK];
    dsp_init();ambient_room_init();ambient_room_set(amount);
    /* A silent second settles Amount and feedback smoothing before excitation. */
    for(uint32_t frame=0;frame<SR; ) {
        int n=chunk(frame,SR,block);
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        ambient_room_process(l,r,sl,sr,n);frame+=(uint32_t)n;
    }
    for(at=0;at<SR*SECONDS; ) {
        int n=chunk(at,SR*SECONDS,block);
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        if(!at)sl[0]=sr[0]=.3f;
        ambient_room_process(l,r,sl,sr,n);
        for(int i=0;i<n;++i) { float_sample(out,l[i]);float_sample(out,r[i]); }
        at+=(uint32_t)n;
    }
    fprintf(stderr,"ROOM IMPULSE amount=%.2f frames=%u nominal_T60=%.7f storage_bytes=%zu\n",
        (double)amount,at,(double)ambient_room_tail_seconds(),ambient_room_storage_bytes());
}
static void performance(FILE *out,const char *path,int world,float amount,int block) {
    char name[1024];
    assert(snprintf(name,sizeof name,"%s.events.csv",path)<(int)sizeof name);
    trace=fopen(name,"w");assert(trace);fprintf(trace,"ack_frame,on,owner,hz,velocity\n");
    engine_init();engine_set_world(world);engine_set_gen_seed(1234);
    engine_set_activity(.5f);engine_set_color(.5f);engine_set_attack(.5f);engine_set_release(.5f);
    engine_set_master_volume(.6f);engine_set_room(amount);engine_set_nature(0);
    engine_set_fx_mode(amount>0 ? 1 : 0);
    int16_t pcm[MAX_BLOCK*2];
    for(uint32_t frame=0;frame<SR; ) {
        int n=chunk(frame,SR,block);engine_render(pcm,n);
        for(int i=0;i<n*2;++i)assert(pcm[i]==0);
        frame+=(uint32_t)n;
    }
    engine_set_note_hook(hook);engine_set_generative(true,-1);
    int maximum=0,peak=0;uint32_t final_ms=0;bool stopped=false;
    for(at=0;at<SR*SECONDS; ) {
        if(at%TICK==0) {
            uint32_t ms=at*1000u/SR;
            if(ms==STOP*1000u) { engine_set_generative(false,-1);stopped=true; }
            engine_generative_tick(ms);
            int count=engine_active_voices();assert(count<=(world==1 ? 2 : 3));
            if(count>maximum)maximum=count;
            if(stopped && !count && !final_ms)final_ms=ms;
        }
        int n=chunk(at,SR*SECONDS,block);engine_render(pcm,n);
        for(int i=0;i<n*2;++i) {
            int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;
            u16(out,(uint16_t)pcm[i]);
        }
        at+=(uint32_t)n;
    }
    engine_generative_tick(SECONDS*1000u);
    assert(stopped && starts>=3 && maximum==(world==1 ? 2 : 3));
    assert(engine_active_voices()==0 && final_ms);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fprintf(stderr,"ROOM ENGINE world=%d amount=%.2f frames=%u starts=%d max_sources=%d final_sources=0 final_ms=%u peak=%.8f\n",
        world,(double)amount,at,starts,maximum,final_ms,(double)peak/32768);
    int error=ferror(trace);if(fclose(trace))error=1;assert(!error);
}
int main(int argc,char **argv) {
    if(argc!=6)return 2;
    bool diagnostic=!strcmp(argv[1],"impulse");
    if(!diagnostic && strcmp(argv[1],"engine"))return 2;
    char *end;long world=strtol(argv[2],&end,10);
    if(*end || world<0 || world>2)return 2;
    float amount=strtof(argv[3],&end);
    if(*end || !isfinite(amount) || amount<0 || amount>1)return 2;
    long block=strtol(argv[4],&end,10);
    if(*end || block<1 || block>MAX_BLOCK)return 2;
    FILE *out=fopen(argv[5],"wb");if(!out)return 2;header(out,diagnostic);
    if(diagnostic)impulse(out,amount,(int)block);
    else performance(out,argv[5],(int)world,amount,(int)block);
    /* A failed fputc may set the error flag before fclose itself succeeds. */
    int error=ferror(out);if(fclose(out))error=1;return error ? 2 : 0;
}
