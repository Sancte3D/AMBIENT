/* Actual complete Product Engine. Identical physical Cell/Generate commands
 * with Nature0/positive; no external score, rain samples or post-mixed noise. */
#include "engine.h"
#include "engine_product.h"
#include "controls.h"
#include "cells.h"
#include "dsp.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
enum { SR=44100,TICK=441 };
static FILE *events;
static uint32_t frame;
static unsigned starts;
static void heard(int on,uint8_t owner,float hz,float velocity) {
    fprintf(events,"%u,%d,%u,%.7f,%.6f\n",frame,on,owner,(double)hz,(double)velocity);
    if(on>0)++starts;
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
int main(int argc,char **argv) {
    if(argc!=9)return 2;
    int world=atoi(argv[2]),mode=atoi(argv[6]),cap=atoi(argv[7]),seconds=atoi(argv[8]);
    uint32_t seed=(uint32_t)strtoul(argv[3],NULL,10);
    float amount=strtof(argv[4],NULL),room=strtof(argv[5],NULL);
    if(world<0 || world>2 || mode<0 || mode>2 || cap<1 || cap>512 || seconds<32 || seconds>180)return 2;
    if(!isfinite(amount) || amount<0 || amount>1 || !isfinite(room) || room<0 || room>1)return 2;
    FILE *out=fopen(argv[1],"wb");if(!out)return 2;
    char path[1024];snprintf(path,sizeof path,"%s.events.csv",argv[1]);events=fopen(path,"w");if(!events)return 2;
    fputs("frame,on,owner,hz,velocity\n",events);
    uint32_t bytes=(uint32_t)seconds*SR*4;
    fwrite("RIFF",1,4,out);u32(out,36+bytes);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);fwrite("data",1,4,out);u32(out,bytes);
    engine_init();controls_init();engine_set_world(world);engine_set_gen_seed(seed);
    engine_set_room(room);engine_set_nature(amount);engine_set_master_volume(.75f);engine_set_note_hook(heard);
    if(mode==2)engine_set_generative(true,-1);
    unsigned maximum=0;
    for(frame=0;frame<(uint32_t)seconds*SR;) {
        if(frame%TICK==0) {
            uint32_t ms=(uint32_t)((uint64_t)frame*1000/SR);
            if(mode==1) {
                if(ms==8000 || ms==18000) {
                    uint8_t cell=ms==8000 ? 0 : 2;
                    controls_cell_press(cell,CELL_AMP_MAX*.7f);controls_cell_physical(cell,true);
                }
                if(ms==30000)for(uint8_t cell=0;cell<=2;cell+=2) {
                    controls_cell_physical(cell,false);controls_cell_release(cell);
                }
            }
            if(mode==2 && ms==(uint32_t)(seconds-16)*1000)engine_set_generative(false,-1);
            engine_generative_tick(ms);
            unsigned count=(unsigned)engine_active_voices();if(count>maximum)maximum=count;
            assert(count<=(unsigned)(world==1 ? 2 : 3));
        }
        int n=(int)((frame/TICK+1)*TICK-frame);if(n>cap)n=cap;
        int16_t pcm[1024];engine_render(pcm,n);
        for(int i=0;i<2*n;++i)u16(out,(uint16_t)pcm[i]);
        frame+=(uint32_t)n;
    }
    assert((mode==0 && !starts && !maximum) || (mode==1 && starts==2) || (mode==2 && starts>=3));
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
    int error=ferror(out)||ferror(events);if(fflush(out))error=1;if(fflush(events))error=1;
    if(fclose(events))error=1;
    if(fclose(out))error=1;
    printf("world=%d mode=%d Nature=%.2f seed=%u seconds=%d starts=%u max_sources=%u\n",world,mode,(double)amount,seed,seconds,starts,maximum);
    return error;
}
