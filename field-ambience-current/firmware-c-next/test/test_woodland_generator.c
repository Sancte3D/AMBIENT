/* Actual long string, private register, common chord roles and audio acks. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include "pluck.h"
#include "shape.h"
#include "tuning.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,BLOCK=512 };
static int16_t pcm[BLOCK*2];
static uint32_t now,last_at,stop_at;
static int starts,center,dark,peak;
static const int major[6]={-12,-8,-3,0,-8,-5},minor[6]={-12,-9,-2,0,-9,-5};
static void hook(int on,uint8_t owner,float hz,float velocity) {
    (void)velocity;if(on<=0)return;
    assert(starts<6 && owner==(starts&1 ? 7 : 6));
    assert(fabsf(1200*log2f(hz/tuning_hz((float)(center+(dark ? minor : major)[starts]))))<.01f);
    if(starts==2 || starts==4)assert(pluck_active_sources()&(1u<<7)); /* real retained common chord tone */
    if(starts>=2)assert(now-last_at>=2500); /* no short random arpeggio */
    last_at=now;if(++starts==6)stop_at=now+6500;
}
static void pure(void) {
    for(int key=0;key<12;++key)for(int m=0;m<2;++m)for(uint32_t seed=1;seed<=32;++seed) {
        woodland_phrase_t g;woodland_phrase_init(&g,seed);
        for(int i=0;i<6;++i) {
            woodland_phrase_t before=g;
            woodland_offer_t a=woodland_phrase_propose(&g,key,m,.5f),b=woodland_phrase_propose(&g,key,m,.5f);
            assert(!memcmp(&before,&g,sizeof g) && !memcmp(&a,&b,sizeof a));
            assert(woodland_phrase_pitch_allowed(a.midi,key,m) && a.hold_ms>=6000);
            assert(woodland_phrase_heard(&g,&a,(uint32_t)i*1000));
            assert(!woodland_phrase_heard(&g,&a,(uint32_t)i*1000));
        }
        assert(g.notes==6 && g.phase==6 && g.episodes==1 && g.answers==1);
    }
    woodland_phrase_t g;woodland_phrase_init(&g,1234);
    woodland_offer_t a=woodland_phrase_propose(&g,2,false,.5f);
    assert(woodland_phrase_heard(&g,&a,0u-a.gap_ms));
    assert(g.next_ms==0 && !woodland_phrase_due(&g,UINT32_MAX) && woodland_phrase_due(&g,0));
}
static void source_contract(void) {
    dsp_init();shape_init();pluck_init();pluck_set_ambient(true);pluck_set_damp(.015f);
    assert(pluck_note_on(1,146.83238f,.22f));
    float l[BLOCK],r[BLOCK],sl[BLOCK],sr[BLOCK];double onset=0,body=0,late=0;
    for(int f=0;f<9*SR;) {
        int n=9*SR-f;if(n>BLOCK)n=BLOCK;
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        pluck_render_mix(l,r,sl,sr,n);
        for(int i=0;i<n;++i) {
            double e=(double)l[i]*l[i]+(double)r[i]*r[i];
            if(f+i<SR/10)onset+=e;
            if(f+i>=2*SR && f+i<3*SR)body+=e;
            if(f+i>=8*SR)late+=e;
        }
        f+=n;
    }
    double attack=sqrt(onset*10/body),carry=sqrt(late/body);
    assert(attack<.1 && carry>.2 && pluck_active_count()==1);
    pluck_quiet_source(1,4410);
    for(int remaining=4410;remaining>0;) {
        int n=remaining>BLOCK ? BLOCK : remaining;pluck_render_mix(l,r,sl,sr,n);remaining-=n;
    }
    assert(pluck_active_count()==0);
    printf("WOODLAND SOURCE PASS: onset/body=%.5f eight-second/body=%.5f, bounded context handover\n",attack,carry);
}
static void one(int key,int m,int just) {
    engine_init();engine_set_world(WORLD_WOODLAND);engine_set_key_pc(key);engine_set_mode(m);
    engine_set_tuning(just);engine_set_gen_seed(1234);engine_set_note_hook(hook);
    engine_set_activity(key&1 ? 1 : 0);engine_set_release(key&1 ? 1 : 0);
    engine_set_attack(key&2 ? 1 : 0);engine_set_color(key&4 ? 1 : 0);
    engine_set_room(1);engine_set_master_volume(1);
    center=60+key;if(center>68)center-=12;dark=m;starts=peak=0;last_at=stop_at=0;
    engine_set_generative(true,-1);bool stopped=false;
    for(uint32_t f=0;f<120u*SR;f+=BLOCK) {
        now=(uint32_t)((uint64_t)f*1000/SR);engine_generative_tick(now);
        if(stop_at && (int32_t)(now-stop_at)>=0 && !stopped) {engine_set_generative(false,-1);stopped=true;}
        assert(engine_active_voices()<=2);engine_render(pcm,BLOCK);
        for(int i=0;i<2*BLOCK;++i) {int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;}
        if(stopped && engine_active_voices()==0)break;
    }
    assert(stopped && starts==6 && engine_active_voices()==0 && engine_generative_episode_count()==1);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0 && peak<16384);
    printf("WOODLAND engine key=%d minor=%d just=%d last-ms=%u peak=%d six-starts PASS\n",key,m,just,last_at,peak);
}
int main(void) {
    pure();source_contract();
    engine_init();engine_set_world(WORLD_WOODLAND);engine_set_key_pc(9);
    assert(!engine_try_note_on(0,tuning_hz(45),.7f));
    engine_set_generative(true,-1);
    assert(!engine_try_world_note_on(15,tuning_hz(45),.7f));
    assert(engine_active_voices()==0 && engine_generative_melody_count()==0);
    for(int k=0;k<12;++k)for(int m=0;m<2;++m)for(int j=0;j<2;++j)one(k,m,j);
    puts("WOODLAND GENERATOR PASS: 48 actual phrases, long source, real ownership/release, collection/tuning and wrap");
    return 0;
}
