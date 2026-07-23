/*
 * render_catalog.c — the SOUND CATALOG renderer (r19.49).
 *
 * One binary that renders ANY single sound source of the instrument in
 * ISOLATION, so every voice / bed / ambience layer / effect / world can be
 * auditioned on its own. This is the diagnostic backbone: if something is
 * broken or missing, you hear exactly which element.
 *
 *   render_catalog <category> <name> <out.wav> [secs]
 *
 * Categories & names:
 *   voice   pluck | glass | ember | bowed_opensea | bowed_fjords | horn
 *   bed     alps | opensea | fjords | moss | desert         (PADsynth pad per world)
 *   ambience alps | opensea | fjords | moss | desert        (wind/rain/waves/hum, no music)
 *   bass    root | deep
 *   drone   default
 *   fx      bypass|reverb|delay|chorus|tape|swell|shimmer|blur|dream
 *   world   alps | opensea | fjords | moss | desert         (full engine, generative)
 *
 * Extensible: add a name to a category's switch here + a line to the manifest
 * in render_catalog.sh. Deterministic.
 */
#include "engine.h"
#include "brain.h"
#include "tuning.h"
#include "worlds.h"
#include "dsp.h"
#include "pad.h"
#include "padsynth.h"
#include "pluck.h"
#include "glass.h"
#include "ember.h"
#include "bowed.h"
#include "horn.h"
#include "bass.h"
#include "drone.h"
#include "ambience.h"
#include "fx_master.h"
#include "reverb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SR    44100
#define BLOCK 256

static void pu32(FILE *f, uint32_t v){ for(int i=0;i<4;++i){fputc(v&0xff,f);v>>=8;} }
static void pu16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void hdr(FILE *f, uint32_t n){ uint32_t d=n*4u;
    fwrite("RIFF",1,4,f);pu32(f,36+d);fwrite("WAVE",1,4,f);fwrite("fmt ",1,4,f);
    pu32(f,16);pu16(f,1);pu16(f,2);pu32(f,SR);pu32(f,SR*4u);pu16(f,4);pu16(f,16);
    fwrite("data",1,4,f);pu32(f,d); }

static int16_t clip16(float x){ if(x>1.f)x=1.f; if(x<-1.f)x=-1.f; return (int16_t)(x*30000.f); }

static int world_by_name(const char *n){
    if(!strcmp(n,"alps"))return 0; if(!strcmp(n,"opensea"))return 1;
    if(!strcmp(n,"fjords"))return 2; if(!strcmp(n,"moss"))return 3;
    if(!strcmp(n,"desert"))return 4; return 0;
}

/* shared slow phrase (MIDI) used by the voice + fx renders, so they compare */
static const int  PHRASE[] = {50,57,62,66,69,62,64,57,72,69,62,57};
static const float PAMP[]  = {.5f,.42f,.55f,.5f,.48f,.5f,.46f,.44f,.4f,.5f,.52f,.45f};
#define PHRASE_N ((int)(sizeof(PHRASE)/sizeof(PHRASE[0])))

/* ------------------------------------------------------------------ voices */
enum { V_PLUCK, V_GLASS, V_EMBER, V_BOWED_OS, V_BOWED_FJ, V_HORN };

static void render_voice(int v, FILE *f, int secs){
    dsp_init();
    pluck_init(); glass_init(); ember_init(); bowed_init(); horn_init();
    if (v==V_BOWED_OS) bowed_set_colour(0);
    if (v==V_BOWED_FJ) bowed_set_colour(1);
    reverb_init(); reverb_set(0.82f, 0.38f);

    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t buf[BLOCK*2];
    const int NB=(int)(2.2f*SR/BLOCK);          /* new note ~every 2.2 s */
    int next=0,ni=0; uint32_t done=0,blk=0;
    while(done<total){
        memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);
        memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
        if((int)blk>=next && ni<PHRASE_N){
            float hz=dsp_midi_to_hz((float)PHRASE[ni]), a=PAMP[ni]; ni++; next+=NB;
            switch(v){
              case V_PLUCK: pluck_note(hz,a); break;
              case V_GLASS: glass_note(hz,a); break;
              case V_EMBER: ember_note(hz,a); break;
              case V_BOWED_OS: case V_BOWED_FJ: bowed_note(hz,a); break;
              case V_HORN: horn_note(hz,a); break;
            }
        }
        switch(v){
          case V_PLUCK: pluck_render_mix(dL,dR,sL,sR,BLOCK); break;
          case V_GLASS: glass_render_mix(dL,dR,sL,sR,BLOCK); break;
          case V_EMBER: ember_render_mix(dL,dR,sL,sR,BLOCK); break;
          case V_BOWED_OS: case V_BOWED_FJ: bowed_render_mix(dL,dR,sL,sR,BLOCK,0.55f); break;
          case V_HORN: horn_render_mix(dL,dR,sL,sR,BLOCK,0.5f); break;
        }
        reverb_render(sL,sR,wL,wR,BLOCK);
        for(int n=0;n<BLOCK;++n){ buf[n*2]=clip16(dL[n]+wL[n]*0.6f); buf[n*2+1]=clip16(dR[n]+wR[n]*0.6f); }
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK; blk++;
    }
}

