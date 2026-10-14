/* Genuine SCN7 frontend state, NOR failures/retries and complete Product PCM. */
#include "scenes_save_async.h"
#include "scenes.h"
#include "engine.h"
#include "engine_product.h"
#include "menu.h"
#include "params.h"
#include "controls.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { NATIVE=368, WORD=32, RECORD=544, SECTOR=128*1024, SR=44100, SECONDS=12, FRAMES=SR*SECONDS };
static uint8_t flash[2][SECTOR],programmed[2][SECTOR/WORD];
static uint8_t old_blob[NATIVE],expected[NATIVE],last_blob[NATIVE],old_record[RECORD],borrowed_copy[WORD];
static uint8_t protected_record[RECORD];
static unsigned protected_sector,protected_offset;
static scene_journal_t journal;
static scene_journal_async_t writer;
static scene_journal_recovery_t recovery;
static unsigned bytes,io_calls,fault_at,wait_per_io,wait_ticks,sector,offset,ticks,checks,cases;
static const uint8_t *borrowed;
static bool active,erasing,in_step,pcm;
static int16_t reference[FRAMES*2],trial[FRAMES*2];
typedef struct { unsigned ms;int on;uint8_t owner;float hz,velocity; } event_t;
static event_t events[128],saved[128];
static unsigned event_count,saved_count,now,finished_ms;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static bool scene_write(const void *p,unsigned n) {
    CHECK(n==NATIVE && scenes_save_info().status!=SCENES_SAVE_BUSY);
    memcpy(last_blob,p,n); return true;
}
static bool scene_read(void *p,unsigned n) {
    CHECK(scenes_save_info().status!=SCENES_SAVE_BUSY);
    if(n!=NATIVE)return false;
    memcpy(p,old_blob,n);return true;
}
static bool io_fault(void) { ++io_calls;return fault_at && io_calls==fault_at; }
static scene_journal_read_result_t read_flash(void *c,unsigned s,unsigned at,void *p,unsigned n) {
    (void)c;CHECK(!active && s<2 && n && at+n<=bytes);
    if(in_step)CHECK(n<=WORD);
    if(io_fault())return SCENE_JOURNAL_READ_ERROR;
    memcpy(p,flash[s]+at,n);return SCENE_JOURNAL_READ_OK;
}
static bool sync_erase(void *c,unsigned s) {
    (void)c;CHECK(!in_step && !pcm && s<2);memset(flash[s],255,bytes);
    memset(programmed[s],0,sizeof programmed[s]);return true;
}
static bool sync_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;CHECK(!in_step && !pcm && s<2 && at%WORD==0 && at+WORD<=bytes);
    CHECK((uintptr_t)p%WORD==0 && !programmed[s][at/WORD]);programmed[s][at/WORD]=1;
    for(unsigned i=0;i<WORD;++i)flash[s][at+i]&=p[i];
    return true;
}
static bool start_erase(void *c,unsigned s) {
    (void)c;CHECK(!active && s<2);
    if(io_fault())return false;
    active=erasing=true;sector=s;wait_ticks=wait_per_io;return true;
}
static bool start_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;CHECK(!active && s<2 && at%WORD==0 && at+WORD<=bytes);
    CHECK((uintptr_t)p%WORD==0 && !programmed[s][at/WORD]);
    if(io_fault())return false;
    active=true;erasing=false;sector=s;offset=at;borrowed=p;
    memcpy(borrowed_copy,p,WORD);wait_ticks=wait_per_io;return true;
}
static scene_async_io_result_t poll(void *c) {
    (void)c;CHECK(active);
    if(!erasing)CHECK(!memcmp(borrowed,borrowed_copy,WORD));
    bool bad=io_fault();
    if(wait_ticks && !bad){--wait_ticks;return SCENE_ASYNC_IO_BUSY;}
    active=false;
    if(erasing){memset(flash[sector],255,bad ? bytes/2u : bytes);memset(programmed[sector],0,sizeof programmed[sector]);}
    else {
        programmed[sector][offset/WORD]=1;
        for(unsigned i=0;i<(bad ? 17u : WORD);++i)flash[sector][offset+i]&=borrowed[i];
    }
    return bad ? SCENE_ASYNC_IO_ERROR : SCENE_ASYNC_IO_DONE;
}
static void note(int on,uint8_t owner,float hz,float v) {
    CHECK(event_count<128);event_t *e=&events[event_count++];memset(e,0,sizeof *e);
    e->ms=now;e->on=on;e->owner=owner;e->hz=hz;e->velocity=v;
}
static void controls(int world,int key,unsigned seed,int bright) {
    menu_state_t m={.world=(uint8_t)world,.key_pc=(uint8_t)key,.space=100,.motion=50,
                   .fx=1,.attack=100,.release=100};
    menu_apply_state(&m);engine_set_gen_seed(seed);params_set_bright(bright);
}
static void setup(int world,int target,unsigned size,unsigned waits,bool force_recovery) {
    CHECK(scenes_save_info().status!=SCENES_SAVE_BUSY && !active);
    CHECK(scenes_save_async_configure(NULL,NULL));
    pcm=in_step=false;fault_at=0;bytes=size;wait_per_io=waits;ticks=0;
    engine_init();engine_boot_mute();controls_init();
    menu_callbacks_t cb={.set_world=engine_set_world,.set_key=engine_set_key_pc,.set_tuning=engine_set_tuning,
        .set_space=engine_set_space,.set_atmosphere=engine_set_atmosphere,.set_motion=engine_set_motion,
        .set_fx=engine_set_fx_mode,.set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb);params_init();scenes_init(scene_write,NULL);
    controls(world,2,42,-600);CHECK(scenes_save(0,0));memcpy(old_blob,last_blob,NATIVE);
    controls(world,7,1234,800);CHECK(scenes_save(target,1));memcpy(expected,last_blob,NATIVE);
    CHECK(memcmp(old_blob,expected,NATIVE));
    scenes_init(scene_write,scene_read);CHECK(scenes_recall(0,2) && scenes_active()==0);
    controls(world,7,1234,800);engine_set_master_volume(.6f);
    memset(flash,255,sizeof flash);memset(programmed,0,sizeof programmed);
    scene_journal_io_t io={NULL,bytes,read_flash,sync_erase,sync_program};
    scene_journal_configure(&journal,&io);CHECK(scene_journal_write(&journal,old_blob,NATIVE));
    memcpy(old_record,flash[0],RECORD);
    memcpy(protected_record,old_record,RECORD);protected_sector=protected_offset=0;
    scene_journal_async_io_t ai={NULL,start_erase,start_program,poll};
    scene_journal_async_configure(&writer,&journal,&ai);
    scene_journal_recovery_configure(&recovery,&journal);
    if(force_recovery)journal.initialized=false;
    CHECK(scenes_save_async_configure(&writer,&recovery));io_calls=0;
    CHECK(scenes_save_info().status==SCENES_SAVE_IDLE && scenes_save_info().slot==-1);
}
static void begin(int target) {
    unsigned before=io_calls;
    CHECK(scenes_save_begin(target,UINT32_MAX-7u));
    scenes_save_info_t i=scenes_save_info();
    CHECK(io_calls==before && i.status==SCENES_SAVE_BUSY && i.error==SCENES_SAVE_ERROR_NONE &&
          i.slot==target && i.requested_ms==UINT32_MAX-7u && !i.cancel_requested && !i.storage_uncertain);
    CHECK(scenes_active()==0 && scenes_used(0) && (target==0 || !scenes_used(target)));
}
static scenes_save_status_t step(void) {
    unsigned before=io_calls;in_step=true;
    scenes_save_status_t s=scenes_save_step(++ticks);
    in_step=false;CHECK(io_calls-before<=1);
    CHECK(!memcmp(protected_record,flash[protected_sector]+protected_offset,RECORD));
    /* A late failed acknowledgement can leave a VALID new physical commit.
     * Successful recovery makes that record the protected winner, even though
     * RAM still retains the old Scene. A retry then erases the opposite sector. */
    if(journal.initialized && journal.has_record &&
       (s==SCENES_SAVE_DONE || (recovery.status==SCENE_RECOVERY_DONE && writer.status!=SCENE_ASYNC_BUSY))) {
        protected_sector=journal.sector;protected_offset=journal.slot*RECORD;
        memcpy(protected_record,flash[protected_sector]+protected_offset,RECORD);
    }
    if(s==SCENES_SAVE_BUSY)CHECK(scenes_active()==0 && scenes_used(0));
    return s;
}
static scenes_save_status_t finish(void) {
    for(unsigned n=0;n<20000;++n){scenes_save_status_t s=step();if(s!=SCENES_SAVE_BUSY)return s;}
    CHECK(false);return SCENES_SAVE_FAILED;
}
static void unchanged_info(scenes_save_info_t a,scenes_save_info_t b) {
    CHECK(a.status==b.status && a.error==b.error && a.slot==b.slot && a.requested_ms==b.requested_ms &&
          a.finished_ms==b.finished_ms && a.cancel_requested==b.cancel_requested &&
          a.storage_uncertain==b.storage_uncertain);
}
static void blocked(int target) {
    scenes_save_info_t before=scenes_save_info();unsigned calls=io_calls;
    CHECK(!scenes_save_begin(target,10) && !scenes_save_begin(-1,10) && !scenes_save_begin(5,10));
    CHECK(!scenes_save_async_configure(NULL,NULL) && !scenes_save_async_configure(&writer,&recovery));
    CHECK(!scenes_save(target,10) && !scenes_recall(0,10));
    scenes_init(scene_write,scene_read);
    unchanged_info(before,scenes_save_info());CHECK(io_calls==calls && scenes_active()==0);
    if(active && !erasing)CHECK(!memcmp(borrowed,borrowed_copy,WORD));
}
static void confirmed(int target) {
    scenes_save_info_t i=scenes_save_info();
    CHECK(i.status==SCENES_SAVE_DONE && i.error==SCENES_SAVE_ERROR_NONE && !i.storage_uncertain &&
          i.slot==target && i.finished_ms==ticks && !active && scenes_active()==target && scenes_used(target));
    uint8_t p[NATIVE];CHECK(scene_journal_read(&journal,p,NATIVE) && !memcmp(p,expected,NATIVE));
    unsigned before=io_calls;CHECK(scenes_save_step(20)==SCENES_SAVE_DONE && !scenes_save_cancel());
    CHECK(!scenes_save_begin(-1,20));unchanged_info(i,scenes_save_info());CHECK(io_calls==before);
    CHECK(scenes_recall(target,30));menu_state_t m;menu_get_state(&m);
    CHECK(m.key_pc==7 && engine_gen_seed()==1234 && params_bright_hz()==800);
    CHECK(scenes_save(target,31) && !memcmp(last_blob,expected,NATIVE));
}
static void failed_state(scenes_save_error_t error) {
    scenes_save_info_t i=scenes_save_info();
    CHECK(i.status==SCENES_SAVE_FAILED && i.error==error && i.finished_ms==ticks && !active && scenes_active()==0);
    unsigned before=io_calls;CHECK(scenes_save_step(50)==SCENES_SAVE_FAILED && !scenes_save_cancel());
    CHECK(!scenes_save_begin(5,50));unchanged_info(i,scenes_save_info());CHECK(io_calls==before);
    CHECK(scenes_recall(0,51));menu_state_t m;menu_get_state(&m);
    CHECK(m.key_pc==2 && engine_gen_seed()==42 && params_bright_hz()==-600);
    CHECK(scenes_save(0,52) && !memcmp(last_blob,old_blob,NATIVE));
    controls(0,7,1234,800);
}
static void model(void) {
    CHECK(!scenes_save_begin(0,0) && scenes_save_info().status==SCENES_SAVE_IDLE);
    for(int slot=0;slot<5;++slot) {
        setup(0,slot,4*RECORD,0,false);begin(slot);
        controls(1,11,999,0);CHECK(finish()==SCENES_SAVE_DONE);confirmed(slot);++cases;
    }
    setup(0,1,4*RECORD,0,false);begin(1);CHECK(finish()==SCENES_SAVE_DONE);
    unsigned calls=io_calls,write_steps=ticks;confirmed(1);
    unsigned physically_complete_failures=0;
    for(unsigned at=1;at<=calls;++at) {
        setup(0,1,4*RECORD,0,false);fault_at=at;begin(1);
        CHECK(finish()==SCENES_SAVE_FAILED);failed_state(SCENES_SAVE_ERROR_WRITE);
        CHECK(scenes_save_info().storage_uncertain && !journal.initialized);
        fault_at=0;scene_journal_t reboot;scene_journal_configure(&reboot,&journal.io);
        CHECK(scene_journal_recover(&reboot));
        if(reboot.sequence==1)++physically_complete_failures;
        begin(1);CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);++cases;
    }
    setup(0,1,4*RECORD,3,false);begin(1);CHECK(finish()==SCENES_SAVE_DONE);
    unsigned cancel_positions=ticks;confirmed(1);
    for(unsigned at=0;at<cancel_positions;++at) {
        setup(0,1,4*RECORD,3,false);begin(1);
        for(unsigned n=0;n<at;++n)CHECK(step()==SCENES_SAVE_BUSY);
        blocked(1);CHECK(scenes_save_cancel());blocked(1);
        CHECK(finish()==SCENES_SAVE_FAILED);failed_state(SCENES_SAVE_ERROR_CANCELLED);
        begin(1);CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);++cases;
    }
    setup(0,1,4*RECORD,0,true);begin(1);
    unsigned recovery_positions=0;
    while(writer.status==SCENE_ASYNC_IDLE){CHECK(step()==SCENES_SAVE_BUSY);++recovery_positions;}
    unsigned recovery_reads=io_calls;
    CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);
    setup(0,1,4*RECORD,0,true);begin(1);
    while(writer.status==SCENE_ASYNC_IDLE)CHECK(step()==SCENES_SAVE_BUSY);
    unsigned scan_reads=io_calls;
    CHECK(finish()==SCENES_SAVE_DONE);unsigned fresh_calls=io_calls-scan_reads;confirmed(1);
    for(unsigned at=1;at<=fresh_calls;++at) {
        setup(0,1,4*RECORD,0,true);fault_at=scan_reads+at;begin(1);
        CHECK(finish()==SCENES_SAVE_FAILED);failed_state(SCENES_SAVE_ERROR_WRITE);
        CHECK(scenes_save_info().storage_uncertain && !journal.initialized);
        fault_at=0;begin(1);CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);++cases;
    }
    setup(0,1,4*RECORD,3,true);begin(1);
    while(writer.status==SCENE_ASYNC_IDLE)CHECK(step()==SCENES_SAVE_BUSY);
    unsigned start_tick=ticks;
    CHECK(finish()==SCENES_SAVE_DONE);unsigned fresh_cancel_positions=ticks-start_tick;confirmed(1);
    for(unsigned at=0;at<fresh_cancel_positions;++at) {
        setup(0,1,4*RECORD,3,true);begin(1);
        while(writer.status==SCENE_ASYNC_IDLE)CHECK(step()==SCENES_SAVE_BUSY);
        for(unsigned n=0;n<at;++n)CHECK(step()==SCENES_SAVE_BUSY);
        blocked(1);CHECK(scenes_save_cancel());blocked(1);
        CHECK(finish()==SCENES_SAVE_FAILED);failed_state(SCENES_SAVE_ERROR_CANCELLED);
        begin(1);CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);++cases;
    }
    /* A retry captures the player's NEW state, rather than replaying a failed snapshot. */
    setup(0,1,4*RECORD,0,false);fault_at=2;begin(1);
    CHECK(finish()==SCENES_SAVE_FAILED);fault_at=0;controls(0,11,999,0);
    begin(1);CHECK(finish()==SCENES_SAVE_DONE && scenes_recall(1,100));
    menu_state_t newer;menu_get_state(&newer);
    CHECK(newer.key_pc==11 && engine_gen_seed()==999 && params_bright_hz()==0);
    uint8_t latest[NATIVE];CHECK(scene_journal_read(&journal,latest,NATIVE));
    CHECK(scenes_save(1,101) && !memcmp(last_blob,latest,NATIVE));++cases;
    for(unsigned at=0;at<recovery_positions;++at) {
        setup(0,1,4*RECORD,0,true);begin(1);
        for(unsigned n=0;n<at;++n)CHECK(step()==SCENES_SAVE_BUSY);
        blocked(1);CHECK(scenes_save_cancel());CHECK(finish()==SCENES_SAVE_FAILED);
        failed_state(SCENES_SAVE_ERROR_CANCELLED);begin(1);
        CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);++cases;
    }
    /* Fail every recovery read, including after finding a valid candidate. */
    for(unsigned at=1;at<=recovery_reads;++at) {
        setup(0,1,4*RECORD,0,true);fault_at=at;begin(1);
        CHECK(finish()==SCENES_SAVE_FAILED);failed_state(SCENES_SAVE_ERROR_RECOVERY);
        CHECK(!journal.initialized && scenes_save_info().storage_uncertain);
        fault_at=0;begin(1);CHECK(finish()==SCENES_SAVE_DONE);confirmed(1);++cases;
    }
    CHECK(physically_complete_failures>0);
    /* Failed reconfiguration must preserve the terminal result. */
    scenes_save_info_t before=scenes_save_info();scene_journal_recovery_t wrong={0};
    CHECK(!scenes_save_async_configure(&writer,&wrong));unchanged_info(before,scenes_save_info());
    CHECK(scenes_save_async_configure(NULL,NULL));
    /* Detaching/init must not abandon externally started IO on an attached pair. */
    setup(0,1,4*RECORD,3,false);
    CHECK(scene_journal_async_begin(&writer,expected,NATIVE));
    CHECK(scene_journal_async_step(&writer)==SCENE_ASYNC_BUSY && active);
    unsigned held_calls=io_calls;
    CHECK(!scenes_save_async_configure(NULL,NULL) && !scenes_save_begin(1,0));
    scenes_init(scene_write,scene_read);
    CHECK(io_calls==held_calls && scenes_active()==0 && !scenes_used(1));
    CHECK(scene_journal_async_cancel(&writer));
    while(scene_journal_async_step(&writer)==SCENE_ASYNC_BUSY)CHECK(active);
    CHECK(!active && scenes_save_async_configure(NULL,NULL));++cases;
    printf("{\"category\":\"frontend\",\"status\":\"passed\",\"cases\":%u,\"checks\":%u,\"append_writer_io_fault_positions\":%u,\"fresh_writer_io_fault_positions\":%u,\"recovery_read_fault_positions\":%u,\"writer_steps_no_busy\":%u,\"append_writer_cancel_positions\":%u,\"fresh_writer_cancel_positions\":%u,\"recovery_cancel_positions\":%u,\"late_failures_with_recoverable_commit\":%u,\"max_io_per_step\":1,\"native_bytes\":368}\n",
           cases,checks,calls,fresh_calls,recovery_reads,write_steps,cancel_positions,fresh_cancel_positions,recovery_positions,physically_complete_failures);
}
/* Modes: success, automatic pre-save recovery, partial program error+retry,
 * recovery read error+retry, in-flight cancel+retry. Begin is the actual API. */
