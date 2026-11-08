/* Real Product sources/Room and public note attempts. Including the actual
 * engine additionally permits clearly separated bounded-state/wrap checks;
 * these synthetic ledger checks are not presented as rendered tail energy. */
#include "../src/engine_product.c"
#include <assert.h>
#include <stdio.h>
enum { SR=44100,CONTROL=441,CAP=SR*110 };
static int16_t trial[CAP*2],reference[CAP*2],pcm[CONTROL*2];
static uint32_t on_frame,old_off_frame,at;
static unsigned ons,offs;
static float old_hz,new_hz;
static uint64_t compared_frames;
static unsigned real_cases,common_cases,cancel_cases,state_checks;
typedef struct {uint32_t retired,accepted,frames;float wet_at_retirement;} result_t;
static result_t rows[3][2][2][2];
static void hook(int on,uint8_t owner,float hz,float velocity) {
    (void)velocity;
    if(on>0) {assert(owner==ons&&hz==(owner ? new_hz : old_hz));++ons;on_frame=at;}
    else if(on==0) {++offs;if(owner==0)old_off_frame=at;}
}
static void setup(int w,int release,int room,int wrap) {
    engine_init();engine_set_world(w);engine_set_attack(1);
    engine_set_release((float)release);engine_set_color(1);engine_set_room((float)room);
    engine_set_master_volume(.6f);engine_set_nature(0);
    if(wrap) {
        atomic_store(&audio_frames,UINT32_MAX-2u*SR+1u);
        atomic_store(&audio_epoch,UINT32_MAX-1u);
    }
    old_hz=tuning_hz(54);new_hz=old_hz;
    ons=offs=at=on_frame=old_off_frame=0;engine_set_note_hook(hook);
}
static void audio(uint32_t n,int block,int16_t *out) {
    for(uint32_t i=0;i<n;) {
        int count=(int)(n-i);if(count>block)count=block;
        engine_render(out+2*i,count);i+=(uint32_t)count;
    }
    (void)engine_active_voices();
    assert(engine_nonfinite_samples()==0&&engine_output_limited_samples()==0);
}
static bool represented(float hz) {
    float f[19];int n=engine_sounding_frequencies(f,19);
    for(int i=0;i<n;++i)if(f[i]==hz)return true;
    return false;
}
/* Probe each10 ms; the control instead issues only the eventually accepted
 * request at that exact frame. Every preceding rejection must be inaudible. */
