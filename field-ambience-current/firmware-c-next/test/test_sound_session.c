/* Real SCN8 frontend, JSS1 journal and H743 IRQ adapter. Only NOR hardware is
 * modelled. Including the three owners also permits a genuine reset of their
 * volatile state while persistent flash/ECC words survive a simulated reboot. */
#include "../src/scenes.c"
#include "../src/scenes_save_async.c"
#include "../src/hal_h743/scenes_storage_h743.c"
#include "controls.h"
#include <assert.h>
#include <stdlib.h>

enum { SECTOR=128*1024,WORD=32,FRAMES=12*44100 };
static uint8_t flash[2][SECTOR],written[2][SECTOR/WORD],poison[2][SECTOR/WORD];
static uint8_t baseline[2][SECTOR],baseline_written[2][SECTOR/WORD],baseline_poison[2][SECTOR/WORD];
static bool active,erasing,read_error,program_error;
static unsigned target_sector,target_offset,latency,starts,reads,programs,erases,ticks,checks,cuts;
static const uint8_t *borrowed;
static uint8_t word_copy[WORD];
static int16_t reference[2*FRAMES],trial[2*FRAMES];
typedef struct {unsigned ms;int on;uint8_t owner;float hz,velocity;} note_t;
static note_t notes[128],saved_notes[128];
static unsigned note_count,saved_note_count,now,loop_notes;
#define CHECK(x) do {++checks;assert(x);} while(0)

