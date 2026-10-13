/* SD37: paired actual Product PCM for manual owners, FDN and latest Nature. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "ambient_room.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,FRAMES=4*SR,TICK=441 };
static int16_t reference[2*FRAMES],trial[2*FRAMES],buffer[1024];
static void configure(int from,float nature) {
    engine_init();engine_set_world(from);engine_set_gen_seed(1234);
    engine_set_room(1);engine_set_color(1);engine_set_attack(1);engine_set_release(1);
    engine_set_nature(0);engine_set_master_volume(.6f);
    for(int frame=0;frame<SR;) {int n=SR-frame;if(n>512)n=512;engine_render(buffer,n);frame+=n;}
    engine_set_nature(nature);
}
static void manual_capture(int from,int to,int mode,bool changed,int16_t *out) {
    configure(from,0);int count=from==1 ? 2 : 3;
    const int midi[3]={50,57,62};
    for(int i=0;i<count;++i)assert(engine_try_note_on((uint8_t)i,dsp_midi_to_hz((float)midi[i]),.75f));
    for(int at=0;at<FRAMES;) {
        if(at%TICK==0) {
            int tick=at/TICK;
            if(mode==1 && tick==80)for(int i=0;i<count;++i)engine_note_off((uint8_t)i);
            if(mode==2 && tick==60)engine_set_generative(true,-1);
            if(mode==2 && tick==70)assert(engine_active_voices()==0); /* exact100ms entry */
            if(mode==2 && tick==75)engine_set_generative(false,-1);
            if(tick==100) {
                float before[24],after[24];int n=engine_sounding_frequencies(before,24);
                if(mode==2)assert(!engine_active_voices() && n>0 && ambient_room_peak()>0);
                else assert(engine_active_voices()==count && n>=count);
                if(changed)engine_set_world(to);
                assert(engine_sounding_frequencies(after,24)==n && !memcmp(before,after,(size_t)n*sizeof *before));
            }
            if(mode==0 && tick==200)for(int i=0;i<count;++i)engine_note_off((uint8_t)i);
        }
        int n=(at/TICK+1)*TICK-at;if(n>512)n=512;engine_render(out+2*at,n);at+=n;
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
static void generated_capture(int from,bool same_value,int16_t *out) {
    configure(from,0);engine_set_generative(true,-1);
    for(int at=0;at<FRAMES;) {
        if(at%TICK==0) {if(same_value && at==SR)engine_set_world(from);engine_generative_tick((uint32_t)(at/TICK)*10u);}
        int n=(at/TICK+1)*TICK-at;if(n>512)n=512;engine_render(out+2*at,n);at+=n;
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
static void nature_capture(int from,int to,bool batch,int16_t *out) {
    configure(from,1);
    for(int at=0;at<FRAMES;) {
        if(at==SR) {if(batch) {engine_set_world(3-from-to);engine_set_world(from);}engine_set_world(to);}
        int n=SR-(at%SR);if(n>512)n=512;engine_render(out+2*at,n);at+=n;
        assert(!engine_active_voices());
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
int main(void) {
    int owners=0,room=0,nature=0;
    for(int from=0;from<3;++from)for(int to=0;to<3;++to)if(from!=to) {
        for(int mode=0;mode<3;++mode) {
            manual_capture(from,to,mode,false,reference);manual_capture(from,to,mode,true,trial);
            assert(!memcmp(reference,trial,sizeof reference));if(mode==2)++room;else ++owners;
        }
        nature_capture(from,to,false,reference);nature_capture(from,to,true,trial);
        assert(!memcmp(reference,trial,sizeof reference));++nature;
    }
    for(int world=0;world<3;++world) {
        generated_capture(world,false,reference);generated_capture(world,true,trial);
        assert(!memcmp(reference,trial,sizeof reference));
    }
    printf("WORLD CONTRACT PASS: %d held/releasing manual pairs, %d source-free live-room pairs, "
        "%d latest-Nature batches and3 generated idempotence pairs preserve whole4s PCM; "
        "all six directions, actual Hz/history retained, manual-to-Generate entry ends by100ms; noClear/Mute\n",owners,room,nature);
}
