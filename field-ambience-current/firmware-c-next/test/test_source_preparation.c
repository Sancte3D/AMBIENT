/* Include the actual source, with observation-only hooks inserted in a
 * temporary copy. Audio interrupts Main; Main never interrupts audio. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "dsp.h"
#include "shape.h"
#include SOURCE_IMPLEMENTATION
#if FAMILY==0
#define INIT bowed_init
#define START bowed_try_note_on
#define MASK bowed_active_sources
#define OFF bowed_note_off
#define CAPACITY 3
#elif FAMILY==1
#define INIT horn_init
#define START horn_try_note_on
#define MASK horn_active_sources
#define OFF horn_note_off
#define CAPACITY 3
#else
#define INIT pluck_init
#define START pluck_note_on
#define MASK pluck_active_sources
#define OFF pluck_note_off
#define CAPACITY PLUCK_VOICES
#endif
enum { AFTER=1024,MAX_PAUSE=1536,CAP=4*(AFTER+MAX_PAUSE) };
static float expected[CAP],actual[CAP];
static unsigned checks,cases,interrupts,visits,chosen,block_size,pauses;
static bool armed,after_publication;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"prep family=%d case=%u point=%u line=%d: %s\n",FAMILY,cases,chosen,__LINE__,#x);exit(1);} }while(0)
static void render(float *out,unsigned frames) {
    float l[512],r[512],sl[512],sr[512];
    for(unsigned at=0;at<frames;) {
        unsigned n=frames-at<block_size?frames-at:block_size;
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
#if FAMILY==0
        bowed_render_mix(l,r,sl,sr,(int)n,.35f);
#elif FAMILY==1
        horn_render_mix(l,r,sl,sr,(int)n,.35f);
#else
        pluck_render_mix(l,r,sl,sr,(int)n);
#endif
        if(out)for(unsigned i=0;i<n;++i){
            out[4*(at+i)]=l[i];out[4*(at+i)+1]=r[i];
            out[4*(at+i)+2]=sl[i];out[4*(at+i)+3]=sr[i];
        }
        at+=n;
    }
}
void source_preparation_probe(unsigned point) {
    ++visits;if(!armed || point!=chosen)return;
    armed=false;
    CHECK(MASK()==(after_publication ? 6u : 2u));
    /* Audio release handling may inspect sources while another slot is
     * being prepared. An unrelated owner must not change that preparation. */
    OFF(15);
    for(unsigned half=0;half<pauses;++half) {
        ++interrupts;render(actual+4*half*block_size,block_size);
    }
}
static void setup(void) {
    armed=false;INIT();shape_init();
#if FAMILY==2
    pluck_set_ambient(true);
#endif
    CHECK(START(1,196.0f,.18f));render(NULL,256);
    CHECK(MASK()==2u);
}
static void one(unsigned point,float hz,unsigned block,unsigned halves) {
    ++cases;chosen=point;block_size=block;pauses=halves;after_publication=point==9;
    unsigned pause=block*halves;
    setup();
    if(after_publication)CHECK(START(2,hz,.22f));
    render(expected,pause);
    if(!after_publication)CHECK(START(2,hz,.22f));
    render(expected+4*pause,AFTER);
    setup();armed=true;CHECK(START(2,hz,.22f));CHECK(!armed);
    render(actual+4*pause,AFTER);
    unsigned count=4*(pause+AFTER);
    if(memcmp(expected,actual,count*sizeof(float))) {
        unsigned first=0;while(first<count && !memcmp(expected+first,actual+first,4))++first;
        fprintf(stderr,"changed actual source PCM at %u\n",first);CHECK(false);
    }
    for(unsigned i=0;i<count;++i)CHECK(isfinite(actual[i]));
}
static void fill(void) {
    INIT();shape_init();
#if FAMILY==2
    pluck_set_ambient(true);
#endif
    for(int i=0;i<CAPACITY;++i)CHECK(START(i,196.0f+35.0f*i,.22f));
}
static void rejected(void) {
    ++cases;block_size=512;armed=false;fill();render(expected,AFTER);
    fill();unsigned before=visits;
    CHECK(!START(0,440,.3f));CHECK(!START(14,440,.3f));
    CHECK(!START(14,NAN,.3f));CHECK(!START(14,440,0));
    CHECK(visits==before);render(actual,AFTER);
    CHECK(!memcmp(expected,actual,4*AFTER*sizeof(float)));
}
int main(int argc,char **argv) {
    dsp_init();block_size=64;
    if(argc==3 && !strcmp(argv[1],"--trace")) {
        FILE *f=fopen(argv[2],"wb");CHECK(f);
        for(int k=0;k<4;++k){const float hz[]={60,146.83238f,440,8000};setup();CHECK(START(2,hz[k],.22f));render(actual,AFTER);CHECK(fwrite(actual,sizeof(float),4*AFTER,f)==4*AFTER);}
        CHECK(fclose(f)==0);return 0;
    }
    CHECK(argc==1);
    for(unsigned point=0;point<10;++point)for(int f=0;f<4;++f)
        for(unsigned b=64;b<=512;b*=8)for(unsigned h=1;h<=3;h+=2) {
            const float hz[]={60,146.83238f,440,8000};one(point,hz[f],b,h);
        }
#if FAMILY==2
    /* Every write iteration in the three real excitation passes, at both
     * the public source floor and the ordinary Product low register. */
    for(int f=0;f<2;++f) {
        float hz=f ? 146.83238f : PLUCK_MIN_HZ;
        int n=(int)(44100.0f/hz)+1;
        for(unsigned k=0;k<1024;++k)one(32+k,hz,64,1);
        for(int k=0;k<n;++k){one(2048+(unsigned)k,hz,64,1);one(4096+(unsigned)k,hz,64,1);}
    }
#endif
    rejected();
    printf("{\"family\":%d,\"cases\":%u,\"assertions\":%u,\"audio_half_preemptions\":%u,\"normal_float_pcm_equal\":true}\n",FAMILY,cases,checks,interrupts);
    return 0;
}
