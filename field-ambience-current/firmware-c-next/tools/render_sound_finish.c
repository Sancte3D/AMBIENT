#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { SR=44100,SEGMENT=32,TICK=441 };
static FILE *trace;
static uint32_t frame;
static int world;
static unsigned starts;
static void event(int on,uint8_t owner,float hz,float velocity) {
    fprintf(trace,"%d,%u,%d,%u,%.7f,%.6f\n",world,frame,on,owner,(double)hz,(double)velocity);
    if(on>0)++starts;
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
int main(int argc,char **argv) {
    if(argc<3 || argc>4)return 2;
    bool neutral=argc==4;if(neutral && strcmp(argv[3],"neutral"))return 2;
    int cap=atoi(argv[2]);if(cap<1 || cap>512)return 2;
    FILE *out=fopen(argv[1],"wb");if(!out)return 2;
    char path[1024];snprintf(path,sizeof path,"%s.events.csv",argv[1]);trace=fopen(path,"w");if(!trace)return 2;
    fputs("world,frame,on,owner,hz,velocity\n",trace);
    uint32_t bytes=3u*SR*SEGMENT*4u;
    fwrite("RIFF",1,4,out);u32(out,36+bytes);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);fwrite("data",1,4,out);u32(out,bytes);
    for(world=0;world<3;++world) {
        engine_init();engine_set_world(world);engine_set_gen_seed(1234);
        engine_set_world_character(!neutral);engine_set_woodland_resonator(!neutral);
        engine_set_master_volume(.75f);engine_set_room(.5f);engine_set_nature(0);
        engine_set_note_hook(event);engine_set_generative(true,-1);starts=0;
        for(frame=0;frame<SR*SEGMENT;) {
            if(frame%TICK==0) {
                uint32_t ms=frame*1000u/SR;if(ms==24000)engine_set_generative(false,-1);
                engine_generative_tick(ms);assert(engine_active_voices()<=(world==1 ? 2 : 3));
            }
            int n=(int)((frame/TICK+1)*TICK-frame);if(n>cap)n=cap;
            int16_t pcm[1024];engine_render(pcm,n);
            for(int i=0;i<2*n;++i)u16(out,(uint16_t)pcm[i]);
            frame+=(uint32_t)n;
        }
        assert(starts>=2 && !engine_nonfinite_samples() && !engine_output_limited_samples());
    }
    int error=ferror(out)||ferror(trace);if(fclose(trace))error=1;if(fclose(out))error=1;return error;
}
