/* Loud endpoint combinations through actual Product ownership and output. */
#include "engine.h"
#include "engine_product.h"
#include "controls.h"
#include "cells.h"
#include "dsp.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static int16_t pcm[1024];
static void render_frames(int frames,int world) {
    while(frames>0) {
        int n=frames>512 ? 512 : frames;engine_render(pcm,n);frames-=n;
        assert(engine_active_voices()<=(world==1 ? 2 : 3));
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
int main(void) {
    unsigned cases=0;
    for(int world=0;world<3;++world)for(int seed=0;seed<2;++seed)
    for(int minor=0;minor<2;++minor)for(int just=0;just<2;++just)for(int color=0;color<2;++color) {
        engine_init();controls_init();engine_set_world(world);engine_set_key_pc((seed*5+world*3)%12);
        engine_set_mode(minor);engine_set_tuning(just);engine_set_gen_seed(1234u+seed);
        engine_set_world_character(true);engine_set_master_volume(1);
        engine_set_color((float)color);engine_set_room(1);engine_set_nature(1);
        engine_set_attack(seed ? 1 : 0);engine_set_release(1);controls_modifier(MOD_HOLD,true);
        for(int cell=0;cell<5;cell+=2) {
            controls_cell_press((uint8_t)cell,CELL_AMP_MAX);controls_cell_physical((uint8_t)cell,true);
        }
        render_frames(8*DSP_SAMPLE_RATE_HZ,world);
        for(int cell=0;cell<5;cell+=2)controls_cell_release((uint8_t)cell);
        render_frames(4*DSP_SAMPLE_RATE_HZ,world);
        if(seed)engine_set_muted(true);else controls_modifier(MOD_CLEAR,true);
        render_frames(DSP_SAMPLE_RATE_HZ/2,world);assert(engine_active_voices()==0);
        for(int i=0;i<1024;++i)assert(pcm[i]==0);
        ++cases;
    }
    printf("World character endpoints: %u max-velocity/volume/room/nature cases, major/minor, ET/JI, Color ends, long envelopes and Clear/Mute pass\n",cases);
}
