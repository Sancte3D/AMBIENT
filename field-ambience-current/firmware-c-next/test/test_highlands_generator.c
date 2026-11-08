/* Actual wide Horn chords, retained owner, retirement-based silence/reprise. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include "horn.h"
#include "tuning.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,BLOCK=512 };
static int16_t pcm[BLOCK*2];
static uint32_t now,last_at,stop_at,quiet_at;
static int starts,center,dark,variant,peak,offs[16];
static const int major[8]={-12,7,4,-3,0,-12,7,4},minor[8]={-12,7,3,-5,0,-12,7,3};
static const uint8_t owners[8]={6,7,15,6,7,6,7,15};
static void hook(int on,uint8_t owner,float hz,float velocity) {
    (void)velocity;
    if(on==0) {++offs[owner];return;}
    if(on<0)return;
    assert(starts<8 && owner==owners[starts]);
    int offset=dark ? minor[starts] : starts==3 && variant ? -5 : major[starts];
    assert(fabsf(1200*log2f(hz/tuning_hz((float)(center+offset))))<.01f);
    if(starts==3 || starts==4) {
        assert((horn_active_sources()&(1u<<15)) && offs[15]==0);
        assert(starts==3 ? offs[6]==1 : offs[7]==1);
    }
    if(starts==5)assert(quiet_at && now-quiet_at>=8000u);
    last_at=now;if(++starts==8)stop_at=now+7800;
}
static void pure(void) {
    for(int key=0;key<12;++key)for(int m=0;m<2;++m)for(uint32_t seed=1;seed<=32;++seed) {
        highlands_phrase_t g;highlands_phrase_init(&g,seed);
        for(int i=0;i<8;++i) {
            if(i==5) {
                assert(highlands_phrase_propose(&g,key,m,.5f).midi==-1);
                uint32_t rng=g.rng;
                assert(highlands_phrase_pause(&g,20000,.5f));
                assert(g.rng==rng && g.next_ms>=32000 && g.next_ms<=34000);
                highlands_phrase_t paused=g;
                assert(!highlands_phrase_pause(&g,21000,1) && !memcmp(&paused,&g,sizeof g));
            }
            highlands_phrase_t before=g;
            highlands_offer_t a=highlands_phrase_propose(&g,key,m,.5f),b=highlands_phrase_propose(&g,key,m,.5f);
            assert(!memcmp(&before,&g,sizeof g) && !memcmp(&a,&b,sizeof a));
            assert(highlands_phrase_pitch_allowed(a.midi,key,m));
            assert(a.role==(owners[i]==6 ? 0 : owners[i]==7 ? 1 : 2));
            assert(i==2 ? a.hold_ms==0 : a.hold_ms>=6000 && a.hold_ms<=7750);
            highlands_offer_t wrong=a;wrong.role=(a.role+1u)%3u;
            assert(!highlands_phrase_heard(&g,&wrong,0) && !memcmp(&before,&g,sizeof g));
            assert(highlands_phrase_heard(&g,&a,(uint32_t)i*1000));
            assert(!highlands_phrase_heard(&g,&a,(uint32_t)i*1000));
        }
        assert(g.notes==8 && g.phase==8 && g.episodes==1 && g.returns==1);
        assert(highlands_phrase_pause(&g,0,1));
        assert(g.next_ms>=8000 && g.next_ms<=9100);
        highlands_phrase_restart(&g);
        assert(g.phase==0 && g.notes==8 && !g.timing_valid && !g.rest_valid);
    }
    highlands_phrase_t g;highlands_phrase_init(&g,1234);
    highlands_offer_t a=highlands_phrase_propose(&g,2,false,.5f);
    assert(highlands_phrase_heard(&g,&a,0u-a.gap_ms));
    assert(g.next_ms==0 && !highlands_phrase_due(&g,UINT32_MAX) && highlands_phrase_due(&g,0));
    g.phase=5;g.rest_valid=0;
    assert(highlands_phrase_pause(&g,0,.5f));uint32_t rest=g.next_ms;g.rest_valid=0;
    assert(highlands_phrase_pause(&g,0u-rest,.5f));
    assert(g.next_ms==0 && !highlands_phrase_due(&g,UINT32_MAX) && highlands_phrase_due(&g,0));
}
static int canceled;
static void cancel_hook(int on,uint8_t owner,float hz,float velocity) {
    (void)owner;(void)hz;(void)velocity;if(on>0)++canceled;
}
static void cancellation(void) {
    for(int op=0;op<10;++op) {
        engine_init();engine_set_world(WORLD_HIGHLANDS);engine_set_note_hook(cancel_hook);
        canceled=0;engine_set_generative(true,-1);engine_generative_tick(0);
        assert(engine_active_voices()==1 && engine_generative_melody_count()==0);
        switch(op) {
            case 0:engine_set_generative(false,-1);break;
            case 1:engine_set_autoplay_melody(0);break;
            case 2:engine_set_user_presence(true);engine_generative_tick(0);break;
            case 3:engine_set_muted(true);break;
            case 4:engine_all_off();break;
            case 5:engine_set_gen_seed(42);break;
            case 6:engine_set_world(WORLD_COAST);break;
            case 7:engine_set_key_pc(5);break;
            case 8:engine_set_mode(1);break;
            case 9:engine_set_tuning(1);break;
        }
        for(int f=0;f<SR/5;f+=BLOCK)engine_render(pcm,BLOCK);
        assert(engine_active_voices()==0 && engine_generative_melody_count()==0 && canceled==0);
    }
}
static void one(int key,int m,int just,uint32_t seed,uint32_t base) {
    engine_init();engine_set_world(WORLD_HIGHLANDS);engine_set_key_pc(key);engine_set_mode(m);
    engine_set_tuning(just);engine_set_gen_seed(seed);engine_set_note_hook(hook);
    engine_set_activity(key&1 ? 1 : 0);engine_set_release(key&1 ? 1 : 0);
    engine_set_attack(key&2 ? 1 : 0);engine_set_color(key&4 ? 1 : 0);
    engine_set_room(1);engine_set_master_volume(1);
    center=60+key;if(center>68)center-=12;dark=m;variant=seed&1u;starts=peak=0;last_at=stop_at=quiet_at=0;
    memset(offs,0,sizeof offs);engine_generative_tick(base);engine_set_generative(true,-1);bool stopped=false;int maximum=0;
    for(uint32_t f=0;f<140u*SR;f+=BLOCK) {
        now=base+(uint32_t)((uint64_t)f*1000/SR);engine_generative_tick(now);
        if(stop_at && (int32_t)(now-stop_at)>=0 && !stopped) {engine_set_generative(false,-1);stopped=true;}
        int n=engine_active_voices();assert(n<=3);if(n>maximum)maximum=n;
        if(starts==5 && !n && !quiet_at)quiet_at=now;
        engine_render(pcm,BLOCK);
        for(int i=0;i<2*BLOCK;++i) {int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;}
        if(stopped && engine_active_voices()==0)break;
    }
    assert(stopped && starts==8 && maximum==3 && engine_active_voices()==0);
    assert(engine_generative_episode_count()==1 && engine_generative_return_count()==1);
    assert(offs[6]==3 && offs[7]==3 && offs[15]==2);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0 && peak<16384);
    printf("HIGHLANDS engine key=%d minor=%d just=%d seed=%u base=%u reprise-last-ms=%u peak=%d eight-starts PASS\n",key,m,just,seed,base,last_at,peak);
}
int main(void) {
    pure();cancellation();
    engine_init();engine_set_world(WORLD_HIGHLANDS);engine_set_key_pc(9);
    assert(!engine_try_note_on(0,tuning_hz(45),.7f));
    engine_set_generative(true,-1);
    assert(!engine_try_world_note_on(15,tuning_hz(45),.7f));
    assert(engine_active_voices()==0 && engine_generative_melody_count()==0);
    for(int k=0;k<12;++k)for(int m=0;m<2;++m)for(int j=0;j<2;++j)one(k,m,j,1234,0);
    one(2,0,0,1,0);one(8,0,1,91267,0xffffe000u);
    one(9,1,1,42,0);one(11,1,0,1,0xffffe000u);
    puts("HIGHLANDS GENERATOR PASS: 48 full-key phrases + 4 seed/wrap phrases, retained common source, actual silence/reprise, cancellation and collection/tuning");
    return 0;
}
