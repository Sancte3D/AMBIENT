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
#ifdef FAM_SOUND_FINISH_CANDIDATE
#include "gesture.h"
#endif
#ifdef FAM_SOUND_AMBIENT_V3
#include "ambient_motion.h"
#include "ambient_choir.h"
#include "ambient_modal.h"
#include "ambient_world_fx.h"
#endif
#include <stdatomic.h>
#include <string.h>
_Static_assert(ATOMIC_INT_LOCK_FREE==2,"Audio needs lock-free word atomics");
#ifdef FAM_SOURCE_HANDOFF_TEST
extern void engine_source_handoff_probe(unsigned point,uint8_t owner);
#define SOURCE_HANDOFF_POINT(point,owner) engine_source_handoff_probe(point,owner)
#else
#define SOURCE_HANDOFF_POINT(point,owner) ((void)0)
#endif
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
static coast_phrase_t coast;
static coast_offer_t coast_tickets[3];
static uint8_t coast_pending;
static const uint8_t coast_owners[3]={6,7,15};
static woodland_phrase_t woodland;
static woodland_offer_t woodland_ticket;
static bool woodland_pending;
static highlands_phrase_t highlands;
static highlands_offer_t highlands_ticket;
static bool highlands_pending;
static engine_note_hook_t note_hook;
static engine_note_block_t note_block;
engine_note_block_t engine_note_block_reason(void) {return note_block;}
static int world,minor,tonic_pc,fx_mode,last_midi,note_count;
#ifdef FAM_SOUND_AMBIENT_V3
/* `world` is the voice family whose grammar, scales and admission apply;
 * world_id the selected World: FJORD, MEADOW and GLACIER are the families
 * COAST, WOODLAND and HIGHLANDS with their own instrument and effects. */
static int world_id;
#endif
static uint32_t seed,now_ms,last_user_ms,retry_ms,rejects;
static bool world_character;
static bool generate,autoplay,user_present,user_seen,suppressed,clearing,muted,resume_nature;
static float activity,color,room,nature_amount;
/* quick_mask: one bit per owner for context stops, then Cell handovers. */
static _Atomic uint32_t quick_mask,release_mask,audio_mask,started_mask,audio_epoch,audio_frames,volume_bits,color_bits,limited,faults;
static _Atomic bool clear_request,clear_done,output_enabled,room_quiet;
static float dry_l[BLOCK],dry_r[BLOCK],send_l[BLOCK],send_r[BLOCK];
static float volume_cur,gate_cur,dc_l,dc_r;
#ifdef FAM_SOUND_FINISH_CANDIDATE
static int last_voicing[CORE_WORLD_COUNT][3];
static uint8_t loop_lane[2];
static bool freeze_seen;
/* Main reserves incoming Cell notes while Audio ramps released sources down.
 * The existing source pools stay fixed and preparation stays in Main. */
typedef struct {
    float hz,velocity;
    uint8_t owner;
    bool down;
} cell_press_t;
static cell_press_t cell_queue[SLOTS];
static unsigned cell_queue_count;
static uint32_t cell_retiring;
enum { CELL_FADE_FRAMES = DSP_SAMPLE_RATE_HZ / 50 }; /* 20 ms */
_Static_assert(2*SOURCES<=32,"Two stop bits per audio owner");
static void reset_voicing(void) {
    for(int w=0;w<CORE_WORLD_COUNT;++w)for(int r=0;r<3;++r)last_voicing[w][r]=-1;
}
#endif
static uint32_t quiet_frames;
/* Plucked voices the next WOODLAND-family note may use: the kalimba rings
 * three tines, the resonator body keeps its two. */
static int pluck_capacity(void) {
#ifdef FAM_SOUND_AMBIENT_V3
    return pluck_voice_limit();
#else
    return PLUCK_VOICES;
#endif
}
static uint32_t bits(float v) { uint32_t b; memcpy(&b,&v,4); return b; }
static float value(uint32_t b) { float v; memcpy(&v,&b,4); return v; }
static uint32_t clock_frames(void) { return atomic_load_explicit(&audio_frames,memory_order_acquire); }
#ifdef FAM_SOUND_AMBIENT_V3
/* DRONE: a soft, slowly breathing bed in the colour of the World. A layer of
 * its own: never a note slot, never admitted, no tail history, so it can
 * neither block nor steal a Cell. Main publishes level, colour and root;
 * Audio ramps (about 5 s in, 8 s out) and glides 1.5 s to a new key. A new
 * colour (World change, later the display UI) dips the bed out in .5 s,
 * switches at silence and returns in 1.5 s, so it never sweeps an octave.
 *   Strings (COAST):    tonic+fifth from D3, smooth string slope, slow ensemble beat.
 *   Wood (WOODLAND):    tonic+fifth from A3, hollow 1:4 marimba-resonator partial,
 *                       slower and deeper bloom.
 *   Breath (HIGHLANDS): tonic+fifth from A3, nearly pure, air tuned to each tone.
 * Roots stay in D3..C#4 (Strings) and A3..G#4 (Wood, Breath); the highest
 * fifth is 622 Hz, below the register that made long high tones unsettling.
 * Trims give the three colours equal loudness (K-weighted).
 * The instrument Worlds play their bed with their own instrument:
 *   Choir (FJORD):     three soft "ooh" voices, tonic, third (else sixth or
 *                      fourth) and fifth from A3, slowly breathing.
 *   Kalimba (MEADOW):  no held tone: soft tines from the World's first five
 *                      notes above A4, unhurried and irregular.
 *   Bells (GLACIER):   distant strokes on tonic, fifth and octave every
 *                      3.5-7 s, ringing into each other. */
enum { DRONE_VOICES=2,BED_VOICES=3,BED_TINES=4,BED_BELLS=3,BED_DROPS=5 };
#define DRONE_TRIM_CHOIR 1.9f
#define DRONE_TRIM_KALIMBA 1.7f
#define DRONE_TRIM_BELLS 2.6f
#define DRONE_TRIM_WOOD .56f
#define DRONE_TRIM_BREATH .51f
#define DRONE_AIR 1.4f
static const float DRONE_LEVEL=.022f; /* Strings: about -12 dB re a held COAST/HIGHLANDS Cell */
typedef struct {
    int lowest;          /* lowest root, MIDI: the 12-note window starts here */
    float w[4];          /* partials 1..4 */
    float tilt;          /* brightness-drift depth on partials 2..4 */
    float fifth;         /* level of the fifth voice */
    float partner;       /* slow-beat partner of each tone */
    float beat_hz[2];    /* partner beat, root and fifth */
    float bloom;         /* level-drift depth, 1 = +-1.4 dB */
    float breath;        /* tuned air, 0 = none */
    float trim;          /* equal-loudness trim */
} drone_color_t;
static const drone_color_t DRONE_COLOR[ENGINE_DRONE_BREATH+1]={
    /* AUTO (never sounds) */{50,{1,.34f,.16f,.08f},.40f,.70f,.30f,{.11f,.17f},1.0f,0,1.0f},
    /* Strings */            {50,{1,.34f,.16f,.08f},.40f,.70f,.30f,{.11f,.17f},1.0f,0,1.0f},
    /* Wood */               {57,{1,.08f,.03f,.16f},.30f,.45f,.18f,{.05f,.08f},1.8f,0,DRONE_TRIM_WOOD},
    /* Breath */             {57,{1,.07f,.02f,0},   .20f,.50f,.20f,{.08f,.13f},1.0f,DRONE_AIR,DRONE_TRIM_BREATH},
};
static _Atomic uint32_t drone_level_bits,drone_root_bits,drone_color_bits;
static _Atomic uint32_t bed_upper_bits,bed_drop_bits[BED_DROPS];
static bool drone_on;
static int drone_color_sel;   /* ENGINE_DRONE_AUTO: the World's colour */
static float drone_level_db;  /* user trim, -12..+6 dB */
typedef struct {
    float ph,ph2,beat,lev,drift,air_hz;
    uint32_t rng;
    dsp_svf_t air1,air2;
    ambient_motion_t motion;
} drone_voice_t;
typedef struct { ambient_choir_t choir; ambient_motion_t motion; float lev,drift; } bed_voice_t;
typedef struct { ambient_modal_t body; float pan; bool live; } bed_strike_t;
static struct {
    drone_voice_t v[DRONE_VOICES]; float level,root,gate; int col;
    bed_voice_t bed[BED_VOICES]; bed_strike_t tine[BED_TINES],bell[BED_BELLS];
    uint32_t rng; int32_t next;
} drone;
static bool drone_is_bed(int col) { return col>=ENGINE_DRONE_CHOIR && col<ENGINE_DRONE_COLORS; }
static int drone_resolved(void) {
    if(drone_color_sel>ENGINE_DRONE_AUTO && drone_color_sel<ENGINE_DRONE_COLORS) return drone_color_sel;
    if(world_id>=CORE_WORLD_COUNT)
        return world==WORLD_WOODLAND ? ENGINE_DRONE_KALIMBA : world==WORLD_HIGHLANDS ? ENGINE_DRONE_BELLS : ENGINE_DRONE_CHOIR;
    return world==WORLD_WOODLAND ? ENGINE_DRONE_WOOD : world==WORLD_HIGHLANDS ? ENGINE_DRONE_BREATH : ENGINE_DRONE_STRINGS;
}
static float drone_root_hz(int col) {
    int lo=drone_is_bed(col) ? 57 : DRONE_COLOR[col].lowest,pc=tonic_pc<0 ? 2 : tonic_pc;
    return tuning_hz((float)(lo+(((pc-lo)%12)+12)%12));
}
/* The beds' pitches: kalimba tines from the World's first five notes above
 * A4; the choir's upper voice on the third, else the sixth, else the fourth. */
static void bed_publish_pitches(void) {
    int lo=57,pc=tonic_pc<0 ? 2 : tonic_pc,rm=lo+(((pc-lo)%12)+12)%12;
    float root=tuning_hz((float)rm),drop[BED_DROPS]; int k=0;
    for(int m=69;m<=WORLD_LIVE_MIDI_HIGH && k<BED_DROPS;++m)
        if(world_live_pitch_allowed(m,pc,minor!=0)) drop[k++]=tuning_hz((float)m);
    for(;k<BED_DROPS;++k) drop[k]=k ? drop[k-1] : 2.0f*root;
    for(int i=0;i<BED_DROPS;++i) atomic_store_explicit(&bed_drop_bits[i],bits(drop[i]),memory_order_release);
    static const int upper[4]={4,3,9,5}; int up=7;
    for(int c=3;c>=0;--c) if(world_live_pitch_allowed(rm+upper[c]+(rm+upper[c]<WORLD_LIVE_MIDI_LOW ? 12 : 0),pc,minor!=0)) up=upper[c];
    atomic_store_explicit(&bed_upper_bits,bits(tuning_hz((float)(rm+up))/root),memory_order_release);
}
/* 10^(dB/20) without libm in this file (the hot-path lint reads whole
 * modules): whole dB by multiplication, the fraction interpolated, error
 * below .02 dB over the -12..+6 dB trim. Main only. */
