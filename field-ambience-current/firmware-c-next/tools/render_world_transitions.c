/* SD37 real Product worlds. mode:0 held,1 pending,2 releasing,3 batch,
 * 4 rapid with audio,5 full manual. Request/ack/physical families are traced. */
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "tuning.h"
#include "ambient_room.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
enum { SR=44100,TICK=441,BLOCK=512 };
static FILE *events,*actions;
static uint32_t at;
static int starts,offs,last_family[16];
static uint16_t family_mask(int family) {return family==0 ? bowed_active_sources() : family==1 ? pluck_active_sources() : horn_active_sources();}
static uint16_t mask(void) {return family_mask(0)|family_mask(1)|family_mask(2);}
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(on==0 || on==1);int family=last_family[owner];
    if(on) {
        int matches=0;for(int i=0;i<3;++i)if(family_mask(i)&(1u<<owner)) {family=i;++matches;}
        assert(matches==1);last_family[owner]=family;++starts;
        assert(hz>=105 && hz<=(family==0 ? 850 : family==1 ? 470 : 650));
    } else ++offs;
    fprintf(events,"%u,%d,%u,%.7f,%.8f,%d,%d,%d,%u\n",at,on,owner,(double)hz,(double)velocity,
        family,engine_product_world(),engine_generative_phrase_kind(),engine_generative_episode_count());
}
static void select_world(int world) {
    int on_before=starts;uint16_t before=mask();float wet=ambient_room_peak();
    engine_set_world(world);
    assert(starts==on_before && mask()==before && ambient_room_peak()==wet);
    fprintf(actions,"%u,world,%d,%u,%.9g\n",at,world,before,(double)wet);
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
static int integer(char *s,int lo,int hi) {char *end;long n=strtol(s,&end,10);assert(!*end && n>=lo && n<=hi);return (int)n;}
static int close_checked(FILE *f) {int e=ferror(f);if(fflush(f))e=1;if(fsync(fileno(f)))e=1;if(fclose(f))e=1;return e;}
int main(int argc,char **argv) {
    if(argc!=9)return 2;
    int from=integer(argv[1],0,2),to=integer(argv[2],0,2),mode=integer(argv[3],0,5),context=integer(argv[4],0,1);
    int block=integer(argv[6],1,BLOCK),wrap=integer(argv[7],0,1);char *end;float nature=strtof(argv[5],&end);
    uint32_t base=wrap ? 0xfffff000u : 0;
    assert(from!=to && !*end && isfinite(nature) && nature>=0 && nature<=1);
    int seconds=mode==5 ? 80 : 128,stop=mode==5 ? 4 : 96;
    FILE *out=fopen(argv[8],"wb");if(!out)return 2;
    fwrite("RIFF",1,4,out);u32(out,36+(uint32_t)SR*seconds*4);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);fwrite("data",1,4,out);u32(out,(uint32_t)SR*seconds*4);
    char path[1024];
    assert(snprintf(path,sizeof path,"%s.events.csv",argv[8])<(int)sizeof path);events=fopen(path,"w");if(!events)return 2;
    fprintf(events,"ack_frame,on,owner,hz,velocity,family,selected_world,kind,completed_phrases\n");
    assert(snprintf(path,sizeof path,"%s.actions.csv",argv[8])<(int)sizeof path);actions=fopen(path,"w");if(!actions)return 2;
    fprintf(actions,"frame,action,target,physical_mask,room_peak_last_block\n");
    assert(snprintf(path,sizeof path,"%s.state.csv",argv[8])<(int)sizeof path);FILE *state=fopen(path,"w");if(!state)return 2;
    fprintf(state,"frame,world,active_sources,kind,completed_phrases,memory_returns,rejections,before_mask,after_mask,bowed_mask,pluck_mask,horn_mask,room_peak_last_block,pitch_history_count\n");
    engine_init();engine_set_world(from);engine_set_gen_seed(context ? 42 : 1234);
    engine_set_key_pc(context ? 9 : 2);engine_set_mode(context);engine_set_tuning(context);
    engine_set_activity(.5f);engine_set_color(1);engine_set_attack(1);engine_set_release(1);
    engine_set_room(1);engine_set_master_volume(.6f);engine_set_nature(0);
    int16_t pcm[2*BLOCK];
    for(int frame=0;frame<SR;) {int n=SR-frame;if(n>block)n=block;engine_render(pcm,n);for(int i=0;i<2*n;++i)assert(!pcm[i]);frame+=n;}
    engine_set_note_hook(hook);engine_set_nature(nature);engine_generative_tick(base);
    if(mode!=5)engine_set_generative(true,-1);
    int maximum=0,peak=0;bool stopped=false,manual_incoming=false;
    for(at=0;at<(uint32_t)SR*seconds;) {
        if(at%TICK==0) {
            int tick=(int)(at/TICK);uint16_t before=mask();uint32_t ms=base+(uint32_t)tick*10u;
            if(mode==1 && tick==0) {engine_generative_tick(ms);assert(engine_active_voices()==(from==0 ? 2 : 1));select_world(to);}
            if(mode==2 && tick==270)engine_set_generative(false,-1);
            if(mode==2 && tick==271)select_world(to);
            if(mode==2 && tick==330)engine_set_generative(true,-1);
            if((mode==0 || mode==3 || mode==5) && tick==270) {
                int expected=from==1 ? 2 : 3;assert(engine_active_voices()==expected);
                if(mode==3) {select_world(3-from-to);select_world(from);}
                select_world(to);
                if(mode==5) {
                    manual_incoming=engine_try_note_on(3,tuning_hz(66),.75f);assert(manual_incoming==(from==1));
                }
            }
            if(mode==4 && tick>=270 && tick<280) {
                int target=tick==279 ? to : ((tick-270)%3==0 ? to : (tick-270)%3==1 ? 3-from-to : from);
                select_world(target);
            }
            if(mode==5 && tick==0) {const int notes[3]={50,57,62};int n=from==1 ? 2 : 3;
                for(int i=0;i<n;++i)assert(engine_try_note_on((uint8_t)i,tuning_hz((float)notes[i]),.75f));}
            if(mode==5 && tick==280)assert(!engine_try_note_on(4,tuning_hz(69),.75f));
            if(tick==stop*100) {
                if(mode==5) {for(int i=0;i<5;++i)engine_note_off((uint8_t)i);engine_set_nature(0);}
                else engine_set_generative(false,-1);
                stopped=true;
            }
            engine_generative_tick(ms);
            int active=engine_active_voices();if(active>maximum)maximum=active;
            uint16_t b=family_mask(0),p=family_mask(1),h=family_mask(2);
            assert(active<=3 && pluck_active_count()<=2 && !(b&p) && !(b&h) && !(p&h));
            float history[24];int count=engine_sounding_frequencies(history,24);
            fprintf(state,"%u,%d,%d,%d,%u,%u,%u,%u,%u,%u,%u,%u,%.9g,%d\n",at,engine_product_world(),active,
                engine_generative_phrase_kind(),engine_generative_episode_count(),engine_generative_memory_return_count(),
                engine_admission_rejections(),before,mask(),b,p,h,(double)ambient_room_peak(),count);
        }
        int n=(int)((at/TICK+1)*TICK-at);if(n>block)n=block;
        engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {int value=abs((int)pcm[i]);if(value>peak)peak=value;
            if(at>=(uint32_t)SR*(seconds-1))assert(!pcm[i]);
            u16(out,(uint16_t)pcm[i]);}
        at+=(uint32_t)n;
    }
    assert(stopped && !engine_active_voices() && !engine_nonfinite_samples() && !engine_output_limited_samples());
    assert(mode==5 ? starts==3 : starts>=8);
    assert(mode!=1 || starts==engine_generative_melody_count());
    fprintf(stderr,"WORLD from=%d to=%d mode=%d context=%d nature=%.3f starts=%d offs=%d max_sources=%d "
        "phrases=%u limited=0 nonfinite=0 peak=%.8f incoming=%d\n",from,to,mode,context,(double)nature,
        starts,offs,maximum,engine_generative_episode_count(),(double)peak/32768,manual_incoming);
    int error=close_checked(state);if(close_checked(events))error=1;
    if(close_checked(actions))error=1;
    if(close_checked(out))error=1;
    return error ? 2 : 0;
}
