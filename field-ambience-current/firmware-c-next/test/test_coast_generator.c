/* Real Product engine, including owner/tail admission, audio-start acks,
 * all keys/collections/tunings and the shared destination. No DSP surrogate. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,BLOCK=512 };
static int16_t pcm[BLOCK*2];
static uint32_t now,stop_at;
static int lower[4],upper[3],nl,nu,anchor,center,onsets,peak;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    (void)velocity;if(on<=0)return;
    int midi=(int)lroundf(69+12*log2f(hz/440));
    assert(midi>=45 && midi<=80);
    ++onsets;
    if(owner==6) {assert(nl<4);lower[nl++]=midi;if(nl==4)stop_at=now+6000;}
    else if(owner==7) {assert(nu<3);upper[nu++]=midi;}
    else {assert(owner==15 && anchor==0);++anchor;}
}
static void pure_contract(void) {
    for(int key=0;key<12;++key)for(int dark=0;dark<2;++dark)for(uint32_t seed=1;seed<=32;++seed) {
        coast_phrase_t g;coast_phrase_init(&g,seed);
        for(int phase=0;phase<4;++phase) {
            int roles=phase==0 ? 3 : phase==3 ? 1 : 2;
            for(int role=0;role<roles;++role) {
                coast_phrase_t before=g;
                coast_offer_t a=coast_phrase_propose(&g,role,key,dark,.5f);
                coast_offer_t b=coast_phrase_propose(&g,role,key,dark,.5f);
                assert(!memcmp(&before,&g,sizeof g) && !memcmp(&a,&b,sizeof a));
                assert(coast_phrase_pitch_allowed(a.midi,key,dark));
                assert(coast_phrase_heard(&g,&a,1000u*(uint32_t)(phase*4+role)));
            }
        }
        assert(g.phase==4 && g.notes==8 && g.episodes==1);
    }
    coast_phrase_t g;coast_phrase_init(&g,1234);
    coast_offer_t a=coast_phrase_propose(&g,0,2,false,.5f);
    uint32_t base=0u-a.gap_ms;
    assert(coast_phrase_heard(&g,&a,base));
    assert(!coast_phrase_heard(&g,&a,base)); /* duplicate ack is not another note */
    for(int role=1;role<3;++role) {
        coast_offer_t b=coast_phrase_propose(&g,role,2,false,.5f);
        assert(coast_phrase_heard(&g,&b,base+700));
    }
    assert(g.next_ms==0 && !coast_phrase_due(&g,UINT32_MAX) && coast_phrase_due(&g,0));
}
static void one(int key,int dark,int just) {
    engine_init();engine_set_key_pc(key);engine_set_mode(dark);engine_set_tuning(just);
    engine_set_gen_seed(1234);engine_set_release(key&1 ? 1 : 0);
    engine_set_room(1);engine_set_color(1);engine_set_master_volume(1);
    engine_set_activity(key&1 ? 1 : 0);engine_set_note_hook(hook);
    center=60+key;if(center>68)center-=12;
    nl=nu=anchor=onsets=peak=0;stop_at=0;bool stopped=false;
    engine_set_generative(true,-1);
    for(uint32_t frame=0;frame<180u*SR;frame+=BLOCK) {
        now=(uint32_t)((uint64_t)frame*1000/SR);
        engine_generative_tick(now);
        if(stop_at && (int32_t)(now-stop_at)>=0 && !stopped) {
            engine_set_generative(false,-1);stopped=true;
        }
        assert(engine_active_voices()<=3);
        engine_render(pcm,BLOCK);
        for(int i=0;i<2*BLOCK;++i) {int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;}
        if(stopped && engine_active_voices()==0)break;
    }
    assert(stopped && onsets==8 && nl==4 && nu==3 && anchor==1);
    assert(lower[0]==center-12 && upper[0]==center+12 && lower[3]==center);
    for(int i=1;i<4;++i)assert(lower[i]>lower[i-1]);
    for(int i=1;i<3;++i)assert(upper[i]<upper[i-1] && upper[i]>center);
    assert(engine_generative_episode_count()==1 && engine_active_voices()==0);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0 && peak<16384);
    printf("COAST engine key=%d minor=%d just=%d meeting-before-ms=%u peak=%d eight-starts PASS\n",key,dark,just,stop_at,peak);
}
int main(void) {
    pure_contract();
    for(int key=0;key<12;++key)for(int dark=0;dark<2;++dark)for(int just=0;just<2;++just)
        one(key,dark,just);
    puts("COAST GENERATOR PASS: 48 actual engine phrases, both macro release endpoints, shared goal, actual retirement and pure/wrap contracts");
    return 0;
}
