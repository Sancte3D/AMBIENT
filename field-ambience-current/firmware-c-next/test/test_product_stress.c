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
#include <stdlib.h>
enum { SR=44100,BLOCK=512 };
static int16_t pcm[BLOCK*2];
static uint32_t hook_now,last_on,longest_gap,onsets,peak;
static double energy,mono;static uint32_t measured;
static void hook(int on,uint8_t source,float hz,float velocity) {
    (void)source;(void)velocity;
    if(on>0) {
        bool phrase=(source==6 || source==7 || source==15);
        assert(isfinite(hz)&&hz>=(phrase ? 105 : 140)&&hz<=(phrase ? (engine_product_world()==WORLD_COAST ? 850 : engine_product_world()==WORLD_HIGHLANDS ? 650 : 470) : 470));
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
    /* All six directions also at a full manual pool, extreme release and
     * maximum room: held sources survive World selection, entry retires them,
     * shared tails remain represented and rapid targets keep only the latest. */
    for(int from=0;from<3;++from)for(int to=0;to<3;++to)if(from!=to) {
        engine_init();engine_set_world(from);engine_set_room(1);
        engine_set_attack(1);engine_set_release(1);engine_set_color(1);
        int n=from==WORLD_WOODLAND ? 2 : 3;const int notes[3]={50,57,62};
        for(int i=0;i<n;++i)assert(engine_try_note_on((uint8_t)i,dsp_midi_to_hz(notes[i]),1));
        audio(.6);assert(engine_active_voices()==n);
        float before[24],after[24];assert(engine_sounding_frequencies(before,24)==n);
        engine_set_world(to);assert(engine_active_voices()==n);
        assert(engine_sounding_frequencies(after,24)==n && !memcmp(before,after,(size_t)n*sizeof(float)));
        engine_set_generative(true,-1);audio(.12);assert(engine_active_voices()==0);
        assert(ambient_room_peak()>0 && engine_sounding_frequencies(after,24)>=n);
        engine_set_world((to+1)%3);engine_set_world(to);assert(engine_product_world()==to);
        engine_generative_tick(1000);audio(.05);
        assert(engine_active_voices()<=3);
        assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
        engine_all_off();audio(.1);assert(engine_active_voices()==0);
    }
    for(int from=0;from<3;++from)for(int to=0;to<3;++to)if(from!=to) {
        engine_init();engine_set_world(from);engine_set_room(1);
        engine_set_generative(true,-1);engine_generative_tick(0);
        audio(.08);assert(engine_active_voices()==(from==WORLD_COAST ? 2 : 1));
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

static void mute_contract(void) {
    for(int world=0;world<3;++world) {
        engine_init();engine_set_world(world);engine_set_room(1);
        assert(engine_try_note_on(0,220,1));audio(.5);
        assert(engine_active_voices()==1 && ambient_room_peak()>0);
        engine_set_muted(true);assert(engine_muted());audio(.06);
        assert(!engine_clear_pending() && engine_active_voices()==0);
        assert(ambient_room_peak()==0);
        for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
        assert(!engine_try_note_on(1,220,1));
        engine_set_world((world+1)%3);engine_set_gen_seed(321);
        engine_set_color(1);engine_set_room(1);audio(.2);
        for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
        engine_set_muted(false);audio(.5);
        assert(!engine_muted() && engine_active_voices()==0 && ambient_room_peak()==0);
        for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0); /* no old room revival */

        engine_init();engine_set_world(world);engine_set_nature(1);
        engine_set_generative(true,-1);engine_generative_tick(0);audio(.1);
        engine_generative_tick(100);int heard=engine_generative_melody_count();
        int initial=world==WORLD_COAST ? 2 : 1;
        assert(heard==initial);
        engine_set_muted(true);audio(.06);
        engine_generative_tick(20000);assert(engine_active_voices()==0);
        assert(engine_generative_melody_count()==heard);
        for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
        engine_set_nature(1);audio(.2); /* target edits do not defeat mute */
        for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);
        engine_set_muted(false);engine_generative_tick(21000);audio(.1);
        engine_generative_tick(21100);
        assert(engine_generative_melody_count()==heard+initial && engine_active_voices()>0);
        assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
        engine_all_off();audio(.1);

        engine_init();engine_set_world(world);engine_set_nature(1);audio(.2);
        engine_set_muted(true);audio(.005);engine_set_muted(false);
        audio(.06);assert(!engine_clear_pending());
        assert(engine_active_voices()==0 && ambient_room_peak()==0);
        /* Nature intentionally rises over 2 s and has irregular event gaps.
         * A fresh start need not cross PCM16 quantisation after only 200 ms. */
        int p=0;
        for(int frame=0;frame<SR*6;frame+=BLOCK) {
            engine_render(pcm,BLOCK);
            for(int i=0;i<BLOCK*2;++i)if(abs(pcm[i])>p)p=abs(pcm[i]);
        }
        assert(p>0 && engine_active_voices()==0 && ambient_room_peak()==0);
        engine_all_off();audio(.1);
    }
    puts("PRODUCT MUTE PASS: 40 ms exact zero, no tail revival, fresh Generate, protected targets and early unmute");
}


static void pluck_tap_contract(void) {
    engine_init();engine_set_world(WORLD_WOODLAND);engine_set_fx_mode(0);
    audio(.05);assert(engine_try_note_on(0,220,1));audio(.03);
    engine_note_off(0);audio(.1);
    assert(engine_active_voices()==1); /* short tap retains the actual string */
    int p=0;for(int i=0;i<BLOCK*2;++i)if(abs(pcm[i])>p)p=abs(pcm[i]);
    assert(p>100); /* audible tonal release, not a 20 ms killed impulse */
    assert(!engine_try_note_on(0,220,1)); /* owner still occupies its tail */
    audio(70);assert(engine_active_voices()==0); /* longer natural string */

    engine_init();engine_set_world(WORLD_WOODLAND);
    assert(engine_try_note_on(0,220,1));engine_note_off(0);audio(.1);
    assert(engine_active_voices()==0); /* pre-DSP cancellation remains silent */
    for(int i=0;i<BLOCK*2;++i)assert(pcm[i]==0);

    engine_init();engine_set_world(WORLD_WOODLAND);
    engine_set_generative(true,-1);engine_generative_tick(0);audio(.1);
    engine_set_generative(false,-1);audio(.1);
    assert(engine_active_voices()==1); /* musical Stop preserves the one-shot */
    engine_all_off();audio(.1);assert(engine_active_voices()==0);
    puts("PRODUCT PLUCK TAP PASS: natural key-up/Stop, owned tail, pre-start cancel and Clear");
}


static void pitch_context_contract(void) {
    for(int world=0;world<3;++world)for(int operation=0;operation<3;++operation) {
        engine_init();engine_set_world(world);onsets=0;engine_set_note_hook(hook);
        engine_set_generative(true,-1);engine_generative_tick(0);
        if(operation==0)engine_set_key_pc(5);
        else if(operation==1)engine_set_mode(1);
        else engine_set_tuning(1);
        audio(.1);
        assert(engine_active_voices()==0 && onsets==0 && engine_generative_melody_count()==0);
        hook_now=100;engine_generative_tick(100);audio(.1);engine_generative_tick(200);
        int initial=world==WORLD_COAST ? 2 : 1;
        assert(engine_active_voices()==initial && onsets==(uint32_t)initial && engine_generative_melody_count()==initial);
        float hz[24];assert(engine_sounding_frequencies(hz,24)==initial);
        float actual[2]={hz[0],hz[initial-1]};
        /* Reapplying the same settings is not another reset or pending loss. */
        if(operation==0)engine_set_key_pc(5);
        else if(operation==1)engine_set_mode(1);
        else engine_set_tuning(1);
        audio(.02);assert(engine_sounding_frequencies(hz,24)==initial && hz[0]==actual[0] && hz[initial-1]==actual[1]);
        engine_all_off();audio(.1);
    }
    puts("PRODUCT PITCH CONTEXT PASS: prepared cancellation, fresh-context start, actual-Hz hold and idempotent targets");
}

int main(void) {
    pitch_context_contract();
    pluck_tap_contract();
    mute_contract();
    transitions();
    for(int w=0;w<3;++w)for(int c=0;c<8;++c)source_limits(w,c);
    for(int w=0;w<3;++w)for(int c=0;c<4;++c)long_case(w,c);
    puts("PRODUCT STRESS PASS: six directed Worlds, pending cancellation, bounded entry, retained limits, wrap and 36 minutes PCM");
    return 0;
}
