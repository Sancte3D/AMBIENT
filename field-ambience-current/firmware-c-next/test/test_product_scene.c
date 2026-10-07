/* Real product sources plus actual menu, controls and persisted Scene wire. */
#include "engine.h"
#include "engine_product.h"
#include "menu.h"
#include "worlds.h"
#include "world_grammar.h"
#include "params.h"
#include "scenes.h"
#include "controls.h"
#include "cells.h"
#include "tuning.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
static unsigned char blob[512]; static unsigned len,writes; static bool fail_write;
static bool write_blob(const void *p,unsigned n) {
    if(fail_write) return false;
    assert(n<=sizeof blob); memcpy(blob,p,n); len=n; ++writes; return true;
}
static bool read_blob(void *p,unsigned n) {
    if(!len) return false;
    memset(p,0,n); memcpy(p,blob,n<len ? n : len); return true;
}
static void tick(void) { static int16_t b[1024]; engine_render(b,512); engine_generative_tick(12); }
static void setup(void) {
    engine_init(); params_init(); controls_init();
    menu_callbacks_t cb={.set_world=engine_set_world,.set_key=engine_set_key_pc,
        .set_tuning=engine_set_tuning,.set_space=engine_set_room,.set_atmosphere=engine_set_nature,
        .set_motion=engine_set_activity,.set_fx=engine_set_fx_mode,
        .set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb);
}
typedef struct { uint32_t magic; menu_state_t menu; int8_t drive; int16_t bright; uint32_t seed; } old6_slot;
typedef struct { uint32_t magic; old6_slot slot[5]; } old6_store;
typedef struct { uint8_t v[16]; uint16_t locks; } old5_menu;
typedef struct { uint32_t magic; old5_menu menu; int8_t drive; int16_t bright; uint32_t seed; } old5_slot;
typedef struct { uint32_t magic; old5_slot slot[5]; } old5_store;
_Static_assert(sizeof(old6_slot)==72 && sizeof(old5_slot)==32,"Fixture ABI");
int main(void) {
    setup();
    assert(worlds_count()==3 && engine_fx_mode_count()==2 && engine_fx_mode()==1);
    const menu_param_t available[]={MP_WORLD,MP_KEY,MP_TUNING,MP_SPACE,MP_ATMOS,MP_MOTION,MP_FX,MP_ATTACK,MP_RELEASE};
    for(int i=0;i<9;++i) { assert(menu_current()==available[i]); menu_rotate(1); }
    assert(menu_current()==MP_WORLD && !menu_toggle_lock_current());
    menu_state_t st={.key_pc=8,.tuning=1,.space=78,.atmos=0,.motion=23,.fx=0,.attack=32,.release=71};
    menu_apply_state(&st); params_set_bright(320);
    menu_push(); menu_rotate(1); menu_push(); /* Change only the World. */
    menu_state_t after; menu_get_state(&after);
    assert(after.world==1 && after.key_pc==8 && after.space==78 && after.fx==0 && after.motion==23);
    assert(params_bright_hz()==320);
    params_encoder(0,INT_MIN,0); params_encoder(255,INT_MAX,0);
    params_set_bright(NAN); params_apply_scene(100,INFINITY); assert(params_bright_hz()==320);
    assert(params_drive_pct()==0);
    for(int key=0;key<12;++key) for(int minor=0;minor<2;++minor) {
        engine_set_key_pc(key); engine_set_mode(minor);
        for(int c=0;c<5;++c) for(int upper=0;upper<2;++upper) {
            int m=engine_product_cell_midi(c,upper); float hz=tuning_hz((float)m);
            assert(m>=50 && m<=69 && hz>=140 && hz<=470);
        }
    }
    assert(engine_product_cell_midi(5,false)==-1);
    setup(); controls_modifier(MOD_HOLD,true);
    controls_cell_press(0,CELL_AMP_MAX); controls_cell_release(0); tick();
    assert(controls_hold_base(0) && engine_active_voices()==1);
    float original[24]; assert(engine_sounding_frequencies(original,24)==1);
    engine_set_world(1); engine_set_key_pc(5); controls_refresh_held_pitches(); tick();
    float held[24]; assert(engine_sounding_frequencies(held,24)==1 && held[0]==original[0]);
    assert(controls_hold_base(0));
    controls_cell_press(0,CELL_AMP_MAX); controls_cell_release(0);
    assert(!controls_hold_base(0));
    engine_all_off(); for(int i=0;i<12;++i) tick();

    /* All old IDs are explicit approximations; no old backend can return. */
    setup();int saved_volume=params_volume_pct();
    engine_set_nature(0);assert(engine_try_note_on(0,220,.7f));tick();
    assert(params_toggle_mute()==1 && params_muted() && engine_muted());
    for(int i=0;i<12;++i)tick();
    assert(!engine_clear_pending() && engine_active_voices()==0);
    assert(params_volume_pct()==saved_volume);
    assert(params_toggle_mute()==0 && !engine_muted() && !params_muted());
    static int16_t silent[1024];
    engine_render(silent,512);for(int i=0;i<1024;++i)assert(silent[i]==0);
    assert(engine_try_note_on(0,220,.7f));tick();
    (void)params_toggle_mute();for(int i=0;i<12;++i)tick();
    params_encoder(PARAM_ENC_VOLUME,1,1); /* turning explicitly unmutes */
    assert(!params_muted() && !engine_muted() && params_volume_pct()>saved_volume);
    engine_all_off();for(int i=0;i<12;++i)tick();
    setup();

    old6_store old6={0}; old6.magic=0x53434e36u;
    const int map[5]={2,0,0,1,1};
    for(int i=0;i<5;++i) {
        old6.slot[i].magic=old6.magic;
        old6.slot[i].menu=(menu_state_t){.world=(uint8_t)i,.key_pc=2,.tuning=1,
            .synth=6,.voice=6,.cell=2,.bass=3,.fx=8,.space=68,.atmos=90,
            .motion=90,.shimmer=90,.age=90,.echo=90,.blur=90,.locks=0xffff,
            .reso=90,.attack=50,.release=60,.sweep=90,.envmod=90};
        old6.slot[i].bright=-220; old6.slot[i].drive=90; old6.slot[i].seed=0x12340000u+(unsigned)i;
    }
    memcpy(blob,&old6,sizeof old6); len=sizeof old6;
    unsigned before=writes; int volume=params_volume_pct();
    scenes_init(write_blob,read_blob);
    assert(writes==before && engine_active_voices()==0);
    for(int i=0;i<5;++i) {
        assert(scenes_used(i) && scenes_migrated(i) && scenes_recall(i,100));
        menu_get_state(&after);
        assert(after.world==map[i] && after.synth==0 && after.cell==0 && after.bass==0);
        assert(after.fx==1 && after.atmos==0 && after.motion==50 && after.locks==0);
        assert(after.shimmer==0 && after.age==0 && after.echo==0 && after.blur==0);
        assert(engine_product_collection()==(i>=2) && engine_gen_seed()==0x12340000u+(unsigned)i);
        assert(engine_synth()==0 && !engine_bass_active() && engine_active_voices()==0);
        assert(params_volume_pct()==volume && params_bright_hz()==-220);
    }
    engine_set_mode(1); engine_set_gen_seed(0xabcdu); params_set_bright(240);
    assert(scenes_save(2,500)); assert(!scenes_migrated(2));
    uint32_t magic; memcpy(&magic,blob,4); assert(magic==0x53434e37u && len==368);
    scenes_init(write_blob,read_blob);
    assert(scenes_migrated(0) && !scenes_migrated(2));
    engine_set_mode(0); params_set_bright(-300); assert(scenes_recall(2,600));
    assert(engine_product_collection()==1 && params_bright_hz()==240 && engine_gen_seed()==0xabcdu);
    fail_write=true; int active=scenes_active(); params_set_bright(700);
    assert(!scenes_save(2,700) && scenes_active()==active);
    assert(scenes_recall(2,800) && params_bright_hz()==240);
    fail_write=false;
    blob[40]^=1; scenes_init(write_blob,read_blob);
    for(int i=0;i<5;++i) assert(!scenes_used(i)); /* corrupt/power-interrupted store */

    old5_store old5={0}; old5.magic=old5.slot[1].magic=0x53434e35u;
    old5.slot[1].menu.v[0]=3; old5.slot[1].menu.v[1]=255; old5.slot[1].menu.v[8]=0;
    old5.slot[1].bright=2000; old5.slot[1].seed=99;
    memcpy(blob,&old5,sizeof old5); len=sizeof old5;
    scenes_init(write_blob,read_blob);
    assert(scenes_used(1) && scenes_migrated(1) && !scenes_used(0));
    assert(scenes_recall(1,900)); menu_get_state(&after);
    assert(after.world==1 && after.key_pc==2 && after.fx==0 && after.attack==50 && after.release==50);
    assert(params_bright_hz()==800 && engine_gen_seed()==99 && params_volume_pct()==volume);
    puts("PRODUCT SCENE PASS: SCN5/6/7 migration, CRC, failed Save rollback, shared World parameters, bounded cells and held ownership");
    return 0;
}
