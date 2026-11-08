/* Actual float room under deterministic Main -> audio preemption. The old
 * source is compiled with the same observation-only publication hooks. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#define FAM_ROOM_HANDOFF_TEST
#ifndef ROOM_IMPLEMENTATION
#define ROOM_IMPLEMENTATION "../src/ambient_room.c"
#endif
#include ROOM_IMPLEMENTATION
enum { WARM=8192, AFTER=8192, MAX_PAUSE=1536, CAP=2*(AFTER+MAX_PAUSE) };
static float expected[CAP],actual[CAP];
static unsigned checks,cases,interrupts;
static bool armed;
static unsigned chosen,caller_block,pause_frames;
static bool interleave_writer;
static unsigned read_point;
static float writer_amount;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"Room check failed line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
static void render(float *capture,unsigned start,unsigned frames,unsigned block) {
    float l[512],r[512],sl[512],sr[512];
    for(unsigned at=0;at<frames;) {
        unsigned n=frames-at<block ? frames-at : block;
        for(unsigned i=0;i<n;++i) {
            uint32_t x=(start+at+i)*1664525u+1013904223u;
            sl[i]=(float)(int32_t)x*(.125f/2147483648.0f);
            sr[i]=(float)(int32_t)(x*22695477u+1u)*(.125f/2147483648.0f);
            l[i]=sl[i];r[i]=sr[i];
        }
        ambient_room_process(l,r,sl,sr,(int)n);
        if(capture)for(unsigned i=0;i<n;++i) {
            capture[2*(at+i)]=l[i];capture[2*(at+i)+1]=r[i];
        }
        at+=n;
    }
}
void ambient_room_publish_probe(unsigned point) {
    if(armed && point==chosen) {
        armed=false;++interrupts;
        render(actual,WARM,pause_frames,caller_block);
    }
}
void ambient_room_accept_probe(unsigned point) {
    if(interleave_writer && point==read_point) {
        interleave_writer=false;ambient_room_set(writer_amount);
    }
}
static void setup(float old) {
    armed=false;ambient_room_init();ambient_room_set(old);
    render(NULL,0,WARM,caller_block);
}
static void one(float old,float next,unsigned point,unsigned block,unsigned pauses,bool wrap) {
    chosen=point;caller_block=block;pause_frames=pauses*512;
    setup(old);
    if(point==12)ambient_room_set(next); /* complete publication precedes IRQ */
    render(expected,WARM,pause_frames,block);
    if(point!=12)ambient_room_set(next);
    render(expected+2*pause_frames,WARM+pause_frames,AFTER,block);
    setup(old);
#ifndef FAM_LEGACY_ROOM
    if(wrap)atomic_store(&target_version,UINT32_MAX-1u);
#else
    (void)wrap;
#endif
    armed=true;ambient_room_set(next);CHECK(!armed);
#ifndef FAM_LEGACY_ROOM
    if(wrap)CHECK(atomic_load(&target_version)==0);
#endif
    render(actual+2*pause_frames,WARM+pause_frames,AFTER,block);
    unsigned samples=2*(pause_frames+AFTER);
    if(memcmp(expected,actual,samples*sizeof(float))) {
        unsigned first=0;while(first<samples && memcmp(&expected[first],&actual[first],4)==0)++first;
        fprintf(stderr,"incoherent Room: old=%g next=%g point=%u block=%u pauses=%u first_sample=%u\n",
            (double)old,(double)next,point,block,pauses,first);
        exit(1);
    }
    CHECK(memcmp(expected,actual,samples*sizeof(float))==0);
    for(unsigned i=0;i<samples;++i)CHECK(isfinite(actual[i]));
    ++cases;
}
int main(void) {
    const float pairs[][2]={{0,1},{1,0},{.2f,.9f},{.9f,.2f},{.5f,.5f},{-1,2}};
    for(unsigned p=0;p<sizeof pairs/sizeof pairs[0];++p)
        for(unsigned point=0;point<=12;++point)
            for(unsigned b=0;b<2;++b)
                for(unsigned pauses=1;pauses<=3;pauses+=2)
                    one(pairs[p][0],pairs[p][1],point,b ? 512 : 64,pauses,false);
#ifndef FAM_LEGACY_ROOM
    for(unsigned point=0;point<=12;++point)one(.2f,.9f,point,64,1,true);
    /* A true thread-model interleaving (Main cannot preempt the H743 audio
     * IRQ): a writer changes generation between reader loads. The reader
     * must decline the whole candidate without retrying or mixing fields. */
    for(read_point=0;read_point<=9;++read_point) {
        caller_block=64;setup(.2f);
        render(expected,WARM,64,64);ambient_room_set(.9f);
        render(expected+128,WARM+64,AFTER,64);
        setup(.2f);writer_amount=.9f;interleave_writer=true;
        render(actual,WARM,64,64);CHECK(!interleave_writer);
        render(actual+128,WARM+64,AFTER,64);
        CHECK(memcmp(expected,actual,2*(64+AFTER)*sizeof(float))==0);++cases;
    }
    setup(.5f);uint32_t before=atomic_load(&target_version);
    ambient_room_set(NAN);ambient_room_set(INFINITY);ambient_room_set(-INFINITY);
    CHECK(atomic_load(&target_version)==before);
    /* Latest complete control value is retained even if many edits precede
     * one audio block. Rendering acknowledges no extra setter work. */
    for(unsigned i=0;i<1000;++i)ambient_room_set((float)(i%101)/100.0f);
    render(actual,0,64,64);CHECK(accepted_amount==.9f);
#endif
    printf("{\"status\":\"passed\",\"cases\":%u,\"checks\":%u,\"interruptions\":%u,\"publication_points\":13,\"reader_interleavings\":10,\"max_audio_attempts\":1}\n",cases,checks,interrupts);
    return 0;
}