bool scenes_flash_h743_model_setup(void) {return !active;}
bool scenes_flash_h743_model_context_ready(void) {return true;}
bool scenes_flash_h743_model_quiescent(void) {return !active;}
bool scenes_flash_h743_model_lock(void) {CHECK(!active);return true;}
bool scenes_flash_h743_model_start_erase(unsigned sector) {
    CHECK(!active && sector<2);++starts;++erases;
    active=erasing=true;target_sector=sector;latency=2;return true;
}
bool scenes_flash_h743_model_start_program(unsigned sector,unsigned offset,const uint8_t word[32]) {
    CHECK(!active && sector<2 && offset%WORD==0 && offset+WORD<=SECTOR);
    CHECK((uintptr_t)word%WORD==0 && !written[sector][offset/WORD]);
    ++starts;++programs;active=true;erasing=false;target_sector=sector;target_offset=offset;
    borrowed=word;memcpy(word_copy,word,WORD);latency=2;return true;
}
scene_journal_read_result_t scenes_flash_h743_model_read_word(uint32_t address,uint32_t *value) {
    CHECK(!active && address>=SCENES_FLASH_H743_BASE && !(address&3u));++reads;
    unsigned relative=address-SCENES_FLASH_H743_BASE,s=relative/SECTOR,at=relative%SECTOR;
    CHECK(s<2 && at+4<=SECTOR);
    if(read_error)return SCENE_JOURNAL_READ_ERROR;
    if(poison[s][at/WORD])return SCENE_JOURNAL_READ_CORRUPT;
    memcpy(value,flash[s]+at,4);return SCENE_JOURNAL_READ_OK;
}
static void program_bytes(unsigned n) {
    CHECK(active && !erasing && !memcmp(borrowed,word_copy,WORD));
    written[target_sector][target_offset/WORD]=1;
    for(unsigned i=0;i<n;++i)flash[target_sector][target_offset+i]&=word_copy[i];
    poison[target_sector][target_offset/WORD]=(uint8_t)(n!=WORD);
}
void scenes_flash_h743_model_dispatch_irq(void) {
    if(!active)return;
    if(!erasing)CHECK(!memcmp(borrowed,word_copy,WORD));
    if(latency){--latency;return;}
    if(erasing) {
        memset(flash[target_sector],255,SECTOR);memset(written[target_sector],0,SECTOR/WORD);
        memset(poison[target_sector],0,SECTOR/WORD);
    } else program_bytes(program_error ? 17 : WORD);
    active=false;
    if(program_error){program_error=false;scenes_flash_h743_async_error(0);}
    else scenes_flash_h743_async_complete(erasing ? UINT32_MAX :
        SCENES_FLASH_H743_BASE+target_sector*SECTOR+target_offset);
}
static bool loop_note(uint8_t cell,bool upper,bool on,float velocity,uint32_t ms) {
    (void)cell;(void)upper;(void)on;(void)velocity;(void)ms;++loop_notes;return true;
}
static void note(int on,uint8_t owner,float hz,float velocity) {
    CHECK(note_count<128);note_t *n=&notes[note_count++];memset(n,0,sizeof *n);
    n->ms=now;n->on=on;n->owner=owner;n->hz=hz;n->velocity=velocity;
}
static void control_state(int world,int key,unsigned seed) {
    menu_state_t m={.world=(uint8_t)world,.key_pc=(uint8_t)key,.space=100,
                   .motion=50,.fx=1,.attack=100,.release=100};
    menu_apply_state(&m);engine_set_gen_seed(seed);params_set_bright(800);
}
static void boot(void) {
    /* Power loss discards all volatile owners and unfinished IRQs, never flash. */
    active=read_error=program_error=false;borrowed=NULL;
    memset(&storage_adapter,0,sizeof storage_adapter);
    configured=available=false;writer=NULL;recovery=NULL;snapshot=NULL;length=phase=0;
    info=(scenes_save_info_t){.slot=-1};ticks=0;loop_notes=0;
    engine_init();engine_boot_mute();controls_init();gesture_init_owned(loop_note);
    menu_callbacks_t cb={.set_world=engine_set_world,.set_key=engine_set_key_pc,.set_tuning=engine_set_tuning,
        .set_space=engine_set_space,.set_atmosphere=engine_set_atmosphere,.set_motion=engine_set_motion,
        .set_fx=engine_set_fx_mode,.set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb);params_init();scenes_init(scenes_flash_write,scenes_flash_read);
    CHECK(scenes_storage_h743_bind());
    CHECK(gesture_state()==GESTURE_IDLE && !gesture_count() && !loop_notes && scenes_active()==-1);
}
static scenes_save_status_t step(void) {
    scenes_flash_h743_async_flash_irq();unsigned r=reads,s=starts;
    scenes_save_status_t status=scenes_save_step(++ticks);
    CHECK(reads-r<=8 && starts-s<=1 && !(reads!=r && starts!=s));return status;
}
static scenes_save_status_t finish(void) {
    for(unsigned i=0;i<20000;++i){scenes_save_status_t s=step();if(s!=SCENES_SAVE_BUSY)return s;}
    CHECK(false);return SCENES_SAVE_FAILED;
}
static void fresh(void) {
    memset(flash,255,sizeof flash);memset(written,0,sizeof written);memset(poison,0,sizeof poison);
    boot();CHECK(!scenes_used(0));
}
static void save_baseline(void) {
    memcpy(baseline,flash,sizeof flash);memcpy(baseline_written,written,sizeof written);
    memcpy(baseline_poison,poison,sizeof poison);
}
static void restore_baseline(void) {
    memcpy(flash,baseline,sizeof flash);memcpy(written,baseline_written,sizeof written);
    memcpy(poison,baseline_poison,sizeof poison);boot();
}
static void make_take(unsigned count) {
    gesture_clear(0);gesture_toggle(100);
    for(unsigned i=0;i<count;++i) {
        gesture_set_record_context((i/2)&1u,.25f+.25f*(float)((i/2)%3));
        gesture_record_cell((uint8_t)((i/2)%5),!(i&1u),100+5*i);
    }
    gesture_stop(100+5*count);CHECK(gesture_count()==(int)count && gesture_state()==GESTURE_IDLE);
}
static sound_parameters_t custom_effects(void) {
    sound_parameters_t p=sound_parameters_default();
    p.delay=.7f;p.grain=.6f;p.drive=.2f;p.crush=.1f;p.phaser=.5f;
    p.delay_left_ms=700;p.delay_right_ms=470;p.feedback=.47f;p.grain_ms=260;p.phaser_hz=.19f;
    p.freeze=true;p.plate=false;return p;
}
static void legacy_migration(void) {
    fresh();scn7_store_t old={.magic=0x53434e37u};
    old.slot[0]=(scn7_slot_t){.magic=0x53434e37u,.menu={.world=1,.key_pc=2,.fx=1,
        .space=75,.motion=50,.attack=100,.release=100},.collection=1,.bright_hz=-600,.gen_seed=42};
    old.checksum=sound_wire_crc((const uint8_t *)&old,offsetof(scn7_store_t,checksum));
    memcpy(flash[1],&old,sizeof old);
    for(unsigned i=0;i<(sizeof old+31)/32;++i)written[1][i]=1;
    unsigned before=starts;boot();CHECK(starts==before && scenes_used(0) && scenes_migrated(0));
    engine_set_master_volume(.31f);CHECK(scenes_recall(0,10));menu_state_t m;menu_get_state(&m);
    CHECK(m.world==1 && m.key_pc==2 && engine_gen_seed()==42 && params_bright_hz()==-600);
    sound_parameters_t p;engine_get_sound_parameters(&p);CHECK(p.feedback==.32f && p.plate && !p.freeze);
    CHECK(!gesture_count() && !loop_notes);save_baseline();
    /* The first new-format transaction must keep raw SCN7 in the other sector. */
    control_state(2,7,1234);make_take(4);CHECK(scenes_save_begin(1,20));
    CHECK(finish()==SCENES_SAVE_DONE && !memcmp(flash[1],&old,sizeof old));
    boot();CHECK(scenes_used(0) && scenes_used(1) && scenes_recall(1,30) && gesture_count()==4);
    /* Power loss during first payload never destroys the read-only legacy copy. */
    restore_baseline();control_state(2,7,1234);CHECK(scenes_save_begin(1,20));
    while(!active || erasing)CHECK(step()==SCENES_SAVE_BUSY);
    program_bytes(17);boot();CHECK(scenes_used(0) && !scenes_used(1) && scenes_migrated(0));
}
static void roundtrip(void) {
    fresh();control_state(1,7,1234);make_take(128);sound_parameters_t p=custom_effects();
    CHECK(engine_apply_sound_parameters(&p));uint8_t take[GESTURE_ARCHIVE_MAX],wire[SOUND_PARAMETERS_BYTES];
    CHECK(gesture_export(take,sizeof take)==sizeof take);p.freeze=false;CHECK(sound_parameters_encode(&p,wire));
    unsigned before=starts+reads;scenes_ui_open(10);scenes_ui_cell(4,true,10);
    CHECK(scenes_save_info().status==SCENES_SAVE_BUSY && starts+reads==before && !scenes_used(4));
    control_state(0,11,999);gesture_clear(0);p=sound_parameters_default();CHECK(engine_apply_sound_parameters(&p));
    CHECK(finish()==SCENES_SAVE_DONE && scenes_used(4) && scenes_active()==4);
    CHECK(scenes_recall(4,20) && gesture_count()==128 && gesture_state()==GESTURE_IDLE && !loop_notes);
    CHECK(!memcmp(s_store.slot[4].effects,wire,sizeof wire) &&
          !memcmp(s_store.slot[4].loop,take,sizeof take));
    engine_get_sound_parameters(&p);uint8_t actual[SOUND_PARAMETERS_BYTES];
    CHECK(sound_parameters_encode(&p,actual) && !memcmp(actual,wire,sizeof wire));
    boot();CHECK(scenes_used(4) && scenes_recall(4,30) && gesture_count()==128 && !loop_notes);
    CHECK(!memcmp(s_store.slot[4].loop,take,sizeof take));
    gesture_play(100);gesture_tick(100);CHECK(loop_notes==1);gesture_stop(101);CHECK(loop_notes==2);
    /* Unfinished recording rejects before IO and leaves the last confirmed slot. */
    gesture_clear(200);gesture_toggle(200);before=starts+reads;
    CHECK(!scenes_save_begin(0,201) && starts+reads==before && !scenes_used(0));
    gesture_stop(202);
    /* A partial program error and cancel both drain borrowed hardware ownership. */
    control_state(2,5,55);CHECK(scenes_save_begin(0,300));
    while(!active || erasing)CHECK(step()==SCENES_SAVE_BUSY);
    program_error=true;CHECK(finish()==SCENES_SAVE_FAILED && !active && !scenes_used(0));
    CHECK(scenes_save_begin(0,400) && finish()==SCENES_SAVE_DONE);
    CHECK(scenes_save_begin(1,500));while(!active || erasing)CHECK(step()==SCENES_SAVE_BUSY);
    CHECK(scenes_save_cancel() && finish()==SCENES_SAVE_FAILED && !active && !scenes_used(1));
    CHECK(scenes_save_begin(1,600) && finish()==SCENES_SAVE_DONE);
    /* Fill and roll over the real 128KiB sectors, preserving all five slots. */
    for(unsigned i=0;i<34;++i) {
        control_state((int)(i%3),(int)(i%12),1000+i);
        CHECK(scenes_save_begin((int)(i%5),1000+i) && finish()==SCENES_SAVE_DONE);
    }
    CHECK(erases>=3);boot();for(int i=0;i<5;++i)CHECK(scenes_used(i) && scenes_recall(i,2000));
}
static void power_cuts(void) {
    fresh();control_state(0,2,42);make_take(4);
    CHECK(scenes_save_begin(0,0) && finish()==SCENES_SAVE_DONE);save_baseline();
    control_state(2,7,1234);make_take(128);CHECK(scenes_save_begin(1,1));
    unsigned count=0;
    while(scenes_save_info().status==SCENES_SAVE_BUSY) {
        if(active && !erasing && latency==2)++count;
        step();
    }
    CHECK(count==(sizeof(scene_store_t)+31)/32+1);
    static const unsigned widths[]={0,1,7,16,31,32};
    unsigned new_commits=0;
    for(unsigned word=1;word<=count;++word)for(unsigned width=0;width<6;++width) {
        restore_baseline();control_state(2,7,1234);make_take(128);
        CHECK(scenes_save_begin(1,1));unsigned seen=0;
        for(;;) {
            if(active && !erasing && latency==2 && ++seen==word)break;
            CHECK(step()==SCENES_SAVE_BUSY);
        }
        /* On an unacknowledged final header, a complete commit MAY recover. */
        program_bytes(widths[width]);boot();CHECK(scenes_used(0) && scenes_recall(0,10));
        CHECK(engine_gen_seed()==42 && gesture_count()==4 && !loop_notes);
        if(scenes_used(1)) {
            CHECK(word==count && widths[width]==32 && scenes_recall(1,11));
            CHECK(engine_gen_seed()==1234 && gesture_count()==128);++new_commits;
        }
        CHECK(scenes_save_begin(2,12) && finish()==SCENES_SAVE_DONE);++cuts;
    }
    CHECK(new_commits==1);
}
static void fail_closed(void) {
    fresh();control_state(1,2,42);CHECK(scenes_save_begin(0,0) && finish()==SCENES_SAVE_DONE);
    control_state(1,7,99);CHECK(scenes_save_begin(1,1) && finish()==SCENES_SAVE_DONE);
    unsigned at=storage_journal.slot*SCENE_JOURNAL_RECORD_BYTES;
    /* A CRC-damaged newest record falls back to the older committed store. */
    flash[storage_journal.sector][at+32+100]^=1;boot();CHECK(scenes_used(0) && !scenes_used(1));
    /* Correctly checksummed but semantically invalid FX never gets applied. */
    scene_store_t bad=s_store;bad.slot[0].effects[0]^=1;bad.checksum=store_checksum(&bad);
    uint8_t *header=flash[storage_journal.sector]+storage_journal.slot*SCENE_JOURNAL_RECORD_BYTES;
    memcpy(header+32,&bad,sizeof bad);sound_wire_put32(header+16,sound_wire_crc((const uint8_t *)&bad,sizeof bad));
    sound_wire_put32(header+28,sound_wire_crc(header,28));boot();CHECK(!scenes_used(0));
    /* A valid incompatible JSC1 journal blocks writes instead of being erased. */
    memset(flash,255,sizeof flash);uint8_t *old=flash[0];
    sound_wire_put32(old,0x3143534au);sound_wire_put32(old+4,1);sound_wire_put32(old+12,368);
    sound_wire_put32(old+28,sound_wire_crc(old,28));
    active=false;memset(&storage_adapter,0,sizeof storage_adapter);configured=available=false;
    writer=NULL;recovery=NULL;info=(scenes_save_info_t){.slot=-1};
    unsigned before=starts;scenes_init(scenes_flash_write,scenes_flash_read);
    CHECK(!scenes_storage_h743_bind() && !scenes_save_begin(0,0) && starts==before);
}
static void capture(int world,int block,bool saving,int16_t *out) {
    fresh();control_state(world,7,1234);engine_set_master_volume(.6f);
    sound_parameters_t p=custom_effects();p.freeze=false;p.plate=true;
    CHECK(engine_apply_sound_parameters(&p));note_count=0;memset(notes,0,sizeof notes);
    engine_set_note_hook(note);engine_set_generative(true,-1);bool complete=false;
    for(now=0;now<12000;now+=10) {
        engine_generative_tick(now);
        if(saving && now==1500)CHECK(scenes_save_begin(1,now));
        /* Main services inputs/Scene steps much faster than the 10ms music
         * planner. Run the real asynchronous state machine on that same grid. */
        for(unsigned service=0;service<10;++service) {
            scenes_save_status_t s=step();if(s==SCENES_SAVE_DONE)complete=true;
        }
        for(int at=0;at<441;) {
            int n=block<441-at ? block : 441-at;
            engine_render(out+2*(now*44100/1000+at),n);at+=n;
        }
    }
    CHECK(!saving || complete);CHECK(!engine_nonfinite_samples() && !engine_output_limited_samples());
    int peak=0;for(unsigned i=0;i<2*FRAMES;++i)if(abs(out[i])>peak)peak=abs(out[i]);CHECK(peak>0);
}
int main(int argc,char **argv) {
    CHECK(sizeof(scene_slot_t)==1168 && sizeof(scene_store_t)==5848);
    legacy_migration();roundtrip();power_cuts();fail_closed();
    printf("{\"category\":\"session\",\"status\":\"passed\",\"checks\":%u,\"power_cuts\":%u,\"store_bytes\":%zu,\"slot_bytes\":%zu,\"max_events\":128,\"max_read_per_live_step\":32,\"max_start_per_live_step\":1}\n",checks,cuts,sizeof(scene_store_t),sizeof(scene_slot_t));
    if(argc==2 && !strcmp(argv[1],"--model"))return 0;
    for(int world=0;world<3;++world) {
        capture(world,512,false,reference);saved_note_count=note_count;memcpy(saved_notes,notes,sizeof notes);
        for(unsigned cap=0;cap<2;++cap) {
            capture(world,cap ? 512 : 37,true,trial);
            CHECK(!memcmp(reference,trial,sizeof reference) && saved_note_count==note_count &&
                  !memcmp(saved_notes,notes,note_count*sizeof *notes));
        }
    }
    puts("{\"category\":\"pcm\",\"status\":\"passed\",\"worlds\":3,\"capture_seconds\":12,\"captures\":9,\"stereo_frames\":4762800,\"save_vs_no_save_exact\":true,\"partitions_37_512_exact\":true,\"planner_hook_traces_exact\":true}");
}
