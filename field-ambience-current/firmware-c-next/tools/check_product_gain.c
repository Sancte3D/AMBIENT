/* SD34 actual PCM: common normalised velocity, legacy adapter and ceiling. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "cells.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,FRAMES=4*SR };
static int16_t reference[2*FRAMES],candidate[2*FRAMES],buffer[1024];
static int starts,expected_owner;
static float heard_velocity;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(on==1 && owner==expected_owner && hz==220);++starts;heard_velocity=velocity;
}
static void capture(int world,float v,int api,int block,int16_t *out) {
    engine_init();engine_set_world(world);engine_set_nature(0);
    for(int at=0;at<SR;) {int n=SR-at;if(n>512)n=512;engine_render(buffer,n);at+=n;}
    starts=0;heard_velocity=0;expected_owner=api==1 ? 6 : 0;engine_set_note_hook(hook);
    if(api==1) {engine_set_generative(true,-1);assert(engine_try_world_note_on(6,220,v));}
    else if(api==2)engine_note_on(0,220,v*CELL_AMP_MAX);
    else assert(engine_try_note_on(0,220,v));
    for(int at=0;at<FRAMES;) {
        int n=FRAMES-at;if(n>block)n=block;engine_render(out+2*at,n);at+=n;assert(engine_active_voices()==1);
    }
    assert(starts==1 && heard_velocity==fminf(v,1));
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
int main(void) {
    int clamps=0,adapters=0,worlds=0,invalids=0;
    for(int w=0;w<3;++w) {
        capture(w,1,0,512,reference);capture(w,2,0,64,candidate);
        assert(!memcmp(reference,candidate,sizeof reference));++clamps;
        for(int i=0;i<2;++i) {
            float v=i ? 1 : .5f;
            capture(w,v,0,512,reference);capture(w,v,2,64,candidate);
            assert(!memcmp(reference,candidate,sizeof reference));++adapters;
            capture(w,v,1,64,candidate);assert(!memcmp(reference,candidate,sizeof reference));++worlds;
        }
        const float invalid[]={0,-1,NAN,INFINITY,-INFINITY};
        for(unsigned i=0;i<sizeof invalid/sizeof *invalid;++i) {
            engine_init();engine_set_world(w);assert(!engine_try_note_on(0,220,invalid[i]));engine_render(buffer,512);
            assert(!engine_active_voices());for(int j=0;j<1024;++j)assert(!buffer[j]);++invalids;
        }
    }
    printf("GAIN PASS: %d ceiling, %d legacy adapter, %d manual/World cases preserve full4s PCM across64/512; %d invalid inputs reject silently\n",clamps,adapters,worlds,invalids);
}