static float drone_db_gain(float db) {
    const float step=1.1220185f; float g=1.0f;
    int n=(int)(db+100.0f)-100; float f=db-(float)n;
    for(;n>0;--n) g*=step;
    for(;n<0;++n) g/=step;
    return g*(1.0f+f*(step-1.0f));
}
static void drone_publish(void) {
    bool on=drone_on && !muted; int col=drone_resolved();
    if(drone_is_bed(col)) bed_publish_pitches();
    /* Colour before root: Audio never glides toward another colour's register. */
    atomic_store_explicit(&drone_color_bits,(uint32_t)col,memory_order_release);
    atomic_store_explicit(&drone_root_bits,bits(drone_root_hz(col)),memory_order_release);
    float gain=DRONE_LEVEL*drone_db_gain(drone_level_db);
    atomic_store_explicit(&drone_level_bits,bits(on ? gain : 0.0f),memory_order_release);
    if(on && !clearing) atomic_store(&output_enabled,true);
}
/* Audio-side state: engine_init and the IRQ Clear only. col 0 adopts the
 * published colour directly at the next start. */
static void drone_reset_audio(void) {
    memset(&drone,0,sizeof drone);
    for(int i=0;i<DRONE_VOICES;++i) {
        drone_voice_t *d=&drone.v[i];
        ambient_motion_prepare(&d->motion,0xD7053u+0x9E3779B9u*(uint32_t)(i+1));
        d->lev=ambient_motion_peek(&d->motion,.45f,&d->drift);
        d->ph2=.37f*(float)(i+1);
        d->rng=0xA17B5EEDu+0x9E3779B9u*(uint32_t)i;
    }
    drone.rng=0xA17B5EEDu;
}
static void drone_adopt(int col,float root) {
    drone.col=col; drone.root=root;
    if(drone_is_bed(col)) {
        /* Preparing a choir voice computes its tract once: at most three per
         * kind change, outside the per-sample loop. */
        static const float ratio[BED_VOICES]={1.0f,1.0f,1.4983071f};
        float upper=value(atomic_load_explicit(&bed_upper_bits,memory_order_acquire));
        for(int v=0;v<BED_VOICES;++v) {
            bed_voice_t *d=&drone.bed[v];
            ambient_choir_prepare(&d->choir,root*(v==1 ? upper : ratio[v]),0xC401u+977u*(uint32_t)v,.85f);
            ambient_motion_prepare(&d->motion,0xD7053u+0x9E3779B9u*(uint32_t)(v+1));
            d->lev=ambient_motion_peek(&d->motion,.45f,&d->drift);
        }
        for(int i=0;i<BED_TINES;++i) drone.tine[i].live=false;
        for(int i=0;i<BED_BELLS;++i) drone.bell[i].live=false;
        drone.next=(int32_t)(.3f*DSP_SAMPLE_RATE_HZ);
        return;
    }
    for(int v=0;v<DRONE_VOICES;++v) {
        drone_voice_t *d=&drone.v[v];
        d->beat=DRONE_COLOR[col].beat_hz[v]/(float)DSP_SAMPLE_RATE_HZ;
        dsp_svf_reset(&d->air1); dsp_svf_reset(&d->air2); d->air_hz=0;
    }
}
static inline float drone_noise(uint32_t *r) {
    *r=*r*1664525u+1013904223u;
    return (float)(int32_t)*r*(1.0f/2147483648.0f);
}
/* Harmonics 2-4 come from sin/cos of the fundamental (exact identities); the
 * slow drifts and the normalisation run at block rate and are interpolated
 * per sample: 3 table sines per voice and sample, no division. Breath adds
 * noise through two band-passes at the tone (coefficients only on a pitch
 * change). */
static void drone_tone(float *l,float *r,int n,float target,float root,bool fading,float gate_step) {
    const drone_color_t *c=&DRONE_COLOR[drone.col];
    const float up=1.0f/(1.2f*DSP_SAMPLE_RATE_HZ),down=1.0f/(1.8f*DSP_SAMPLE_RATE_HZ),glide=1.0f/(1.5f*DSP_SAMPLE_RATE_HZ);
    const float W0=c->w[0],W1=c->w[1],W2=c->w[2],W3=c->w[3],Wu=W1+W2+W3,step=1.0f/(float)n;
    float lev[DRONE_VOICES],dlev[DRONE_VOICES],dr[DRONE_VOICES],ddr[DRONE_VOICES],inv[DRONE_VOICES],dinv[DRONE_VOICES];
    for(int v=0;v<DRONE_VOICES;++v) {
        drone_voice_t *d=&drone.v[v];
        float drift1,lev1=ambient_motion_skip(&d->motion,.45f,n,&drift1);
        float l0=1.0f+(d->lev-1.0f)*c->bloom,l1=1.0f+(lev1-1.0f)*c->bloom;
        float t0=1.0f+c->tilt*(-.6f+d->drift),t1=1.0f+c->tilt*(-.6f+drift1);
        float i0=1.0f/(W0+t0*Wu),i1=1.0f/(W0+t1*Wu);
        lev[v]=l0; dlev[v]=(l1-l0)*step;
        dr[v]=d->drift; ddr[v]=(drift1-d->drift)*step;
        inv[v]=i0; dinv[v]=(i1-i0)*step;
        d->lev=lev1; d->drift=drift1;
        if(c->breath>0) {
            float hz=drone.root*(v ? 1.4983071f : 1.0f);
            if(fabsf(hz-d->air_hz)>.0005f*hz) {
                dsp_svf_set(&d->air1,hz,2.5f);
                d->air2.a1=d->air1.a1; d->air2.a2=d->air1.a2; d->air2.a3=d->air1.a3; d->air2.k=d->air1.k;
                d->air_hz=hz;
            }
        }
    }
    for(int i=0;i<n;++i) {
        drone.level+=(target-drone.level)*(target>drone.level ? up : down);
        if(!fading) drone.root+=(root-drone.root)*glide;
        drone.gate=dsp_clampf(drone.gate+gate_step,0,1);
        float g=drone.gate*drone.gate*(3.0f-2.0f*drone.gate)*c->trim;
        float sl=0,sr=0;
        for(int v=0;v<DRONE_VOICES;++v) {
            drone_voice_t *d=&drone.v[v];
            lev[v]+=dlev[v]; dr[v]+=ddr[v]; inv[v]+=dinv[v];
            float tilt=1.0f+c->tilt*(-.6f+dr[v]);
            float inc=drone.root*(v ? 1.4983071f : 1.0f)/(float)DSP_SAMPLE_RATE_HZ;
            float s1=dsp_sin(d->ph),c1=dsp_sin(d->ph+.25f),q=s1*s1;
            float s2=2.0f*s1*c1,s3=s1*(3.0f-4.0f*q),s4=2.0f*s2*(1.0f-2.0f*q);
            float tone=(W0*s1+tilt*(W1*s2+W2*s3+W3*s4))*inv[v];
            float gv=v ? c->fifth : 1.0f;
            float s=tone*lev[v]*gv;
            if(c->breath>0)
                s+=dsp_svf_bp(&d->air2,dsp_svf_bp(&d->air1,drone_noise(&d->rng)))*c->breath*(1.0f+.8f*dr[v])*gv;
            float p=dsp_sin(d->ph2)*c->partner*(2.0f-lev[v])*gv;
            d->ph+=inc; if(d->ph>=1.0f) d->ph-=1.0f;
            d->ph2+=inc+d->beat; if(d->ph2>=1.0f) d->ph2-=1.0f;
            /* Root centred with its partner opposite the fifth's. */
            if(v) { sl+=.62f*s+.38f*p; sr+=.38f*s+.62f*p; } else { sl+=.5f*s+.62f*p; sr+=.5f*s+.38f*p; }
        }
        l[i]+=drone.level*g*sl; r[i]+=drone.level*g*sr;
    }
}
/* The beds share the tone colours' ramps: level, glide (never while
 * dipping) and the dip gate, smoothstepped. */
