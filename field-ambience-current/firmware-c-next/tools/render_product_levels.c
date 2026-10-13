/* SD34: actual Product admission/master; recipe tick,owner,world,midi,velocity,accept.
 * generate,world,seed selects the actual autonomous score. Manual Clear10s is
 * a bounded level probe, not a natural release measurement. */
#define _POSIX_C_SOURCE 200809L
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "tuning.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
enum { SR=44100,TICK=441,BLOCK=512,MAX_NOTES=4 };
typedef struct {int tick,owner,world,midi,accept;float velocity;} note_t;
static FILE *events;
static uint32_t at;
static int starts;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    fprintf(events,"%u,%d,%u,%.7f,%.8f,%d,%u\n",at,on,owner,(double)hz,(double)velocity,
        engine_generative_phrase_kind(),engine_generative_episode_count());
    if(on==1)++starts;
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
static int close_checked(FILE *f) {
    int e=ferror(f);if(fflush(f))e=1;if(fsync(fileno(f)))e=1;if(fclose(f))e=1;return e;
}
static float unit(char *s) {
    char *end;float x=strtof(s,&end);assert(!*end && isfinite(x) && x>=0 && x<=1);return x;
}
static int integer(char *s,int lo,int hi) {
    char *end;long x=strtol(s,&end,10);assert(!*end && x>=lo && x<=hi);return (int)x;
}
int main(int argc,char **argv) {
    if(argc!=10)return 2;
    int key=integer(argv[1],0,11),minor=integer(argv[2],0,1),block=integer(argv[7],1,BLOCK);
    float color=unit(argv[3]),room=unit(argv[4]),nature=unit(argv[5]),volume=unit(argv[6]);
    FILE *recipe=fopen(argv[8],"r");if(!recipe)return 2;
    char first[128];assert(fgets(first,sizeof first,recipe));rewind(recipe);
    int gen_world=0;unsigned gen_seed=0;
    bool generated=sscanf(first,"generate,%d,%u",&gen_world,&gen_seed)==2;
    assert(!generated || (gen_world>=0 && gen_world<3 && gen_seed>0));
    note_t notes[MAX_NOTES];int count=0,expected=0;
    if(!generated)for(;;) {
        note_t n;int fields=fscanf(recipe,"%d,%d,%d,%d,%f,%d",&n.tick,&n.owner,&n.world,&n.midi,&n.velocity,&n.accept);
        if(fields==EOF)break;
        assert(fields==6 && count<MAX_NOTES && n.tick>=0 && n.tick<1000 && n.owner>=0 && n.owner<5);
        assert(n.world>=0 && n.world<3 && n.midi>=50 && n.midi<=69 && isfinite(n.velocity));
        assert(n.accept==0 || n.accept==1);notes[count++]=n;expected+=n.accept;
    }
    fclose(recipe);assert(generated || (count && expected>0 && expected<=3));
    int seconds=generated ? 112 : 12;
    FILE *out=fopen(argv[9],"wb");if(!out)return 2;
    fwrite("RIFF",1,4,out);u32(out,36+(uint32_t)SR*seconds*4);fwrite("WAVEfmt ",1,8,out);u32(out,16);
    u16(out,1);u16(out,2);u32(out,SR);u32(out,SR*4);u16(out,4);u16(out,16);
    fwrite("data",1,4,out);u32(out,(uint32_t)SR*seconds*4);
    char path[1024];
    if(snprintf(path,sizeof path,"%s.events.csv",argv[9])>=(int)sizeof path)return 2;
    events=fopen(path,"w");if(!events)return 2;
    fprintf(events,"ack_frame,on,owner,hz,velocity,kind,completed_phrases\n");
    if(snprintf(path,sizeof path,"%s.state.csv",argv[9])>=(int)sizeof path)return 2;
    FILE *state=fopen(path,"w");if(!state)return 2;
    fprintf(state,"frame,active_sources,kind,completed_phrases,memory_returns,physical_source_mask,admission_rejections\n");
    engine_init();engine_set_world(generated ? gen_world : notes[0].world);
    engine_set_gen_seed(generated ? gen_seed : 1234);engine_set_key_pc(key);engine_set_mode(minor);
    engine_set_attack(.5f);engine_set_release(.5f);engine_set_color(color);
    engine_set_room(room);engine_set_fx_mode(room>0);engine_set_nature(0);engine_set_master_volume(volume);
    int16_t pcm[2*BLOCK];
    for(int frame=0;frame<SR;) {int n=SR-frame;if(n>BLOCK)n=BLOCK;engine_render(pcm,n);for(int i=0;i<2*n;++i)assert(!pcm[i]);frame+=n;}
    engine_set_note_hook(hook);engine_set_nature(nature);if(generated)engine_set_generative(true,-1);
    int maximum=0,peak=0,admitted=0;
    for(at=0;at<(uint32_t)SR*seconds;) {
        if(at%TICK==0) {
            int tick=(int)(at/TICK);
            if(generated && tick==8000)engine_set_generative(false,-1);
            engine_generative_tick((uint32_t)tick*10u);
            for(int i=0;i<count;++i)if(notes[i].tick==tick) {
                note_t *n=&notes[i];engine_set_world(n->world);
                bool ok=engine_try_note_on((uint8_t)n->owner,tuning_hz((float)n->midi),n->velocity);
                assert(ok==(n->accept!=0));admitted+=ok;
            }
            if(!generated && tick==1000)engine_all_off();
            int active=engine_active_voices();assert(active<=3 && pluck_active_count()<=2);
            if(active>maximum)maximum=active;
            fprintf(state,"%u,%d,%d,%u,%u,%u,%u\n",at,active,engine_generative_phrase_kind(),
                engine_generative_episode_count(),engine_generative_memory_return_count(),
                bowed_active_sources()|horn_active_sources()|pluck_active_sources(),engine_admission_rejections());
        }
        int n=(int)(((at/TICK+1)*TICK)-at);if(n>block)n=block;
        engine_render(pcm,n);
        for(int i=0;i<2*n;++i) {int a=abs((int)pcm[i]);if(a>peak)peak=a;if(at>=(uint32_t)SR*(seconds-1))assert(!pcm[i]);u16(out,(uint16_t)pcm[i]);}
        at+=(uint32_t)n;
    }
    assert(generated ? starts>=6 : (admitted==expected && starts==expected));
    assert(!engine_active_voices() && !engine_nonfinite_samples() && !engine_output_limited_samples());
    fprintf(stderr,"LEVELS starts=%d max_sources=%d nonfinite=0 limited=0 peak=%.8f generated=%d stop_or_clear_at=%d final_second_exact_zero=1\n",
        starts,maximum,(double)peak/32768,generated,generated ? 80 : 10);
    int error=close_checked(state);if(close_checked(events))error=1;if(close_checked(out))error=1;
    return error ? 2 : 0;
}