static result_t trajectory(int w,int release,int room,int block,int wrap,
                           uint32_t forced_accept,int16_t *out) {
    setup(w,release,room,wrap);assert(engine_try_note_on(0,old_hz,.9f));
    result_t r={0};bool cleared=false;
    for(at=0;at<CAP;at+=CONTROL) {
        if(at==2u*SR) {
            engine_set_tuning(1);new_hz=tuning_hz(54);
            assert(fabsf(old_hz-new_hz)>.1f&&represented(old_hz));
        }
        if(at==3u*SR)engine_note_off(0);
        if(at>=3u*SR&&!r.retired&&engine_active_voices()==0) {
            r.retired=at;r.wet_at_retirement=ambient_room_peak();
            assert(represented(old_hz));
        }
        if(at>=2u*SR&&!r.accepted&&(!forced_accept||at==forced_accept)) {
            bool ok=engine_try_note_on(1,new_hz,.9f);
            if(ok) {
                assert(r.retired&&at>=r.retired&&!represented(old_hz));
                assert(at-r.retired<=((uint32_t)(7.2f*SR)+CONTROL));
                assert(ons==1);r.accepted=at;
            } else {
                assert(!forced_accept&&represented(old_hz));
                assert(ons==1);
            }
        }
        if(r.accepted&&at==r.accepted+2u*SR) {engine_all_off();cleared=true;}
        audio(CONTROL,block,out+2*at);
        if(r.accepted&&!cleared)assert(ons==2&&on_frame==r.accepted);
        if(cleared&&at+CONTROL>=r.accepted+2u*SR+SR/20u) {
            assert(!engine_clear_pending()&&engine_active_voices()==0);
            float f[19];assert(engine_sounding_frequencies(f,19)==0);
            for(int i=0;i<CONTROL*2;++i)assert(out[2*at+(uint32_t)i]==0);
            r.frames=at+CONTROL;break;
        }
    }
    assert(r.retired&&r.accepted&&r.frames&&old_off_frame==3u*SR);
    return r;
}
static void actual(int w,int release,int room,int b) {
    int block=b ? 512 : 64;
    result_t a=trajectory(w,release,room,block,0,0,trial);
    result_t c=trajectory(w,release,room,block,0,a.accepted,reference);
    assert(a.frames==c.frames&&a.retired==c.retired&&a.accepted==c.accepted);
    assert(!memcmp(trial,reference,(size_t)a.frames*2*sizeof(int16_t)));
    compared_frames+=a.frames;
    c=trajectory(w,release,room,block,1,0,reference);
    assert(a.frames==c.frames&&a.retired==c.retired&&a.accepted==c.accepted);
    assert(!memcmp(trial,reference,(size_t)a.frames*2*sizeof(int16_t)));
    compared_frames+=a.frames;rows[w][release][room][b]=a;++real_cases;
}
static void common_and_cancel(int w,int block) {
    setup(w,1,1,0);assert(engine_try_note_on(0,old_hz,.9f));
    for(at=0;at<2u*SR;at+=CONTROL)audio(CONTROL,block,pcm);
    engine_set_generative(true,-1); /* real100-ms retirement of manual source */
    for(unsigned i=0;i<15;++i) {at+=CONTROL;audio(CONTROL,block,pcm);}
    engine_set_generative(false,-1);
    assert(engine_active_voices()==0&&ambient_room_peak()>0&&represented(old_hz));
    assert(engine_try_note_on(1,old_hz,.9f));
    audio(CONTROL,block,pcm);assert(ons==2&&represented(old_hz));++common_cases;

    setup(w,1,1,0);assert(engine_try_note_on(0,old_hz,.9f));engine_note_off(0);
    audio(CONTROL,block,pcm);float f[19];
    assert(engine_active_voices()==0&&engine_sounding_frequencies(f,19)==0&&ons==0);
    ++cancel_cases;
}
static void state_boundaries(void) {
    /* Synthetic state tests use real remember/reap but no fabricated PCM. */
    engine_init();atomic_store(&room_quiet,false);
    uint32_t lifetime=(uint32_t)(7.2f*SR);
    atomic_store(&audio_frames,0u-lifetime);remember(220);
    assert(tails[0].used&&tails[0].until==0);++state_checks;
    atomic_store(&audio_frames,UINT32_MAX);reap();assert(tails[0].used);++state_checks;
    atomic_store(&audio_frames,0);reap();assert(!tails[0].used);++state_checks;

    engine_init();atomic_store(&room_quiet,false);
    for(int i=0;i<12;++i)remember(220.0f+i*.0011f);
    assert(pitch_clear(220));++state_checks;
    remember(220.0f+12*.0011f);assert(!pitch_clear(220));++state_checks;
    for(int i=13;i<16;++i)remember(220.0f+i*.0011f);
    pitch_tail_t saved[TAILS];memcpy(saved,tails,sizeof saved);
    remember(330);assert(!memcmp(saved,tails,sizeof saved));++state_checks;
    atomic_store(&audio_frames,100);remember(tails[0].hz);
    assert(tails[0].until==100+lifetime);++state_checks;
    for(int i=1;i<16;++i)assert(!memcmp(&saved[i],&tails[i],sizeof tails[i]));
    ++state_checks;
    atomic_store(&audio_frames,lifetime);reap();
    assert(tails[0].used);for(int i=1;i<16;++i)assert(!tails[i].used);++state_checks;
    atomic_store(&audio_frames,100+lifetime);reap();assert(!tails[0].used);++state_checks;
}
int main(void) {
    state_boundaries();
    for(int w=0;w<3;++w)for(int release=0;release<2;++release)
        for(int room=0;room<2;++room)for(int b=0;b<2;++b)actual(w,release,room,b);
    for(int w=0;w<3;++w)for(int b=0;b<2;++b)common_and_cancel(w,b ? 512 : 64);
    uint32_t delta=0;
    for(int w=0;w<3;++w)for(int r=0;r<2;++r)for(int room=0;room<2;++room) {
        uint32_t a=rows[w][r][room][0].accepted,b=rows[w][r][room][1].accepted;
        uint32_t d=a>b ? a-b : b-a;if(d>delta)delta=d;
        assert(d<=CONTROL+512); /* block observation plus the10-ms Main raster */
    }
    printf("{\"real_tail_cases\":%u,\"common_tone_cases\":%u,\"cancel_cases\":%u,"
           "\"synthetic_state_checks\":%u,\"whole_pcm_stereo_frames_compared\":%llu,"
           "\"max_partition_accept_delta_frames\":%u,\"rows\":[",
           real_cases,common_cases,cancel_cases,state_checks,(unsigned long long)compared_frames,delta);
    unsigned n=0;
    for(int w=0;w<3;++w)for(int r=0;r<2;++r)for(int room=0;room<2;++room)for(int b=0;b<2;++b) {
        result_t x=rows[w][r][room][b];
        printf("%s{\"world\":%d,\"release\":%d,\"room\":%d,\"block\":%d,"
               "\"retired_frame\":%u,\"accepted_frame\":%u,\"wait_frames\":%u,"
               "\"wet_at_retirement\":%.9g}",n++ ? "," : "",w,r,room,b ? 512 : 64,
               x.retired,x.accepted,x.accepted-x.retired,(double)x.wet_at_retirement);
    }
    puts("]}");return 0;
}
