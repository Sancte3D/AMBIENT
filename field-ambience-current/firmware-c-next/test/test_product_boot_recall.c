/* SD41: actual Product PCM/owners through the real Scene/menu/controls wire.
 * Same pre-DMA boot order as H743, not a HAL/flash/IRQ simulator. */
#include "engine.h"
#include "engine_product.h"
#include "menu.h"
#include "params.h"
#include "controls.h"
#include "scenes.h"
#include "cells.h"
#include "tuning.h"
#include "ambient_room.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SR=44100, TICK=441, CAPTURE=8*SR };
#define DEFAULT_SEED UINT32_C(0xA6B13E7D)
typedef struct { menu_state_t menu; int minor,bright; uint32_t seed; } preset_t;
typedef struct { uint32_t ms; int on,family; uint8_t owner; float hz,velocity; } event_t;
static uint8_t blob[512];
static unsigned blob_len,writes; static bool reject_write;
static int16_t reference[CAPTURE*2],trial[CAPTURE*2],buffer[1024];
static event_t events[128],saved_events[128];
static unsigned event_count,saved_event_count,ons,offs,hook_ms;
static uint64_t rendered_frames;
static unsigned boot_cases,fallback_cases,legacy_cases,off_cases,manual_pairs,
    failed_save_pairs,seed_pairs,seed_wrap_pairs,pending_cases,live_cases,gate_cases;
