/* Targeted retained-parameter limits, six directed Worlds, source handovers,
 * timer wrap and long PCM recovery. No user-facing long audio export. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include "ambient_room.h"
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <string.h>
enum { SR=44100,BLOCK=512 };
static int16_t pcm[BLOCK*2];
static uint32_t hook_now,last_on,longest_gap,onsets,peak;
static double energy,mono;static uint32_t measured;
static void hook(int on,uint8_t source,float hz,float velocity) {
    (void)source;(void)velocity;
    if(on>0) {
        assert(isfinite(hz)&&hz>=140&&hz<=470);
        uint32_t gap=hook_now-last_on;if(onsets && gap>longest_gap)longest_gap=gap;
        last_on=hook_now;++onsets;
    }
}
static void collect(int frames) {
    assert(engine_active_voices()<=3);
    for(int i=0;i<frames;++i) {
        int L=pcm[2*i],R=pcm[2*i+1];
        int a=L<0?-L:L,b=R<0?-R:R;
        if((uint32_t)a>peak)peak=(uint32_t)a;
        if((uint32_t)b>peak)peak=(uint32_t)b;
        energy+=(double)L*L+(double)R*R;mono+=(double)(L+R)*(L+R)*.5;
    }
    measured+=(uint32_t)frames;
}
static void audio(float seconds) {
    int left=(int)(seconds*SR);
    while(left>0) { int n=left>BLOCK?BLOCK:left;engine_render(pcm,n);left-=n; }
    (void)engine_active_voices();
}
static void source_limits(int world,int c) {
    engine_init();engine_set_world(world);engine_set_master_volume(1);
    engine_set_color(c&1 ? 1 : 0);engine_set_attack(c&2 ? 1 : 0);engine_set_release(c&4 ? 1 : 0);
    engine_set_room(1);engine_set_nature(c&1 ? 1 : 0);
    peak=measured=0;energy=mono=0;
    const int notes[3]={50,57,62};
    for(int i=0;i<(world==1?2:3);++i)assert(engine_try_note_on((uint8_t)i,dsp_midi_to_hz(notes[i]),1));
    for(int frame=0;frame<SR*8;frame+=BLOCK) {
        if(frame>=SR*2 && frame<SR*2+BLOCK) {engine_set_color(1);engine_set_room(0);}
        if(frame>=SR*4 && frame<SR*4+BLOCK) {engine_set_color(0);engine_set_room(1);}
        if(frame>=SR*5 && frame<SR*5+BLOCK)for(int i=0;i<3;++i)engine_note_off((uint8_t)i);
        engine_render(pcm,BLOCK);collect(BLOCK);
    }
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    assert(peak<16384); /* >6 dB sample headroom, true peak measured separately. */
    assert(mono/energy>.65);
    printf("LIMIT world=%d case=%d peak_dbfs=%.3f mono_ratio=%.5f\n",world,c,20*log10((double)peak/32768),mono/energy);
    engine_all_off();audio(.1);assert(!engine_clear_pending());
    for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
}
static void long_case(int world,int c) {
    engine_init();engine_set_world(world);engine_set_key_pc((world*5+c*3)%12);
    engine_set_mode((world+c)&1);engine_set_tuning(c&1);
    engine_set_gen_seed((uint32_t)(100+17*world+c));engine_set_activity(c&2 ? 1 : 0);
    engine_set_color(c&1 ? 1 : 0);engine_set_attack(c&2 ? 1 : 0);engine_set_release(1);
    engine_set_room(1);engine_set_nature(c&1 ? 1 : 0);engine_set_master_volume(1);
    engine_set_note_hook(hook);engine_set_generative(true,-1);
    onsets=longest_gap=last_on=peak=measured=0;energy=mono=0;
    for(uint32_t frame=0;frame<SR*180;frame+=BLOCK) {
        hook_now=(uint32_t)((uint64_t)frame*1000/SR);
        engine_generative_tick(hook_now);engine_render(pcm,BLOCK);collect(BLOCK);
    }
    hook_now=180000;engine_generative_tick(hook_now);
    assert(onsets>=6 && longest_gap<65000);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0 && peak<16384);
    printf("LONG world=%d case=%d notes=%u returns=%u episodes=%u rejects=%u gap_ms=%u peak_dbfs=%.3f\n",
        world,c,onsets,engine_generative_return_count(),engine_generative_episode_count(),
        engine_admission_rejections(),longest_gap,20*log10((double)peak/32768));
    engine_set_generative(false,-1);audio(20);
    assert(engine_active_voices()==0);engine_all_off();audio(.1);
    for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
}
static void transitions(void) {
    for(int from=0;from<3;++from)for(int to=0;to<3;++to)if(from!=to) {
        engine_init();engine_set_world(from);engine_set_room(1);
        engine_set_generative(true,-1);engine_generative_tick(0);
        audio(.08);assert(engine_active_voices()==1);
        engine_set_world(to);assert(engine_product_world()==to);
        audio(.12);assert(engine_active_voices()==0);
        assert(ambient_room_peak()>0); /* No World room reset. */
        float old[24];assert(engine_sounding_frequencies(old,24)>0);
        engine_set_world((to+1)%3);engine_set_world(to); /* only newest target */
        assert(engine_product_world()==to && engine_active_voices()==0);
        engine_all_off();audio(.1);
    }
    for(int world=0;world<3;++world) {
        engine_init();engine_set_world(world);
        assert(engine_try_note_on(0,220,.9f));audio(.10);
        engine_set_generative(true,-1);audio(.12);
        assert(engine_active_voices()==0); /* bounded entry from manual */
        engine_set_generative(false,-1);engine_all_off();audio(.1);

        engine_init();engine_set_world(world);onsets=0;engine_set_note_hook(hook);
        engine_set_generative(true,-1);engine_generative_tick(0);
        engine_set_world((world+1)%3);audio(.12);
        assert(engine_active_voices()==0 && onsets==0); /* canceled before DSP */
        engine_all_off();audio(.1);
    }
    engine_init();uint32_t base=0xffffe000u;
    engine_generative_tick(base);engine_set_generative(true,-1);
    for(uint32_t frame=0;frame<SR*40;frame+=BLOCK) {
        uint32_t now=base+(uint32_t)((uint64_t)frame*1000/SR);
        engine_generative_tick(now);engine_render(pcm,BLOCK);
        assert(engine_active_voices()<=3);
    }
    assert(engine_generative_melody_count()>2);
    engine_all_off();audio(.1);
}
int main(void) {
    transitions();
    for(int w=0;w<3;++w)for(int c=0;c<8;++c)source_limits(w,c);
    for(int w=0;w<3;++w)for(int c=0;c<4;++c)long_case(w,c);
    puts("PRODUCT STRESS PASS: six directed Worlds, pending cancellation, bounded entry, retained limits, wrap and 36 minutes PCM");
    return 0;
}
