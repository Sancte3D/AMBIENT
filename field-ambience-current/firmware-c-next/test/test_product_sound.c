/* Tests actual PCM from the reduced core, not a surrogate score renderer. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include "tuning.h"
#include "ambient_room.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
enum { SR=44100,N=SR*4 };
static int16_t reference[N*2],trial[N*2],buffer[2048*2];
static int ons,offs;
static void hook(int on,uint8_t owner,float hz,float v) {
    (void)owner; (void)v;
    if(on>0) {
        ++ons;
        bool phrase=engine_product_world()==WORLD_COAST && (owner==6 || owner==7 || owner==15);
        assert(hz>=(phrase ? 105 : 140) && hz<=(phrase ? 850 : 470));
    }
    if(on==0) { ++offs; assert(hz>0); }
}
static void audio(double seconds) {
    int frames=(int)(seconds*SR);
    while(frames>0) { int n=frames>512 ? 512 : frames; engine_render(buffer,n); frames-=n; }
    (void)engine_active_voices();
}
static void configure(int w) {
    engine_init(); engine_set_world(w); engine_set_note_hook(hook);
    ons=offs=0;
}
static void captured(int w,int block,bool invalid,int16_t *out) {
    configure(w); engine_set_color(.63f); engine_set_room(.78f);
    assert(engine_try_note_on(0,dsp_midi_to_hz(62),.8f));
    for(int at=0;at<N;) {
        if(at==SR) engine_note_off(0);
        int n=block;
        if(n>N-at) n=N-at;
        if(at<SR && n>SR-at) n=SR-at;
        if(invalid) {
            float bad[3]={NAN,INFINITY,-INFINITY};
            for(int i=0;i<3;++i) {
                engine_set_master_volume(bad[i]); engine_set_color(bad[i]);
                engine_set_room(bad[i]); engine_set_nature(bad[i]);
                engine_set_activity(bad[i]); engine_set_attack(bad[i]); engine_set_release(bad[i]);
                bowed_set_tone(bad[i]); horn_set_tone(bad[i]); pluck_set_damp(bad[i]);
                assert(!engine_try_note_on(3,bad[i],.5f));
                assert(!engine_try_note_on(3,220,bad[i]));
            }
        }
        engine_render(out+2*at,n); at+=n;
    }
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
}
static void one_world(int w,uint32_t seed,int duration) {
    configure(w); engine_set_gen_seed(seed); engine_set_generative(true,-1);
    uint32_t frames=0; double energy=0; int peak=0;
    while(frames<(uint32_t)(duration*SR)) {
        engine_generative_tick((uint32_t)((uint64_t)frames*1000/SR));
        assert(engine_ambient_source_count()<=3);
        engine_render(buffer,512);
        for(int i=0;i<1024;++i) { int a=abs(buffer[i]); if(a>peak) peak=a; energy+=(double)buffer[i]*buffer[i]; }
        frames+=512;
    }
    engine_generative_tick((uint32_t)((uint64_t)frames*1000/SR));
    assert(engine_generative_melody_count()>=3);
    if(w==WORLD_HIGHLANDS) assert(engine_admission_rejections()<15);
    assert(peak>100 && peak<24576 && energy>100000);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    printf("PRODUCT world=%d seed=%u duration=%d notes=%d answers=%d rejects=%u rms=%.7f peak=%.7f\n",
       w,seed,duration,engine_generative_melody_count(),engine_generative_dejavu_count(),
       engine_admission_rejections(),sqrt(energy/(2.0*frames))/32768.0,(double)peak/32768);
    engine_set_generative(false,-1); audio(20);
    assert(engine_active_voices()==0);
}
int main(void) {
    configure(0); audio(.1); for(int i=0;i<1024;++i) assert(buffer[i]==0);
    assert(!engine_try_note_on(0,20,.8f)); assert(!engine_try_note_on(0,880,.8f));
    assert(!engine_try_note_on(0,dsp_midi_to_hz(51),.8f)); /* valid Hz, outside D core */
    assert(!engine_try_note_on(16,220,.8f));
    assert(!engine_try_note_on(5,220,.8f) && !engine_try_note_on(8,220,.8f) && !engine_try_note_on(6,220,.8f));
    assert(engine_try_note_on(0,dsp_midi_to_hz(50),.7f));
    assert(engine_try_note_on(1,dsp_midi_to_hz(57),.7f));
    assert(engine_try_note_on(2,dsp_midi_to_hz(62),.7f));
    assert(engine_active_voices()==3 && ons==0); /* prepared, not yet heard */
    assert(!engine_try_note_on(3,dsp_midi_to_hz(66),.7f));
    audio(.02); assert(ons==3);
    engine_note_off(1); assert(offs==1 && engine_active_voices()==3);
    assert(!engine_try_note_on(1,220,.7f)); /* released source is occupied */
    audio(13); assert(engine_active_voices()==2);
    assert(engine_try_note_on(1,220,.7f));
    engine_set_world(1); /* original Bowed owners remain through release */
    engine_note_off(0); engine_note_off(1); engine_note_off(2);
    audio(14); assert(engine_active_voices()==0);
    assert(engine_try_note_on(0,220,.7f)); assert(engine_try_note_on(1,330,.7f));
    assert(!engine_try_note_on(2,440,.7f)); /* local Pluck limit two */
    audio(.02); engine_all_off(); audio(.1); assert(!engine_clear_pending());
    audio(1); for(int i=0;i<1024;++i) assert(buffer[i]==0);
    assert(engine_active_voices()==0);

    configure(0); assert(engine_try_note_on(0,220,.7f)); engine_note_off(0); audio(.02);
    assert(ons==0 && offs==0 && engine_active_voices()==0); /* cancelled preparation */
    configure(1); engine_set_generative(true,-1); engine_generative_tick(0);
    assert(engine_generative_melody_count()==0);
    engine_set_autoplay_melody(0); audio(.02); engine_generative_tick(20);
    assert(engine_generative_melody_count()==0 && ons==0);

    for(int w=0;w<3;++w) {
        captured(w,512,false,reference);
        captured(w,64,false,trial);
        int max_delta=0;
        for(int i=0;i<2*N;++i) { int d=abs((int)reference[i]-trial[i]); if(d>max_delta) max_delta=d; }
        assert(max_delta<=1); /* whole source/room/master, sample-based ramps */
        captured(w,512,true,trial);
        assert(!memcmp(reference,trial,sizeof reference));
        printf("PRODUCT trajectory world=%d block64-vs512 max_delta=%d invalid_input bit-identical\n",w,max_delta);
    }
    /* Equal->Just keeps held actual Hz and rejects the beating new version. */
    configure(0); engine_set_tuning(0); float equal=tuning_hz(54);
    assert(engine_try_note_on(0,equal,.8f)); audio(.02);
    engine_set_tuning(1); float just=tuning_hz(54);
    assert(fabsf(equal-just)>.1f && !engine_try_note_on(1,just,.8f));
    float hz[24]; int n=engine_sounding_frequencies(hz,24);
    assert(n>0 && hz[0]==equal);
    engine_all_off(); audio(.1);
    assert(engine_try_note_on(1,just,.8f)); audio(.02); /* Clear removed all memory */

    for(int w=0;w<3;++w) for(uint32_t seed=1;seed<=2;++seed) one_world(w,seed,120);
    assert(ambient_room_storage_bytes()<62000);
    printf("PRODUCT SOUND PASS: ownership, capacity, cancellation, real PCM, input invariance, tuning tails and 12 minutes rendered\n");
    return 0;
}