static inline float bed_step(float target,float root,bool fading,float gate_step,float trim) {
    const float up=1.0f/(1.2f*DSP_SAMPLE_RATE_HZ),down=1.0f/(1.8f*DSP_SAMPLE_RATE_HZ),glide=1.0f/(1.5f*DSP_SAMPLE_RATE_HZ);
    drone.level+=(target-drone.level)*(target>drone.level ? up : down);
    if(!fading) drone.root+=(root-drone.root)*glide;
    drone.gate=dsp_clampf(drone.gate+gate_step,0,1);
    return drone.level*drone.gate*drone.gate*(3.0f-2.0f*drone.gate)*trim;
}
static inline float bed_unit(void) {
    drone.rng=drone.rng*1664525u+1013904223u;
    return (float)(drone.rng>>8)*(1.0f/16777216.0f);
}
static void bed_choir(float *l,float *r,int n,float target,float root,bool fading,float gate_step) {
    static const float gain[BED_VOICES]={1.0f,.45f,.62f},pan[BED_VOICES]={.5f,.35f,.65f};
    const float upper=value(atomic_load_explicit(&bed_upper_bits,memory_order_acquire));
    const float ratio[BED_VOICES]={1.0f,upper,1.4983071f},step=1.0f/(float)n;
    float lev[BED_VOICES],dlev[BED_VOICES];
    for(int v=0;v<BED_VOICES;++v) {
        bed_voice_t *d=&drone.bed[v];
        float drift1,lev1=ambient_motion_skip(&d->motion,.45f,n,&drift1);
        lev[v]=d->lev; dlev[v]=(lev1-d->lev)*step; d->lev=lev1; d->drift=drift1;
        ambient_choir_retune(&d->choir,drone.root*ratio[v]);
    }
    for(int i=0;i<n;++i) {
        float g=bed_step(target,root,fading,gate_step,DRONE_TRIM_CHOIR),sl=0,sr=0;
        for(int v=0;v<BED_VOICES;++v) {
            float side,x=ambient_choir_sample(&drone.bed[v].choir,-.4f,&side)*(lev[v]+=dlev[v])*gain[v];
            sl+=(1.0f-pan[v])*x+pan[v]*side; sr+=pan[v]*x+(1.0f-pan[v])*side;
        }
        l[i]+=g*sl; r[i]+=g*sr;
    }
}
static void bed_kalimba(float *l,float *r,int n,float target,float root,bool fading,float gate_step) {
    float pitch[BED_DROPS];
    for(int k=0;k<BED_DROPS;++k) pitch[k]=value(atomic_load_explicit(&bed_drop_bits[k],memory_order_acquire));
    for(int i=0;i<n;++i) {
        float g=bed_step(target,root,fading,gate_step,DRONE_TRIM_KALIMBA),sl=0,sr=0;
        if(--drone.next<=0) {
            bool pair=bed_unit()<.15f;
            drone.next=(int32_t)((pair ? .12f+.10f*bed_unit() : .6f+1.6f*bed_unit())*DSP_SAMPLE_RATE_HZ);
            for(int k=0;k<BED_TINES;++k) if(!drone.tine[k].live) {
                float hz=pitch[(int)(bed_unit()*BED_DROPS)%BED_DROPS];
                ambient_kalimba_prepare(&drone.tine[k].body,hz>0 && hz<4000 ? hz : 440,.45f+.35f*bed_unit(),.15f+.15f*bed_unit(),1.0f);
                drone.tine[k].pan=.25f+.5f*bed_unit(); drone.tine[k].live=true;
                break;
            }
        }
        for(int k=0;k<BED_TINES;++k) if(drone.tine[k].live) {
            float x=ambient_modal_step(&drone.tine[k].body);
            sl+=x*(1.0f-drone.tine[k].pan); sr+=x*drone.tine[k].pan;
        }
        l[i]+=g*sl; r[i]+=g*sr;
    }
    for(int k=0;k<BED_TINES;++k) if(drone.tine[k].live && ambient_modal_energy(&drone.tine[k].body)<1e-10f) drone.tine[k].live=false;
}
static void bed_bells(float *l,float *r,int n,float target,float root,bool fading,float gate_step) {
    static const float ratio[3]={1.0f,1.4983071f,2.0f};
    for(int i=0;i<n;++i) {
        float g=bed_step(target,root,fading,gate_step,DRONE_TRIM_BELLS),sl=0,sr=0;
        if(--drone.next<=0) {
            drone.next=(int32_t)((3.5f+3.5f*bed_unit())*DSP_SAMPLE_RATE_HZ);
            int best=0; float least=1e30f;
            for(int k=0;k<BED_BELLS;++k) {
                float e=drone.bell[k].live ? ambient_modal_energy(&drone.bell[k].body) : 0;
                if(e<least) { least=e; best=k; }
            }
            if(least<1e-6f || !drone.bell[best].live) {
                ambient_bell_prepare(&drone.bell[best].body,drone.root*ratio[(int)(bed_unit()*3.0f)%3],.5f+.3f*bed_unit(),.20f,1.2f);
                drone.bell[best].pan=.3f+.4f*bed_unit(); drone.bell[best].live=true;
            }
        }
        for(int k=0;k<BED_BELLS;++k) if(drone.bell[k].live) {
            float x=ambient_modal_step(&drone.bell[k].body);
            sl+=x*(1.0f-drone.bell[k].pan); sr+=x*drone.bell[k].pan;
        }
        l[i]+=g*sl; r[i]+=g*sr;
    }
    for(int k=0;k<BED_BELLS;++k) if(drone.bell[k].live && ambient_modal_energy(&drone.bell[k].body)<1e-10f) drone.bell[k].live=false;
}
static void drone_render(float *l,float *r,int n) {
    float target=value(atomic_load_explicit(&drone_level_bits,memory_order_acquire));
    if(target<=0 && drone.level<1e-6f) { drone.level=0; drone.col=0; return; }
    float root=value(atomic_load_explicit(&drone_root_bits,memory_order_acquire));
    int want=(int)atomic_load_explicit(&drone_color_bits,memory_order_acquire);
    if(!(root>0 && root<1000) || want<=ENGINE_DRONE_AUTO || want>=ENGINE_DRONE_COLORS || n<=0) return;
    if(drone.col==0) { drone_adopt(want,root); drone.gate=1; }
    else if(want!=drone.col && drone.gate<=0) drone_adopt(want,root);
    const bool fading=want!=drone.col;
    const float gate_step=fading ? -1.0f/(.5f*DSP_SAMPLE_RATE_HZ) : 1.0f/(1.5f*DSP_SAMPLE_RATE_HZ);
    if(drone.col==ENGINE_DRONE_CHOIR) bed_choir(l,r,n,target,root,fading,gate_step);
    else if(drone.col==ENGINE_DRONE_KALIMBA) bed_kalimba(l,r,n,target,root,fading,gate_step);
    else if(drone.col==ENGINE_DRONE_BELLS) bed_bells(l,r,n,target,root,fading,gate_step);
    else drone_tone(l,r,n,target,root,fading,gate_step);
}
/* Generate as loops: state here, logic next to the World ticks. */
typedef struct {
    uint32_t due_ms;                      /* the loop's own clock: next onset */
    uint32_t next_ms;                     /* next attempt (due, or a retry) */
    int8_t step[2];                       /* steps through the loop's band */
    uint8_t len,phase,fires,evolve_at,retries;
    int midi;
    bool pending;
} gen_loop_t;
static gen_loop_t gen_loops[3];
static uint32_t gen_rng;
static float gen_scale=1.0f;              /* one tempo per performance: keeps the ratios */
static void gen_loop_heard(int role,source_slot_t *s);
static void gen_loops_reset(uint32_t ms);
static void gen_loops_cancel(void) { for(int r=0;r<3;++r) gen_loops[r].pending=false; }
#endif
static void remember(float hz) {
    /* Max room T60 4.8 s: nominal -90 dB after another 1.5*T60 from real
     * source retirement. Also allow 250 ms of measured quiet wet return.
     * Reserve all three outgoing slots; never evict an audible history item. */
    uint32_t until=clock_frames()+(uint32_t)(
#ifdef FAM_SOUND_FINISH_CANDIDATE
        14.0f*DSP_SAMPLE_RATE_HZ
#else
        7.2f*DSP_SAMPLE_RATE_HZ
#endif
    );
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
#ifdef FAM_SOUND_FINISH_CANDIDATE
        cell_retiring=0;
#endif
        coast_pending=0; woodland_pending=highlands_pending=false;
        clearing=false; atomic_store(&clear_request,false);
        if(resume_nature && !muted) {
            nature_set_amount(nature_amount);
            if(nature_amount>0) atomic_store(&output_enabled,true);
        }
        resume_nature=false;
#ifdef FAM_SOUND_AMBIENT_V3
        /* A Flush keeps the DRONE mode: its bed fades back in after Clear. */
        if(drone_on && !muted) atomic_store(&output_enabled,true);
#endif
    }
    uint32_t frames=clock_frames();
    bool quiet=atomic_load_explicit(&room_quiet,memory_order_acquire);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    bool frozen=ambient_room_retains_history();
    if(freeze_seen && !frozen)for(int i=0;i<TAILS;++i)if(tails[i].used)tails[i].until=frames+14u*DSP_SAMPLE_RATE_HZ;
    freeze_seen=frozen;
    quiet=quiet && !frozen;
#endif
    for(int i=0;i<TAILS;++i) if(tails[i].used &&
       (quiet || (
#ifdef FAM_SOUND_FINISH_CANDIDATE
        !frozen &&
#endif
        (int32_t)(frames-tails[i].until)>=0))) tails[i].used=false;
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
            for(int role=0;role<3;++role) if(s->owner==coast_owners[role] &&
                                          (coast_pending&(1u<<role))) {
                coast_offer_t *o=&coast_tickets[role];
                coast_pending&=(uint8_t)~(1u<<role);
                if(!clearing && !muted && !suppressed && generate && autoplay &&
                   world==WORLD_COAST && coast_phrase_heard(&coast,o,now_ms)) {
                    last_midi=o->midi; ++note_count;
#ifdef FAM_SOUND_FINISH_CANDIDATE
                    last_voicing[world][o->role]=o->midi;
#endif
                    if(o->hold_ms) {s->timed=true; s->off_ms=now_ms+o->hold_ms;}
                    if(o->phase==3) for(int k=0;k<SLOTS;++k)
                        if(slots[k].used && slots[k].owner==coast_owners[2] && !slots[k].released) {
                            slots[k].timed=true; slots[k].off_ms=now_ms+o->hold_ms;
                        }
                }
            }
            if(woodland_pending && s->owner==coast_owners[woodland_ticket.role]) {
                woodland_pending=false;
                if(!clearing && !muted && !suppressed && generate && autoplay &&
                   world==WORLD_WOODLAND && woodland_phrase_heard(&woodland,&woodland_ticket,now_ms)) {
                    last_midi=woodland_ticket.midi;++note_count;
#ifdef FAM_SOUND_FINISH_CANDIDATE
                    last_voicing[world][woodland_ticket.role]=woodland_ticket.midi;
#endif
                    s->timed=true;s->off_ms=now_ms+woodland_ticket.hold_ms;
                }
            }
            if(highlands_pending && s->owner==coast_owners[highlands_ticket.role]) {
                highlands_pending=false;
                if(!clearing && !muted && !suppressed && generate && autoplay &&
                   world==WORLD_HIGHLANDS && highlands_phrase_heard(&highlands,&highlands_ticket,now_ms)) {
                    highlands_offer_t *o=&highlands_ticket;
                    last_midi=o->midi;++note_count;
#ifdef FAM_SOUND_FINISH_CANDIDATE
                    last_voicing[world][o->role]=o->midi;
#endif
                    if(o->hold_ms) {s->timed=true;s->off_ms=now_ms+o->hold_ms;}
                    /* The shared third stays in its original source through
                     * both actual partner starts. Release it with the last
                     * partner; longer DSP retirement delays remain owned. */
                    if(o->phase==4)for(int k=0;k<SLOTS;++k)
                        if(slots[k].used && slots[k].owner==coast_owners[2] && !slots[k].released) {
                            slots[k].timed=true;slots[k].off_ms=now_ms+o->hold_ms;
                        }
                }
            }
#ifdef FAM_SOUND_AMBIENT_V3
            for(int role=0;role<3;++role)
                if(gen_loops[role].pending && s->owner==coast_owners[role]) gen_loop_heard(role,s);
