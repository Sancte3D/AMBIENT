/* Actual Product engine and sources. A probe pauses Main and runs complete
 * audio halves; it never lets Main preempt an audio half or fabricates DSP
 * retirement. No pthread scheduling model or replacement source renderer. */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define FAM_SOURCE_HANDOFF_TEST
#ifndef ENGINE_IMPLEMENTATION
#define ENGINE_IMPLEMENTATION "../src/engine_product.c"
#endif
#include ENGINE_IMPLEMENTATION

static unsigned cases,checks,interruptions,points_seen;
static unsigned on_count[SOURCES],off_count[SOURCES];
static int armed=-1,probe_block,probe_halves;
static uint8_t probe_owner;
static int16_t pcm[1024];
#define CHECK(condition) do { ++checks; if(!(condition)) { \
    fprintf(stderr,"source handoff: case=%u line=%d: %s\n",cases,__LINE__,#condition);exit(1); \
} } while(0)

static void audio(int frames) {
    while(frames>0) {
        int n=frames>512 ? 512 : frames;
        engine_render(pcm,n);frames-=n;
        CHECK(engine_nonfinite_samples()==0);
    }
}
void engine_source_handoff_probe(unsigned point,uint8_t owner) {
    if(armed!=(int)point || owner!=probe_owner)return;
    armed=-1;points_seen|=1u<<point;
    for(int half=0;half<probe_halves;++half) {
        ++interruptions;audio(probe_block);
    }
}
static void arm(unsigned point,uint8_t owner,int block,int halves) {
    CHECK(armed==-1);armed=(int)point;probe_owner=owner;
    probe_block=block;probe_halves=halves;
}
static void hook(int on,uint8_t owner,float hz,float velocity) {
    (void)hz;(void)velocity;
    if(on>0)++on_count[owner];else if(on==0)++off_count[owner];
}
static bool generated(uint8_t owner) { return owner==6 || owner==7 || owner==15; }
static bool start(uint8_t owner,float velocity) {
    return generated(owner) ? engine_try_world_note_on(owner,dsp_midi_to_hz(50),velocity) :
        engine_try_note_on(owner,dsp_midi_to_hz(50),velocity);
}
static void fresh(int w,uint8_t owner) {
    CHECK(armed==-1);engine_init();engine_set_world(w);engine_set_note_hook(hook);
    memset(on_count,0,sizeof on_count);memset(off_count,0,sizeof off_count);
    if(generated(owner))engine_set_generative(true,-1);
}
static void context(uint8_t owner) {
    if(generated(owner))engine_set_gen_seed(engine_gen_seed()+1u);
    else engine_set_generative(true,-1);
}
static void manual_again(uint8_t owner) {
    if(!generated(owner))engine_set_generative(false,-1);
}
/* Leave a real old voice 32 samples short of its 100 ms context retirement.
 * An IRQ inside the next Main handover retires it. The resumed Main then
 * posts the old quick request, including when note_off's reap freed the slot. */
static void stale_quick(int w,uint8_t owner,unsigned point,int block,int halves) {
    fresh(w,owner);CHECK(start(owner,.8f));audio(512);
    CHECK(engine_active_voices()==1 && on_count[owner]==1);
    context(owner);audio(4410-32);CHECK(engine_active_voices()==1);
    manual_again(owner);arm(point,owner,block,halves);context(owner);
    CHECK(armed==-1);manual_again(owner);
    CHECK(engine_active_voices()==0);
    CHECK((atomic_load(&quick_mask)&(1u<<owner))!=0);
    CHECK((atomic_load(&audio_mask)&(1u<<owner))==0);
}
static void heard_once(uint8_t owner) {
    audio(512);CHECK(engine_active_voices()==1);
    CHECK(on_count[owner]==1 && off_count[owner]==0);
    audio(8192);CHECK(engine_active_voices()==1 && on_count[owner]==1);
    CHECK(engine_admission_rejections()==0);
}
static void reuse_quick(int old_world,int new_world,uint8_t owner,unsigned retire_point,
                        int admit_point,int block,int halves) {
    ++cases;stale_quick(old_world,owner,retire_point,block,halves);
    engine_set_world(new_world);
    memset(on_count,0,sizeof on_count);memset(off_count,0,sizeof off_count);
    if(admit_point>=0)arm((unsigned)admit_point,owner,block,halves);
    CHECK(start(owner,.8f));CHECK(armed==-1);
    heard_once(owner);
    CHECK((atomic_load(&quick_mask)&(1u<<owner))==0);
    CHECK((atomic_load(&release_mask)&(1u<<owner))==0);
}
/* A legal, very quiet string can naturally retire in its first DSP half.
 * Main's pre-first-sample note_off was paused just before its release store.
 * The old unacknowledged preparation must not cancel the following note. */
