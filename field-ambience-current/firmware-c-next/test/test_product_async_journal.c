/* Actual Product PCM while a prepared journal transaction waits between ticks.
 * RAM NOR driver and actual SCN7 snapshot, not the H743 flash/frontend adapter. */
#include "scenes_journal_async.h"
#include "engine.h"
#include "engine_product.h"
#include "scenes.h"
#include "menu.h"
#include "params.h"
#include "controls.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
enum { SR=44100, FRAMES=8*SR, SECTOR=2*SCENE_JOURNAL_RECORD_BYTES };
static uint8_t flash[2][SECTOR],scene_blob[512],borrowed_copy[32];
static unsigned scene_length,io_calls,sector,offset,waiting,programs,steps,finished_ms;
static const uint8_t *borrowed; static bool active,erasing;
static int16_t reference[FRAMES*2],trial[FRAMES*2];
typedef struct { unsigned ms; int on; uint8_t owner; float hz,velocity; } event_t;
static event_t events[128],saved[128]; static unsigned event_count,saved_count,now;
static void note(int on,uint8_t owner,float hz,float v) {
    assert(event_count<128); event_t *e=&events[event_count++]; memset(e,0,sizeof *e);
    e->ms=now;e->on=on;e->owner=owner;e->hz=hz;e->velocity=v;
}
static bool scene_write(const void *p,unsigned n) {
    assert(n==368); memcpy(scene_blob,p,n); scene_length=n; return true;
}
static scene_journal_read_result_t read_flash(void *c,unsigned s,unsigned at,void *p,unsigned n) {
    (void)c; ++io_calls; assert(s<2 && at+n<=SECTOR); memcpy(p,flash[s]+at,n);
    return SCENE_JOURNAL_READ_OK;
}
static bool forbidden_erase(void *c,unsigned s) { (void)c;(void)s;assert(false);return false; }
static bool forbidden_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;(void)s;(void)at;(void)p;assert(false);return false;
}
static bool start_erase(void *c,unsigned s) {
    (void)c; ++io_calls; assert(!active && s<2); active=erasing=true;sector=s;waiting=20;return true;
}
static bool start_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c; ++io_calls; assert(!active && s<2 && at%32==0 && at+32<=SECTOR);
    assert((uintptr_t)p%32==0);active=true;erasing=false;sector=s;offset=at;
    borrowed=p;memcpy(borrowed_copy,p,32);waiting=20;++programs;return true;
}
static scene_async_io_result_t poll_io(void *c) {
    (void)c; ++io_calls;assert(active);
    if(!erasing) assert(!memcmp(borrowed,borrowed_copy,32));
    if(waiting) {--waiting;return SCENE_ASYNC_IO_BUSY;}
    if(erasing) memset(flash[sector],255,SECTOR);
    else for(unsigned i=0;i<32;++i) flash[sector][offset+i]&=borrowed[i];
    active=false;return SCENE_ASYNC_IO_DONE;
}
static void capture(int world,int block,bool store,int16_t *out) {
    engine_init();engine_boot_mute(); controls_init();
    menu_callbacks_t cb={.set_world=engine_set_world,.set_key=engine_set_key_pc,.set_tuning=engine_set_tuning,
        .set_space=engine_set_space,.set_atmosphere=engine_set_atmosphere,
        .set_motion=engine_set_motion,.set_fx=engine_set_fx_mode,
        .set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb);params_init();scenes_init(scene_write,NULL);
    menu_state_t m={.world=(uint8_t)world,.key_pc=2,.space=100,.motion=50,
                   .fx=1,.attack=100,.release=100};
    menu_apply_state(&m);params_set_bright(800);engine_set_master_volume(.6f);
    engine_set_gen_seed(1234);assert(scenes_save(0,0));
    memset(flash,255,sizeof flash);memcpy(flash[1],scene_blob,scene_length);
    scene_journal_t journal;scene_journal_async_t writer;
    scene_journal_io_t io={NULL,SECTOR,read_flash,forbidden_erase,forbidden_program};
    scene_journal_configure(&journal,&io);assert(scene_journal_recover(&journal));
    scene_journal_async_io_t ai={NULL,start_erase,start_program,poll_io};
    scene_journal_async_configure(&writer,&journal,&ai);
    io_calls=programs=steps=finished_ms=0;active=false;event_count=0;
    memset(events,0,sizeof events);engine_set_note_hook(note);engine_set_generative(true,-1);
    for(now=0;now<8000;now+=10) {
        engine_generative_tick(now);
        if(store && now==2700) assert(scene_journal_async_begin(&writer,scene_blob,scene_length));
        if(writer.status==SCENE_ASYNC_BUSY) {
            unsigned before=io_calls;
            scene_async_status_t status=scene_journal_async_step(&writer);++steps;
            assert(io_calls-before<=1);
            if(status==SCENE_ASYNC_DONE) finished_ms=now;
            else assert(status==SCENE_ASYNC_BUSY);
            assert(!memcmp(flash[1],scene_blob,scene_length)); /* legacy stays intact */
        }
        for(int at=0;at<441;) {
            int n=block<441-at ? block : 441-at;
            engine_render(out+2*(now*SR/1000+at),n);at+=n;
        }
        assert(engine_ambient_source_count()<=3);
    }
    engine_generative_tick(8000);
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
    if(store) {
        assert(writer.status==SCENE_ASYNC_DONE && !active && programs==13 && finished_ms<8000);
        uint8_t decoded[368]; assert(scene_journal_read(&journal,decoded,sizeof decoded));
        assert(!memcmp(decoded,scene_blob,sizeof decoded));
    }
    int peak=0;for(unsigned i=0;i<FRAMES*2;++i)if(abs(out[i])>peak)peak=abs(out[i]);
    assert(peak>0 && engine_generative_melody_count()>=2);
}
int main(void) {
    for(int world=0;world<3;++world) {
        capture(world,512,false,reference);saved_count=event_count;memcpy(saved,events,sizeof saved);
        for(int block=64;block<=512;block+=448) {
            capture(world,block,true,trial);
            assert(!memcmp(reference,trial,sizeof reference));
            assert(event_count==saved_count && !memcmp(events,saved,event_count*sizeof *events));
            printf("{\"world\":%d,\"block\":%d,\"whole_pcm_and_hooks_equal\":true,\"steps\":%u,\"completed_ms\":%u,\"programmed_words\":%u}\n",
                world,block,steps,finished_ms,programs);
        }
    }
    puts("{\"status\":\"passed\",\"pcm_triplets\":3,\"rendered_stereo_frames\":3175200,\"control_tick_ms\":10,\"capture_seconds\":8}");
    return 0;
}