#endif
        }
        if(epoch!=s->issued_epoch && !(mask&(1u<<s->owner))) {
            if(s->on_sent && !s->off_sent && note_hook) note_hook(0,s->owner,s->hz,0);
            if(s->started) remember(s->hz);
#ifdef FAM_SOUND_FINISH_CANDIDATE
            cell_retiring &= ~(1u << s->owner);
#endif
            for(int role=0;role<3;++role) if(s->owner==coast_owners[role])
                coast_pending&=(uint8_t)~(1u<<role);
            if(woodland_pending && s->owner==coast_owners[woodland_ticket.role])woodland_pending=false;
            if(highlands_pending && s->owner==coast_owners[highlands_ticket.role])highlands_pending=false;
#ifdef FAM_SOUND_AMBIENT_V3
            /* Cancelled before it was heard: that loop simply tries again soon. */
            for(int role=0;role<3;++role) if(gen_loops[role].pending && s->owner==coast_owners[role]) {
                gen_loops[role].pending=false; gen_loops[role].next_ms=now_ms+700u;
            }
#endif
            s->used=false;
        }
    }
}
static bool compatible(float a,float b,bool enforce_register) {
    float cents=fabsf(1200.0f*log2f(a/b));
    if(cents<3) return true; /* room tails may share an exactly tuned tone */
    if(cents<90) return false; /* different JI/ET versions of the same MIDI */
    float cls=fmodf(cents,1200);
    if(cls>600) cls=1200-cls;
    if(fabsf(cls-100)<45 || fabsf(cls-600)<45) return false;
    if(!enforce_register) return true;
    float lo=fminf(a,b);
    if(lo<261.63f && cents<249.5f) return false;
    if(lo<196 && cents<299.5f) return false;
    return true;
}
static bool source_pitch_conflict(float hz,float other) {
    /* Two independently restarted identical oscillators can cancel. */
    return fabsf(1200.0f*log2f(hz/other))<3 || !compatible(hz,other,true);
}
static bool tail_pitch_clear(float hz,bool cell,int reserve) {
    bool frozen=false;
#ifdef FAM_SOUND_FINISH_CANDIDATE
    frozen=ambient_room_retains_history();
#endif
    int n=0;
    for(int i=0;i<TAILS;++i) if(tails[i].used) {
        ++n; if(!compatible(hz,tails[i].hz,!cell || frozen)) {
            note_block=frozen ? ENGINE_NOTE_FROZEN_PITCH : ENGINE_NOTE_TAIL_PITCH;
            return false;
        }
    }
    if(n+reserve>=TAILS-SLOTS) {note_block=ENGINE_NOTE_VOICES_FULL;return false;}
    return true;
}
static bool pitch_clear(float hz) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used && source_pitch_conflict(hz,slots[i].hz)) {
        note_block=ENGINE_NOTE_HELD_PITCH;return false;
    }
    return tail_pitch_clear(hz,false,0);
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
#ifdef FAM_SOUND_AMBIENT_V3
/* Ambient voicing: Generate keeps its grammar, memory and voice leading and
 * the per-World ceilings, but every phrase pitch below A3 sounds an octave
 * higher. Phrases live in the comfortable middle (~220-830 Hz): no 110 Hz
 * drones and no 1 kHz whistles held for seconds. Cells use the D4.. window
 * of world_pitch_midi, SHIFT is an exact octave above. Live admission keeps
 * the previous D3.. window too (world_live_pitch_allowed). */
enum { PHRASE_LIFT_BELOW=57, CELL_SHIFT_STEPS=5 };
#define PHRASE_LO_HZ 200.0f
#define CELL_LO_HZ 140.0f
#define CELL_HI_HZ 1250.0f
static int phrase_midi(int midi) { return midi<PHRASE_LIFT_BELOW ? midi+12 : midi; }
#else
enum { CELL_SHIFT_STEPS=3 };
#define PHRASE_LO_HZ 105.0f
#define CELL_LO_HZ 140.0f
#define CELL_HI_HZ 470.0f
static int phrase_midi(int midi) { return midi; }
#endif
static float phrase_hz(int midi) { return tuning_hz((float)phrase_midi(midi)); }
static float phrase_hi_hz(void) {
    return world==WORLD_COAST ? 850.0f : world==WORLD_HIGHLANDS ? 650.0f : 470.0f;
}
static bool in_collection(float hz,bool phrase) {
    if(phrase) {
        int top=world==WORLD_WOODLAND ? 68 : world==WORLD_HIGHLANDS ? 75 : 80;
        for(int m=45;m<=top;++m) if(world==WORLD_WOODLAND ?
                woodland_phrase_pitch_allowed(m,tonic_pc,minor!=0) : world==WORLD_HIGHLANDS ?
                highlands_phrase_pitch_allowed(m,tonic_pc,minor!=0) : coast_phrase_pitch_allowed(m,tonic_pc,minor!=0)) {
            float allowed=phrase_hz(m);
            if(fabsf(1200.0f*log2f(hz/allowed))<3)return true;
        }
        return false;
    }
#ifdef FAM_SOUND_AMBIENT_V3
    for(int m=WORLD_LIVE_MIDI_LOW;m<=WORLD_LIVE_MIDI_HIGH;++m) if(world_live_pitch_allowed(m,tonic_pc,minor!=0)) {
        if(fabsf(1200.0f*log2f(hz/tuning_hz((float)m)))<3) return true;
    }
#else
    int n=world_pitch_count(tonic_pc,minor!=0);
    for(int i=0;i<n;++i) {
        float allowed=tuning_hz((float)world_pitch_midi(i,tonic_pc,minor!=0));
        if(fabsf(1200.0f*log2f(hz/allowed))<3) return true;
    }
#endif
    return false; /* Reject outside pitches; never silently clamp or transpose. */
}
static bool note_request_valid(uint8_t owner,float hz,float velocity,bool phrase) {
    note_block=ENGINE_NOTE_OK;
    if(clearing || muted) {note_block=ENGINE_NOTE_SILENCED;return false;}
    if(owner>=SOURCES || !isfinite(hz) || !isfinite(velocity) || velocity<=0 ||
       hz<(phrase ? PHRASE_LO_HZ : CELL_LO_HZ) || hz>(phrase ? phrase_hi_hz() : CELL_HI_HZ) ||
       !in_collection(hz,phrase)) {note_block=ENGINE_NOTE_INVALID;return false;}
    return true;
}
static bool start_source(int empty,uint8_t owner,float hz,float velocity) {
    velocity=dsp_clampf(velocity,0,1);
    /* Fixed dry-source calibration candidate. Shared by manual/generator;
     * no master AGC, automatic gain compensation or implicit second source. */
    float woodland_level=.22f;
#ifdef FAM_SOUND_FINISH_CANDIDATE
    woodland_level=.32f; /* compensate the broader, peak-bounded modal body */
#endif
    float amp=velocity*(world==WORLD_WOODLAND ? woodland_level : .50f);
    /* Main is the sole request writer. Reap proved this owner has no old
     * source, but its release/quick request may have been posted after the
     * retiring audio IRQ. Discard those old commands before publishing the
     * new preparation. Audio only consumes bits; an IRQ at either clear is
     * safe because the new source is not yet visible. No IRQ masking or
     * added work in render; these two word RMWs run only in Main. */
    SOURCE_HANDOFF_POINT(4,owner);
    atomic_fetch_and_explicit(&release_mask,~(1u<<owner),memory_order_acq_rel);
    SOURCE_HANDOFF_POINT(5,owner);
    uint32_t stop_bits=1u<<owner;
#ifdef FAM_SOUND_FINISH_CANDIDATE
    stop_bits|=stop_bits<<SOURCES;
#endif
    atomic_fetch_and_explicit(&quick_mask,~stop_bits,memory_order_acq_rel);
    engine_set_cell_held(owner,false);
    SOURCE_HANDOFF_POINT(6,owner);
    bool ok=world==WORLD_COAST ? bowed_try_note_on(owner,hz,amp) :
            world==WORLD_WOODLAND ? pluck_note_on(owner,hz,amp) : horn_try_note_on(owner,hz,amp);
    SOURCE_HANDOFF_POINT(7,owner);
    if(!ok) {note_block=ENGINE_NOTE_VOICES_FULL;return false;}
    slots[empty]=(source_slot_t){.hz=hz,.velocity=velocity,.owner=owner,.family=(uint8_t)world,
        .used=true,.held=world!=WORLD_WOODLAND,.issued_epoch=atomic_load(&audio_epoch)};
    SOURCE_HANDOFF_POINT(8,owner);
    nature_set_amount(nature_amount);
    atomic_store_explicit(&output_enabled,true,memory_order_release);
    SOURCE_HANDOFF_POINT(9,owner);
    return true;
}
static bool admit(uint8_t owner,float hz,float velocity,bool phrase) {
    reap();
    if(!note_request_valid(owner,hz,velocity,phrase))return false;
#ifdef FAM_SOUND_FINISH_CANDIDATE
    /* Already accepted live Cells own the slots reserved for their handover. */
    if(cell_queue_count) {note_block=ENGINE_NOTE_VOICES_FULL;return false;}
#endif
    int empty=-1,local=0;
    for(int i=0;i<SLOTS;++i) {
        if(!slots[i].used) { if(empty<0) empty=i; continue; }
        if(slots[i].owner==owner) {
            note_block=slots[i].released ? ENGINE_NOTE_TAIL_PITCH : ENGINE_NOTE_HELD_PITCH;
            return false;
        }
        local+=slots[i].family==world;
    }
    if(empty<0 || (world==WORLD_WOODLAND && local>=pluck_capacity())) {note_block=ENGINE_NOTE_VOICES_FULL;return false;}
    if(!pitch_clear(hz))return false;
    return start_source(empty,owner,hz,velocity);
}
static bool cell_owner(uint8_t owner) {
    return owner<5 || (owner>=9 && owner<=13);
}
#ifdef FAM_SOUND_FINISH_CANDIDATE
static bool plan_cell_press(uint8_t owner,float hz,float velocity) {
    reap();
    if(!note_request_valid(owner,hz,velocity,false))return false;
    unsigned total=cell_queue_count,local=cell_queue_count;
    for(unsigned i=0;i<cell_queue_count;++i) {
        if(cell_queue[i].owner==owner || source_pitch_conflict(hz,cell_queue[i].hz)) {
            note_block=ENGINE_NOTE_HELD_PITCH;
            return false;
        }
    }

    uint32_t victims=0;
    bool frozen=ambient_room_retains_history();
    int empty=-1,active=0;
    for(int i=0;i<SLOTS;++i) {
        const source_slot_t *s=&slots[i];
        if(!s->used) { if(empty<0)empty=i; continue; }
        ++active;
        uint32_t bit=1u<<s->owner;
        bool conflict=s->owner==owner || source_pitch_conflict(hz,s->hz);
        if(!(cell_retiring&bit) && (!conflict || !s->released || !cell_owner(s->owner))) {
            if(conflict) {
                note_block=s->released ? ENGINE_NOTE_TAIL_PITCH : ENGINE_NOTE_HELD_PITCH;
                return false;
            }
            ++total;
            local+=s->family==world;
            continue;
        }
        /* A released Cell may yield its direct source. Its wet history still
         * enforces pitch identity, semitones and tritones, or the full rule
         * if Freeze is retaining material. */
        if(!compatible(hz,s->hz,frozen)) {
            note_block=frozen ? ENGINE_NOTE_FROZEN_PITCH : ENGINE_NOTE_TAIL_PITCH;
            return false;
        }
        victims|=bit;
    }
    bool local_full=world==WORLD_WOODLAND && local>=(unsigned)pluck_capacity();
    if(total>=SLOTS || local_full) {
        int oldest=-1;
        uint32_t age=0,epoch=atomic_load(&audio_epoch);
        for(int i=0;i<SLOTS;++i) {
            const source_slot_t *s=&slots[i];
            if(!s->used || !s->released || !cell_owner(s->owner) ||
               (victims&(1u<<s->owner)) || (local_full && s->family!=world))continue;
            uint32_t candidate_age=epoch-s->issued_epoch;
            if(oldest<0 || candidate_age>age) { oldest=i; age=candidate_age; }
        }
        if(oldest<0) {note_block=ENGINE_NOTE_VOICES_FULL;return false;}
        victims|=1u<<slots[oldest].owner;
    }
    bool wait=victims || cell_queue_count;
    if(!tail_pitch_clear(hz,true,wait ? active : 0))return false;
    if(!wait)return start_source(empty,owner,hz,velocity);

    cell_queue[cell_queue_count++]=(cell_press_t){.hz=hz,.velocity=velocity,.owner=owner};
    uint32_t fade=victims&~cell_retiring;
    cell_retiring|=fade;
    if(fade)atomic_fetch_or_explicit(&quick_mask,fade<<SOURCES,memory_order_release);
    return true;
}
bool engine_try_cell_on(uint8_t owner,float hz,float velocity) {
    if(generate || !cell_owner(owner))return engine_try_note_on(owner,hz,velocity);
    if(plan_cell_press(owner,hz,velocity))return true;
    ++rejects;
    return false;
}
static void service_cell_queue(void) {
    if(cell_retiring || !cell_queue_count)return;
    unsigned count=cell_queue_count;
    cell_queue_count=0;
    for(unsigned q=0;q<count;++q) {
        const cell_press_t *press=&cell_queue[q];
        if(!tail_pitch_clear(press->hz,true,0))continue; /* e.g. new Freeze capture */
        for(int i=0;i<SLOTS;++i) if(!slots[i].used) {
            if(start_source(i,press->owner,press->hz,press->velocity))
                engine_set_cell_held(press->owner,press->down);
            break;
        }
    }
}
#endif
void engine_set_world_character(bool enabled) {
    world_character=enabled;
    bowed_set_character(seed,enabled); horn_set_character(seed,enabled); pluck_set_character(seed,enabled);
}
void engine_set_woodland_body(bool enabled) { pluck_set_body(enabled); }
void engine_set_woodland_resonator(bool enabled) { pluck_set_resonator(enabled); }
void engine_set_cell_held(uint8_t owner,bool down) {
    if(!cell_owner(owner)) return;
#ifdef FAM_SOUND_FINISH_CANDIDATE
    for(unsigned i=0;i<cell_queue_count;++i) if(cell_queue[i].owner==owner) {
        cell_queue[i].down=down;
        return;
    }
#endif
    if(down) {
        bool admitted=false;
        for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].owner==owner && !slots[i].released)
            admitted=true;
        if(!admitted || clearing || muted || generate) return;
    }
    bowed_set_cell_held(owner,down); horn_set_cell_held(owner,down); pluck_set_cell_held(owner,down);
}
static void clear_cell_unfold(void) {
    for(uint8_t i=0;i<SOURCES;++i) engine_set_cell_held(i,false);
}
bool engine_try_note_on(uint8_t owner,float hz,float v) {
    if(generate) {note_block=ENGINE_NOTE_GENERATE;return false;}
    if(!cell_owner(owner)) {note_block=ENGINE_NOTE_INVALID;return false;}
    bool ok=admit(owner,hz,v,false); if(!ok) ++rejects; return ok;
}
void engine_note_on(uint8_t owner,float hz,float legacy_amp) {
    /* Existing Hall/MIDI adapters pass CELL_AMP_MIN..MAX. Product API above
     * is explicitly normalised velocity; physical mapping remains separate. */
    if(isfinite(legacy_amp)) (void)engine_try_note_on(owner,hz,legacy_amp/CELL_AMP_MAX);
}
bool engine_try_world_note_on(uint8_t owner,float hz,float v) {
    if(!generate || !autoplay || suppressed || !(owner==6 || owner==7 || owner==15)) return false;
    bool ok=admit(owner,hz,v,false); if(!ok) ++rejects; return ok;
}
void engine_note_off(uint8_t owner) {
    if(owner>=SOURCES) return;
    engine_set_cell_held(owner,false);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    for(unsigned i=0;i<cell_queue_count;++i) if(cell_queue[i].owner==owner) {
        --cell_queue_count;
        memmove(&cell_queue[i],&cell_queue[i+1],(cell_queue_count-i)*sizeof cell_queue[0]);
        break;
    }
#endif
    reap();
    for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].owner==owner && !slots[i].released) {
        source_slot_t *s=&slots[i]; s->held=s->timed=false; s->released=true;
        /* Manual string key-up retains its natural ring. Generate's long
         * chord holds end with the source's own soft musical release.
         * Cancel preparations before their first DSP sample.
         * Context handovers still use quick_mask; Clear/Mute gate all paths. */
        if(s->family!=WORLD_WOODLAND || !s->started || owner==6 || owner==7 || owner==15
#ifdef FAM_SOUND_FINISH_CANDIDATE
           || owner==8 || owner==14
#endif
        ) {
            SOURCE_HANDOFF_POINT(2,owner);
            atomic_fetch_or_explicit(&release_mask,1u<<owner,memory_order_release);
            SOURCE_HANDOFF_POINT(3,owner);
        }
        if(s->on_sent && !s->off_sent) {
            if(note_hook) note_hook(0,owner,s->hz,0);
            s->off_sent=true;
        }
    }
}
static void release_all(void) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used) engine_note_off(slots[i].owner);
}
static void retire_context(bool all) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used &&
       (all || slots[i].owner==6 || slots[i].owner==7 || slots[i].owner==15)) {
        uint8_t owner=slots[i].owner;
#ifdef FAM_SOUND_FINISH_CANDIDATE
        if(cell_owner(owner))cell_retiring|=1u<<owner;
#endif
        SOURCE_HANDOFF_POINT(0,owner);
        engine_note_off(owner);
        SOURCE_HANDOFF_POINT(1,owner);
        atomic_fetch_or_explicit(&quick_mask,1u<<owner,memory_order_release);
    }
    coast_pending=0;woodland_pending=highlands_pending=false;
