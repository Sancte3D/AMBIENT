/* Real product chain. All exported files are exactly 27 s, no hidden bed.
 * Usage: render_product_preview WORLD SEED dry|world|score_dry|nature|color|shape|limits OUT.wav
 * Fixed engine amplitude/volume; listening matching is a separate constant
 * file gain and recorded in the manifest. No callback changes in audio ISR. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
enum { SR=44100,SECONDS=27,BLOCK=512 };
static FILE *trace;
static uint32_t at;
static void hook(int on,uint8_t owner,float hz,float v) {
    if(trace) fprintf(trace,"%u,%d,%u,%.7f,%.5f\n",at,on,owner,(double)hz,(double)v);
}
static void u16(FILE *f,uint16_t v) { fputc(v&255,f); fputc(v>>8,f); }
static void u32(FILE *f,uint32_t v) { u16(f,v&65535); u16(f,v>>16); }
static void header(FILE *f,int seconds) {
    uint32_t bytes=seconds*SR*4;
    fwrite("RIFF",1,4,f);u32(f,36+bytes);fwrite("WAVEfmt ",1,8,f);u32(f,16);
    u16(f,1);u16(f,2);u32(f,SR);u32(f,SR*4);u16(f,4);u16(f,16);
    fwrite("data",1,4,f);u32(f,bytes);
}
static void setup(int world) {
    engine_init(); engine_set_world(world); engine_set_master_volume(.6f);
    engine_set_color(.5f);engine_set_room(.5f);engine_set_activity(.5f);
    engine_set_attack(.5f);engine_set_release(.5f);engine_set_nature(0);
    engine_set_note_hook(hook);
}
static void dry_setup(int world) {
    setup(world);engine_set_fx_mode(0);
    int16_t silence[512*2];
    for(int i=0;i<9;++i)engine_render(silence,512);
    /* Complete the 40 ms Dry transition before exciting any source. */
}
int main(int argc,char **argv) {
    if(argc!=5) { fprintf(stderr,"WORLD SEED dry|world|score_dry|nature|color|shape|limits OUT.wav\n");return 2; }
    int world=atoi(argv[1]);uint32_t seed=(uint32_t)strtoul(argv[2],0,0);
    if(world<0||world>=CORE_WORLD_COUNT) return 2;
    bool dry=!strcmp(argv[3],"dry"),nature=!strcmp(argv[3],"nature");
    bool color=!strcmp(argv[3],"color"),shape=!strcmp(argv[3],"shape");
    bool score_dry=!strcmp(argv[3],"score_dry"),limits=!strcmp(argv[3],"limits");
    if(!dry && !nature && !color && !shape && !score_dry && !limits && strcmp(argv[3],"world"))return 2;
    if(limits && seed>7)return 2;
    int seconds=limits ? 8 : SECONDS;
    FILE *out=fopen(argv[4],"wb");if(!out)return 2; header(out,seconds);
    char path[1024];if(snprintf(path,sizeof path,"%s.events.csv",argv[4])>=(int)sizeof path)return 2;
    trace=fopen(path,"w");if(!trace)return 2;
    fprintf(trace,"frame,on,owner,hz,velocity\n");
    if(dry||nature||color||shape||score_dry)dry_setup(world);else setup(world);
    engine_set_gen_seed(seed);
    if(nature)engine_set_nature(.7f);
    if(!dry&&!nature&&!color&&!shape&&!limits)engine_set_generative(true,-1);
    if(limits) {
        engine_set_master_volume(1);engine_set_room(1);
        engine_set_color(seed&1 ? 1 : 0);engine_set_nature(seed&1 ? 1 : 0);
        engine_set_attack(seed&2 ? 1 : 0);engine_set_release(seed&4 ? 1 : 0);
        const int notes[3]={50,57,62};
        for(int i=0;i<(world==1?2:3);++i)
            assert(engine_try_note_on((uint8_t)i,dsp_midi_to_hz(notes[i]),1));
    }
    int segment=-1;bool released=false,cleared=false;
    static const int midi[3]={50,62,69};static int16_t pcm[BLOCK*2];
    for(at=0;at<(uint32_t)(SR*seconds);) {
        uint32_t boundary=(uint32_t)(SR*seconds);
        if(dry||color||shape) {
            int s=(int)(at/(SR*9));
            if(s!=segment) {
                dry_setup(world);segment=s;released=cleared=false;
                float v=.5f*(float)s;
                if(color)engine_set_color(v);
                if(shape) {engine_set_attack(v);engine_set_release(v);}
                /* Reach an endpoint before its note, not midway through a ramp. */
                int16_t settle[BLOCK*2];for(int i=0;i<100;++i)engine_render(settle,BLOCK);
                int note=dry ? midi[s] : 62;
                assert(engine_try_note_on(0,dsp_midi_to_hz((float)note),.75f));
            }
            uint32_t elapsed=at-(uint32_t)segment*SR*9;
            if(elapsed>=SR*3 && !released) {engine_note_off(0);released=true;}
            if(elapsed>=SR*35/4 && !cleared) {engine_all_off();cleared=true;}
            boundary=(uint32_t)(segment+1)*SR*9;
            if(!released && boundary>at+SR*3-elapsed) boundary=at+SR*3-elapsed;
            if(!cleared && boundary>at+SR*35/4-elapsed) boundary=at+SR*35/4-elapsed;
        }
        if(limits) {
            if(at==SR*2) {engine_set_color(1);engine_set_room(0);}
            if(at==SR*4) {engine_set_color(0);engine_set_room(1);}
            if(at==SR*5)for(int i=0;i<3;++i)engine_note_off((uint8_t)i);
            const uint32_t points[3]={SR*2,SR*4,SR*5};
            for(int i=0;i<3;++i)if(points[i]>at && points[i]<boundary)boundary=points[i];
        }
        engine_generative_tick((uint32_t)((uint64_t)at*1000/SR));
        int n=boundary-at>BLOCK ? BLOCK : (int)(boundary-at);
        engine_render(pcm,n);
        for(int i=0;i<n*2;++i) u16(out,(uint16_t)pcm[i]);
        at+=(uint32_t)n;
    }
    engine_generative_tick((uint32_t)(seconds*1000));
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fprintf(stderr,"PREVIEW world=%d mode=%s seed=%u frames=%d limited=%u nonfinite=%u\n",
        world,argv[3],seed,SR*seconds,engine_output_limited_samples(),engine_nonfinite_samples());
    fclose(trace);if(fclose(out))return 2;return 0;
}
