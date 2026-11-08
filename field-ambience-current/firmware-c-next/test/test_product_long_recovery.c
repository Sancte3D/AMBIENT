/* SD45: one real engine session across repeated Worlds, Scene failures and
 * restart. RAM is only the documented Scene backend, never substitute DSP.
 * Complete PCM and hooks compare against the same session over Main wrap. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "menu.h"
#include "params.h"
#include "controls.h"
#include "scenes.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include "ambient_room.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum { SR=44100,TICK=441,SECONDS=800,EVENTS=4096 };
typedef struct { uint32_t ms,on,owner,family,hz,velocity; } event_t;
typedef struct {
    uint32_t starts,reservations,maximum,failed_saves,retries,corruptions,
        recalls,stops,restarts,clears,peak,events;
    uint64_t pcm_hash;
} result_t;
static uint8_t blob[512];
static unsigned blob_len;
static bool reject_write;
static int16_t pcm[1024],expected[1024];
static event_t events[EVENTS],reference_events[EVENTS];
static uint32_t relative_ms,generation[16],acknowledged[16];
static uint16_t previous_mask;
static unsigned event_count;
static result_t result;
static bool generating;
static uint32_t last_on;

static uint16_t family_mask(int family) {
    return family==0 ? bowed_active_sources() : family==1 ? pluck_active_sources() : horn_active_sources();
}
static void observe(void) {
    uint16_t a=family_mask(0),b=family_mask(1),c=family_mask(2),all=(uint16_t)(a|b|c);
    assert(!(a&b) && !(a&c) && !(b&c));
    int count=bowed_active_count()+pluck_active_count()+horn_active_count();
    assert(count<=3 && pluck_active_count()<=2);
    if((unsigned)count>result.maximum)result.maximum=(unsigned)count;
    for(int i=0;i<16;++i)if((all&(1u<<i)) && !(previous_mask&(1u<<i))) {
        ++generation[i];++result.reservations;
    }
    previous_mask=all;
}
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(event_count<EVENTS && owner<16);
    int family=-1;
    for(int f=0;f<3;++f)if(family_mask(f)&(1u<<owner))family=f;
    if(on>0) {
        assert(family>=0 && generation[owner]>acknowledged[owner]);
        acknowledged[owner]=generation[owner];
        assert(isfinite(hz) && hz>=105 && hz<=850 && isfinite(velocity));
        ++result.starts;last_on=relative_ms;
    }
    event_t e={.ms=relative_ms,.on=(uint32_t)on,.owner=owner,.family=(uint32_t)family};
    memcpy(&e.hz,&hz,4);memcpy(&e.velocity,&velocity,4);events[event_count++]=e;
}
static bool write_blob(const void *p,unsigned n) {
    if(reject_write)return false;
    assert(n<=sizeof blob);memcpy(blob,p,n);blob_len=n;return true;
}
static bool read_blob(void *p,unsigned n) {
    if(!blob_len)return false;
    assert(n<=sizeof blob);memset(p,0xff,n);memcpy(p,blob,n<blob_len ? n : blob_len);return true;
}
static void set_world(int w) { engine_set_world(w);controls_refresh_held_pitches(); }
static void set_key(int k) { engine_set_key_pc(k);controls_refresh_held_pitches(); }
static void setup(uint32_t seed,uint32_t base) {
    engine_init(); /* Exactly once per complete 800-second capture. */
    engine_generative_tick(base);
    menu_callbacks_t cb={.set_world=set_world,.set_key=set_key,.set_tuning=engine_set_tuning,
        .set_space=engine_set_space,.set_atmosphere=engine_set_atmosphere,
        .set_motion=engine_set_motion,.set_fx=engine_set_fx_mode,
        .set_attack=engine_set_attack,.set_release=engine_set_release};
    menu_init(&cb);controls_init();params_init();
    blob_len=0;reject_write=false;scenes_init(write_blob,read_blob);
    params_encoder(PARAM_ENC_VOLUME,-100,0);params_encoder(PARAM_ENC_VOLUME,60,0);
    const int worlds[5]={0,1,2,0,2};
    for(int slot=0;slot<5;++slot) {
        menu_state_t p={.world=(uint8_t)worlds[slot],.key_pc=(uint8_t)(slot<3 ? 2 : 9),
            .tuning=(uint8_t)((slot+(seed==42))&1),.space=100,.atmos=0,
            .motion=(uint8_t)(slot&1 ? 25 : 75),.fx=1,.attack=100,.release=100};
        engine_set_mode((slot+(seed==42))&1);menu_apply_state(&p);
        params_set_bright(slot&1 ? -300 : 400);engine_set_gen_seed(seed+(uint32_t)slot);
        assert(scenes_save(slot,base));
    }
    assert(blob_len==368);scenes_init(write_blob,read_blob);
    memset(&result,0,sizeof result);result.pcm_hash=UINT64_C(14695981039346656037);
    memset(generation,0,sizeof generation);memset(acknowledged,0,sizeof acknowledged);
    memset(events,0,sizeof events);event_count=previous_mask=0;relative_ms=last_on=0;
    generating=false;engine_set_note_hook(hook);
}
static void start_generate(void) {
    engine_set_generative(true,-1);generating=true;last_on=relative_ms;
}
static void stop_generate(void) {
    engine_set_generative(false,-1);engine_set_nature(0);generating=false;++result.stops;
}
static void actions(uint32_t now) {
    if(relative_ms<720000) {
        unsigned cycle=relative_ms/120000,at=relative_ms%120000;
        const int slots[6]={0,1,2,3,4,1};int slot=slots[cycle];
        if(at==0) {
            assert(scenes_recall(slot,now));++result.recalls;
            start_generate();engine_set_nature(cycle&1 ? .35f : 0);
        }
        if(at==40000) {
            engine_set_color(cycle&1 ? 1 : 0);engine_set_activity(cycle&1 ? 0 : 1);
            engine_set_attack(cycle&1 ? 0 : 1);engine_set_release(1);
            engine_set_room(cycle&1 ? 0 : 1);
        }
        if(at==55000) {
            uint8_t saved[512];memcpy(saved,blob,sizeof blob);int active=scenes_active();
            reject_write=true;assert(!scenes_save(slot,now));reject_write=false;
            assert(scenes_active()==active && !memcmp(saved,blob,sizeof blob));++result.failed_saves;
        }
        if(at==56000) { assert(scenes_save(slot,now));++result.retries; }
        if(at==65000) { assert(scenes_recall((slot+2)%5,now));++result.recalls; }
        if(at==70000) {
            uint8_t saved[512];memcpy(saved,blob,sizeof blob);unsigned before=event_count;
            blob[20]^=1;scenes_init(write_blob,read_blob);
            for(int i=0;i<5;++i)assert(!scenes_used(i));
            assert(!scenes_recall(slot,now) && event_count==before);
            memcpy(blob,saved,sizeof blob);scenes_init(write_blob,read_blob);
            for(int i=0;i<5;++i)assert(scenes_used(i));
            assert(scenes_recall(slot,now));++result.recalls;++result.corruptions;
        }
        if(at==75000)stop_generate();
        if(at==95000)assert(bowed_active_count()+horn_active_count()+pluck_active_count()==0);
        if(at==100000) { start_generate();++result.restarts; }
        if(at==103000)stop_generate();
        if(at==110000) { engine_set_muted(true);generating=false;++result.clears; }
        if(at==110060)engine_set_muted(false);
        if(at>=111000) {
            assert(!engine_clear_pending() && !engine_muted());
            assert(bowed_active_count()+horn_active_count()+pluck_active_count()==0);
            assert(ambient_room_peak()==0);
        }
    } else {
        if(relative_ms==720000) {
            menu_state_t p={.world=WORLD_WOODLAND,.key_pc=2,.space=100,.fx=1,.motion=50,.attack=100,.release=100};
            engine_set_mode(0);menu_apply_state(&p);engine_set_nature(0);
            assert(engine_try_note_on(0,220,.9f));
        }
        if(relative_ms==722000)engine_note_off(0);
        if(relative_ms==722100)assert(pluck_active_count()==1);
        if(relative_ms==725000) {
            float before[24],after[24];assert(engine_sounding_frequencies(before,24)==1);
            engine_set_world(WORLD_COAST);assert(pluck_active_count()==1);
            assert(engine_sounding_frequencies(after,24)==1 && before[0]==after[0]);
        }
    }
}
static result_t session(uint32_t seed,int cap,uint32_t base,FILE *control,bool compare) {
    setup(seed,base);
    for(relative_ms=0;relative_ms<SECONDS*1000u;relative_ms+=10) {
        engine_generative_tick(base+relative_ms);observe();actions(base+relative_ms);observe();
        if(generating)assert(relative_ms-last_on<90000);
        for(int done=0;done<TICK;) {
            int n=TICK-done<cap ? TICK-done : cap;engine_render(pcm,n);observe();
            for(int i=0;i<2*n;++i) {
                unsigned p=(unsigned)abs(pcm[i]);if(p>result.peak)result.peak=p;
                uint16_t v=(uint16_t)pcm[i];
                result.pcm_hash=(result.pcm_hash^(v&255u))*UINT64_C(1099511628211);
                result.pcm_hash=(result.pcm_hash^(v>>8))*UINT64_C(1099511628211);
                if((relative_ms<720000 && relative_ms%120000>=111000) || relative_ms>=799000)assert(pcm[i]==0);
            }
            if(compare) { assert(fread(expected,sizeof(int16_t),(size_t)n*2,control)==(size_t)n*2);assert(!memcmp(expected,pcm,(size_t)n*4)); }
            else assert(fwrite(pcm,sizeof(int16_t),(size_t)n*2,control)==(size_t)n*2);
            done+=n;
        }
        assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
        if(relative_ms%120000==119990) { fprintf(stderr,"SD45 seed=%u cap=%d wrap=%d cycle=%u/6 passed\n",seed,cap,compare,relative_ms/120000+1);fflush(stderr); }
    }
    engine_generative_tick(base+relative_ms);observe();
    assert(engine_active_voices()==0 && !engine_clear_pending() && result.starts>=30 && result.peak<16384);
    assert(result.failed_saves==6 && result.retries==6 && result.corruptions==6 && result.recalls==18);
    assert(result.stops==12 && result.restarts==6 && result.clears==6);
    assert(!ferror(control));result.events=event_count;return result;
}
int main(void) {
    const uint32_t seeds[2]={1234,42};const int caps[2]={64,512};
    puts("{\"seconds_per_capture\":800,\"engine_initializations_per_capture\":1,\"cases\":[");
    for(int s=0;s<2;++s)for(int b=0;b<2;++b) {
        FILE *f=tmpfile();assert(f);
        result_t a=session(seeds[s],caps[b],0,f,false);
        memcpy(reference_events,events,sizeof events);assert(fflush(f)==0 && fseek(f,0,SEEK_SET)==0);
        result_t c=session(seeds[s],caps[b],UINT32_C(0xffffe000),f,true);
        assert(a.events==c.events && !memcmp(reference_events,events,a.events*sizeof(event_t)));
        assert(a.starts==c.starts && a.reservations==c.reservations && a.maximum==c.maximum && a.peak==c.peak && a.pcm_hash==c.pcm_hash);
        assert(fgetc(f)==EOF && !ferror(f));assert(fclose(f)==0);
        printf("%s{\"seed\":%u,\"render_cap\":%d,\"starts\":%u,\"reservations\":%u,\"max_sources\":%u,\"events\":%u,\"sample_peak\":%u,\"failed_saves\":6,\"save_retries\":6,\"crc_recoveries\":6,\"live_recalls\":18,\"stops\":12,\"restarts\":6,\"mute_clears\":6,\"pcm_fnv1a64\":\"%016" PRIx64 "\"}",s||b ? ",\n" : "",seeds[s],caps[b],a.starts,a.reservations,a.maximum,a.events,a.peak,a.pcm_hash);
        fflush(stdout);
    }
    puts("],\"whole_wrap_pcm_stereo_frames_compared\":141120000,\"complete_hook_comparisons\":4,\"captures\":8}");return 0;
}