#ifdef FAM_SOUND_AMBIENT_V3
    gen_loops_cancel();
#endif
}
static void release_generated(void) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used &&
       (slots[i].owner==6 || slots[i].owner==7 || slots[i].owner==15)) engine_note_off(slots[i].owner);
    coast_pending=0; coast_phrase_restart(&coast);
    woodland_pending=false;woodland_phrase_restart(&woodland);
    highlands_pending=false;highlands_phrase_restart(&highlands);
#ifdef FAM_SOUND_AMBIENT_V3
    gen_loops_cancel();
#endif
}
static void quiet_chain(void) {
#ifdef FAM_SOUND_FINISH_CANDIDATE
    gesture_stop(now_ms);
    cell_queue_count=0;
#endif
    clear_cell_unfold();
    coast_pending=0; coast_phrase_restart(&coast); resume_nature=false;
    woodland_pending=false;woodland_phrase_restart(&woodland);
    highlands_pending=false;highlands_phrase_restart(&highlands);
#ifdef FAM_SOUND_AMBIENT_V3
    gen_loops_cancel();
#endif
    nature_set_amount(0); release_all(); clearing=true;
    atomic_store(&clear_done,false); atomic_store(&output_enabled,false);
    atomic_store_explicit(&clear_request,true,memory_order_release);
    if(note_hook) note_hook(-1,0,0,0);
}
void engine_all_off(void) {
    reap(); generate=false; suppressed=false; quiet_chain();
}
void engine_set_muted(bool on) {
    reap(); if(muted==on) return;
    muted=on;
    if(on) quiet_chain();
    else {
        /* Restore only the optional layer target; old tonal/room state is
         * gone. Generate keeps intent and heard memory, and schedules a new
         * event after clear acknowledgement. No held-key retrigger. */
        resume_nature=clearing;
        if(!clearing) {
            nature_set_amount(nature_amount);
            if(nature_amount>0) atomic_store(&output_enabled,true);
        }
    }
#ifdef FAM_SOUND_AMBIENT_V3
    drone_publish();
#endif
}
bool engine_muted(void) { return muted; }
bool engine_clear_pending(void) { reap(); return clearing; }
void engine_set_note_hook(engine_note_hook_t h) { note_hook=h; }
void engine_set_master_volume(float v) {
    if(isfinite(v)) atomic_store_explicit(&volume_bits,bits(dsp_clampf(v,0,1)),memory_order_release);
}
void engine_boot_mute(void) { volume_cur=0; engine_set_master_volume(0); } /* before DMA starts */
void engine_set_color(float v) {
    if(!isfinite(v)) return;
    color=dsp_clampf(v,0,1); atomic_store(&color_bits,bits(color));
    bowed_set_tone(color); horn_set_tone(color);
    pluck_set_damp(.025f-.020f*.5f); pluck_set_color(color);
}
void engine_set_activity(float v) { if(isfinite(v)) activity=dsp_clampf(v,0,1); }
void engine_set_room(float v) { if(isfinite(v)) { room=dsp_clampf(v,0,1); ambient_room_set(room); } }
void engine_set_nature(float v) {
    if(!isfinite(v)) return;
    nature_amount=dsp_clampf(v,0,1); nature_set_amount(clearing || muted ? 0 : nature_amount);
    /* An explicit optional-Nature target is audible without a phantom note.
     * Init/boot/World defaults are zero; Clear still ends the whole session. */
    if(nature_amount>0 && !clearing && !muted) atomic_store_explicit(&output_enabled,true,memory_order_release);
}
#ifdef FAM_SOUND_AMBIENT_V3
/* The selected World's instrument, space and effects; Main, future notes. */
static void world_strings_tune(void) {
    if(world_id<CORE_WORLD_COUNT) return;
    /* Tonic from A3, fifth, octave, and the World's third (else sixth,
     * else fourth) an octave up: four strings in every key and collection. */
    int pc=tonic_pc<0 ? 2 : tonic_pc,rm=57+(((pc-57)%12)+12)%12;
    static const int colour[4]={4,3,9,5}; int up=7;
    for(int c=3;c>=0;--c) if(world_live_pitch_allowed(rm+12+colour[c],pc,minor!=0)) up=colour[c];
    float hz[WFX_STRINGS]={tuning_hz((float)rm),tuning_hz((float)(rm+7)),tuning_hz((float)(rm+12)),tuning_hz((float)(rm+12+up))};
    wfx_tune(hz,WFX_PRESET[world_id-CORE_WORLD_COUNT+1].t60);
}
static void world_voicing_apply(void) {
    bowed_set_choir(world_id==WORLD_FJORD);
    pluck_set_kalimba(world_id==WORLD_MEADOW);
    horn_set_bells(world_id==WORLD_GLACIER);
    ambient_room_set_world(world_id);
    world_strings_tune();
    wfx_set_preset(world_id>=CORE_WORLD_COUNT ? world_id-CORE_WORLD_COUNT+1 : 0);
}
#endif
void engine_set_world(int i) {
#ifdef FAM_SOUND_AMBIENT_V3
    if(i<0 || i>=AMBIENT_WORLD_COUNT || i==world_id) return;
#else
    if(i<0 || i>=CORE_WORLD_COUNT || i==world) return;
#endif
    clear_cell_unfold();
#ifdef FAM_SOUND_FINISH_CANDIDATE
    reset_voicing();
    cell_queue_count=0;
#endif
#ifdef FAM_SOUND_AMBIENT_V3
    reap(); retire_context(false); world=i%CORE_WORLD_COUNT; world_id=i; nature_set_world(world);retry_ms=now_ms;
#else
    reap(); retire_context(false); world=i; nature_set_world(world);retry_ms=now_ms;
#endif
    coast_pending=0; coast_phrase_init(&coast,seed);
    woodland_pending=false;woodland_phrase_init(&woodland,seed);
    highlands_pending=false;highlands_phrase_init(&highlands,seed);
#ifdef FAM_SOUND_AMBIENT_V3
    world_voicing_apply();
    gen_loops_reset(now_ms);
    drone_publish(); /* AUTO colour follows the World */
#endif
}
#ifdef FAM_SOUND_AMBIENT_V3
int engine_product_world(void) { return world_id; }
#else
int engine_product_world(void) { return world; }
#endif
int engine_product_collection(void) { return minor; }
int engine_product_cell_midi(int cell,bool upper) {
    if(cell<0 || cell>=CELL_COUNT) return -1;
    return world_pitch_midi(cell+(upper ? CELL_SHIFT_STEPS : 0),tonic_pc,minor!=0);
}
#ifdef FAM_SOUND_FINISH_CANDIDATE
bool engine_apply_sound_parameters(const sound_parameters_t *p) {
    return ambient_room_finish_apply(p);
}
void engine_get_sound_parameters(sound_parameters_t *p) {ambient_room_finish_get(p);}
void engine_set_sound_effects(float delay,float grain,bool freeze,float drive,float crush,float phaser,bool plate) {
    sound_parameters_t p;engine_get_sound_parameters(&p);
    p.delay=delay;p.grain=grain;p.drive=drive;p.crush=crush;p.phaser=phaser;p.freeze=freeze;p.plate=plate;
    (void)engine_apply_sound_parameters(&p);
}
bool engine_gesture_note(uint8_t cell,bool upper,bool pressed,float velocity,uint32_t ms) {
    (void)ms;if(cell>=5)return false;
    static const uint8_t owners[2]={8,14};uint8_t lane=(uint8_t)(1+cell+5*upper);
    reap();
    if(!pressed) {
        for(int k=0;k<2;++k)if(loop_lane[k]==lane) {engine_note_off(owners[k]);loop_lane[k]=0;}
        return true;
    }
    if(generate || clearing || muted || !isfinite(velocity) || velocity<=0)return false;
    for(int k=0;k<2;++k) {
        bool active=false;
        for(int i=0;i<SLOTS;++i)if(slots[i].used && slots[i].owner==owners[k])active=true;
        if(!active)loop_lane[k]=0;
        if(loop_lane[k]==lane)return true;
        if(!loop_lane[k] && admit(owners[k],tuning_hz((float)engine_product_cell_midi(cell,upper)),velocity,false)) {
            loop_lane[k]=lane;return true;
        }
    }
    return false;
}
#endif
static void reset_score_context(void) {
#ifdef FAM_SOUND_FINISH_CANDIDATE
    reset_voicing();
    cell_queue_count=0;
#endif
    reap();
    /* A prepared old-context note is not a newly heard new-context event.
     * Already acknowledged voices keep their actual Hz. Forget degree memory
     * when its collection/register meaning changes, rather than relabel it. */
    release_generated();
    coast_pending=0; coast_phrase_init(&coast,seed);
    woodland_pending=false;woodland_phrase_init(&woodland,seed);
    highlands_pending=false;highlands_phrase_init(&highlands,seed);
    retry_ms=now_ms;
}
void engine_set_key_pc(int pc) {
    pc%=12; if(pc<0) pc+=12;
    if(pc==tonic_pc)return;
    reset_score_context();tonic_pc=pc;
    int root=50+((pc-2+12)%12);brain_set_key(root);tuning_set_key(root);
#ifdef FAM_SOUND_AMBIENT_V3
    if(drone_on) drone_publish(); /* the bed glides to the new tonic */
    world_strings_tune();
#endif
}
void engine_set_key(int midi) { engine_set_key_pc(midi%12); }
void engine_set_mode(int i) {
    if(i>=0 && i<=1 && i!=minor) {reset_score_context();minor=i;brain_set_mode(minor ? 5 : 0);
#ifdef FAM_SOUND_AMBIENT_V3
        if(drone_on) drone_publish();
        world_strings_tune();
#endif
    }
}
void engine_set_tuning(int just) {
    if((just==0 || just==1) && just!=tuning_mode()) {reset_score_context();tuning_set_mode(just);}
}
void engine_set_attack(float v) { if(isfinite(v)) shape_set_attack(.35f+.30f*dsp_clampf(v,0,1)); }
void engine_set_release(float v) { if(isfinite(v)) shape_set_release(.35f+.30f*dsp_clampf(v,0,1)); }
bool engine_cell_sample(uint8_t cell,float position,uint32_t ms) {
    if(generate || !isfinite(position)) return false;
    cell_event_t e=cells_update(cell,position,ms);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    if(e.kind==CELL_EVENT_PRESS) return engine_try_cell_on(e.cell,tuning_hz((float)engine_product_cell_midi(e.cell,false)),e.amp/CELL_AMP_MAX);
#else
    if(e.kind==CELL_EVENT_PRESS) return engine_try_note_on(e.cell,tuning_hz((float)engine_product_cell_midi(e.cell,false)),e.amp/CELL_AMP_MAX);
#endif
    if(e.kind==CELL_EVENT_RELEASE) { engine_note_off(e.cell); return true; }
    return false;
}
void engine_set_user_presence(bool on) {
    user_present=on; if(on) { user_seen=true; last_user_ms=now_ms; }
}
void engine_set_generative(bool on,int program) {
    (void)program; reap(); if(on==generate) return;
#ifdef FAM_SOUND_FINISH_CANDIDATE
    if(on)gesture_stop(now_ms);
    cell_queue_count=0;
#endif
    if(on)retire_context(true);else release_all();
    generate=on; suppressed=false; retry_ms=now_ms;
    coast_pending=0;
    if(on)coast_phrase_init(&coast,seed);else coast_phrase_restart(&coast);
    woodland_pending=false;
    if(on)woodland_phrase_init(&woodland,seed);else woodland_phrase_restart(&woodland);
    highlands_pending=false;
    if(on)highlands_phrase_init(&highlands,seed);else highlands_phrase_restart(&highlands);
    nature_set_amount(on && !muted && !clearing ? nature_amount : 0);
#ifdef FAM_SOUND_AMBIENT_V3
    if(on) gen_loops_reset(now_ms);
#endif
}
void engine_set_autoplay_melody(int on) { autoplay=on!=0; if(!autoplay) release_generated(); }
int engine_autoplay_melody(void) { return autoplay; }
uint32_t engine_gen_seed(void) { return seed; }
void engine_set_gen_seed(uint32_t v) {
    reap(); retire_context(false); seed=v ? v : 0xA6B13E7Du; nature_set_seed(seed);
    engine_set_world_character(world_character);
    retry_ms=now_ms;
    coast_pending=0; coast_phrase_init(&coast,seed);
    woodland_pending=false;woodland_phrase_init(&woodland,seed);
    highlands_pending=false;highlands_phrase_init(&highlands,seed);
#ifdef FAM_SOUND_AMBIENT_V3
    gen_loops_reset(now_ms);
#endif
}
void engine_generative_new_field(uint32_t v) { engine_set_gen_seed(v); }
static bool coast_owner_free(int role) {
    for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].owner==coast_owners[role])return false;
    return true;
}
#ifdef FAM_SOUND_FINISH_CANDIDATE
/* Bounded same-pitch-class octave selection. Failed admission never commits
 * memory; live common tones, actual Hz and all tail checks stay authoritative. */
