/* Matched actual Product sources, no Nature and no substitute score/DSP.
 * manual: one admitted pitch, physical expression, release and real tail.
 * generate: actual World grammar and admitted event/source observations. */
#include "engine.h"
#include "engine_product.h"
#include "gesture.h"
#include "dsp.h"
#include "tuning.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
enum { SR=44100,TICK=441 };
static FILE *events,*states;
static uint32_t frame;
static unsigned ons,offs,maximum;
static void heard(int on,uint8_t owner,float hz,float velocity) {
    fprintf(events,"%u,%d,%u,%.7f,%.7f\n",frame,on,owner,(double)hz,(double)velocity);
    if(on>0)++ons;
    if(on==0)++offs;
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
int main(int argc,char **argv) {
    if(argc!=9)return 2;
    int world=atoi(argv[2]),midi=atoi(argv[3]),fx=atoi(argv[4]),cap=atoi(argv[6]);
    float color=strtof(argv[5],NULL);uint32_t seed=(uint32_t)strtoul(argv[7],NULL,10);
    bool generate=atoi(argv[8])!=0;int seconds=generate ? 112 : 18;
    if(world<0 || world>2 || midi<50 || midi>69 || fx<0 || fx>1 || cap<1 || cap>512 || !isfinite(color) || color<0 || color>1)return 2;
    FILE *out=fopen(argv[1],"wb");if(!out)return 2;
    char path[1024];snprintf(path,sizeof path,"%s.events.csv",argv[1]);events=fopen(path,"w");if(!events)return 2;
    snprintf(path,sizeof path,"%s.states.csv",argv[1]);states=fopen(path,"w");if(!states)return 2;
    fputs("frame,on,owner,hz,velocity\n",events);fputs("frame,sources\n",states);
    uint32_t bytes=(uint32_t)seconds*SR*4;
    fwrite("RIFF",1,4,out);u32(out,bytes+36);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);fwrite("data",1,4,out);u32(out,bytes);
    engine_init();gesture_init_owned(engine_gesture_note);engine_set_world(world);engine_set_gen_seed(seed);
    engine_set_color(color);engine_set_master_volume(.75f);engine_set_nature(0);engine_set_room(fx ? .5f : 0);
    engine_set_note_hook(heard);
    float hz=tuning_hz((float)midi);
    for(frame=0;frame<(uint32_t)seconds*SR;) {
        if(frame%TICK==0) {
            uint32_t ms=(uint32_t)((uint64_t)frame*1000/SR);
            if(generate) {
                if(ms==1000)engine_set_generative(true,-1);
                if(ms==81000)engine_set_generative(false,-1);
            } else {
                if(ms==1000) {assert(engine_try_note_on(0,hz,.7f));engine_set_cell_held(0,true);}
                if(ms==11000) {engine_set_cell_held(0,false);engine_note_off(0);}
            }
            engine_generative_tick(ms);
            unsigned count=(unsigned)engine_active_voices();if(count>maximum)maximum=count;
            assert(count<=(unsigned)(world==1 ? 2 : 3));fprintf(states,"%u,%u\n",frame,count);
        }
        int n=(int)((frame/TICK+1)*TICK-frame);if(n>cap)n=cap;
        int16_t pcm[1024];engine_render(pcm,n);
        for(int i=0;i<2*n;++i)u16(out,(uint16_t)pcm[i]);
        frame+=(uint32_t)n;
    }
    assert(ons>0 && offs>0 && !engine_nonfinite_samples() && !engine_output_limited_samples());
    if(!generate)assert(ons==1 && offs==1);
    int error=ferror(out)||ferror(events)||ferror(states);
    if(fflush(out)||fflush(events)||fflush(states))error=1;
    if(fclose(out))error=1;
    if(fclose(events))error=1;
    if(fclose(states))error=1;
    printf("world=%d note=%d fx=%d color=%.2f generate=%d starts=%u offs=%u maximum=%u\n",
           world,midi,fx,(double)color,generate,ons,offs,maximum);
    return error;
}
