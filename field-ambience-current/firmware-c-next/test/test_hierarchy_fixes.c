/* Regressions for the four measured hierarchy failures, through public DSP. */
#include "engine.h"
#include "engine_product.h"
#include "ambient_room.h"
#include "nature.h"
#include "tuning.h"
#include "dsp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,BLOCK=512,WEATHER_SECONDS=18 };
static float l[BLOCK],r[BLOCK],sl[BLOCK],sr[BLOCK];
static float reference[2*SR*WEATHER_SECONDS],governed[2*SR*WEATHER_SECONDS];
static float neutral[2*SR],colored[2*SR];
static int16_t pcm[882];
static void tick(unsigned ms) {engine_generative_tick(ms);engine_render(pcm,441);}
static void colors(int model,int color,float *out) {
    ambient_room_init();ambient_room_set(.5f);
    sound_parameters_t p=sound_parameters_default();
    p.delay=p.grain=p.drive=p.crush=p.phaser=0;p.plate=model!=0;
    if(color==1)p.drive=1;
    if(color==2)p.crush=1;
    if(color==3)p.phaser=1;
    assert(ambient_room_finish_apply(&p));
    for(unsigned f=0;f<4u*SR;) {
        unsigned n=4u*SR-f;if(n>BLOCK)n=BLOCK;
        for(unsigned i=0;i<n;++i) {
            sl[i]=.1f*dsp_sin((float)((f+i)%SR)*196/SR);
            sr[i]=.08f*dsp_sin((float)((f+i)%SR)*293.6648f/SR);l[i]=r[i]=0;
        }
        ambient_room_process(l,r,sl,sr,(int)n);
        for(unsigned i=0;i<n;++i)if(f+i>=3u*SR) {
            out[2*(f+i-3u*SR)]=l[i];out[2*(f+i-3u*SR)+1]=r[i];
        }
        f+=n;
    }
}
static void color_scope(void) {
    for(int model=0;model<2;++model) {
        colors(model,0,neutral);
        for(int color=1;color<=3;++color) {
            colors(model,color,colored);double difference=0,energy=0;
            for(unsigned i=0;i<2u*SR;++i) {
                double d=colored[i]-neutral[i];difference+=d*d;energy+=(double)neutral[i]*neutral[i];
            }
            assert(energy>1e-6 && difference>energy*.0001);
            printf("Room%d color%d relative RMS change %.6f\n",model,color,sqrt(difference/energy));
        }
    }
}
static void freeze_policy(void) {
    engine_init();assert(engine_try_note_on(0,tuning_hz(50),.5f));tick(0);
    assert(!engine_try_note_on(0,tuning_hz(57),.5f) && engine_note_block_reason()==ENGINE_NOTE_HELD_PITCH);
    engine_note_off(0);tick(10);
    assert(!engine_try_note_on(0,tuning_hz(57),.5f) && engine_note_block_reason()==ENGINE_NOTE_TAIL_PITCH);
    engine_init();sound_parameters_t p=sound_parameters_default(),q;
    p.grain=0;p.freeze=true;assert(engine_apply_sound_parameters(&p));
    engine_get_sound_parameters(&q);assert(!q.freeze);
    /* An empty positive-Grain Freeze must not invent a retained pitch. */
    p.grain=1;p.freeze=true;assert(engine_apply_sound_parameters(&p));
    for(unsigned ms=0;ms<1000;ms+=10)tick(ms);
    assert(!ambient_room_retains_history());
    /* Actual audible captured D3 still protects harmony after source death. */
    engine_init();p=sound_parameters_default();p.grain=.8f;
    assert(engine_apply_sound_parameters(&p));assert(engine_try_note_on(0,tuning_hz(50),.5f));
    for(unsigned ms=0;ms<2000;ms+=10)tick(ms);
    engine_note_off(0);p.freeze=true;assert(engine_apply_sound_parameters(&p));
    for(unsigned ms=2000;ms<26000;ms+=10)tick(ms);
    assert(!engine_active_voices() && ambient_room_retains_history());
    engine_set_key_pc(4);assert(!engine_try_note_on(1,tuning_hz(52),.5f));
    assert(engine_note_block_reason()==ENGINE_NOTE_FROZEN_PITCH);
    p.grain=0;assert(engine_apply_sound_parameters(&p));engine_get_sound_parameters(&q);assert(!q.freeze);
    for(unsigned ms=26000;ms<42000;ms+=10)tick(ms);
    assert(!ambient_room_retains_history());float hz[32];assert(!engine_sounding_frequencies(hz,32));
    assert(engine_try_note_on(1,tuning_hz(52),.5f));
    /* Reopening Grain restores fresh live input, never the discarded D3. */
    q.grain=1;assert(engine_apply_sound_parameters(&q));
    for(unsigned ms=42000;ms<44000;ms+=10)tick(ms);
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
    assert(engine_note_block_reason()==ENGINE_NOTE_OK);
    puts("Freeze: zero Grain disarms; empty capture has no pitch lock; audible capture rejects with reason; reopening after safe retirement uses fresh history");
}
static void weather(int world,int cap,int music,float *out) {
    nature_init();nature_set_world(world);nature_set_seed(1234);nature_set_amount(1);
    for(unsigned f=0;f<WEATHER_SECONDS*SR;) {
        unsigned n=WEATHER_SECONDS*SR-f;if(n>(unsigned)cap)n=(unsigned)cap;
        for(unsigned i=0;i<n;++i) {
            float carrier=(f+i>=2u*SR && f+i<10u*SR) ?
                .001f*dsp_sin((float)((f+i)%SR)*196/SR) : 0;
            l[i]=r[i]=music==2 ? carrier : 0;
            sl[i]=sr[i]=music==2 ? 0 : carrier;
        }
        if(music)nature_render_music(l,r,sl,sr,1,(int)n);else nature_render(l,r,(int)n);
        for(unsigned i=0;i<n;++i) {
            float carrier=music==2 && f+i>=2u*SR && f+i<10u*SR ?
                .001f*dsp_sin((float)((f+i)%SR)*196/SR) : 0;
            out[2*(f+i)]=l[i]-carrier;out[2*(f+i)+1]=r[i]-carrier;
        }
        f+=n;
    }
}
static void weather_hierarchy(void) {
    for(int world=0;world<3;++world)for(int music=1;music<=2;++music) {
        weather(world,512,0,reference);weather(world,512,music,governed);
        double solo=0,returned=0;
        for(unsigned i=16u*SR*2;i<WEATHER_SECONDS*SR*2;++i) {
            solo+=(double)reference[i]*reference[i];returned+=(double)governed[i]*governed[i];
        }
        assert(solo>1e-15 && returned>solo*.81 && returned<=solo*1.000001);
        for(unsigned second=6;second<10;++second) {
            double e=0;for(unsigned i=second*SR*2;i<(second+1)*SR*2;++i)e+=(double)governed[i]*governed[i];
            assert(e/(2*SR)<.001*.001*.5*.01); /* At least20dB below the ongoing source. */
        }
        weather(world,37,music,reference);assert(!memcmp(reference,governed,sizeof governed));
        printf("Nature%d foreground%d: quiet-note/held-FX hierarchy, recovery RMS %.6f of solo, complete18s float partition equality\n",world,music,sqrt(returned/solo));
    }
}
int main(void) {
    dsp_init();color_scope();freeze_policy();weather_hierarchy();
    puts("Hierarchy repairs: each color works on each room; safe Freeze lifecycle; quiet Nature and slow silent recovery pass");
}
