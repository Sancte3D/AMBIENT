/* Product candidate: one source per event, three global slots, one room.
 * Main owns preparation, proposals, ledger and hooks. DMA audio owns release,
 * source retirement, room/nature and ramps. Tickets are acknowledged after a
 * real DSP block; cancelled preparations cannot become heard score memory.
 * Fixed pools, lock-free word mailboxes, no score/note preparation in render. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "dsp.h"
#include "audio.h"
#include "shape.h"
#include "tuning.h"
#include "brain.h"
#include "cells.h"
#include "bowed.h"
#include "horn.h"
#include "pluck.h"
#include "ambient_room.h"
#include "nature.h"
#include <stdatomic.h>
#include <string.h>
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"Audio needs lock-free word atomics");
enum { SLOTS=3,TAILS=16,SOURCES=16,BLOCK=AUDIO_BUFFER_FRAMES };
typedef struct {
    float hz,velocity;
    uint32_t issued_epoch,off_ms;
    uint8_t owner,family;
    bool used,held,timed,released,started,on_sent,off_sent;
} source_slot_t;
typedef struct { float hz; uint32_t until; bool used; } pitch_tail_t;
static source_slot_t slots[SLOTS];
static pitch_tail_t tails[TAILS];
static world_grammar_t grammar;
static world_offer_t pending_offer;
static bool pending;
static int pending_index,pending_owner,pending_world;
static engine_note_hook_t note_hook;
static int world,minor,tonic_pc,fx_mode,last_midi,note_count;
static uint32_t seed,now_ms,last_user_ms,retry_ms,rejects;
static bool generate,autoplay,user_present,user_seen,suppressed,clearing;
static float activity,color,room,nature_amount;
static _Atomic uint32_t release_mask,audio_mask,started_mask,audio_epoch,audio_frames,volume_bits,color_bits,limited,faults;
static _Atomic bool clear_request,clear_done,output_enabled,room_quiet;
static float dry_l[BLOCK],dry_r[BLOCK],send_l[BLOCK],send_r[BLOCK];
static float volume_cur,gate_cur,dc_l,dc_r;
static uint32_t quiet_frames;
static uint32_t bits(float v) { uint32_t b; memcpy(&b,&v,4); return b; }
static float value(uint32_t b) { float v; memcpy(&v,&b,4); return v; }
static uint32_t clock_frames(void) { return atomic_load_explicit(&audio_frames,memory_order_acquire); }
static void remember(float hz) {
    /* Max room T60 4.8 s: nominal -90 dB after another 1.5*T60 from real
     * source retirement. Also allow 250 ms of measured quiet wet return.
     * Reserve all three outgoing slots; never evict an audible history item. */
    uint32_t until=clock_frames()+(uint32_t)(7.2f*DSP_SAMPLE_RATE_HZ);
    for(int i=0;i<TAILS;++i) if(tails[i].used && fabsf(tails[i].hz-hz)<.001f) {
        tails[i].until=until; return;
    }
    for(int i=0;i<TAILS;++i) if(!tails[i].used) {
        tails[i]=(pitch_tail_t){hz,until,true}; return;
    }
}
static void reap(void) {
    if(clearing && atomic_load_explicit(&clear_done,memory_order_acquire)) {
        memset(slots,0,sizeof slots); memset(tails,0,sizeof tails);
        pending=false; clearing=false; atomic_store(&clear_request,false);
    }
    uint32_t frames=clock_frames();
    bool quiet=atomic_load_explicit(&room_quiet,memory_order_acquire);
    for(int i=0;i<TAILS;++i) if(tails[i].used &&
       (quiet || (int32_t)(frames-tails[i].until)>=0)) tails[i].used=false;
    uint32_t epoch=atomic_load_explicit(&audio_epoch,memory_order_acquire);
    uint32_t mask=atomic_load_explicit(&audio_mask,memory_order_acquire);
    uint32_t heard=atomic_exchange_explicit(&started_mask,0,memory_order_acquire);
    for(int i=0;i<SLOTS;++i) if(slots[i].used) {
        source_slot_t *s=&slots[i];
        if(heard&(1u<<s->owner)) {
            s->started=true;
            if(!clearing && !s->on_sent) {
                if(note_hook) note_hook(1,s->owner,s->hz,s->velocity);
                s->on_sent=true;
                if(s->released && !s->off_sent) {
                    if(note_hook) note_hook(0,s->owner,s->hz,0);
                    s->off_sent=true;
                }
            }
            if(pending && pending_owner==s->owner) {
                if(!clearing && generate && autoplay && pending_world==world) {
                    world_grammar_commit(&grammar,&pending_offer,pending_index,now_ms);
                    last_midi=world_pitch_midi(pending_index,tonic_pc,minor!=0);
                    ++note_count;
                }
                pending=false;
            }
        }
        if(epoch!=s->issued_epoch && !(mask&(1u<<s->owner))) {
            if(s->on_sent && !s->off_sent && note_hook) note_hook(0,s->owner,s->hz,0);
            if(s->started) remember(s->hz);
            if(pending && pending_owner==s->owner) pending=false;
            s->used=false;
        }
    }
}
static bool compatible(float a,float b) {
    float cents=fabsf(1200.0f*log2f(a/b));
    if(cents<3) return true; /* room tails may share an exactly tuned tone */
    if(cents<90) return false; /* different JI/ET versions of the same MIDI */
    float cls=fmodf(cents,1200);
    if(cls>600) cls=1200-cls;
    if(fabsf(cls-100)<45 || fabsf(cls-600)<45) return false;
    float lo=fminf(a,b);
    if(lo<261.63f && cents<249.5f) return false;
    if(lo<196 && cents<299.5f) return false;
    return true;
}
static bool pitch_clear(float hz) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used) {
        /* Two independently restarted identical oscillators can cancel.
         * Keep the existing common tone; another role chooses another pitch. */
        if(fabsf(1200.0f*log2f(hz/slots[i].hz))<3 || !compatible(hz,slots[i].hz)) return false;
    }
    int n=0;
    for(int i=0;i<TAILS;++i) if(tails[i].used) {
        ++n; if(!compatible(hz,tails[i].hz)) return false;
    }
    return n<TAILS-SLOTS;
}
int engine_ambient_source_count(void) {
    reap(); int n=0; for(int i=0;i<SLOTS;++i) n+=slots[i].used; return n;
}
int engine_world_source_count(void) { return engine_ambient_source_count(); }
int engine_active_voices(void) { return engine_ambient_source_count(); }
int engine_sounding_frequencies(float *out,int max) {
    reap(); if(!out || max<=0) return 0;
    int n=0;
    for(int i=0;i<SLOTS && n<max;++i) if(slots[i].used) out[n++]=slots[i].hz;
    for(int i=0;i<TAILS && n<max;++i) if(tails[i].used) out[n++]=tails[i].hz;
    return n;
}
int engine_sounding_notes(int *out,int max) {
    float hz[TAILS+SLOTS]; int n=engine_sounding_frequencies(hz,TAILS+SLOTS),count=0;
    if(!out || max<=0) return 0;
    for(int i=0;i<n && count<max;++i) {
        int m=(int)lroundf(69+12*log2f(hz[i]/440));
        bool duplicate=false;
        for(int k=0;k<count;++k) if(out[k]==m) duplicate=true;
        if(!duplicate) out[count++]=m;
    }
    return count;
}
static bool admit(uint8_t owner,float hz,float velocity) {
    reap();
    if(owner>=SOURCES || !isfinite(hz) || !isfinite(velocity) || velocity<=0 ||
       hz<140 || hz>470 || clearing) return false;
    int empty=-1,local=0;
    for(int i=0;i<SLOTS;++i) {
        if(!slots[i].used) { if(empty<0) empty=i; continue; }
        if(slots[i].owner==owner) return false;
        local+=slots[i].family==world;
    }
    if(empty<0 || (world==WORLD_WOODLAND && local>=PLUCK_VOICES) || !pitch_clear(hz)) return false;
    velocity=dsp_clampf(velocity,0,1);
    /* Fixed dry-source calibration candidate. Shared by manual/generator;
     * no master AGC, automatic gain compensation or implicit second source. */
    float amp=velocity*(world==WORLD_WOODLAND ? .22f : .50f);
    bool ok=world==WORLD_COAST ? bowed_try_note_on(owner,hz,amp) :
            world==WORLD_WOODLAND ? pluck_note_on(owner,hz,amp) : horn_try_note_on(owner,hz,amp);
    if(!ok) return false;
    slots[empty]=(source_slot_t){.hz=hz,.velocity=velocity,.owner=owner,.family=(uint8_t)world,
        .used=true,.held=world!=WORLD_WOODLAND,.issued_epoch=atomic_load(&audio_epoch)};
    atomic_store_explicit(&output_enabled,true,memory_order_release);
    return true;
}
bool engine_try_note_on(uint8_t owner,float hz,float v) {
    if(generate) return false;
    bool ok=admit(owner,hz,v); if(!ok) ++rejects; return ok;
}
void engine_note_on(uint8_t owner,float hz,float legacy_amp) {
    /* Existing Hall/MIDI adapters pass CELL_AMP_MIN..MAX. Product API above
     * is explicitly normalised velocity; physical mapping remains separate. */
    if(isfinite(legacy_amp)) (void)engine_try_note_on(owner,hz,legacy_amp/CELL_AMP_MAX);
}
bool engine_try_world_note_on(uint8_t owner,float hz,float v) {
    if(!generate || !autoplay || suppressed) return false;
    bool ok=admit(owner,hz,v); if(!ok) ++rejects; return ok;
}
void engine_note_off(uint8_t owner) {
    if(owner>=SOURCES) return;
    reap();
    for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].owner==owner && !slots[i].released) {
        source_slot_t *s=&slots[i]; s->held=s->timed=false; s->released=true;
        atomic_fetch_or_explicit(&release_mask,1u<<owner,memory_order_release);
        if(s->on_sent && !s->off_sent) {
            if(note_hook) note_hook(0,owner,s->hz,0);
            s->off_sent=true;
        }
    }
}
static void release_all(void) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used) engine_note_off(slots[i].owner);
}
static void release_generated(void) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used &&
       (slots[i].owner==6 || slots[i].owner==7 || slots[i].owner==15)) engine_note_off(slots[i].owner);
    pending=false;
}
void engine_all_off(void) {
    reap(); generate=false; suppressed=false; pending=false;
    release_all(); clearing=true;
    atomic_store(&clear_done,false); atomic_store(&output_enabled,false);
    atomic_store_explicit(&clear_request,true,memory_order_release);
    if(note_hook) note_hook(-1,0,0,0);
}
bool engine_clear_pending(void) { reap(); return clearing; }
void engine_set_note_hook(engine_note_hook_t h) { note_hook=h; }
void engine_set_master_volume(float v) {
    if(isfinite(v)) atomic_store_explicit(&volume_bits,bits(dsp_clampf(v,0,1)),memory_order_release);
}
void engine_boot_mute(void) { volume_cur=0; engine_set_master_volume(0); } /* before DMA starts */
void engine_set_color(float v) {
    if(!isfinite(v)) return;
    color=dsp_clampf(v,0,1); atomic_store(&color_bits,bits(color));
    bowed_set_tone(color); horn_set_tone(color); pluck_set_damp(.65f-.35f*color);
}
void engine_set_activity(float v) { if(isfinite(v)) activity=dsp_clampf(v,0,1); }
void engine_set_room(float v) { if(isfinite(v)) { room=dsp_clampf(v,0,1); ambient_room_set(room); } }
void engine_set_nature(float v) { if(isfinite(v)) { nature_amount=dsp_clampf(v,0,1); nature_set_amount(nature_amount); } }
void engine_set_world(int i) {
    if(i<0 || i>=CORE_WORLD_COUNT || i==world) return;
    reap(); release_all(); pending=false; world=i; nature_set_world(world);
    world_grammar_init(&grammar,world,seed^(uint32_t)(world*0x9E3779B9u)); retry_ms=now_ms;
}
int engine_product_world(void) { return world; }
void engine_set_key_pc(int pc) {
    pc%=12; if(pc<0) pc+=12;
    /* Cancel an unacknowledged score proposal, preserve actual held pitches. */
    reap(); pending=false; tonic_pc=pc;
    int root=50+((pc-2+12)%12); brain_set_key(root); tuning_set_key(root);
}
void engine_set_key(int midi) { engine_set_key_pc(midi%12); }
void engine_set_mode(int i) { if(i>=0 && i<=1) { reap(); pending=false; minor=i; brain_set_mode(minor ? 5 : 0); } }
void engine_set_tuning(int just) { if(just==0 || just==1) { reap(); pending=false; tuning_set_mode(just); } }
void engine_set_attack(float v) { if(isfinite(v)) shape_set_attack(.35f+.30f*dsp_clampf(v,0,1)); }
void engine_set_release(float v) { if(isfinite(v)) shape_set_release(.35f+.30f*dsp_clampf(v,0,1)); }
bool engine_cell_sample(uint8_t cell,float position,uint32_t ms) {
    if(generate || !isfinite(position)) return false;
    cell_event_t e=cells_update(cell,position,ms);
    if(e.kind==CELL_EVENT_PRESS) return engine_try_note_on(e.cell,tuning_hz((float)brain_cell_root(e.cell)),e.amp/CELL_AMP_MAX);
    if(e.kind==CELL_EVENT_RELEASE) { engine_note_off(e.cell); return true; }
    return false;
}
void engine_set_user_presence(bool on) {
    user_present=on; if(on) { user_seen=true; last_user_ms=now_ms; }
}
void engine_set_generative(bool on,int program) {
    (void)program; reap(); if(on==generate) return;
    release_all(); pending=false; generate=on; suppressed=false; retry_ms=now_ms;
    if(on) world_grammar_init(&grammar,world,seed^(uint32_t)(world*0x9E3779B9u));
    nature_set_amount(on ? nature_amount : 0);
}
void engine_set_autoplay_melody(int on) { autoplay=on!=0; if(!autoplay) release_generated(); }
int engine_autoplay_melody(void) { return autoplay; }
uint32_t engine_gen_seed(void) { return seed; }
void engine_set_gen_seed(uint32_t v) {
    release_generated(); seed=v ? v : 0xA6B13E7Du;
    world_grammar_init(&grammar,world,seed^(uint32_t)(world*0x9E3779B9u)); retry_ms=now_ms;
}
void engine_generative_new_field(uint32_t v) { engine_set_gen_seed(v); }
void engine_generative_tick(uint32_t ms) {
    now_ms=ms; reap();
    for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].timed &&
       (int32_t)(ms-slots[i].off_ms)>=0) engine_note_off(slots[i].owner);
    if(user_present) { last_user_ms=ms; user_seen=true; }
    bool wait=user_seen && (user_present || (uint32_t)(ms-last_user_ms)<8000u);
    if(wait && !suppressed) release_generated();
    suppressed=wait;
    if(!generate || !autoplay || suppressed || clearing || pending ||
       (int32_t)(ms-retry_ms)<0 || !world_grammar_due(&grammar,ms)) return;
    world_offer_t offer=world_grammar_propose(&grammar,ms,activity);
    if(offer.rest) { world_grammar_commit(&grammar,&offer,offer.index,ms); return; }
    int held=0; for(int i=0;i<SLOTS;++i) held+=slots[i].used && slots[i].held;
    if(held>=offer.held_limit || engine_ambient_source_count()>=SLOTS) { retry_ms=ms+250; return; }
    static const uint8_t owners[3]={6,7,15};
    int owner=-1;
    for(int i=0;i<3 && owner<0;++i) {
        bool used=false; for(int j=0;j<SLOTS;++j) if(slots[j].used && slots[j].owner==owners[i]) used=true;
        if(!used) owner=owners[i];
    }
    if(owner<0) { retry_ms=ms+250; return; }
    int count=world_pitch_count(tonic_pc,minor!=0);
    int index=offer.index<count ? offer.index : count-1;
    int midi=world_pitch_midi(index,tonic_pc,minor!=0);
    float hz=tuning_hz((float)midi);
    if((world==WORLD_COAST || world==WORLD_HIGHLANDS) && !pitch_clear(hz)) {
        static const int offset[5]={-1,1,-2,2,0};
        for(int i=0;i<5;++i) {
            int candidate=index+offset[i]; if(candidate<0 || candidate>=count) continue;
            if(world==WORLD_HIGHLANDS && grammar.last>=0) {
                int direction=offer.index>grammar.last ? 1 : -1;
                if((candidate-grammar.last)*direction<=0) continue;
            }
            int m=world_pitch_midi(candidate,tonic_pc,minor!=0); float h=tuning_hz((float)m);
            if(pitch_clear(h)) { index=candidate; midi=m; hz=h; break; }
        }
    }
    if(!engine_try_world_note_on((uint8_t)owner,hz,offer.velocity)) { retry_ms=ms+500; return; }
    for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].owner==owner && offer.hold_ms) {
        slots[i].timed=true; slots[i].off_ms=ms+offer.hold_ms;
    }
    pending_offer=offer; pending_index=index; pending_owner=owner; pending_world=world; pending=true;
}
int engine_generative_advance(void) { return generate && !suppressed ? (int)(grammar.episodes%4u)+1 : -1; }
void engine_generative_nudge(int cell,uint32_t ms) { (void)cell; (void)ms; }
int engine_generative_suppressed(void) { return suppressed; }
int engine_generative_last_melody_midi(void) { return last_midi; }
int engine_generative_melody_count(void) { return note_count; }
int engine_generative_dejavu_count(void) { return (int)grammar.answers; }
uint32_t engine_admission_rejections(void) { return rejects; }
uint32_t engine_output_limited_samples(void) { return atomic_load(&limited); }
uint32_t engine_nonfinite_samples(void) { return atomic_load(&faults); }
bool engine_listening_tail_active(void) { return engine_active_voices()>0 || !atomic_load(&room_quiet); }
void engine_set_fx_mode(int i) { if(i>=0 && i<=1) { fx_mode=i; ambient_room_enable(i==1); } }
int engine_fx_mode(void) { return fx_mode; }
int engine_fx_mode_count(void) { return 2; }
const char *engine_fx_mode_name(int i) { return i==0 ? "DRY" : i==1 ? "ROOM" : ""; }
/* Retired adapters cannot start a hidden source or restore an archived mode.
 * Product UI/catalogue migration must hide them before target activation. */