/* --------------------------------------------------------------------- bed */
static void render_bed(int wi, FILE *f, int secs){
    const world_t *w=worlds_get(wi);
    dsp_init(); brain_init(); pad_init(); padsynth_build(wi,0);
    brain_set_key(w->key_midi); brain_set_mode(w->mode);
    tuning_set_key(w->key_midi);
    pad_set_brightness((float)w->brightness_hz); pad_set_motion(1.0f);
    reverb_init(); reverb_set(0.85f,0.35f);

    int chord[4]; int nc=brain_color_chord(0,w->chord_color,chord,4);
    for(int i=0;i<nc&&i<3;++i) pad_note_on((uint8_t)(1+i), tuning_hz((float)chord[i]), 0.5f);

    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t buf[BLOCK*2]; uint32_t done=0; int released=0;
    while(done<total){
        if(!released && done>(uint32_t)((secs-6)*SR)){ released=1; pad_all_off(); }
        memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);
        memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
        pad_render_mix(dL,dR,sL,sR,BLOCK,0.5f);
        reverb_render(sL,sR,wL,wR,BLOCK);
        for(int n=0;n<BLOCK;++n){ buf[n*2]=clip16(dL[n]+wL[n]*0.6f); buf[n*2+1]=clip16(dR[n]+wR[n]*0.6f); }
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK;
    }
}

/* ---------------------------------------------------------------- ambience */
static void render_ambience(int wi, FILE *f, int secs){
    dsp_init(); ambience_init(); ambience_set_world(wi); ambience_set_level(1.0f);
    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK]; int16_t buf[BLOCK*2]; uint32_t done=0;
    while(done<total){
        memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);
        memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
        ambience_render_mix(dL,dR,sL,sR,BLOCK,0.4f);
        for(int n=0;n<BLOCK;++n){ buf[n*2]=clip16(dL[n]); buf[n*2+1]=clip16(dR[n]); }
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK;
    }
}

/* -------------------------------------------------------------------- bass */
static void render_bass(int deep, FILE *f, int secs){
    dsp_init(); bass_init(); bass_set_depth(deep?0.9f:0.6f);
    reverb_init(); reverb_set(0.7f,0.5f);
    const int roots[]={36,43,36,41}; int nr=4;
    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t buf[BLOCK*2]; const int NB=(int)(4.0f*SR/BLOCK); int next=0,ni=0; uint32_t done=0,blk=0;
    while(done<total){
        memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
        if((int)blk>=next && ni<nr){ bass_note(dsp_midi_to_hz((float)roots[ni])); ni++; next+=NB; }
        bass_render_mix(dL,dR,sL,sR,BLOCK);
        reverb_render(sL,sR,wL,wR,BLOCK);
        for(int n=0;n<BLOCK;++n){ buf[n*2]=clip16(dL[n]+wL[n]*0.4f); buf[n*2+1]=clip16(dR[n]+wR[n]*0.4f); }
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK; blk++;
    }
}

/* ------------------------------------------------------------------- drone */
static void render_drone(FILE *f, int secs){
    dsp_init(); drone_init(); drone_set_root_midi(45); drone_enable(true);
    reverb_init(); reverb_set(0.88f,0.4f);
    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK],wL[BLOCK],wR[BLOCK];
    int16_t buf[BLOCK*2]; uint32_t done=0; int off=0;
    while(done<total){
        if(!off && done>(uint32_t)((secs-6)*SR)){ off=1; drone_enable(false); }
        memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
        drone_render_mix(dL,dR,sL,sR,BLOCK);
        reverb_render(sL,sR,wL,wR,BLOCK);
        /* the drone is a quiet background bed; audition makeup so it's clearly
         * hearable in isolation (×3). */
        for(int n=0;n<BLOCK;++n){ buf[n*2]=clip16((dL[n]+wL[n]*0.5f)*3.0f); buf[n*2+1]=clip16((dR[n]+wR[n]*0.5f)*3.0f); }
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK;
    }
}

