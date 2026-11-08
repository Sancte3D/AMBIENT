/* Unmodified real Product sources around the changed Room handoff. Raw PCM
 * and actual note hooks are internal before/after proofs, not hearing WAVs. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
enum { SR=44100, END=12*SR };
static uint32_t frame;
static FILE *events;
static void hook(int on,uint8_t owner,float hz,float v) {
    fprintf(events,"%u,%d,%u,%.9g,%.9g\n",frame,on,owner,(double)hz,(double)v);
}
static void start(int gen) {
    if(gen)engine_set_generative(true,-1);
    else {
        assert(engine_try_note_on(0,dsp_midi_to_hz(50),.75f));
        assert(engine_try_note_on(1,dsp_midi_to_hz(57),.75f));
        if(engine_product_world()!=1)assert(engine_try_note_on(2,dsp_midi_to_hz(62),.75f));
    }
}
int main(int argc,char **argv) {
    assert(argc==7);
    int w=atoi(argv[1]),gen=atoi(argv[2]),block=atoi(argv[3]),nature=atoi(argv[4]);
    FILE *out=fopen(argv[5],"wb");events=fopen(argv[6],"w");assert(out && events);
    engine_init();engine_set_world(w);engine_set_note_hook(hook);engine_set_gen_seed(1234);
    engine_set_master_volume(.6f);engine_set_color(1);engine_set_attack(1);engine_set_release(1);
    engine_set_nature((float)nature);start(gen);
    const uint32_t actions[]={22050,66150,88200,110250,132300,136710,154350,176400,198450,264600,441000,485100,END};
    unsigned action=0;uint32_t tick=0;int16_t pcm[1024];
    while(frame<END) {
        if(frame==actions[action]) {
            switch(action++) {
                case 0:engine_set_room(0);break;
                case 1:engine_set_room(1);break;
                case 2:engine_set_room(.2f);break;
                case 3:engine_set_room(.8f);break;
                case 4:engine_set_room(.8f);break;
                case 5:engine_set_room(NAN);engine_set_room(INFINITY);break;
                case 6:engine_set_room(0);break;
                case 7:engine_set_room(1);break;
                case 8:engine_all_off();break;
                case 9:engine_set_room(.8f);start(gen);break;
                case 10:engine_set_generative(false,-1);for(int i=0;i<3;++i)engine_note_off((uint8_t)i);break;
                case 11:engine_set_room(0);break;
            }
        }
        if(frame==tick) {engine_generative_tick(frame*1000u/SR);tick+=441;}
        int n=block;
        if((uint32_t)n>tick-frame)n=(int)(tick-frame);
        if((uint32_t)n>actions[action]-frame)n=(int)(actions[action]-frame);
        if((uint32_t)n>END-frame)n=(int)(END-frame);
        assert(n>0 && n<=512);engine_render(pcm,n);
        assert(fwrite(pcm,sizeof(int16_t)*2,(size_t)n,out)==(size_t)n);frame+=(uint32_t)n;
    }
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fclose(out);fclose(events);return 0;
}
