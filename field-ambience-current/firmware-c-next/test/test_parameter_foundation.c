#include "engine.h"
#include "engine_product.h"
#include "gesture.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "sound_finish_fx.h"
static bool note(uint8_t c,bool upper,bool on,float v,uint32_t ms) {
    (void)c;(void)upper;(void)on;(void)v;(void)ms;return true;
}
int main(void) {
    sf_init();sf_accept();float old=sf_controls[0];
    unsigned version=atomic_load(&sf_version);
    atomic_store(&sf_version,version+1);atomic_store(&sf_target[0],65536);
    atomic_store(&sf_plate_on,false);sf_accept();
    assert(sf_controls[0]==old && sf_accepted_plate);
    atomic_store(&sf_version,version+2);sf_accept();
    assert(sf_controls[0]==1 && !sf_accepted_plate);
    float zero[1]={0},l[1]={0},r[1]={0};
    sf_process(l,r,zero,zero,1,0,0,false,false,0,0);
    engine_init();sound_parameters_t p,q;
    engine_get_sound_parameters(&p);assert(p.delay==.16f && p.plate && !p.freeze);
    p.delay=2;p.grain=-1;p.phaser=.7f;p.freeze=true;
    assert(engine_apply_sound_parameters(&p));engine_get_sound_parameters(&q);
    assert(q.delay==1 && q.grain==0 && !q.freeze && q.phaser==.7f);
    p.drive=NAN;assert(!engine_apply_sound_parameters(&p));
    engine_get_sound_parameters(&p);assert(p.delay==q.delay && p.grain==q.grain &&
        p.drive==q.drive && p.crush==q.crush && p.phaser==q.phaser &&
        p.freeze==q.freeze && p.plate==q.plate);
    gesture_init_owned(note);gesture_toggle(100);gesture_set_record_context(true,.7f);
    gesture_record_cell(3,true,100);gesture_record_cell(3,false,400);
    uint8_t a[GESTURE_ARCHIVE_MAX],b[GESTURE_ARCHIVE_MAX];
    assert(!gesture_export(a,sizeof a));gesture_stop(1100);
    unsigned n=gesture_export(a,sizeof a);assert(n==32);
    assert(!gesture_export(b,n-1));
    assert(gesture_import(a,n,1200) && gesture_count()==2 && gesture_state()==GESTURE_IDLE);
    assert(gesture_export(b,sizeof b)==n && !memcmp(a,b,n));
    gesture_play(1300);assert(gesture_state()==GESTURE_PLAY);
    for(unsigned i=0;i<n;++i) {
        a[i]^=1;assert(!gesture_import(a,n,1400));a[i]^=1;
        assert(gesture_state()==GESTURE_PLAY && gesture_count()==2);
    }
    assert(!gesture_import(a,n-1,1400));
    assert(gesture_import(a,n,1500) && gesture_state()==GESTURE_IDLE);
    gesture_clear(1600);gesture_toggle(2000);
    for(unsigned i=0;i<GESTURE_MAX;++i)gesture_record_cell((uint8_t)(i%5),(i&1)==0,2000+i);
    gesture_record_cell(0,false,2128);assert(gesture_count()==GESTURE_MAX);
    gesture_toggle(3000);n=gesture_export(a,sizeof a);assert(n==GESTURE_ARCHIVE_MAX);
    assert(gesture_import(a,n,3100));assert(gesture_export(b,sizeof b)==n && !memcmp(a,b,n));
    gesture_clear(3200);gesture_toggle(4000);gesture_record_cell(0,true,4000);
    gesture_record_cell(0,false,5000);gesture_toggle(5000);
    n=gesture_export(a,sizeof a);assert(n==32 && gesture_import(a,n,5100));
    puts("Parameter foundation: atomic invalid-target rejection, clamping, take roundtrip, corruption/truncation rejection, stopped restore pass");
}