/* ---------------------------------------------------------------------- fx */
static int fx_by_name(const char *n){
    const char *names[]={"bypass","reverb","delay","chorus","tape","swell","shimmer","blur","dream"};
    for(int i=0;i<9;++i) if(!strcmp(n,names[i])) return i; return 8;
}
/* Feed the shared phrase (glass voice) through one master-fx mode. */
static void render_fx(int mode, FILE *f, int secs){
    dsp_init(); glass_init(); fx_master_init(); fx_master_set_world(1); fx_master_set_mode(mode);
    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK]; int16_t buf[BLOCK*2];
    const int NB=(int)(2.2f*SR/BLOCK); int next=0,ni=0; uint32_t done=0,blk=0;
    while(done<total){
        memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
        if((int)blk>=next && ni<PHRASE_N){ glass_note(dsp_midi_to_hz((float)PHRASE[ni]),PAMP[ni]); ni++; next+=NB; }
        glass_render_mix(dL,dR,sL,sR,BLOCK);
        fx_master_process(dL,dR,BLOCK);          /* the effect chain, in place */
        for(int n=0;n<BLOCK;++n){ buf[n*2]=clip16(dL[n]); buf[n*2+1]=clip16(dR[n]); }
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK; blk++;
    }
}

/* ------------------------------------------------------------------- world */
static void render_world(int wi, FILE *f, int secs){
    const world_t *w=worlds_get(wi);
    dsp_init(); brain_init(); engine_init();
    engine_set_world(wi); engine_set_voice(w->voice); engine_set_brightness((float)w->brightness_hz);
    engine_set_space(w->space_pct/100.f); engine_set_shimmer(w->shimmer_pct/100.f);
    engine_set_atmosphere(w->atmos_pct/100.f); engine_set_motion(w->motion_pct/100.f);
    engine_set_age(w->age_pct/100.f); engine_set_echo(w->echo_pct/100.f); engine_set_blur(w->blur_pct/100.f);
    engine_set_generative(true,-1);
    uint32_t total=(uint32_t)secs*SR; hdr(f,total);
    int16_t buf[BLOCK*2]; uint32_t done=0,now=0;
    while(done<total){ engine_generative_tick(now); engine_render(buf,BLOCK);
        fwrite(buf,2,BLOCK*2,f); done+=BLOCK; now+=(uint32_t)(BLOCK*1000ull/SR); }
}

int main(int argc,char**argv){
    if(argc<4){ fprintf(stderr,"usage: %s <category> <name> <out.wav> [secs]\n",argv[0]); return 2; }
    const char *cat=argv[1],*name=argv[2],*out=argv[3];
    int secs = (argc>4)? atoi(argv[4]) : 0;
    FILE *f=fopen(out,"wb"); if(!f){ perror(out); return 1; }

    if(!strcmp(cat,"voice")){
        int v = !strcmp(name,"pluck")?V_PLUCK : !strcmp(name,"glass")?V_GLASS :
                !strcmp(name,"ember")?V_EMBER : !strcmp(name,"bowed_opensea")?V_BOWED_OS :
                !strcmp(name,"bowed_fjords")?V_BOWED_FJ : V_HORN;
        render_voice(v,f, secs?secs:30);
    } else if(!strcmp(cat,"bed")){        render_bed(world_by_name(name),f, secs?secs:24);
    } else if(!strcmp(cat,"ambience")){   render_ambience(world_by_name(name),f, secs?secs:24);
    } else if(!strcmp(cat,"bass")){       render_bass(!strcmp(name,"deep"),f, secs?secs:16);
    } else if(!strcmp(cat,"drone")){      render_drone(f, secs?secs:24);
    } else if(!strcmp(cat,"fx")){         render_fx(fx_by_name(name),f, secs?secs:26);
    } else if(!strcmp(cat,"world")){      render_world(world_by_name(name),f, secs?secs:40);
    } else { fprintf(stderr,"unknown category '%s'\n",cat); fclose(f); return 2; }

    fclose(f);
    printf("  %-9s %-14s -> %s\n",cat,name,out);
    return 0;
}
