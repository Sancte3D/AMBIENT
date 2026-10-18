/* Real Nature twice: original module versus observed publication boundaries.
 * Main may be interrupted by a complete audio half; Main never runs in audio.
 * Entire stereo float outputs are compared, including post-publication audio. */
#include "nature.h"
#include "dsp.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
void observed_nature_init(void);
void observed_nature_clear(void);
void observed_nature_set_amount(float);
void observed_nature_set_world(int);
void observed_nature_set_seed(uint32_t);
void observed_nature_render(float *,float *,int);
enum { SR=44100,BLOCK=512,AFTER=4096,CAP=SR*38 };
static float original[CAP*2],observed[CAP*2],l[BLOCK],r[BLOCK];
static unsigned loads,coefs,max_coefs,interrupts,cases;
static uint64_t frames_compared;
static bool armed,in_audio;
static unsigned boundary;
static int irq_frames;
static float *irq_output;
void nature_test_load(void) { ++loads; }
void nature_test_coefficient(void) { ++coefs; }
static void render(bool instrumented,int frames,int block,float *out) {
    while(frames>0) {
        int n=frames>block ? block : frames;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        assert(!in_audio);in_audio=true;loads=coefs=0;
        if(instrumented) observed_nature_render(l,r,n);else nature_render(l,r,n);
        in_audio=false;
        if(instrumented) {
            assert(loads==3);
            /* One reset (two water coefficients), and at most ceil(n/64)+1
             * wind updates if that reset changes the control phase mid-block. */
            assert(coefs<=2u+2u*((unsigned)(n+63)/64u+1u));
            if(coefs>max_coefs)max_coefs=coefs;
        }
        for(int i=0;i<n;++i) {
            assert(isfinite(l[i])&&isfinite(r[i]));
            if(out) {out[2*i]=l[i];out[2*i+1]=r[i];}
        }
        if(out)out+=2*n;
        frames-=n;
    }
}
void nature_test_publication(unsigned after) {
    assert(!in_audio);
    if(!armed||boundary!=after)return;
    armed=false;++interrupts;
    render(true,irq_frames,irq_frames,irq_output);
}
static void setup(bool o,int world) {
    if(o) {
        observed_nature_init();observed_nature_set_world(world);
        observed_nature_set_seed(1234);observed_nature_set_amount(.7f);
    } else {
        nature_init();nature_set_world(world);nature_set_seed(1234);nature_set_amount(.7f);
    }
}
static void target(bool o,int kind,int value) {
    static const uint32_t seeds[4]={0,1,0xA6B13E7Du,UINT32_MAX};
    static const float amounts[3]={0,.2f,1};
    if(kind==0) {if(o)observed_nature_set_world(value);else nature_set_world(value);}
    if(kind==1) {if(o)observed_nature_set_seed(seeds[value]);else nature_set_seed(seeds[value]);}
    if(kind==2) {if(o)observed_nature_set_amount(amounts[value]);else nature_set_amount(amounts[value]);}
}
static void publication_case(int world,int kind,int value,unsigned after,int block) {
    setup(false,world);render(false,SR*3,BLOCK,NULL);
    if(after)target(false,kind,value);
    render(false,block,block,original);
    if(!after)target(false,kind,value);
    render(false,AFTER,BLOCK,original+2*block);

    setup(true,world);render(true,SR*3,BLOCK,NULL);
    boundary=after;irq_frames=block;irq_output=observed;armed=true;
    target(true,kind,value);assert(!armed);
    render(true,AFTER,BLOCK,observed+2*block);
    assert(!memcmp(original,observed,(size_t)(block+AFTER)*2*sizeof(float)));
    frames_compared+=(uint64_t)block+AFTER;++cases;
}
static void invalid(bool o) {
    if(o) {
        observed_nature_set_amount(NAN);observed_nature_set_amount(INFINITY);
        observed_nature_set_world(-1);observed_nature_set_world(3);
    } else {
        nature_set_amount(NAN);nature_set_amount(INFINITY);nature_set_world(-1);nature_set_world(3);
    }
}
/* The same commands at the same actual frame, with different audio partitions.
 * Bursts supersede an unfinished 80 ms World/seed fade. Later: complete idle,
 * audio-owned Clear, fresh target/seed, and restart. No timing from host speed. */
static int timeline(bool o,int world,int block,float *out) {
    setup(o,world);
    const int total=SR*38;
    int frame=0,event=0;
    while(frame<total) {
        int next=event<18 ? SR+event*221 : event==18 ? SR*3 :
                 event==19 ? SR*35 : event==20 ? SR*35+17 : total;
        if(frame==next&&event<21) {
            if(event<18) {
                target(o,0,(world+event)%3);target(o,1,event%4);
                target(o,2,event%3);invalid(o);
            } else if(event==18)target(o,2,0);
            else if(event==19) {
                /* Amount has settled to exact silence before Clear. */
                if(o)observed_nature_clear();else nature_clear();
                target(o,0,(world+2)%3);target(o,1,0);
            } else target(o,2,2);
            ++event;continue;
        }
        int n=next-frame;if(n>block)n=block;
        assert(n>0);render(o,n,block,out+2*frame);
        if(frame>=SR*34&&frame<SR*35+17)
            for(int i=0;i<n*2;++i)assert(out[2*frame+i]==0);
        frame+=n;
    }
    return total;
}
int main(void) {
    dsp_init();
    for(int w=0;w<3;++w)for(int kind=0;kind<3;++kind) {
        int count=kind==1 ? 4 : 3;
        for(int v=0;v<count;++v)for(unsigned b=0;b<2;++b)
            for(int n=64;n<=512;n*=8)publication_case(w,kind,v,b,n);
    }
    for(int w=0;w<3;++w) {
        int frames=timeline(false,w,512,original);
        timeline(false,w,64,observed);
        assert(!memcmp(original,observed,(size_t)frames*2*sizeof(float)));
        frames_compared+=(uint64_t)frames;
        timeline(true,w,512,observed);
        assert(!memcmp(original,observed,(size_t)frames*2*sizeof(float)));
        frames_compared+=(uint64_t)frames;
    }
    assert(cases==120&&interrupts==120);
    printf("{\"publication_cases\":%u,\"audio_preemptions\":%u,"
           "\"timeline_worlds\":3,\"timeline_seconds\":38,"
           "\"full_stereo_frames_compared\":%llu,\"target_loads_per_render\":3,"
           "\"max_observed_svf_coefficients_per_render\":%u}\n",
           cases,interrupts,(unsigned long long)frames_compared,max_coefs);
    return 0;
}