static void capture(int world,int block,int mode,int16_t *out) {
    setup(world,1,SECTOR,20,mode==2 || mode==4);
    event_count=0;memset(events,0,sizeof events);engine_set_note_hook(note);
    engine_set_generative(true,-1);pcm=true;bool started=false,retried=false,complete=false;
    finished_ms=0;
    for(now=0;now<SECONDS*1000;now+=10) {
        engine_generative_tick(now);
        if(mode && now==1500) {
            if(mode==3)fault_at=22; /* First program terminal poll, 17-byte cut. */
            if(mode==4)fault_at=5;
            CHECK(scenes_save_begin(1,now));started=true;
        }
        if(started && !complete) {
            if(mode==5 && !retried && active && !erasing && wait_ticks==10)CHECK(scenes_save_cancel());
            unsigned before=io_calls;in_step=true;
            scenes_save_status_t s=scenes_save_step(now);in_step=false;
            CHECK(io_calls-before<=1 && !memcmp(flash[0],old_record,RECORD));
            if(s==SCENES_SAVE_BUSY)CHECK(scenes_active()==0 && !scenes_used(1));
            else if(s==SCENES_SAVE_FAILED) {
                CHECK(mode>=3 && !retried && !active && scenes_active()==0 && !scenes_used(1));
                CHECK(scenes_save_info().storage_uncertain);
                fault_at=0;CHECK(scenes_save_begin(1,now));retried=true;
            } else {CHECK(s==SCENES_SAVE_DONE && !active && scenes_active()==1 && scenes_used(1));complete=true;finished_ms=now;}
        }
        for(int at=0;at<441;) {
            int n=block<441-at ? block : 441-at;
            engine_render(out+2*(now*SR/1000+at),n);at+=n;
        }
    }
    engine_generative_tick(SECONDS*1000);pcm=false;
    CHECK(!engine_nonfinite_samples() && !engine_output_limited_samples());
    if(mode) {
        CHECK(complete && finished_ms<SECONDS*1000 && retried==(mode>=3));
        uint8_t p[NATIVE];CHECK(scene_journal_read(&journal,p,NATIVE) && !memcmp(p,expected,NATIVE));
        CHECK(scenes_save_info().status==SCENES_SAVE_DONE);
    }
    int peak=0;for(unsigned n=0;n<FRAMES*2;++n)if(abs(out[n])>peak)peak=abs(out[n]);CHECK(peak>0);
    CHECK(scenes_save_async_configure(NULL,NULL));
}
int main(int argc,char **argv) {
    model();
    if(argc==2 && !strcmp(argv[1],"--model"))return 0;
    for(int world=0;world<3;++world) {
        capture(world,512,0,reference);saved_count=event_count;memcpy(saved,events,sizeof saved);
        for(int mode=1;mode<=5;++mode)for(int block=64;block<=512;block+=448) {
            capture(world,block,mode,trial);
            CHECK(!memcmp(reference,trial,sizeof reference));
            CHECK(saved_count==event_count && !memcmp(saved,events,event_count*sizeof *events));
            printf("{\"category\":\"pcm\",\"world\":%d,\"mode\":%d,\"block\":%d,\"whole_pcm_and_hooks_equal\":true,\"completed_ms\":%u}\n",world,mode,block,finished_ms);
        }
    }
    puts("{\"category\":\"summary\",\"status\":\"passed\",\"matched_triplets\":15,\"shared_baselines\":3,\"captures\":33,\"capture_seconds\":12,\"rendered_stereo_frames\":17463600,\"control_tick_ms\":10}");
    return 0;
}
