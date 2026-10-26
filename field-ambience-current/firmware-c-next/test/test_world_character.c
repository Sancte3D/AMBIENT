/* Candidate trajectories + actual Product owners, controls and PCM. */
#include "world_character.h"
#include "engine.h"
#include "engine_product.h"
#include "controls.h"
#include "cells.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define SR DSP_SAMPLE_RATE_HZ
#define SECONDS 16
static int16_t a[2*SECONDS*SR],b[2*SECONDS*SR];
static void capture(int world,uint32_t seed,bool character,int cap,int16_t *out) {
    engine_init();controls_init();engine_set_world(world);engine_set_gen_seed(seed);
    engine_set_world_character(character);controls_modifier(MOD_HOLD,true);
    for(int tick=0;tick<SECONDS*100;++tick) {
        if(tick==0 || tick==100 || tick==1300) {
            int cell=tick==100 ? 2 : 0;
            controls_cell_press((uint8_t)cell,CELL_AMP_MAX*.6f);
            controls_cell_physical((uint8_t)cell,true);
        }
        if(tick==800)controls_cell_release(0);
        if(tick==900)controls_cell_release(2);
        if(tick==1200)controls_modifier(MOD_CLEAR,true);
        for(int done=0;done<441;) {
            int n=441-done;if(n>cap)n=cap;
            engine_render(out+2*(tick*441+done),n);done+=n;
        }
        assert(engine_active_voices()<=(world==1 ? 2 : 3));
        if(tick==1299)assert(engine_active_voices()==0);
    }
    assert(!engine_output_limited_samples() && !engine_nonfinite_samples());
}
int main(void) {
    dsp_init();
    for(int family=0;family<3;++family)for(uint32_t seed=0;seed<12;++seed) {
        world_character_t c;
        world_character_prepare(&c,family,seed,0,196,true);
        assert(c.age==0 && c.tone==1 && c.gain==1 && c.pan==0);
        for(unsigned tick=0;tick<120u*SR/32u;++tick) {
            float last=c.tone;c.clock=31;world_character_tick(&c);
            assert(isfinite(c.tone) && c.tone>=.68f && c.tone<=1.32f);
            assert(c.detail>=.73f && c.detail<=1.27f);
            assert(c.gain>=.944f && c.gain<=1.056f && fabsf(c.pan)<=.0351f);
            assert(fabsf(c.tone-last)<.001f);
        }
        assert(c.age==4u*SR);
        world_character_prepare(&c,family,seed,0,196,false);
        world_character_t off=c;
        for(int i=0;i<SR;++i)world_character_tick(&c);
        assert(!memcmp(&off,&c,sizeof c));
    }
    for(int world=0;world<3;++world) {
        capture(world,42,true,512,a);capture(world,42,true,37,b);
        assert(!memcmp(a,b,sizeof a));
        capture(world,42,false,512,b);assert(memcmp(a,b,sizeof a));
        capture(world,43,true,512,b);assert(memcmp(a,b,sizeof a));
    }
    puts("World character: 72 minutes bounded trajectories, three Worlds, 37/512 exact PCM, seed variants, physical unfold/HOLD, Clear/reuse pass");
}
