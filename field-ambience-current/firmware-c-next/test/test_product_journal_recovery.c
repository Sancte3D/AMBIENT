/* Actual Product PCM through prepared bounded recovery and async save retry.
 * Real SCN7 snapshots, RAM NOR/read failures; no Scene frontend/H743 activation. */
#include "scenes_journal_recovery.h"
#include "scenes_journal_async.h"
#include "engine.h"
#include "engine_product.h"
#include "scenes.h"
#include "menu.h"
#include "params.h"
#include "controls.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { SR=44100, SECONDS=12, FRAMES=SECONDS*SR, SECTOR=128*1024 };
enum { IDLE, FIRST_WRITE, RECOVER, RETRY_WRITE, COMPLETE };
static uint8_t flash[2][SECTOR],programmed[2][SECTOR/32],old_scene[512],scene_blob[512];
static uint8_t old_record[544],borrowed_copy[32];
static unsigned scene_length,io_calls,read_fault_countdown,wait_ticks,active_sector,active_offset;
static unsigned recovery_steps,writer_steps,write_failures,recovery_failures,finished_ms,recovered_ms;
static const uint8_t *borrowed;
static bool capturing,in_step,active,erasing,fail_program_once;
static int16_t reference[FRAMES*2],trial[FRAMES*2];
typedef struct { unsigned ms;int on;uint8_t owner;float hz,velocity; } event_t;
static event_t events[128],saved[128];
static unsigned event_count,saved_count,now;
static void note(int on,uint8_t owner,float hz,float v) {
    assert(event_count<128);event_t *e=&events[event_count++];memset(e,0,sizeof *e);
    e->ms=now;e->on=on;e->owner=owner;e->hz=hz;e->velocity=v;
}
static bool scene_write(const void *p,unsigned n) {
    assert(n==368);memcpy(scene_blob,p,n);scene_length=n;return true;
}
static scene_journal_read_result_t read_flash(void *c,unsigned s,unsigned at,void *p,unsigned n) {
    (void)c;++io_calls;assert(s<2 && n && at+n<=SECTOR && !active);
    if(in_step)assert(n<=32);
    if(read_fault_countdown && !--read_fault_countdown)return SCENE_JOURNAL_READ_ERROR;
    memcpy(p,flash[s]+at,n);return SCENE_JOURNAL_READ_OK;
}
static bool sync_erase(void *c,unsigned s) {
    (void)c;assert(!capturing && s<2);memset(flash[s],255,SECTOR);
    memset(programmed[s],0,sizeof programmed[s]);return true;
}
static bool sync_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;assert(!capturing && s<2 && at%32==0 && at+32<=SECTOR);
    assert((uintptr_t)p%32==0 && !programmed[s][at/32]);programmed[s][at/32]=1;
    for(unsigned i=0;i<32;++i)flash[s][at+i]&=p[i];
    return true;
}
static bool start_erase(void *c,unsigned s) {
    (void)c;++io_calls;assert(!active && s<2);active=erasing=true;active_sector=s;wait_ticks=20;return true;
}
static bool start_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;++io_calls;assert(!active && s<2 && at%32==0 && at+32<=SECTOR);
    assert((uintptr_t)p%32==0 && !programmed[s][at/32]);
    active=true;erasing=false;active_sector=s;active_offset=at;borrowed=p;
    memcpy(borrowed_copy,p,32);wait_ticks=20;return true;
}
static scene_async_io_result_t poll(void *c) {
    (void)c;++io_calls;assert(active);
    if(!erasing)assert(!memcmp(borrowed,borrowed_copy,32));
    if(wait_ticks){--wait_ticks;return SCENE_ASYNC_IO_BUSY;}
    active=false;
    if(erasing){memset(flash[active_sector],255,SECTOR);memset(programmed[active_sector],0,sizeof programmed[active_sector]);}
    else {
        programmed[active_sector][active_offset/32]=1;
        unsigned n=fail_program_once ? 17u : 32u;
        for(unsigned i=0;i<n;++i)flash[active_sector][active_offset+i]&=borrowed[i];
        if(fail_program_once){fail_program_once=false;return SCENE_ASYNC_IO_ERROR;}
    }
    return SCENE_ASYNC_IO_DONE;
}
static void request_recovery(scene_journal_recovery_t *r,scene_journal_async_t *w) {
    unsigned before=io_calls;
    assert(scene_journal_recovery_begin(r));
    assert(!scene_journal_async_begin(w,scene_blob,scene_length));
    assert(io_calls==before && !r->journal->initialized);
}
static void capture(int world,int block,int mode,int16_t *out) {
    capturing=in_step=active=false;read_fault_countdown=0;fail_program_once=false;
    engine_init();engine_boot_mute();controls_init();
    menu_callbacks_t cb={.set_world=engine_set_world,.set_key=engine_set_key_pc,.set_tuning=engine_set_tuning,
        .set_space=engine_set_space,.set_atmosphere=engine_set_atmosphere,.set_motion=engine_set_motion,
        .set_fx=engine_set_fx_mode,.set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb);params_init();scenes_init(scene_write,NULL);
    menu_state_t m={.world=(uint8_t)world,.key_pc=2,.space=100,.motion=50,.fx=1,.attack=100,.release=100};
    menu_apply_state(&m);params_set_bright(800);engine_set_master_volume(.6f);engine_set_gen_seed(1234);
    assert(scenes_save(0,0));memcpy(old_scene,scene_blob,scene_length);
    assert(scenes_save(1,0) && memcmp(old_scene,scene_blob,scene_length));
    memset(flash,255,sizeof flash);memset(programmed,0,sizeof programmed);
    scene_journal_t journal;scene_journal_io_t io={NULL,SECTOR,read_flash,sync_erase,sync_program};
    scene_journal_configure(&journal,&io);assert(scene_journal_write(&journal,old_scene,scene_length));
    memcpy(old_record,flash[0],sizeof old_record);
    scene_journal_async_t writer;scene_journal_async_io_t ai={NULL,start_erase,start_program,poll};
    scene_journal_async_configure(&writer,&journal,&ai);
    scene_journal_recovery_t recovery;scene_journal_recovery_configure(&recovery,&journal);
    io_calls=recovery_steps=writer_steps=write_failures=recovery_failures=finished_ms=recovered_ms=0;
    event_count=0;memset(events,0,sizeof events);engine_set_note_hook(note);engine_set_generative(true,-1);
    int phase=IDLE;capturing=true;
    for(now=0;now<SECONDS*1000;now+=10) {
        engine_generative_tick(now);
        if(mode && now==2700) {
            if(mode==2){fail_program_once=true;assert(scene_journal_async_begin(&writer,scene_blob,scene_length));phase=FIRST_WRITE;}
            else {if(mode==3)read_fault_countdown=5;request_recovery(&recovery,&writer);phase=RECOVER;}
        }
        unsigned before=io_calls;in_step=true;
        if(phase==FIRST_WRITE || phase==RETRY_WRITE) {
            scene_async_status_t s=scene_journal_async_step(&writer);++writer_steps;
            if(s==SCENE_ASYNC_FAILED) {
                assert(phase==FIRST_WRITE && !active && !journal.initialized);
                ++write_failures;request_recovery(&recovery,&writer);phase=RECOVER;
            } else if(s==SCENE_ASYNC_DONE){assert(phase==RETRY_WRITE);phase=COMPLETE;finished_ms=now;}
            else assert(s==SCENE_ASYNC_BUSY);
        } else if(phase==RECOVER) {
            scene_recovery_status_t s=scene_journal_recovery_step(&recovery);++recovery_steps;
            if(s==SCENE_RECOVERY_FAILED) {
                assert(mode==3 && !active && !journal.initialized && !read_fault_countdown);
                ++recovery_failures;request_recovery(&recovery,&writer);
            } else if(s==SCENE_RECOVERY_DONE) {
                assert(journal.initialized && journal.has_record && journal.sequence==0 &&
                       journal.sector==0 && journal.slot==0 && journal.fresh_sector_required);
                recovered_ms=now;assert(scene_journal_async_begin(&writer,scene_blob,scene_length));
                assert(writer.sector==1);phase=RETRY_WRITE;
            } else assert(s==SCENE_RECOVERY_BUSY && !journal.initialized);
        }
        in_step=false;assert(io_calls-before<=1 && !memcmp(flash[0],old_record,sizeof old_record));
        for(int at=0;at<441;) {
            int n=block<441-at ? block : 441-at;
            engine_render(out+2*(now*SR/1000+at),n);at+=n;
        }
        assert(engine_ambient_source_count()<=3);
    }
    engine_generative_tick(SECONDS*1000);capturing=false;
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
    if(mode) {
        assert(phase==COMPLETE && !active && writer.status==SCENE_ASYNC_DONE && finished_ms<SECONDS*1000);
        assert(write_failures==(mode==2) && recovery_failures==(mode==3));
        assert(recovery_steps==(mode==3 ? 497u : 492u));
        uint8_t decoded[368];assert(scene_journal_read(&journal,decoded,sizeof decoded));
        assert(!memcmp(decoded,scene_blob,sizeof decoded) && journal.sequence==1 && journal.sector==1);
    }
    int peak=0;for(unsigned i=0;i<FRAMES*2;++i)if(abs(out[i])>peak)peak=abs(out[i]);
    assert(peak>0 && engine_generative_melody_count()>=2);
}
int main(void) {
    for(int world=0;world<3;++world) {
        capture(world,512,0,reference);saved_count=event_count;memcpy(saved,events,sizeof saved);
        for(int mode=1;mode<=3;++mode)for(int block=64;block<=512;block+=448) {
            capture(world,block,mode,trial);
            assert(!memcmp(reference,trial,sizeof reference));
            assert(event_count==saved_count && !memcmp(events,saved,event_count*sizeof *events));
            printf("{\"world\":%d,\"mode\":%d,\"block\":%d,\"whole_pcm_and_hooks_equal\":true,\"recovery_steps\":%u,\"writer_steps\":%u,\"write_failures\":%u,\"recovery_failures\":%u,\"recovered_ms\":%u,\"completed_ms\":%u}\n",
                world,mode,block,recovery_steps,writer_steps,write_failures,recovery_failures,recovered_ms,finished_ms);
        }
    }
    puts("{\"status\":\"passed\",\"matched_triplets\":9,\"shared_baselines\":3,\"captures\":21,\"capture_seconds\":12,\"rendered_stereo_frames\":11113200,\"control_tick_ms\":10}");
    return 0;
}
