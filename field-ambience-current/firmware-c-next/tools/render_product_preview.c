/* Real product chain. All exported files are exactly 27 s, no hidden bed.
 * Usage: render_product_preview WORLD SEED MODE OUT.wav [START_SECONDS [TO_WORLD]]
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
#include <errno.h>
enum { SR=44100,SECONDS=27,BLOCK=512 };
static FILE *trace;
static uint32_t at;
static uint32_t window_start,window_end;
static unsigned onsets;
static void hook(int on,uint8_t owner,float hz,float v) {
    if(on>0)++onsets;
    if(trace && at>=window_start && at<window_end)
        fprintf(trace,"%u,%d,%u,%.7f,%.5f,%u\n",at-window_start,on,owner,
                (double)hz,(double)v,at);
}
static bool number(const char *text,uint32_t maximum,uint32_t *out) {
    if(!text || !*text || *text=='-' || *text=='+')return false;
    errno=0;char *end;unsigned long value=strtoul(text,&end,0);
    if(errno || *end || value>maximum)return false;
    *out=(uint32_t)value;return true;
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
    if(argc<5 || argc>7) {
        fprintf(stderr,"WORLD SEED dry|world|world_nature|activity_low|activity_high|score_dry|nature|color|shape|attack|release|tail|tail_dry|transition|limits OUT.wav [START_SECONDS [TO_WORLD]]\n");return 2;
    }
    uint32_t world_arg,seed,start=0,to=0;
    if(!number(argv[1],CORE_WORLD_COUNT-1,&world_arg) || !number(argv[2],UINT32_MAX,&seed) ||
       (argc>=6 && !number(argv[5],1800,&start)) ||
       (argc==7 && !number(argv[6],CORE_WORLD_COUNT-1,&to)))return 2;
    int world=(int)world_arg;
    bool dry=!strcmp(argv[3],"dry"),nature=!strcmp(argv[3],"nature");
    bool color=!strcmp(argv[3],"color"),shape=!strcmp(argv[3],"shape");
    bool attack=!strcmp(argv[3],"attack"),release=!strcmp(argv[3],"release");
    bool tail=!strcmp(argv[3],"tail"),tail_dry=!strcmp(argv[3],"tail_dry");
    bool transition=!strcmp(argv[3],"transition");
    bool macro=color||shape||attack||release;
    bool score_dry=!strcmp(argv[3],"score_dry"),limits=!strcmp(argv[3],"limits");
    bool world_nature=!strcmp(argv[3],"world_nature");
    bool activity_low=!strcmp(argv[3],"activity_low"),activity_high=!strcmp(argv[3],"activity_high");
    bool world_score=!strcmp(argv[3],"world")||world_nature||activity_low||activity_high;
    if(!dry && !nature && !macro && !tail && !tail_dry && !transition && !score_dry && !limits && !world_score)return 2;
    if(start && !(world_score || score_dry || nature))return 2;
    if(transition ? argc!=7 || to==world_arg : argc==7)return 2;
    if(limits && seed>31)return 2;
    int seconds=limits ? 8 : SECONDS;
    window_start=start*SR;window_end=window_start+(uint32_t)(seconds*SR);
    FILE *out=fopen(argv[4],"wb");if(!out)return 2; header(out,seconds);
    char path[1024];if(snprintf(path,sizeof path,"%s.events.csv",argv[4])>=(int)sizeof path)return 2;
    trace=fopen(path,"w");if(!trace)return 2;
    fprintf(trace,"frame,on,owner,hz,velocity,absolute_frame\n");
    if(dry||nature||macro||score_dry||tail_dry)dry_setup(world);else setup(world);
    engine_set_gen_seed(seed);
    if(nature)engine_set_nature(.7f);
    if(world_score||score_dry||transition)engine_set_generative(true,-1);
    if(world_nature)engine_set_nature(.7f);
    if(activity_low||activity_high)engine_set_activity(activity_high ? 1 : 0);
    if(transition) {engine_set_room(1);engine_set_release(1);}
    if(tail||tail_dry) {
        engine_set_room(tail ? 1 : 0);engine_set_release(1);
        assert(engine_try_note_on(0,dsp_midi_to_hz(62),.75f));
    }
    if(limits) {
        engine_set_master_volume(1);engine_set_room(1);
        engine_set_color(seed&1 ? 1 : 0);engine_set_nature(seed&1 ? 1 : 0);
        engine_set_attack(seed&2 ? 1 : 0);engine_set_release(seed&4 ? 1 : 0);
        /* Wide, low/dense, upper/dense and mixed-register summation. */
        const int banks[4][3]={{50,57,62},{50,54,57},{62,66,69},{57,62,66}};
        const int *notes=banks[seed/8];
        for(int i=0;i<(world==1?2:3);++i)
            assert(engine_try_note_on((uint8_t)i,dsp_midi_to_hz(notes[i]),1));
    }
    int segment=-1;bool released=false,cleared=false;
    static const int midi[3]={50,62,69};static int16_t pcm[BLOCK*2];
    unsigned written=0;
    for(at=0;at<window_end;) {
        uint32_t boundary=window_end;
        if(dry||macro) {
            int s=(int)(at/(SR*9));
            if(s!=segment) {
                dry_setup(world);segment=s;released=cleared=false;
                float v=.5f*(float)s;
                if(color)engine_set_color(v);
                if(shape) {engine_set_attack(v);engine_set_release(v);}
                if(attack)engine_set_attack(v);
                if(release)engine_set_release(v);
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
        if((tail||tail_dry) && !released) {
            if(at==SR) {engine_note_off(0);released=true;}
            else if(at<SR)boundary=SR;
        }
        if(transition) {
            if(at==SR*12)engine_set_world((int)to);
            if(at<SR*12 && boundary>SR*12)boundary=SR*12;
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
        /* An excerpt is an observation of the running stream. Never split a
         * render/tick merely because its output window starts in this block. */
        unsigned first=at<window_start ? window_start-at : 0;
        if(first<(unsigned)n) {
            for(unsigned i=first*2;i<(unsigned)n*2;++i)u16(out,(uint16_t)pcm[i]);
            written+=(unsigned)n-first;
        }
        at+=(uint32_t)n;
    }
    engine_generative_tick((uint32_t)((uint64_t)window_end*1000/SR));
    assert(written==(unsigned)(SR*seconds));
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fprintf(stderr,"PREVIEW world=%d mode=%s seed=%u frames=%u rendered_frames=%u start_seconds=%u onsets=%u returns=%u episodes=%u limited=%u nonfinite=%u\n",
        world,argv[3],seed,written,window_end,start,onsets,
        engine_generative_return_count(),engine_generative_episode_count(),
        engine_output_limited_samples(),engine_nonfinite_samples());
    bool failed=ferror(trace)||ferror(out);
    if(fclose(trace))failed=true;
    if(fclose(out))failed=true;
    return failed ? 2 : 0;
}