static void reuse_release(int new_world,uint8_t owner,int block,int halves) {
    ++cases;fresh(WORLD_WOODLAND,owner);CHECK(start(owner,.0001f));
    arm(2,owner,block,halves);engine_note_off(owner);CHECK(armed==-1);
    CHECK((atomic_load(&release_mask)&(1u<<owner))!=0);
    CHECK(engine_active_voices()==0 && on_count[owner]==0 && off_count[owner]==0);
    engine_set_world(new_world);CHECK(start(owner,.8f));heard_once(owner);
}
static void valid_cancel(int w,uint8_t owner,unsigned point,int block) {
    ++cases;fresh(w,owner);CHECK(start(owner,.8f));
    if(point==0 || point==1) {arm(point,owner,block,1);context(owner);}
    else {arm(point,owner,block,1);engine_note_off(owner);}
    CHECK(armed==-1);audio(512);
    if(point==0 || point==2) {
        /* Audio started before publication of the current stop. A valid
         * release still retires it, with exactly one actual On and Off. */
        audio(44100*8);CHECK(engine_active_voices()==0);
        CHECK(on_count[owner]==1 && off_count[owner]==1);
    } else {
        CHECK(engine_active_voices()==0);
        CHECK(on_count[owner]==0 && off_count[owner]==0);
        for(int i=0;i<1024;++i)CHECK(pcm[i]==0);
    }
}
static void other_owner_and_rejection(int w,bool quick,int admit_point) {
    uint8_t a=0,b=1;++cases;stale_quick(w,a,0,64,1);
    CHECK(engine_try_note_on(b,dsp_midi_to_hz(57),.8f));
    if(quick) {
        /* Real context handover posts B's current quick stop. */
        engine_set_generative(true,-1);engine_set_generative(false,-1);
    } else engine_note_off(b);
    uint32_t before=atomic_load(&release_mask),fast=atomic_load(&quick_mask);
    CHECK(!engine_try_note_on(b,dsp_midi_to_hz(57),.8f));
    CHECK(before==atomic_load(&release_mask) && fast==atomic_load(&quick_mask));
    CHECK(!engine_try_note_on(a,NAN,.8f));
    CHECK(before==atomic_load(&release_mask) && fast==atomic_load(&quick_mask));
    memset(on_count,0,sizeof on_count);memset(off_count,0,sizeof off_count);
    if(admit_point>=0)arm((unsigned)admit_point,a,64,1);
    CHECK(start(a,.8f));CHECK(armed==-1);audio(512);
    CHECK(engine_active_voices()==1 && on_count[a]==1 && on_count[b]==0);
    CHECK(off_count[b]==0);
}
static void epoch_wrap(int w,unsigned point) {
    uint8_t owner=0;++cases;fresh(w,owner);atomic_store(&audio_epoch,UINT32_MAX);
    arm(point,owner,64,1);CHECK(start(owner,.8f));CHECK(armed==-1);
    heard_once(owner);CHECK(atomic_load(&audio_epoch)<UINT32_MAX);
}
int main(int argc,char **argv) {
    if(argc==3 && strcmp(argv[1],"--quick-reuse")==0) {
        int w=atoi(argv[2]);CHECK(w>=0 && w<3);
        reuse_quick(w,w,0,0,-1,64,1);
        return 0;
    }
    CHECK(argc==1);
    const uint8_t owners[]={0,4,9,13,6,7,15};
    for(int old=0;old<3;++old)for(int next=0;next<3;++next)
        for(unsigned k=0;k<sizeof owners;++k)for(unsigned point=0;point<=1;++point)
            for(int block=64;block<=512;block*=8)for(int halves=1;halves<=3;halves+=2)
                reuse_quick(old,next,owners[k],point,-1,block,halves);
    for(int w=0;w<3;++w)for(unsigned point=4;point<=9;++point)
        for(int block=64;block<=512;block*=8)for(int halves=1;halves<=3;halves+=2)
            reuse_quick(w,w,0,0,(int)point,block,halves);
    for(int next=0;next<3;++next)for(int k=0;k<2;++k)
        for(int block=64;block<=512;block*=8)for(int halves=1;halves<=3;halves+=2)
            reuse_release(next,k ? 6 : 0,block,halves);
    for(int w=0;w<3;++w)for(unsigned point=0;point<=3;++point)
        for(int block=64;block<=512;block*=8)valid_cancel(w,0,point,block);
    for(int w=0;w<3;++w)for(int quick=0;quick<=1;++quick)
        for(int point=-1;point<=6;point+=6)other_owner_and_rejection(w,quick,point);
    for(int w=0;w<3;++w)for(unsigned point=7;point<=9;++point)epoch_wrap(w,point);
    CHECK(points_seen==1023u);
    printf("{\"cases\":%u,\"assertions\":%u,\"audio_half_preemptions\":%u,"
           "\"main_points_seen\":%u,\"new_state_bytes\":0}\n",cases,checks,interruptions,points_seen);
    return 0;
}
