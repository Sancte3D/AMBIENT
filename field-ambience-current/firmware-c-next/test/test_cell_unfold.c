/* Actual Product engine/controls: physical ownership, latch semantics and PCM. */
#include "engine.h"
#include "engine_product.h"
#include "controls.h"
#include "cells.h"
#include "cell_unfold.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define SR DSP_SAMPLE_RATE_HZ
#define FRAMES (8*SR)
static int16_t a[2*FRAMES],b[2*FRAMES];
static void setup(int world) {
    engine_init();controls_init();engine_set_world(world);
    engine_set_nature(0);engine_set_room(.5f);
}
static void render(int16_t *out,int frames,int cap) {
    for(int done=0;done<frames;) {
        int n=frames-done;if(n>cap)n=cap;
        engine_render(out+2*done,n);done+=n;
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
static void press(int cell,bool physical) {
    controls_cell_press((uint8_t)cell,CELL_AMP_MAX*.6f);
    if(physical)controls_cell_physical((uint8_t)cell,true);
}
static void capture(int world,bool physical,int cap,int16_t *out) {
    setup(world);controls_modifier(MOD_HOLD,true);press(0,physical);
    render(out,5*SR,cap);controls_cell_release(0);
    assert(controls_hold_base(0));render(out+10*SR,3*SR,cap);
}
static void after_stop(int world,bool physical,bool mute,int16_t *out) {
    setup(world);press(0,physical);render(out,SR,512);
    if(mute)engine_set_muted(true);else controls_modifier(MOD_CLEAR,true);
    render(out,SR,512);assert(engine_active_voices()==0);
    controls_cell_physical(0,false);if(mute)engine_set_muted(false);
    press(0,false);render(out,2*SR,512);
}
int main(void) {
    uint32_t phase=0;
    for(int n=0;n<4*SR;++n) {
        float x=cell_unfold_step(&phase,1,0);assert(x>=0 && x<=1);
        if(n==2*SR-1)assert(x==.5f);
    }
    assert(phase==12u*SR);
    for(int n=0;n<SR;++n)assert(cell_unfold_step(&phase,1,0)==1);
    for(int n=0;n<SR*6/5;++n)(void)cell_unfold_step(&phase,0,0);
    assert(phase==0);assert(cell_unfold_step(&phase,1,-1)==0);
    assert(cell_unfold_step(&phase,1,16)==0);
    for(int world=0;world<3;++world) {
        capture(world,true,512,a);capture(world,true,37,b);
        assert(!memcmp(a,b,sizeof a));
        capture(world,false,512,b);assert(memcmp(a,b,sizeof a));
        setup(world);press(0,false);engine_set_cell_held(13,true);render(a,FRAMES,512);
        setup(world);press(0,false);render(b,FRAMES,512);assert(!memcmp(a,b,sizeof a));
        setup(world);controls_modifier(MOD_HOLD,true);press(0,true);
        render(a,SR,512);controls_modifier(MOD_SHIFT,true);
        controls_cell_release(0);render(a,2*SR,512);
        setup(world);controls_modifier(MOD_HOLD,true);press(0,true);
        render(b,SR,512);controls_cell_release(0);render(b,2*SR,512);
        assert(!memcmp(a,b,4*SR*sizeof *a));
        /* Rejected re-press cannot intensify the existing latched owner. */
        setup(world);controls_modifier(MOD_HOLD,true);press(0,false);
        render(a,SR,512);controls_cell_release(0);
        controls_modifier(MOD_HOLD,true);press(0,true);render(a,2*SR,512);
        setup(world);controls_modifier(MOD_HOLD,true);press(0,false);
        render(b,SR,512);controls_cell_release(0);
        controls_modifier(MOD_HOLD,true);press(0,false);render(b,2*SR,512);
        assert(!memcmp(a,b,4*SR*sizeof *a));
        for(int mute=0;mute<2;++mute) {
            after_stop(world,true,mute,a);after_stop(world,false,mute,b);
            assert(!memcmp(a,b,4*SR*sizeof *a));
        }
    }
    puts("Cell unfold: exact 4s/1.2s, three Worlds, 37/512 PCM, replay neutrality, HOLD/SHIFT, rejects, Clear/Mute pass");
}
