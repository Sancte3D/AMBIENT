/* Long actual generative music: heard pitches, admission and source limits. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "gesture.h"
#include "tuning.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int world,key,minor;
static unsigned starts;
static void heard(int on,uint8_t owner,float hz,float velocity) {
    if(on<=0)return;
    assert(owner==6 || owner==7 || owner==15);
    assert(isfinite(hz) && hz>=105 && velocity>0 && velocity<=1);
    bool found=false;
    for(int m=45;m<=80;++m) {
        bool allowed=world==0 ? coast_phrase_pitch_allowed(m,key,minor!=0) :
            world==1 ? woodland_phrase_pitch_allowed(m,key,minor!=0) : highlands_phrase_pitch_allowed(m,key,minor!=0);
        if(allowed && fabsf(1200*log2f(hz/tuning_hz((float)m)))<.03f)found=true;
    }
    assert(found);++starts;
}
int main(void) {
    int16_t pcm[882];unsigned total=0;
    for(world=0;world<3;++world)for(int context=0;context<2;++context)for(int just=0;just<2;++just) {
        engine_init();gesture_init_owned(engine_gesture_note);engine_set_world(world);
        key=context ? 9 : 2;minor=context;engine_set_key_pc(key);engine_set_mode(minor);engine_set_tuning(just);
        engine_set_gen_seed(1234u+context);engine_set_room(1);engine_set_master_volume(1);
        engine_set_activity(context ? 0 : 1);engine_set_note_hook(heard);starts=0;
        engine_set_generative(true,-1);double energy=0;
        for(unsigned tick=0;tick<9000;++tick) {
            unsigned ms=tick*10;if(ms==80000)engine_set_generative(false,-1);
            if(ms==88000)engine_all_off();
            engine_generative_tick(ms);engine_render(pcm,441);
            assert(engine_active_voices()<=(world==1 ? 2 : 3));
            for(unsigned i=0;i<882;++i)energy+=(double)pcm[i]*pcm[i];
        }
        assert(starts>=3 && energy>1000 && !engine_active_voices());
        assert(!engine_nonfinite_samples() && !engine_output_limited_samples());total+=starts;
    }
    printf("Completion music: 18 minutes actual PCM, 12 World/key/collection/ET-JI cases, %u heard starts, registered Hz, long holds, bounded owners and zero output protection events pass\n",total);
}
