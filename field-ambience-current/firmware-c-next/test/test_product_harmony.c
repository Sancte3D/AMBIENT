/* Public Product harmony contract over all current keys/collections/tunings.
 * No private admit/compatible call and no DSP replacement. Rejected pairs
 * must preserve the entire continuation of an already playing real source. */
#include "engine.h"
#include "engine_product.h"
#include "tuning.h"
#include "dsp.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { BLOCK=512,FOLLOW=2048 };
static int16_t pcm[FOLLOW*2],control[FOLLOW*2];
static int world,key,minor,just,world_api,ons;
static float heard[3];
static unsigned single_cases,pair_cases,accepted_singles,accepted_pairs,rejected_pairs;
static uint64_t unchanged_frames;
static double min_hz=1e9,max_hz;
static const double ratios[12]={1,16.0/15,9.0/8,6.0/5,5.0/4,4.0/3,
                               45.0/32,3.0/2,8.0/5,5.0/3,9.0/5,15.0/8};
static double expected_hz(int midi) {
    if(!just)return 440.0*exp2((midi-69)/12.0);
    int root=50+(key+10)%12;
    int octave=(int)floor((midi-root)/12.0);
    int degree=midi-root-12*octave;
    return 440.0*exp2((root-69)/12.0)*ldexp(ratios[degree],octave);
}
static bool core(int midi) {
    unsigned pitch=(unsigned)((midi-key+120)%12);
    return ((minor ? 0x4a9u : 0x295u)>>pitch)&1u;
}
static bool manual_pitch(int midi,double hz) {
    return midi>=50&&midi<=69&&core(midi)&&hz>=140&&hz<=470;
}
/* Declarative forbidden bands from the Product spec, expanded across this
 * finite register. Deliberately do not reuse the production octave fold. */
static bool pair_allowed(double a,double b) {
    static const double forbidden[6]={100,600,1100,1300,1800,2300};
    double cents=1200.0*fabs(log2(a/b));
    if(cents<90)return false; /* live unison and alternate tuning of it */
    for(unsigned i=0;i<6;++i)if(fabs(cents-forbidden[i])<45)return false;
    double lower=fmin(a,b);
    if(lower<196&&cents<299.5)return false;
    if(lower<261.63&&cents<249.5)return false;
    return true;
}
static void hook(int on,uint8_t owner,float hz,float v) {
    (void)v;
    if(on<=0)return;
    assert(ons<3&&owner==(uint8_t)(world_api ? 6+ons : ons));
    heard[ons++]=hz;
}
static void setup(void) {
    engine_init();engine_set_world(world);engine_set_key_pc(key);
    engine_set_mode(minor);engine_set_tuning(just);
    engine_set_fx_mode(0);engine_set_room(0);engine_set_nature(0);
    engine_set_attack(0);engine_set_release(1);engine_set_color(1);
    engine_set_master_volume(.6f);engine_set_note_hook(hook);
    ons=0;memset(heard,0,sizeof heard);
    if(world_api)engine_set_generative(true,-1);
}
static bool start(int owner,float hz) {
    return world_api ? engine_try_world_note_on((uint8_t)(6+owner),hz,.75f) :
                       engine_try_note_on((uint8_t)owner,hz,.75f);
}
static void render(int frames,int16_t *out) {
    /* Engine itself handles a positive request exceeding the 512-frame half. */
    engine_render(out,frames);
    (void)engine_active_voices(); /* actual Main acknowledgement */
    assert(engine_nonfinite_samples()==0&&engine_output_limited_samples()==0);
}
static void exact_ledger(int count,float a,float b) {
    float hz[19];int n=engine_sounding_frequencies(hz,19);
    assert(n==count&&hz[0]==a);
    if(count==2)assert(hz[1]==b);
    assert(ons==count&&heard[0]==a);
    if(count==2)assert(heard[1]==b);
}
static void singles(void) {
    static const double offsets[5]={0,-2.9,2.9,-3.1,3.1};
    for(int midi=45;midi<=80;++midi)for(unsigned d=0;d<5;++d) {
        /* Broad chromatic/register scan; tolerance edges only at Core notes
         * of the public manual register, where the +/-3-cent contract applies. */
        if(d&&!(midi>=50&&midi<=69&&core(midi)))continue;
        setup();
        float applied=tuning_hz((float)midi);
        assert(fabs(1200*log2(applied/expected_hz(midi)))<.003);
        float candidate=(float)(applied*exp2(offsets[d]/1200));
        bool expected=manual_pitch(midi,candidate)&&fabs(offsets[d])<3;
        assert(start(0,candidate)==expected);++single_cases;
        render(FOLLOW,pcm);
        assert(engine_active_voices()==(expected ? 1 : 0));
        assert(ons==(expected ? 1 : 0));
        if(expected) {
            exact_ledger(1,candidate,0);++accepted_singles;
            if(candidate<min_hz)min_hz=candidate;
            if(candidate>max_hz)max_hz=candidate;
        } else {
            for(int i=0;i<FOLLOW*2;++i)assert(pcm[i]==0);
            assert(engine_generative_melody_count()==0);
        }
    }
}
static void pairs(void) {
    for(int first=50;first<=69;++first) {
        float a=(float)expected_hz(first);
        if(!manual_pitch(first,a))continue;
        /* Once per first note: the complete control continuation without any
         * rejected second request. Each trial starts from the same real core. */
        setup();assert(start(0,a));render(BLOCK,pcm);render(FOLLOW,control);
        for(int second=50;second<=69;++second) {
            float b=(float)expected_hz(second);
            if(!manual_pitch(second,b))continue;
            bool expected=pair_allowed(a,b);
            setup();assert(start(0,a));render(BLOCK,pcm);
            assert(start(1,b)==expected);++pair_cases;
            render(FOLLOW,pcm);
            exact_ledger(expected ? 2 : 1,a,b);
            if(expected)++accepted_pairs;
            else {
                ++rejected_pairs;
                assert(!memcmp(control,pcm,sizeof control));
                unchanged_frames+=FOLLOW;
            }
        }
    }
}
int main(void) {
    for(world=0;world<3;++world)for(key=0;key<12;++key)
        for(minor=0;minor<2;++minor)for(just=0;just<2;++just)
            for(world_api=0;world_api<2;++world_api) {singles();pairs();}
    assert(accepted_singles>0&&accepted_pairs>0&&rejected_pairs>0);
    printf("{\"worlds\":3,\"keys\":12,\"collections\":2,\"tunings\":2,\"public_apis\":2,"
           "\"single_cases\":%u,\"accepted_singles\":%u,\"pair_cases\":%u,"
           "\"accepted_pairs\":%u,\"rejected_pairs\":%u,"
           "\"unchanged_rejected_stereo_frames\":%llu,"
           "\"minimum_accepted_hz\":%.9g,\"maximum_accepted_hz\":%.9g}\n",
           single_cases,accepted_singles,pair_cases,accepted_pairs,rejected_pairs,
           (unsigned long long)unchanged_frames,min_hz,max_hz);
    return 0;
}