static int revoice(int role,int midi) {
    int prev=last_voicing[world][role],best=midi,cost=1000;
    if(prev<0)return midi;
    for(int delta=-12;delta<=12;delta+=12) {
        int m=midi+delta;
        bool allowed=world==WORLD_COAST ? coast_phrase_pitch_allowed(m,tonic_pc,minor!=0) :
                     world==WORLD_WOODLAND ? woodland_phrase_pitch_allowed(m,tonic_pc,minor!=0) :
                     highlands_phrase_pitch_allowed(m,tonic_pc,minor!=0);
        float hz=phrase_hz(m);
        if(!allowed || hz<PHRASE_LO_HZ || hz>phrase_hi_hz() || !pitch_clear(hz))continue;
        int d=m-prev;if(d<0)d=-d;
        if(d<cost) {cost=d;best=m;}
    }
    return best;
}
#endif
static bool coast_start(const coast_offer_t *o) {
    float hz=phrase_hz(o->midi);
    if(!admit(coast_owners[o->role],hz,o->velocity,true)) {++rejects;return false;}
    coast_tickets[o->role]=*o; coast_pending|=(uint8_t)(1u<<o->role);
    return true;
}
static void coast_tick(uint32_t ms) {
    if(coast_pending || !coast_phrase_due(&coast,ms))return;
    if(coast.phase && !coast.wait_valid) {
        int outgoing=coast.phase==4 ? 3 : 2;
        for(int role=0;role<outgoing;++role)if(!coast_owner_free(role)) {
            retry_ms=ms+250;return;
        }
        (void)coast_phrase_wait_after_retirement(&coast,ms,activity);
        if(!coast_phrase_due(&coast,ms))return;
    }
    if(coast.phase==4) {
        for(int role=0;role<3;++role) if(!coast_owner_free(role)) {retry_ms=ms+250;return;}
        coast_phrase_restart(&coast);
    }
    if(coast.phase==0 && (coast.heard_mask&3u)==3u) {
        if((int32_t)(ms-(coast.phase_ms+700u))<0)return;
        coast_offer_t o=coast_phrase_propose(&coast,2,tonic_pc,minor!=0,activity);
        if(!coast_owner_free(2) || !coast_start(&o))retry_ms=ms+250;
        return;
    }
    if(coast.phase==3) {
        coast_offer_t o=coast_phrase_propose(&coast,0,tonic_pc,minor!=0,activity);
        /* Both outer source slots, including releases, must have retired.
         * Their shared destination is issued once, never as two unisons. */
        if(!coast_owner_free(0) || !coast_owner_free(1) || !coast_start(&o))retry_ms=ms+250;
        return;
    }
    coast_offer_t a=coast_phrase_propose(&coast,0,tonic_pc,minor!=0,activity);
    coast_offer_t b=coast_phrase_propose(&coast,1,tonic_pc,minor!=0,activity);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    a.midi=revoice(a.role,a.midi);b.midi=revoice(b.role,b.midi);
    if(!compatible(phrase_hz(a.midi),phrase_hz(b.midi),true) || a.midi==b.midi) {
        a=coast_phrase_propose(&coast,0,tonic_pc,minor!=0,activity);
        b=coast_phrase_propose(&coast,1,tonic_pc,minor!=0,activity);
    }
#endif
    int free=0;for(int i=0;i<SLOTS;++i)free+=!slots[i].used;
    if(!coast.heard_mask && (free<2 || !coast_owner_free(0) || !coast_owner_free(1) ||
       !pitch_clear(phrase_hz(a.midi)) || !pitch_clear(phrase_hz(b.midi)))) {
        retry_ms=ms+250; return;
    }
    /* Bounded pair preparation in Main. State commits only at audio ack.
     * A partial cancellation retries just its unheard role. */
    if(!(coast.heard_mask&1u) && !coast_start(&a)) {retry_ms=ms+250;return;}
    if(!(coast.heard_mask&2u) && !coast_start(&b))retry_ms=ms+250;
}
static void woodland_tick(uint32_t ms) {
    if(woodland_pending || !woodland_phrase_due(&woodland,ms))return;
    if(woodland.phase==6) {
        if(!coast_owner_free(0) || !coast_owner_free(1)) {retry_ms=ms+250;return;}
        woodland_phrase_restart(&woodland);
    }
    woodland_offer_t o=woodland_phrase_propose(&woodland,tonic_pc,minor!=0,activity);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    o.midi=revoice(o.role,o.midi);
#endif
    if(!coast_owner_free(o.role) || !admit(coast_owners[o.role],phrase_hz(o.midi),o.velocity,true)) {
        ++rejects;retry_ms=ms+250;return;
    }
    woodland_ticket=o;woodland_pending=true;
}
static void highlands_tick(uint32_t ms) {
    if(highlands_pending)return;
    if((highlands.phase==5 || highlands.phase==8) && !highlands.rest_valid) {
        for(int role=0;role<3;++role)if(!coast_owner_free(role)) {retry_ms=ms+250;return;}
        /* No fixed release estimate: open silence starts after the actual
         * last source ends. Room tails retain their ordinary pitch history. */
        (void)highlands_phrase_pause(&highlands,ms,activity);return;
    }
    if(!highlands_phrase_due(&highlands,ms))return;
    if(highlands.phase==8)highlands_phrase_restart(&highlands);
    highlands_offer_t o=highlands_phrase_propose(&highlands,tonic_pc,minor!=0,activity);
#ifdef FAM_SOUND_FINISH_CANDIDATE
    o.midi=revoice(o.role,o.midi);
#endif
    if(!coast_owner_free(o.role)) {retry_ms=ms+250;return;}
    if(!admit(coast_owners[o.role],phrase_hz(o.midi),o.velocity,true)) {
        ++rejects;retry_ms=ms+250;return;
    }
    highlands_ticket=o;highlands_pending=true;
}
#ifdef FAM_SOUND_AMBIENT_V3
/* Generate as incommensurate loops, the Music for Airports principle: three
 * voices (owners 6/7/15), each repeating a short soft note (or a two-note
 * figure) on its own cycle. The cycle lengths share no common period within a
 * performance, so the voices meet in ever new combinations, with open space
 * between. Each loop keeps its own register band; every 3-5 of its notes one
 * step moves through the World's collection, so the material evolves slowly.
 * Admission, tails, owners and ack timing are the existing Main paths.
 *
 * Every pair of cycles stays clear of the simple ratios a listener hears as a
 * polyrhythm (4:3, 3:2, 5:3, 2:1 by >= .08; 5:4, 6:5, 7:4 by >= .06): the
 * cycles relate as 1 : 1.12 : 1.585 or 1 : 1.414 : 1.585. One tempo factor per
 * performance scales all three, and each loop keeps its own clock, so a busy
 * voice delays a single note, never the loop. WOODLAND holds end at ~-29 dB of
 * the mallet ring (its 2 s release follows), so its two voices stay available. */