static uint16_t mask(int family) {
    return family==0 ? bowed_active_sources() : family==1 ? pluck_active_sources() : horn_active_sources();
}
static void physical_budget(void) {
    uint16_t a=mask(0),b=mask(1),c=mask(2);
    assert(!(a&b) && !(a&c) && !(b&c));
    assert(bowed_active_count()+pluck_active_count()+horn_active_count()<=3);
    assert(pluck_active_count()<=2);
}
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(event_count<128);
    event_t *e=&events[event_count++]; memset(e,0,sizeof *e);
    e->ms=hook_ms; e->on=on; e->owner=owner; e->hz=hz; e->velocity=velocity; e->family=-1;
    for(int f=0;f<3;++f) if(mask(f)&(1u<<owner)) e->family=f;
    if(on>0) { ++ons; assert(e->family>=0); }
    if(on==0) ++offs;
}
static void reset_events(void) { memset(events,0,sizeof events); event_count=ons=offs=0; }
static bool write_blob(const void *p,unsigned n) {
    if(reject_write) return false;
    assert(n<=sizeof blob); memcpy(blob,p,n); blob_len=n; ++writes; return true;
}
static bool read_blob(void *p,unsigned n) {
    if(!blob_len) return false;
    memset(p,0xff,n); memcpy(p,blob,n<blob_len ? n : blob_len); return true;
}
static void cb_world(int w) { engine_set_world(w); controls_refresh_held_pitches(); }
static void cb_key(int k) { engine_set_key_pc(k); controls_refresh_held_pitches(); }
static void boot(void) {
    unsigned before=writes;
    engine_init(); engine_boot_mute(); reset_events(); hook_ms=0; engine_set_note_hook(hook);
    menu_callbacks_t cb={.set_world=cb_world,.set_key=cb_key,.set_tuning=engine_set_tuning,
        .set_space=engine_set_space,.set_atmosphere=engine_set_atmosphere,
        .set_motion=engine_set_motion,.set_fx=engine_set_fx_mode,
        .set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb); controls_init(); params_init(); scenes_init(write_blob,read_blob);
    assert(writes==before && scenes_active()==-1 && !scenes_ui_active());
    assert(params_volume_pct()==30 && !params_muted() && !engine_muted());
    assert(engine_active_voices()==0 && ambient_room_peak()==0 && engine_generative_advance()==-1);
    for(int i=0;i<5;++i) assert(!controls_hold_base(i) && !controls_hold_shift(i));
    assert(!controls_modifier_active(MOD_GENERATE));
}
static void volume(int percent) {
    params_encoder(PARAM_ENC_VOLUME,-100,1000);
    params_encoder(PARAM_ENC_VOLUME,percent,2000);
    assert(params_volume_pct()==percent);
}
static preset_t preset(int world,int key,int minor,int just,uint32_t seed,int nature) {
    return (preset_t){.menu={.world=(uint8_t)world,.key_pc=(uint8_t)key,
        .tuning=(uint8_t)just,.space=100,.atmos=(uint8_t)nature,.motion=50,
        .fx=1,.attack=100,.release=100},.minor=minor,.bright=800,.seed=seed};
}
static void apply(const preset_t *p) {
    engine_set_mode(p->minor); menu_apply_state(&p->menu);
    params_set_bright((float)p->bright); engine_set_gen_seed(p->seed);
}
static void stored_at(const preset_t *p,int slot) {
    blob_len=0; reject_write=false; boot(); apply(p); volume(100);
    /* Generate intent is active at Save, but never enters the Scene wire. */
    controls_modifier(MOD_GENERATE,true); assert(scenes_save(slot,0)); assert(blob_len==368);
}
static void stored(const preset_t *p) { stored_at(p,0); }
static void render_only(unsigned ms,int block) {
    int left=(int)((uint64_t)ms*SR/1000);
    while(left) { int n=left<block ? left : block; engine_render(buffer,n); physical_budget(); rendered_frames+=(unsigned)n; left-=n; }
    (void)engine_active_voices();
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
typedef void (*action_t)(unsigned at);
static void capture(unsigned ms,int block,uint32_t clock_base,int16_t *out,action_t action) {
    assert(ms%10==0 && (!out || ms<=8000));
    for(unsigned at=0;at<ms;at+=10) {
        hook_ms=at; engine_generative_tick(clock_base+at);
        if(action) action(at);
        for(int done=0;done<TICK;) {
            int n=block<TICK-done ? block : TICK-done;
            engine_render(out ? out+2*((uint64_t)at*SR/1000+done) : buffer,n);
            physical_budget(); rendered_frames+=(unsigned)n; done+=n;
        }
    }
    hook_ms=ms; engine_generative_tick(clock_base+ms);
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
static int peak(const int16_t *p,unsigned frames) {
    int v=0; for(unsigned i=0;i<2*frames;++i) { int a=abs(p[i]); if(a>v) v=a; } return v;
}
static void silent(unsigned ms) {
    capture(ms,512,0,trial,NULL); assert(peak(trial,ms*SR/1000)==0);
    assert(ons==0 && engine_active_voices()==0 && engine_generative_advance()==-1);
}
static void warm_boot(void) {
    for(int world=0;world<3;++world) for(int gen=0;gen<2;++gen) {
        preset_t p=preset(world,2,0,0,1234,100); stored(&p); boot(); apply(&p); volume(100);
        if(gen) controls_modifier(MOD_GENERATE,true);
        else { controls_modifier(MOD_HOLD,true); controls_cell_press(0,.75f*CELL_AMP_MAX); controls_cell_release(0); }
        capture(1500,512,0,trial,NULL);
        assert(peak(trial,1500*SR/1000)>0 && engine_active_voices()>0 && ambient_room_peak()>0);
        assert(scenes_save(0,1500)); unsigned before=writes;
        boot(); assert(scenes_used(0) && writes==before); silent(1000); ++boot_cases;
    }
}
static void empty_fallback(void) {
    boot(); for(int i=0;i<5;++i) assert(!scenes_used(i)); silent(20); ++fallback_cases;
}
static void invalid_stores(void) {
    preset_t p=preset(1,9,1,1,42,100); stored(&p);
    uint8_t original[368]; memcpy(original,blob,sizeof original);
    /* Every byte covered by native CRC, with non-legacy bad header values.
     * SCN5/6 have no CRC; deliberately valid legacy magic is a separate path. */
    for(unsigned i=0;i<368;++i) {
        memcpy(blob,original,368); blob_len=368; blob[i]^=i<4 ? 0x80 : 1; empty_fallback();
    }
    const unsigned lengths[]={0,1,31,32,163,164,363,364,367};
    for(unsigned i=0;i<sizeof lengths/sizeof *lengths;++i) {
        memcpy(blob,original,368); blob_len=lengths[i]; empty_fallback();
    }
    memset(blob,0xff,sizeof blob); blob_len=512; empty_fallback();
    memset(blob,0,sizeof blob); empty_fallback();
    memcpy(blob,original,368); blob[0]=0x38; blob_len=368; empty_fallback();
}
typedef struct { uint8_t values[16]; uint16_t locks; } old_menu_t;
typedef struct { uint32_t magic; old_menu_t menu; int8_t drive; int16_t bright; uint32_t seed; } old5_slot_t;
typedef struct { uint32_t magic; old5_slot_t slot[5]; } old5_store_t;
typedef struct { uint32_t magic; menu_state_t menu; int8_t drive; int16_t bright; uint32_t seed; } old6_slot_t;
typedef struct { uint32_t magic; old6_slot_t slot[5]; } old6_store_t;
_Static_assert(sizeof(old5_slot_t)==32 && sizeof(old6_slot_t)==72,"legacy wire");
static void legacy(void) {
    static const int world_map[5]={2,0,0,1,1};
    for(int version=5;version<=6;++version) {
        old5_store_t a={0}; old6_store_t b={0}; a.magic=0x53434e35u; b.magic=0x53434e36u;
        for(int i=0;i<5;++i) {
            a.slot[i].magic=a.magic; a.slot[i].menu.values[0]=(uint8_t)i;
            a.slot[i].menu.values[1]=255; a.slot[i].menu.values[8]=8;
            a.slot[i].drive=99; a.slot[i].bright=2000;
            b.slot[i].magic=b.magic; memset(&b.slot[i].menu,255,sizeof b.slot[i].menu);
            b.slot[i].menu.world=(uint8_t)i; b.slot[i].drive=99; b.slot[i].bright=2000;
        }
        memset(blob,0xff,sizeof blob); blob_len=version==5 ? sizeof a : sizeof b;
        memcpy(blob,version==5 ? (void *)&a : (void *)&b,blob_len);
        unsigned before=writes; boot(); silent(1000); assert(writes==before);
        for(int i=0;i<5;++i) {
            assert(scenes_used(i) && scenes_migrated(i) && scenes_recall(i,1000));
            menu_state_t m; menu_get_state(&m);
            assert(m.world==world_map[i] && m.key_pc==2 && m.atmos==0 && m.motion==50);
            assert(m.fx==1 && !m.synth && !m.bass && !m.voice && !m.cell && !m.locks);
            assert(params_volume_pct()==30 && params_bright_hz()==800 && params_drive_pct()==0);
            assert(engine_gen_seed()==DEFAULT_SEED && !engine_bass_active());
            silent(1000); ++legacy_cases;
        }
        assert(writes==before);
    }
}
static void recall_off(void) {
    for(int slot=0;slot<5;++slot) for(int setting=0;setting<4;++setting) {
        preset_t p=preset(slot%3,(slot*5+2)%12,slot%2,slot%2,42+slot,slot%2 ? 100 : 0);
        stored_at(&p,slot); boot();
        const int levels[4]={0,30,100,70}; volume(levels[setting]);
        if(setting==3) { assert(params_toggle_mute()==1); render_only(60,512); }
        assert(scenes_recall(slot,60)); menu_state_t m; menu_get_state(&m);
        assert(m.world==p.menu.world && m.key_pc==p.menu.key_pc && m.atmos==p.menu.atmos);
        assert(engine_product_collection()==p.minor && tuning_mode()==p.menu.tuning);
        assert(engine_gen_seed()==p.seed && params_volume_pct()==levels[setting]);
        assert(params_muted()==(setting==3) && engine_muted()==(setting==3));
        assert(engine_generative_advance()==-1 && !controls_modifier_active(MOD_GENERATE));
        capture(8000,512,0,trial,NULL);
        assert(ons==0 && engine_active_voices()==0 && engine_generative_melody_count()==0);
        if(!p.menu.atmos || setting==0 || setting==3) assert(peak(trial,CAPTURE)==0);
        else assert(peak(trial,CAPTURE)>0); /* explicitly recalled optional Nature, no phantom tone */
        ++off_cases;
    }
}
static int manual_from,manual_to,manual_operation; static bool manual_releasing;
static void key_up(void) {
    if(controls_hold_base(0)) { controls_cell_press(0,.75f*CELL_AMP_MAX); controls_cell_release(0); }
    engine_note_off(1); engine_note_off(2);
}
static void manual_action(unsigned at) {
    if(at==2600 && manual_releasing) key_up();
    if(at==2700 && manual_operation) {
        float before[24],after[24]; int count=engine_sounding_frequencies(before,24);
        int level=params_volume_pct(),active=scenes_active();
        unsigned written=writes; uint8_t previous[512]; memcpy(previous,blob,sizeof blob);
        if(manual_operation==1) {
            assert(scenes_recall(0,2700)); assert(engine_product_world()==manual_to);
            assert(controls_hold_base(0)==!manual_releasing && engine_generative_advance()==-1);
        } else {
            reject_write=true; assert(!scenes_save(0,2700)); reject_write=false;
            assert(writes==written && scenes_active()==active && !memcmp(blob,previous,sizeof blob));
        }
        assert(engine_sounding_frequencies(after,24)==count && !memcmp(before,after,count*sizeof *before));
        assert(params_volume_pct()==level && mask(manual_from)==(manual_from==1 ? 3 : 7));
    }
    if(at==4000) key_up();
}
static void manual_capture(int from,int to,bool releasing,int operation,int block,int16_t *out) {
    preset_t target=preset(to,9,1,1,42,0); stored(&target); boot();
    preset_t old=preset(from,2,0,0,1234,0); apply(&old); volume(70);
    controls_modifier(MOD_HOLD,true); controls_cell_press(0,.75f*CELL_AMP_MAX); controls_cell_release(0);
    assert(controls_hold_base(0)); assert(engine_try_note_on(1,tuning_hz(57),.75f));
    if(from!=1) assert(engine_try_note_on(2,tuning_hz(62),.75f));
    manual_from=from; manual_to=to; manual_releasing=releasing; manual_operation=operation;
    capture(8000,block,0,out,manual_action);
    assert(ons==(unsigned)(from==1 ? 2 : 3) && !controls_hold_base(0));
}
static void save_events(void) { saved_event_count=event_count; memcpy(saved_events,events,sizeof events); }
static void equal_capture(void) {
    assert(!memcmp(reference,trial,sizeof reference));
    assert(event_count==saved_event_count && !memcmp(events,saved_events,event_count*sizeof *events));
}
static void manual(void) {
    for(int from=0;from<3;++from) for(int to=0;to<3;++to) if(from!=to) for(int release=0;release<2;++release) {
        manual_capture(from,to,release,0,512,reference); save_events();
        manual_capture(from,to,release,1,512,trial); equal_capture();
        manual_capture(from,to,release,1,64,trial); equal_capture(); ++manual_pairs;
    }
    for(int from=0;from<3;++from) {
        manual_capture(from,from,false,0,512,reference); save_events();
        manual_capture(from,from,false,2,512,trial); equal_capture();
        manual_capture(from,from,false,2,64,trial); equal_capture();
        engine_all_off(); render_only(80,512); assert(scenes_recall(0,8100));
        menu_state_t m; menu_get_state(&m); assert(m.key_pc==9 && m.tuning==1 && engine_gen_seed()==42);
        assert(params_volume_pct()==70); ++failed_save_pairs;
    }
}
static uint64_t digest(const void *p,size_t n) {
    uint64_t h=UINT64_C(14695981039346656037); const uint8_t *b=p;
    for(size_t i=0;i<n;++i) h=(h^b[i])*UINT64_C(1099511628211);
    return h;
}
static void opening(const preset_t *p,bool recall,int block,uint32_t base,int16_t *out) {
    boot(); engine_generative_tick(base);
    if(recall) assert(scenes_recall(0,base)); else apply(p);
    assert(engine_gen_seed()==(p->seed ? p->seed : DEFAULT_SEED));
    controls_modifier(MOD_GENERATE,true); capture(8000,block,base,out,NULL);
    assert(ons>0 && peak(out,CAPTURE)>0);
}
static void seeds(void) {
    const uint32_t values[]={0,42,0xffffffffu};
    for(int world=0;world<3;++world) for(int i=0;i<3;++i) {
        preset_t p=preset(world,2,0,0,values[i],0); stored(&p);
        opening(&p,false,512,0,reference); save_events();
        opening(&p,true,512,0,trial); equal_capture();
        opening(&p,true,64,0,trial); equal_capture(); ++seed_pairs;
        opening(&p,true,512,0xfffff000u,trial); equal_capture(); ++seed_wrap_pairs;
        printf("{\"category\":\"seed_opening\",\"world\":%d,\"seed\":%" PRIu32 ",\"notes\":%u,\"whole_pcm_fnv1a64\":\"%016" PRIx64 "\",\"whole_event_fnv1a64\":\"%016" PRIx64 "\"}\n",
            world,engine_gen_seed(),ons,digest(reference,sizeof reference),digest(saved_events,saved_event_count*sizeof *saved_events));
    }
}
static float old_hz[24]; static int old_hz_count;
static void live_action(unsigned at) {
    if(at==100) {
        assert((mask(0)|mask(1)|mask(2))==0 && ons==0 && ambient_room_peak()>0);
        float history[24]; int count=engine_sounding_frequencies(history,24);
        for(int i=0;i<old_hz_count;++i) {
            bool found=false; for(int j=0;j<count;++j) if(old_hz[i]==history[j]) found=true;
            assert(found);
        }
    }
}
static void live(void) {
    for(int world=0;world<3;++world) {
        preset_t target=preset(world,2,0,0,42,0),old=preset(world,2,0,0,1234,0);
        stored(&target); boot(); apply(&old); controls_modifier(MOD_GENERATE,true);
        engine_generative_tick(0); assert(engine_active_voices()>0 && ons==0);
        assert(scenes_recall(0,0)); render_only(120,512);
        assert(ons==0 && offs==0 && engine_active_voices()==0 && engine_generative_melody_count()==0);
        capture(1000,512,120,trial,NULL); assert(ons>0 && engine_gen_seed()==42); ++pending_cases;

        boot(); apply(&old); controls_modifier(MOD_GENERATE,true); capture(2700,512,0,trial,NULL);
        assert(engine_active_voices()==(world==1 ? 2 : 3)); reset_events();
        old_hz_count=engine_sounding_frequencies(old_hz,24);
        float wet_before=ambient_room_peak(); assert(wet_before>0);
        assert(scenes_recall(0,2700)); assert(ambient_room_peak()==wet_before);
        capture(1000,512,2700,trial,live_action);
        assert(ons>0 && engine_gen_seed()==42 && engine_generative_advance()>0);
        for(unsigned i=0;i<event_count;++i) if(events[i].on>0) assert(events[i].ms>=100 && events[i].family==world);
        ++live_cases;

        boot(); apply(&old); controls_modifier(MOD_GENERATE,true); engine_set_autoplay_melody(0);
        assert(scenes_recall(0,0)); capture(1000,512,0,trial,NULL);
        assert(ons==0 && peak(trial,SR)==0 && !engine_autoplay_melody());
        engine_set_autoplay_melody(1); capture(1000,512,1000,trial,NULL); assert(ons>0); ++gate_cases;
    }
}
int main(void) {
    warm_boot(); invalid_stores(); legacy(); recall_off(); manual(); seeds(); live();
    printf("{\"category\":\"summary\",\"status\":\"passed\",\"boot_restarts\":%u,\"bad_store_fallbacks\":%u,\"legacy_recalls\":%u,\"off_recalls\":%u,\"manual_pcm_triplets\":%u,\"failed_save_pcm_triplets\":%u,\"seed_pcm_triplets\":%u,\"seed_wrap_pairs\":%u,\"pending_cancellations\":%u,\"live_seed_recalls\":%u,\"autoplay_gates\":%u,\"rendered_stereo_frames\":%" PRIu64 "}\n",
        boot_cases,fallback_cases,legacy_cases,off_cases,manual_pairs,failed_save_pairs,seed_pairs,
        seed_wrap_pairs,pending_cases,live_cases,gate_cases,rendered_frames);
    return 0;
}
