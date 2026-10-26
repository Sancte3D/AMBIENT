#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "ambient_room.h"
#include "gesture.h"
#include "tuning.h"
#include "controls.h"
#include "cells.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,TICK=441,SECONDS=45 };
static int16_t a[SECONDS*SR*2],b[SECONDS*SR*2],pcm[1024];
static unsigned ons,offs,live_offs,calls;
static float played_hz,played_velocity;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    if(on>0) {++ons;if(owner==8 || owner==14) {played_hz=hz;played_velocity=velocity;}}
    if(on==0) {++offs;if(owner==0)++live_offs;}
}
static bool loop(uint8_t c,bool upper,bool down,float velocity,uint32_t ms) {
    ++calls;return engine_gesture_note(c,upper,down,velocity,ms);
}
static void render(unsigned frames) {
    while(frames) {
        int n=frames>512 ? 512 : (int)frames;engine_render(pcm,n);frames-=(unsigned)n;
        assert(engine_active_voices()<=3);
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
static void capture(int world,int cap,int16_t *out) {
    engine_init();gesture_init_owned(loop);engine_set_world(world);engine_set_gen_seed(1234);
    engine_set_generative(true,-1);engine_set_master_volume(.75f);
    for(unsigned tick=0;tick<SECONDS*100u;++tick) {
        unsigned ms=tick*10;
        if(ms==5000)engine_set_sound_effects(.6f,.5f,false,.3f,.1f,.4f,true);
        if(ms==9000)engine_set_sound_effects(.6f,.5f,true,.3f,.1f,.4f,true);
        if(ms==12000)engine_set_sound_effects(.3f,.3f,false,0,0,0,false);
        if(ms==18000)engine_set_sound_effects(.3f,.3f,false,0,0,0,true);
        if(ms==25000)engine_set_fx_mode(0);
        if(ms==27000)engine_set_fx_mode(1);
        if(ms==33000)engine_set_generative(false,-1);
        if(ms==39000)engine_all_off();
        engine_generative_tick(ms);
        for(int done=0;done<TICK;) {
            int n=TICK-done;if(n>cap)n=cap;engine_render(out+2*(tick*TICK+done),n);done+=n;
        }
        assert(engine_active_voices()<=(world==1 ? 2 : 3));
    }
    for(unsigned k=43u*SR*2;k<SECONDS*SR*2u;++k)assert(out[k]==0);
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
static void owned(void) {
    engine_init();gesture_init_owned(loop);engine_set_note_hook(hook);
    ons=offs=live_offs=calls=0;
    float live=tuning_hz((float)engine_product_cell_midi(0,false));
    assert(engine_try_note_on(0,live,.3f));render(441);engine_generative_tick(10);
    gesture_toggle(100);gesture_set_record_context(true,.33f);
    gesture_record_cell(0,true,100);gesture_toggle(1100);
    gesture_tick(1100);render(441);engine_generative_tick(20);
    assert(ons==2 && played_velocity>.32f && played_velocity<.34f);
    float expected=tuning_hz((float)engine_product_cell_midi(0,true));
    assert(fabsf(played_hz-expected)<.001f);
    for(unsigned ms=1200;ms<6100;ms+=100) {gesture_tick(ms);render(441);engine_generative_tick(ms);}
    assert(ons==2 && !live_offs && calls==1); /* exact tie; no restart */
    gesture_stop(6100);assert(gesture_count()==1 && gesture_state()==GESTURE_IDLE);
    assert(!live_offs);render(SR);engine_generative_tick(7100);
    assert(!live_offs);gesture_clear(7100);assert(!gesture_count());
    assert(engine_sounding_notes((int[32]){0},32)>0); /* live owner survives */
    engine_set_muted(true);render(SR/2);assert(!engine_active_voices());
    for(int i=0;i<1024;++i)assert(!pcm[i]);
    /* Full pool/rejected replay cannot release a live Cell on its off edge. */
    engine_init();gesture_init_owned(loop);engine_set_note_hook(hook);live_offs=0;
    assert(engine_try_note_on(0,live,.3f));render(441);engine_generative_tick(10);
    gesture_toggle(0);gesture_set_record_context(false,1);
    gesture_record_cell(0,true,0);gesture_record_cell(0,false,100);gesture_toggle(500);
    gesture_tick(500);gesture_tick(600);gesture_clear(601);assert(!live_offs);
    /* A huge missed interval has bounded callbacks, including uint32 wrap. */
    engine_init();gesture_init_owned(loop);gesture_toggle(UINT32_MAX-1000u);
    gesture_record_cell(0,true,UINT32_MAX-1000u);gesture_toggle(UINT32_MAX-500u);
    calls=0;gesture_tick(UINT32_MAX-500u);gesture_tick(3000000000u);assert(calls<=GESTURE_MAX*2u);
    gesture_clear(0);
}
static void frozen_history(void) {
    engine_init();gesture_init_owned(loop);
    float hz=tuning_hz((float)engine_product_cell_midi(0,false));
    assert(engine_try_note_on(0,hz,.5f));render(2*SR);engine_generative_tick(2000);
    engine_set_sound_effects(.3f,.8f,true,0,0,0,true);
    engine_note_off(0);
    for(int ms=2010;ms<25000;ms+=10) {render(TICK);engine_generative_tick((unsigned)ms);}
    float ledger[32];int count=engine_sounding_frequencies(ledger,32);bool found=false;
    for(int i=0;i<count;++i)if(fabsf(ledger[i]-hz)<.001f)found=true;
    assert(found); /* Original 7.2/14s deadline must not release frozen pitch. */
    engine_set_sound_effects(.3f,.8f,false,0,0,0,true);engine_generative_tick(25000);
    assert(engine_sounding_frequencies(ledger,32)>0);
    engine_all_off();render(SR/2);engine_generative_tick(25500);
    assert(!engine_sounding_frequencies(ledger,32));
    for(int i=0;i<1024;++i)assert(!pcm[i]);
}
int main(void) {
    for(int world=0;world<3;++world) {
        capture(world,512,a);capture(world,37,b);assert(!memcmp(a,b,sizeof a));
        printf("Completion world %d: entire 45s PCM exact 37/512, plate/FDN/Freeze/Dry edits and full Clear pass\n",world);
    }
    owned();frozen_history();
    puts("Sound completion: owned octave/velocity ties, live isolation, bounded lateness/wrap, frozen pitch history and Mute pass");
}