typedef struct { float period_s[3]; uint16_t hold_ms[3]; float lo_hz[3],hi_hz[3],velocity[3]; } gen_world_t;
static const gen_world_t GEN_WORLD[CORE_WORLD_COUNT]={
    /* COAST: pad swells */     {{19.7f,22.1f,31.2f},{4800,4200,3600},{220,294,392},{330,494,850},{.50f,.44f,.36f}},
    /* WOODLAND: mallet rings */{{11.1f,15.7f,17.6f},{4600,4000,3400},{220,262,330},{294,392,470},{.56f,.50f,.44f}},
    /* HIGHLANDS: breath tones */{{15.2f,21.5f,24.1f},{3600,3000,2600},{220,294,392},{330,494,650},{.52f,.46f,.40f}},
};
#ifdef FAM_SOUND_AMBIENT_V3
/* The instrument Worlds: choir swells as COAST; kalimba figures, quicker
 * and shorter (three tines keep them available); bell pairs as HIGHLANDS. */
static const gen_world_t GEN_INSTRUMENT[CORE_WORLD_COUNT]={
    /* FJORD: choir swells */      {{19.7f,22.1f,31.2f},{4800,4200,3600},{220,294,392},{330,494,850},{.50f,.44f,.36f}},
    /* MEADOW: kalimba figures */  {{6.3f,8.9f,10.0f},{1800,1500,1300},{220,262,330},{294,392,470},{.52f,.46f,.40f}},
    /* GLACIER: bell pairs */      {{15.2f,21.5f,24.1f},{3600,3000,2600},{220,294,392},{330,494,650},{.52f,.46f,.40f}},
};
#endif
static const gen_world_t *gen_world(void) {
#ifdef FAM_SOUND_AMBIENT_V3
    if(world_id>=CORE_WORLD_COUNT) return &GEN_INSTRUMENT[world];
#endif
    return &GEN_WORLD[world];
}
static float gen_unit(void) { gen_rng=gen_rng*1664525u+1013904223u; return (float)(gen_rng>>8)*(1.0f/16777216.0f); }
/* The loop's admissible pitches: the World's collection inside its band. */
static int gen_band(int role,int *out,int max) {
    const gen_world_t *g=gen_world();
    int top=world==WORLD_WOODLAND ? 68 : world==WORLD_HIGHLANDS ? 75 : 80,n=0;
    float lo=g->lo_hz[role],hi=fminf(g->hi_hz[role],phrase_hi_hz());
    for(int m=PHRASE_LIFT_BELOW;m<=top && n<max;++m) {
        float hz=tuning_hz((float)m);
        if(hz<lo || hz>hi) continue;
        bool ok=world==WORLD_COAST ? coast_phrase_pitch_allowed(m,tonic_pc,minor!=0) :
                world==WORLD_WOODLAND ? woodland_phrase_pitch_allowed(m,tonic_pc,minor!=0) :
                highlands_phrase_pitch_allowed(m,tonic_pc,minor!=0);
        if(ok) out[n++]=m;
    }
    return n;
}
static uint32_t gen_period_ms(int role) {
    float spacing=1.35f-.70f*(activity>=0 && activity<=1 ? activity : .5f);
    return (uint32_t)(gen_world()->period_s[role]*spacing*gen_scale*(.97f+.06f*gen_unit())*1000.0f);
}
/* Advance a loop's clock by whole cycles until its next onset lies ahead. */
static void gen_loop_advance(int role,uint32_t ms) {
    gen_loop_t *L=&gen_loops[role];
    for(int k=0;k<4 && (int32_t)(L->due_ms-ms)<1000;++k) L->due_ms+=gen_period_ms(role);
    if((int32_t)(L->due_ms-ms)<1000) L->due_ms=ms+gen_period_ms(role);
    L->next_ms=L->due_ms;
}
static void gen_loops_reset(uint32_t ms) {
#ifdef FAM_SOUND_AMBIENT_V3
    gen_rng=seed^0x5EEDF00Du^((uint32_t)world_id*0x9E3779B9u)^((uint32_t)(tonic_pc+1)*7919u)^((uint32_t)minor<<20);
#else
    gen_rng=seed^0x5EEDF00Du^((uint32_t)world*0x9E3779B9u)^((uint32_t)(tonic_pc+1)*7919u)^((uint32_t)minor<<20);
#endif
    gen_scale=.94f+.12f*gen_unit();
    static const float start_s[3]={.3f,3.7f,8.9f};
    for(int r=0;r<3;++r) {
        gen_loop_t *L=&gen_loops[r]; int band[24],n=gen_band(r,band,24);
        memset(L,0,sizeof *L); L->midi=-1;
#ifdef FAM_SOUND_AMBIENT_V3
        L->len=(uint8_t)(world_id==WORLD_GLACIER || gen_unit()<.6f ? 2 : 1); /* bells: melodic pairs */
#else
        L->len=(uint8_t)(gen_unit()<.6f ? 2 : 1);
#endif
        for(int k=0;k<2;++k) L->step[k]=(int8_t)(n>0 ? (int)(gen_unit()*(float)n)%n : 0);
        L->evolve_at=(uint8_t)(3+(int)(gen_unit()*3.0f));
        L->due_ms=L->next_ms=ms+(uint32_t)(start_s[r]*(1.35f-.70f*activity)*1000.0f);
    }
}
static void gen_loop_heard(int role,source_slot_t *s) {
    gen_loop_t *L=&gen_loops[role]; L->pending=false;
    if(clearing || muted || suppressed || !generate || !autoplay) return;
    s->timed=true; s->off_ms=now_ms+gen_world()->hold_ms[role];
    last_midi=L->midi; ++note_count; ++L->fires; L->retries=0;
    if(L->len>1) L->phase^=1u;
    gen_loop_advance(role,now_ms);
    if(L->fires>=L->evolve_at) {
        int band[24],n=gen_band(role,band,24),k=(int)(gen_unit()*(float)L->len)%L->len;
        int st=L->step[k]+(gen_unit()<.5f ? -1 : 1);
        if(st<0) st=1;
        if(st>=n) st=n-2;
        L->step[k]=(int8_t)(st<0 ? 0 : st); L->fires=0; L->evolve_at=(uint8_t)(3+(int)(gen_unit()*3.0f));
    }
}
static void gen_loops_tick(uint32_t ms) {
    for(int r=0;r<3;++r) {
        gen_loop_t *L=&gen_loops[r];
        if(L->pending || (int32_t)(ms-L->next_ms)<0) continue;
        /* Resumed after a pause (user playing, Mute): re-stagger, no burst. */
        if((int32_t)(ms-L->next_ms)>8000) {
            L->retries=0; L->due_ms=L->next_ms=ms+400u+2700u*(uint32_t)r; continue;
        }
        int band[24],n=gen_band(r,band,24);
        if(n<=0) { gen_loop_advance(r,ms); continue; }
        int st=L->step[L->phase%L->len]; if(st>=n) st=n-1;
        float v=gen_world()->velocity[r]+(gen_unit()-.5f)*.08f;
        bool free=coast_owner_free(r); int pick=-1;
        /* The figure's tone; if it would clash with what sounds (unison,
         * semitone, tritone), a neighbour in the band, as a player would. */
        int dir=((L->fires+r)&1u) ? 1 : -1;
        const int try_step[3]={st,st+dir,st-dir};
        for(int k=0;free && k<3 && pick<0;++k) {
            int c=try_step[k]; if(c<0 || c>=n) continue;
            if(admit(coast_owners[r],tuning_hz((float)band[c]),v,true)) pick=c;
            else if(note_block!=ENGINE_NOTE_HELD_PITCH && note_block!=ENGINE_NOTE_TAIL_PITCH &&
                    note_block!=ENGINE_NOTE_FROZEN_PITCH) break;
        }
        if(pick<0) {
            if(free) ++rejects;
            /* A few short retries delay this note only; then the cycle stays
             * silent, the loop steps aside and keeps its clock. */
            if(++L->retries<5) L->next_ms=ms+700u;
            else { L->retries=0; gen_loop_advance(r,ms); L->step[L->phase%L->len]=(int8_t)((st+1)%n); }
            continue;
        }
        L->midi=band[pick]; L->pending=true;
        return; /* one bounded preparation per Main tick */
    }
}
#endif
void engine_generative_tick(uint32_t ms) {
    now_ms=ms; reap();
#ifdef FAM_SOUND_FINISH_CANDIDATE
    service_cell_queue();
#endif
    for(int i=0;i<SLOTS;++i) if(slots[i].used && slots[i].timed &&
       (int32_t)(ms-slots[i].off_ms)>=0) engine_note_off(slots[i].owner);
    if(user_present) { last_user_ms=ms; user_seen=true; }
    bool wait=user_seen && (user_present || (uint32_t)(ms-last_user_ms)<8000u);
    if(wait && !suppressed) release_generated();
    suppressed=wait;
    if(!generate || !autoplay || suppressed || clearing || muted ||
       (int32_t)(ms-retry_ms)<0) return;
#ifdef FAM_SOUND_AMBIENT_V3
    gen_loops_tick(ms); return;
#endif
    if(world==WORLD_COAST) {coast_tick(ms);return;}
    if(world==WORLD_WOODLAND) {woodland_tick(ms);return;}
    highlands_tick(ms);
}
#ifdef FAM_SOUND_AMBIENT_V3
int engine_generative_advance(void) { return generate && !suppressed ? (int)(((unsigned)note_count/3u)%4u)+1 : -1; }
#else
int engine_generative_advance(void) { return generate && !suppressed ?
    (int)((world==WORLD_COAST ? coast.episodes : world==WORLD_WOODLAND ? woodland.episodes : highlands.episodes)%4u)+1 : -1; }
