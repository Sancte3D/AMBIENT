/* Real shared room and optional Nature: no surrogate filters or schedule. */
#include "ambient_room.h"
#include "nature.h"
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
enum { SR=44100,BLOCK=512,CAP=SR*12 };
static float a[CAP*2],b[CAP*2],l[BLOCK],r[BLOCK],sl[BLOCK],sr[BLOCK];
static int16_t pcm[BLOCK*2];
static void zero(void) { memset(l,0,sizeof l); memset(r,0,sizeof r); memset(sl,0,sizeof sl); memset(sr,0,sizeof sr); }
static double room_seconds(float seconds) {
    int count=(int)(seconds*SR); double energy=0;
    while(count>0) {
        int n=count>BLOCK ? BLOCK : count; zero(); ambient_room_process(l,r,sl,sr,n);
        for(int i=0;i<n;++i) { assert(isfinite(l[i]) && isfinite(r[i])); energy+=l[i]*l[i]+r[i]*r[i]; }
        count-=n;
    } return energy;
}
static void room_impulse(float amount) {
    ambient_room_init();ambient_room_set(amount);(void)room_seconds(1);
    double energy=0,mono=0,late=0; float peak=0;
    for(int frame=0;frame<SR*12;) {
        int n=SR*12-frame>BLOCK ? BLOCK : SR*12-frame; zero();
        if(frame==0)sl[0]=sr[0]=.3f;
        ambient_room_process(l,r,sl,sr,n);
        for(int i=0;i<n;++i) {
            float L=l[i],R=r[i];assert(isfinite(L)&&isfinite(R));
            energy+=(double)L*L+(double)R*R; mono+=(double)(L+R)*(L+R)*.5;
            if(frame+i>=SR*9)late+=(double)L*L+(double)R*R;
            peak=fmaxf(peak,fmaxf(fabsf(L),fabsf(R)));
        }frame+=n;
    }
    assert(peak<.3f && late<energy*1e-6+1e-20);
    if(amount>0)assert(energy>0 && mono/energy>.15);
    else assert(energy==0);
    printf("ROOM amount=%.1f nominal_T60=%.3f energy=%.9g mono_ratio=%.5f late9s_ratio=%.9g peak=%.7f\n",
        (double)amount,(double)ambient_room_tail_seconds(),energy,energy>0?mono/energy:1,energy>0?late/energy:0,(double)peak);
}
static void nature_captured(int block,float *out,bool invalid) {
    nature_init();nature_set_seed(1234);nature_set_world(0);nature_set_amount(.7f);
    for(int frame=0;frame<CAP;) {
        if(frame==SR*4)nature_set_world(1);
        if(frame==SR*8) { nature_set_world(2);nature_set_seed(5678); }
        int end=frame<SR*4 ? SR*4 : frame<SR*8 ? SR*8 : CAP;
        int n=end-frame>block ? block : end-frame;zero();
        if(invalid) { nature_set_amount(NAN);nature_set_amount(INFINITY);nature_set_world(-1); }
        nature_render(l,r,n);
        for(int i=0;i<n;++i) { out[2*(frame+i)]=l[i];out[2*(frame+i)+1]=r[i];assert(isfinite(l[i])&&isfinite(r[i])); }
        frame+=n;
    }
}
static uint32_t hook_hash,hook_ms;
static void hook(int on,uint8_t owner,float hz,float v) {
    uint32_t hbits,vbits;memcpy(&hbits,&hz,4);memcpy(&vbits,&v,4);
    uint32_t words[5]={(uint32_t)on,owner,hbits,vbits,hook_ms};
    for(int i=0;i<5;++i)hook_hash=(hook_hash^words[i])*16777619u;
}
static uint32_t trace(int world,float nature) {
    engine_init();engine_set_world(world);engine_set_gen_seed(0x1234);
    engine_set_nature(nature);engine_set_note_hook(hook);engine_set_generative(true,-1);hook_hash=2166136261u;
    for(uint32_t frame=0;frame<SR*90;frame+=BLOCK) {
        hook_ms=(uint32_t)((uint64_t)frame*1000/SR);engine_generative_tick(hook_ms);engine_render(pcm,BLOCK);
    }
    hook_ms=90000;engine_generative_tick(hook_ms);
    assert(engine_nonfinite_samples()==0&&engine_output_limited_samples()==0);return hook_hash;
}
int main(void) {
    dsp_init();
    for(int i=0;i<3;++i)room_impulse(i*.5f);
    ambient_room_init();ambient_room_set(1);
    for(int j=0;j<SR*3/BLOCK;++j) {
        zero();for(int i=0;i<BLOCK;++i)sl[i]=sr[i]=.15f*sinf(6.2831853f*146.8324f*(j*BLOCK+i)/SR);
        ambient_room_process(l,r,sl,sr,BLOCK);
    }
    ambient_room_enable(false);(void)room_seconds(.10f);ambient_room_enable(true);
    assert(room_seconds(2)==0); /* Completed Dry clears old tails; re-enable is fresh. */
    nature_captured(512,a,false);nature_captured(64,b,false);assert(!memcmp(a,b,sizeof a));
    nature_captured(512,b,true);assert(!memcmp(a,b,sizeof a));
    double energy=0;for(int i=0;i<CAP*2;++i)energy+=(double)a[i]*a[i];assert(energy>0);

    /* An inactive optional path neither processes nor ages a hidden weather loop. */
    nature_init();nature_set_seed(99);
    for(int i=0;i<SR*20/BLOCK;++i) { zero();nature_render(l,r,BLOCK);for(int j=0;j<BLOCK;++j)assert(l[j]==0&&r[j]==0); }
    nature_set_amount(.7f);
    for(int frame=0;frame<SR;) {
        zero();int n=SR-frame>BLOCK ? BLOCK : SR-frame;nature_render(l,r,n);
        for(int i=0;i<n;++i) { a[2*(frame+i)]=l[i];a[2*(frame+i)+1]=r[i]; }frame+=n;
    }
    nature_init();nature_set_seed(99);nature_set_amount(.7f);
    for(int frame=0;frame<SR;) {
        zero();int n=SR-frame>BLOCK ? BLOCK : SR-frame;nature_render(l,r,n);
        for(int i=0;i<n;++i) { b[2*(frame+i)]=l[i];b[2*(frame+i)+1]=r[i]; }frame+=n;
    }
    assert(!memcmp(a,b,SR*2*sizeof(float)));
    engine_init();engine_set_nature(.7f);
    long long pcm_energy=0;
    for(int i=0;i<SR*5/BLOCK;++i) {
        engine_render(pcm,BLOCK);for(int j=0;j<BLOCK*2;++j)pcm_energy+=(long long)pcm[j]*pcm[j];
    }
    assert(pcm_energy>0 && engine_active_voices()==0); /* No inaudible phantom tone needed. */
    engine_all_off();
    for(int i=0;i<30;++i)engine_render(pcm,BLOCK);
    for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
    for(int world=0;world<3;++world)assert(trace(world,0)==trace(world,.7f));
    puts("ROOM/NATURE PASS: impulse/decay/mono, no Dry revival, seed/block/input invariance, cold idle, independent score and real Clear");
    return 0;
}