void engine_set_synth_backend(const engine_synth_backend_t *b) { (void)b; }
void engine_set_synth(int i) { (void)i; }
int engine_synth(void) { return 0; }
void engine_set_synth_param(int i,float v) { (void)i; (void)v; }
void engine_set_voice(int i) { (void)i; }
void engine_set_pad_voice(int i) { (void)i; }
void engine_set_drone(bool on) { (void)on; }
void engine_bass_follow(bool on) { (void)on; }
void engine_bass_set(float v) { (void)v; }
void engine_bass_off(void) {}
void engine_bass_glide(float v) { (void)v; }
bool engine_bass_active(void) { return false; }
void engine_motif_strike(float f,float a) { (void)f; (void)a; }
void engine_sparkle_strike(float f,float a) { (void)f; (void)a; }
#define RETIRED(name) void name(float v) { (void)v; }
RETIRED(engine_set_drive)
RETIRED(engine_set_reverb_drive)
RETIRED(engine_set_wet_amp)
RETIRED(engine_set_send)
RETIRED(engine_set_resonance)
RETIRED(engine_set_sweep)
RETIRED(engine_set_envmod)
RETIRED(engine_set_texture)
RETIRED(engine_set_age)
RETIRED(engine_set_echo)
RETIRED(engine_set_blur)
RETIRED(engine_set_shimmer)
RETIRED(engine_set_bass_depth)
float engine_resonance(void) { return 0; }
void engine_set_reverb_size(float v) { engine_set_room(v); }
void engine_set_reverb_damp(float v) { (void)v; }
void engine_set_space(float v) { engine_set_room(v); }
void engine_set_atmosphere(float v) { engine_set_nature(v); }
void engine_set_motion(float v) { engine_set_activity(v); }
void engine_set_mood(float v) { engine_set_color(v); }
void engine_set_brightness(float hz) {
    if(isfinite(hz)) engine_set_color(hz<0 ? .5f+hz/1200.0f : .5f+hz/1600.0f);
}
void engine_set_vibe(int i) { (void)i; }
void engine_init(void) {
    dsp_init(); shape_init(); tuning_set_mode(0); brain_init(); cells_init();
    bowed_init(); horn_init(); pluck_init(); ambient_room_init(); nature_init();
    memset(slots,0,sizeof slots); memset(tails,0,sizeof tails);
    world=minor=0; tonic_pc=2; seed=0xA6B13E7Du; now_ms=last_user_ms=retry_ms=rejects=0;
    generate=user_present=user_seen=suppressed=clearing=pending=false; autoplay=true;
    note_hook=0; fx_mode=1; last_midi=note_count=0;
    atomic_store(&release_mask,0); atomic_store(&audio_mask,0); atomic_store(&started_mask,0);
    atomic_store(&audio_epoch,0); atomic_store(&audio_frames,0); atomic_store(&limited,0); atomic_store(&faults,0);
    atomic_store(&clear_request,false); atomic_store(&clear_done,false);
    atomic_store(&output_enabled,false); atomic_store(&room_quiet,true);
    volume_cur=.6f; gate_cur=dc_l=dc_r=0; quiet_frames=0;
    engine_set_master_volume(.6f); engine_set_key_pc(2); engine_set_activity(.5f);
    engine_set_color(.5f); engine_set_room(.5f); engine_set_nature(0);
    engine_set_attack(.5f); engine_set_release(.5f); world_grammar_init(&grammar,world,seed);
}
static float protect(float x) {
    if(!isfinite(x)) { atomic_fetch_add_explicit(&faults,1,memory_order_relaxed); return 0; }
    float a=fabsf(x); if(a<=.75f) return x;
    atomic_fetch_add_explicit(&limited,1,memory_order_relaxed);
    float excess=a-.75f,y=.75f+.25f*excess/(.25f+excess);
    return x<0 ? -y : y;
}
static int16_t quantize(float x) {
    x=protect(x)*32767;
    return (int16_t)(int32_t)(x+(x<0 ? -.5f : .5f));
}
void engine_render(int16_t *out,int frames) {
    if(!out || frames<=0) return;
    while(frames>0) {
        int n=frames>BLOCK ? BLOCK : frames;
        uint32_t off=atomic_exchange_explicit(&release_mask,0,memory_order_acquire);
        for(int i=0;i<SOURCES;++i) if(off&(1u<<i)) {
            bowed_note_off(i); horn_note_off(i); pluck_note_off((uint8_t)i);
        }
        memset(dry_l,0,n*sizeof(float)); memset(dry_r,0,n*sizeof(float));
        memset(send_l,0,n*sizeof(float)); memset(send_r,0,n*sizeof(float));
        bowed_render_mix(dry_l,dry_r,send_l,send_r,n,.35f);
        horn_render_mix(dry_l,dry_r,send_l,send_r,n,.35f);
        pluck_render_mix(dry_l,dry_r,send_l,send_r,n);
        for(int i=0;i<n;++i) { send_l[i]=.35f*dry_l[i]; send_r[i]=.35f*dry_r[i]; }
        ambient_room_process(dry_l,dry_r,send_l,send_r,n);
        float wet_peak=ambient_room_peak();
        nature_render(dry_l,dry_r,n);
        float target=value(atomic_load_explicit(&volume_bits,memory_order_acquire));
        bool clear=atomic_load_explicit(&clear_request,memory_order_acquire);
        float gate=atomic_load_explicit(&output_enabled,memory_order_acquire) && !clear ? 1 : 0;
        for(int i=0;i<n;++i) {
            volume_cur+=(target-volume_cur)*(1.0f/(.120f*DSP_SAMPLE_RATE_HZ));
            float step=1.0f/(.040f*DSP_SAMPLE_RATE_HZ);
            if(gate_cur<gate) gate_cur=fminf(gate,gate_cur+step);
            else if(gate_cur>gate) gate_cur=fmaxf(gate,gate_cur-step);
            dc_l+=.00228f*(dry_l[i]-dc_l); dc_r+=.00228f*(dry_r[i]-dc_r);
            out[2*i]=quantize((dry_l[i]-dc_l)*volume_cur*gate_cur);
            out[2*i+1]=quantize((dry_r[i]-dc_r)*volume_cur*gate_cur);
        }
        uint32_t mask=bowed_active_sources()|horn_active_sources()|pluck_active_sources();
        bool quiet=!mask && wet_peak<1e-5f;
        quiet_frames=quiet ? quiet_frames+(uint32_t)n : 0;
        atomic_store_explicit(&room_quiet,quiet_frames>=DSP_SAMPLE_RATE_HZ/4,memory_order_release);
        if(clear && gate_cur==0 && !atomic_load(&clear_done)) {
            bowed_init(); horn_init(); pluck_init(); ambient_room_clear(); nature_clear();
            float c=value(atomic_load_explicit(&color_bits,memory_order_acquire));
            bowed_set_tone(c); horn_set_tone(c); pluck_set_damp(.65f-.35f*c);
            dc_l=dc_r=0; mask=0; quiet_frames=DSP_SAMPLE_RATE_HZ/4;
            atomic_store(&release_mask,0); atomic_store(&started_mask,0); atomic_store(&room_quiet,true);
            atomic_store_explicit(&clear_done,true,memory_order_release);
        } else atomic_fetch_or_explicit(&started_mask,mask,memory_order_release);
        atomic_store_explicit(&audio_mask,mask,memory_order_release);
        atomic_fetch_add_explicit(&audio_frames,(uint32_t)n,memory_order_release);
        atomic_fetch_add_explicit(&audio_epoch,1,memory_order_release);
        frames-=n; out+=2*n;
    }
}