#endif
void engine_generative_nudge(int cell,uint32_t ms) { (void)cell; (void)ms; }
int engine_generative_suppressed(void) { return suppressed; }
int engine_generative_last_melody_midi(void) { return last_midi; }
int engine_generative_melody_count(void) { return note_count; }
int engine_generative_dejavu_count(void) { return (int)(world==WORLD_WOODLAND ? woodland.answers : 0); }
uint32_t engine_generative_return_count(void) { return world==WORLD_COAST ? coast.returns : world==WORLD_WOODLAND ? woodland.returns : highlands.returns; }
uint32_t engine_generative_episode_count(void) { return world==WORLD_COAST ? coast.episodes : world==WORLD_WOODLAND ? woodland.episodes : highlands.episodes; }
static const phrase_development_t *current_development(void) {
    return world==WORLD_COAST ? &coast.development : world==WORLD_WOODLAND ? &woodland.development : &highlands.development;
}
int engine_generative_phrase_kind(void) { return current_development()->kind; }
uint32_t engine_generative_memory_return_count(void) { return current_development()->recalls; }
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
#ifdef FAM_SOUND_AMBIENT_V3
void engine_set_drone(bool on) { reap(); drone_on=on; drone_publish(); }
void engine_set_drone_color(int color) {
    if(color<ENGINE_DRONE_AUTO || color>=ENGINE_DRONE_COLORS) return;
    drone_color_sel=color; drone_publish();
}
int engine_drone_color(void) { return drone_color_sel; }
int engine_drone_sounding_color(void) { return drone_resolved(); }
void engine_set_drone_level_db(float db) {
    if(!isfinite(db)) return;
    drone_level_db=dsp_clampf(db,-12.0f,6.0f); drone_publish();
}
float engine_drone_level_db(void) { return drone_level_db; }
#else
void engine_set_drone(bool on) { (void)on; }
#endif
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
#ifdef FAM_SOUND_FINISH_CANDIDATE
    reset_voicing();freeze_seen=false;memset(loop_lane,0,sizeof loop_lane);bowed_set_pad(true);
    cell_queue_count=0;cell_retiring=0;
#else
    bowed_set_pad(false);
#endif
    memset(slots,0,sizeof slots); memset(tails,0,sizeof tails);
#ifdef FAM_SOUND_AMBIENT_V3
    drone_on=false; drone_color_sel=ENGINE_DRONE_AUTO; drone_level_db=0; drone_reset_audio();
    wfx_reset_audio(); world_id=0;
#endif
    world=minor=0; tonic_pc=-1; seed=0xA6B13E7Du; now_ms=last_user_ms=retry_ms=rejects=0;
    generate=user_present=user_seen=suppressed=clearing=false; autoplay=true;
    muted=resume_nature=false;
#if defined(FAM_WORLD_CHARACTER_CANDIDATE) || defined(FAM_SOUND_FINISH_CANDIDATE)
    engine_set_world_character(true);
#else
    engine_set_world_character(false);
#endif
#ifdef FAM_WOODLAND_BODY_CANDIDATE
    engine_set_woodland_body(true);
#else
    engine_set_woodland_body(false);
#endif
#if defined(FAM_WOODLAND_RESONATOR_CANDIDATE) || defined(FAM_SOUND_FINISH_CANDIDATE)
    engine_set_woodland_resonator(true);
#else
    engine_set_woodland_resonator(false);
#endif
    note_hook=0;note_block=ENGINE_NOTE_OK; fx_mode=1; last_midi=note_count=0;
    atomic_store(&quick_mask,0); atomic_store(&release_mask,0); atomic_store(&audio_mask,0); atomic_store(&started_mask,0);
    atomic_store(&audio_epoch,0); atomic_store(&audio_frames,0); atomic_store(&limited,0); atomic_store(&faults,0);
    atomic_store(&clear_request,false); atomic_store(&clear_done,false);
    atomic_store(&output_enabled,false); atomic_store(&room_quiet,true);
    volume_cur=.6f; gate_cur=dc_l=dc_r=0; quiet_frames=0;
    engine_set_master_volume(.6f); engine_set_key_pc(2); engine_set_activity(.5f);
    engine_set_color(.5f); engine_set_room(.5f); engine_set_nature(0);
    engine_set_attack(.5f); engine_set_release(.5f);
    coast_pending=0; coast_phrase_init(&coast,seed);
    woodland_pending=false;woodland_phrase_init(&woodland,seed);pluck_set_ambient(true);
    highlands_pending=false;highlands_phrase_init(&highlands,seed);
#ifdef FAM_SOUND_AMBIENT_V3
    world_voicing_apply();
    gen_loops_reset(0);
    drone_publish();
#endif
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
        uint32_t fast=atomic_exchange_explicit(&quick_mask,0,memory_order_acquire);
#ifdef FAM_SOUND_FINISH_CANDIDATE
        uint32_t cell_fades=fast>>SOURCES;
        fast|=cell_fades;
#endif
        uint32_t off=atomic_exchange_explicit(&release_mask,0,memory_order_acquire)|fast;
        for(int i=0;i<SOURCES;++i) if(off&(1u<<i)) {
            if(fast&(1u<<i)) {
                int fade_frames=(int)(.100f*DSP_SAMPLE_RATE_HZ);
#ifdef FAM_SOUND_FINISH_CANDIDATE
                if(cell_fades&(1u<<i))fade_frames=CELL_FADE_FRAMES;
#endif
                bowed_quiet_source(i,fade_frames);
                horn_quiet_source(i,fade_frames);
                pluck_quiet_source((uint8_t)i,(uint32_t)fade_frames);
            } else {
                bowed_note_off(i);horn_note_off(i);pluck_note_off((uint8_t)i);
            }
        }
        memset(dry_l,0,n*sizeof(float)); memset(dry_r,0,n*sizeof(float));
        memset(send_l,0,n*sizeof(float)); memset(send_r,0,n*sizeof(float));
        bowed_render_mix(dry_l,dry_r,send_l,send_r,n,.35f);
        horn_render_mix(dry_l,dry_r,send_l,send_r,n,.35f);
        pluck_render_mix(dry_l,dry_r,send_l,send_r,n);
#ifdef FAM_SOUND_AMBIENT_V3
        drone_render(dry_l,dry_r,n);
        wfx_process(dry_l,dry_r,n); /* instrument Worlds only; bypassed otherwise */
#endif
        for(int i=0;i<n;++i) { send_l[i]=.35f*dry_l[i]; send_r[i]=.35f*dry_r[i]; }
        ambient_room_process(dry_l,dry_r,send_l,send_r,n);
        float wet_peak=ambient_room_peak();
#ifdef FAM_SOUND_FINISH_CANDIDATE
        nature_render_music(dry_l,dry_r,send_l,send_r,1.0f/.35f,n);
#else
        nature_render(dry_l,dry_r,n);
#endif
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
            pluck_set_ambient(true);
#ifdef FAM_SOUND_AMBIENT_V3
            drone_reset_audio(); wfx_reset_audio();
#endif
            float c=value(atomic_load_explicit(&color_bits,memory_order_acquire));
            bowed_set_tone(c); horn_set_tone(c);
            pluck_set_damp(.025f-.020f*.5f); pluck_set_color(c);
            dc_l=dc_r=0; mask=0; quiet_frames=DSP_SAMPLE_RATE_HZ/4;
            atomic_store(&quick_mask,0); atomic_store(&release_mask,0); atomic_store(&started_mask,0); atomic_store(&room_quiet,true);
            atomic_store_explicit(&clear_done,true,memory_order_release);
        } else atomic_fetch_or_explicit(&started_mask,mask,memory_order_release);
        atomic_store_explicit(&audio_mask,mask,memory_order_release);
        atomic_fetch_add_explicit(&audio_frames,(uint32_t)n,memory_order_release);
        atomic_fetch_add_explicit(&audio_epoch,1,memory_order_release);
        frames-=n; out+=2*n;
    }
}
