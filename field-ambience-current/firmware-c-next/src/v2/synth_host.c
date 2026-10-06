/*
 * synth_host.c — the swappable sound-core host. See synth_host.h.
 *
 * Signal path (global, identical for every engine):
 *   clear dry/send → active->render_mix(dry, send) → reverb_render(send→wet)
 *   → dry + wet*wet_amp → master → beauty_guard limiter → int16
 *
 * The host is allocation-free and block-chunked internally so a caller can
 * ask for any frame count. It does NOT touch engine_v2 (the FIELD engine).
 */
#include "v2/synth_host.h"
#include "v2/beauty_guard.h"
#include "reverb.h"
#include "dsp.h"
#include "synth_controls.h"
#include <math.h>
#include <string.h>

/* Each engine is a const vtable exported from its own .c file. */
extern const synth_engine_t engine_acid;
extern const synth_engine_t engine_fm_glass;
extern const synth_engine_t engine_chorus_mist;
extern const synth_engine_t engine_ion_storm;
extern const synth_engine_t engine_glass_orbit;
extern const synth_engine_t engine_bamboo_circuit;

static const synth_engine_t *const TABLE[SYNTH_COUNT] = {
    [SYNTH_ACID]           = &engine_acid,
    [SYNTH_FM_GLASS]       = &engine_fm_glass,
    [SYNTH_CHORUS_MIST]    = &engine_chorus_mist,
    [SYNTH_ION_STORM]      = &engine_ion_storm,
    [SYNTH_GLASS_ORBIT]    = &engine_glass_orbit,
    [SYNTH_BAMBOO_CIRCUIT] = &engine_bamboo_circuit,
};

/* Fixed output calibration, measured across C3..C5 and three velocities.
 * Pluck uses its first 400 ms against the Ensemble body, not its silent tail.
 * Keep the envelope follower BEFORE these trims: Envmod must retain its
 * native response. Apply equal gain to dry/send and to both crossfade legs.
 * Resonant/Pluck register differences still need voicing review. No AGC. */
static const float CORE_OUTPUT_GAIN[SYNTH_COUNT] = {
    [SYNTH_ACID] = 0.62373484f,       /* -4.1 dB */
    [SYNTH_FM_GLASS] = 0.23988329f,   /* -12.4 dB */
    [SYNTH_CHORUS_MIST] = 1.0f,
    [SYNTH_ION_STORM] = 0.57543994f,    /* -4.8 dB */
    [SYNTH_GLASS_ORBIT] = 0.32359366f,  /* -9.8 dB */
    [SYNTH_BAMBOO_CIRCUIT] = 0.45708819f, /* -6.8 dB */
};

#define HBLOCK 256

static struct {
    const synth_engine_t *active;
    synth_id_t            active_id;
    volatile synth_id_t   requested_id;
    beauty_guard_t        guard;
    float wet_target, wet_amp;
    float master;
    const synth_engine_t *previous;
    synth_id_t previous_id;
    int fade_left;
    float target[SYNTH_COUNT][6], current[6];
    float velocity[SYNTH_COUNT], velocity_target[SYNTH_COUNT];
    float macro_target[4], macro[4], sweep_phase, envelope;
} H;

static float dL[HBLOCK], dR[HBLOCK], sL[HBLOCK], sR[HBLOCK], wL[HBLOCK], wR[HBLOCK];

static float prevSL[HBLOCK], prevSR[HBLOCK];

static int16_t to_i16(float x) {
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return (int16_t)lrintf(x * 32767.0f);
}

void synth_host_init(void) {
    memset(&H, 0, sizeof H);
    reverb_init();
    reverb_set(0.5f, 0.4f);
    reverb_set_drive(0.10f);
    bg_init(&H.guard);
    H.wet_target = 0.25f;
    H.wet_amp    = 0.0f;
    H.master     = 0.9f;
    for (int i=0;i<SYNTH_COUNT;++i) H.velocity[i]=H.velocity_target[i]=1.0f;
    for (int i=0; i<SYNTH_COUNT; ++i) for (int p=0; p<6; ++p)
        H.target[i][p] = synth_control_defaults[i][p] / 100.0f;
    memcpy(H.current, H.target[0], sizeof H.current);
    for (int i = 0; i < SYNTH_COUNT; ++i)
        if (TABLE[i] && TABLE[i]->init) TABLE[i]->init();
    H.active_id = SYNTH_ACID;
    H.active    = TABLE[SYNTH_ACID];
    if (H.active && H.active->activate) H.active->activate();
}

void synth_host_select(synth_id_t id) {
    if (id < 0 || id >= SYNTH_COUNT || !TABLE[id]) return;
    if (H.requested_id == id) return;
    /* Prepare only a dormant core, then publish the request LAST. The DMA
     * renderer owns its active/previous pointers, so it never sees a half-
     * initialised engine. A quick reversal reuses the still-running core. */
    if (TABLE[id] != H.active && TABLE[id] != H.previous) {
        if (TABLE[id]->activate) TABLE[id]->activate();
        for (int p=0;p<6;++p) TABLE[id]->set_param((synth_param_t)p,H.target[id][p]);
    }
    __asm__ volatile("" ::: "memory");
    H.requested_id = id;
}

synth_id_t  synth_host_active(void)      { return H.requested_id; }
const char *synth_host_active_name(void) { return TABLE[H.requested_id]->name; }

static void note_on_pitch(float midi, float vel) {
    if (!isfinite(midi) || !isfinite(vel) || vel <= 0.0f) return;
    vel=dsp_clampf(vel,0.0f,1.0f);
    /* Mist ignored velocity; FM/Orbit/Storm changed colour only. Give every
     * core a playable level response while retaining its native accent. */
    H.velocity_target[H.requested_id]=sqrtf(vel);
    if (TABLE[H.requested_id]->note_on) TABLE[H.requested_id]->note_on(midi,vel);
}
void synth_host_note_on(int midi, float vel) { note_on_pitch((float)midi,vel); }
void synth_host_note_on_hz(float hz, float vel) {
    if (isfinite(hz) && hz >= 20.0f && hz <= 16000.0f)
        note_on_pitch(69.0f + 12.0f * log2f(hz / 440.0f),vel);
}
void synth_host_retune_hz(float hz) {
    if (isfinite(hz) && hz >= 20.0f && hz <= 16000.0f &&
        TABLE[H.requested_id]->retune_hz)
        TABLE[H.requested_id]->retune_hz(hz);
}
void synth_host_set_macro(int slot, float value) {
    if (slot < 0 || slot >= 4 || !isfinite(value)) return;
    H.macro_target[slot] = slot == 0 ? dsp_clampf(value/800.0f,-0.75f,1.0f)
                                          : dsp_clampf(value,0.0f,1.0f);
}
void synth_host_note_off(void) { if (TABLE[H.requested_id]->note_off) TABLE[H.requested_id]->note_off(); }
void synth_host_set_param(synth_param_t p, float v) {
    if ((int)p >= 0 && (int)p < 6 && isfinite(v)) H.target[H.requested_id][p] = dsp_clampf(v, 0.0f, 1.0f);
}
void synth_host_panic(void) {
    if (H.active && H.active->panic) H.active->panic();
    if (H.previous && H.previous->panic) H.previous->panic();
    if (TABLE[H.requested_id]->panic) TABLE[H.requested_id]->panic();
    H.previous = 0; H.fade_left = 0;
}

void synth_host_set_reverb(float size, float wet) {
    reverb_set(dsp_clampf(size, 0.0f, 1.0f), 0.4f);
    H.wet_target = dsp_clampf(wet, 0.0f, 1.0f);
}
void synth_host_set_master(float v) { H.master = dsp_clampf(v, 0.0f, 1.0f); }

void synth_host_render_mix(float *l, float *r, float *sl, float *sr, int frames) {
    for (int done=0; done<frames;) {
        int n=frames-done; if(n>HBLOCK) n=HBLOCK;
        synth_id_t requested=H.requested_id;
        if (requested != H.active_id) {
            if (H.previous && H.previous != TABLE[requested] && H.previous->deactivate)
                H.previous->deactivate();
            H.previous=H.active; H.previous_id=H.active_id; H.fade_left=662;
            H.active=TABLE[requested]; H.active_id=requested;
            memcpy(H.current,H.target[requested],sizeof H.current);
            for (int p=0;p<6;++p) H.active->set_param((synth_param_t)p,H.current[p]);
        }
        memset(dL,0,sizeof(float)*n); memset(dR,0,sizeof(float)*n);
        memset(sL,0,sizeof(float)*n); memset(sR,0,sizeof(float)*n);
        /* Bounded control-rate updates, ~80 ms smoothing, no per-sample pow.
         * Unchanged parameters cost no coefficient recalculation. */
        float k=1.0f-expf(-(float)n/(0.080f*DSP_SAMPLE_RATE_HZ));
        for (int m=0;m<4;++m) H.macro[m]+=k*(H.macro_target[m]-H.macro[m]);
        H.sweep_phase += (float)n / (18.7f * DSP_SAMPLE_RATE_HZ);
        if (H.sweep_phase >= 1.0f) H.sweep_phase -= 1.0f;
        float oct = H.macro[0]*1.5f + H.macro[2]*1.5f*dsp_sin(H.sweep_phase)
                    + H.macro[3]*2.0f*dsp_clampf(H.envelope*3.0f,0.0f,1.0f);
        float colour_scale=exp2f(oct);
        if (H.active->set_colour) H.active->set_colour(colour_scale,H.macro[1]);
        if (H.previous && H.previous->set_colour)
            H.previous->set_colour(colour_scale,H.macro[1]);
        for (int p=0;p<6;++p) {
            float delta=H.target[H.active_id][p]-H.current[p];
            if (fabsf(delta)>0.00001f && H.active && H.active->set_param) {
                H.current[p]+=k*delta;
                H.active->set_param((synth_param_t)p,H.current[p]);
            }
        }
        if (H.active && H.active->render_mix) H.active->render_mix(dL,dR,sL,sR,n);
        float peak=0.0f;
        for (int i=0;i<n;++i) {
            H.velocity[H.active_id]+=0.00283046f*(H.velocity_target[H.active_id]-H.velocity[H.active_id]);
            float gain=H.velocity[H.active_id];
            float a=0.5f*(fabsf(dL[i])+fabsf(dR[i]))*gain;
            if(a>peak) peak=a;
            gain*=CORE_OUTPUT_GAIN[H.active_id];
            dL[i]*=gain; dR[i]*=gain; sL[i]*=gain; sR[i]*=gain;
        }
        float ek=1.0f-expf(-(float)n/((peak>H.envelope ? 0.040f:0.5f)*DSP_SAMPLE_RATE_HZ));
        H.envelope+=ek*(peak-H.envelope);
        if (H.previous) {
            memset(wL,0,sizeof(float)*n); memset(wR,0,sizeof(float)*n);
            memset(prevSL,0,sizeof(float)*n); memset(prevSR,0,sizeof(float)*n);
            H.previous->render_mix(wL,wR,prevSL,prevSR,n);
            for (int i=0;i<n;++i) {
                float gain=H.velocity[H.previous_id]*CORE_OUTPUT_GAIN[H.previous_id];
                wL[i]*=gain; wR[i]*=gain; prevSL[i]*=gain; prevSR[i]*=gain;
                float old=H.fade_left>0 ? H.fade_left/662.0f : 0.0f;
                if(H.fade_left>0) --H.fade_left;
                dL[i]+=old*(wL[i]-dL[i]); dR[i]+=old*(wR[i]-dR[i]);
                sL[i]+=old*(prevSL[i]-sL[i]); sR[i]+=old*(prevSR[i]-sR[i]);
            }
            if (!H.fade_left) {
                if (H.previous->deactivate) H.previous->deactivate();
                H.previous=0;
            }
        }
        for(int i=0;i<n;++i) {
            l[done+i]+=dL[i]; r[done+i]+=dR[i];
            sl[done+i]+=sL[i]; sr[done+i]+=sR[i];
        }
        done+=n;
    }
}

void synth_host_render(int16_t *out, int frames) {
    int done = 0;
    while (done < frames) {
        int n = frames - done; if (n > HBLOCK) n = HBLOCK;

        memset(dL, 0, sizeof(float) * n); memset(dR, 0, sizeof(float) * n);
        memset(sL, 0, sizeof(float) * n); memset(sR, 0, sizeof(float) * n);

        static float rawL[HBLOCK], rawR[HBLOCK], rawSL[HBLOCK], rawSR[HBLOCK];
        memset(rawL,0,sizeof(float)*n); memset(rawR,0,sizeof(float)*n);
        memset(rawSL,0,sizeof(float)*n); memset(rawSR,0,sizeof(float)*n);
        synth_host_render_mix(rawL,rawR,rawSL,rawSR,n);
        memcpy(dL,rawL,sizeof(float)*n); memcpy(dR,rawR,sizeof(float)*n);
        memcpy(sL,rawSL,sizeof(float)*n); memcpy(sR,rawSR,sizeof(float)*n);

        reverb_render(sL, sR, wL, wR, n);

        /* wet amp glides in (~60 ms) so a select() / wet change never zippers */
        float wc = 1.0f - expf(-1.0f / (0.060f * (float)DSP_SAMPLE_RATE_HZ / (float)n));
        H.wet_amp += wc * (H.wet_target - H.wet_amp);

        for (int i = 0; i < n; ++i) {
            dL[i] = (dL[i] + wL[i] * H.wet_amp) * H.master;
            dR[i] = (dR[i] + wR[i] * H.wet_amp) * H.master;
        }

        bg_process(&H.guard, dL, dR, n);

        for (int i = 0; i < n; ++i) {
            out[2 * (done + i) + 0] = to_i16(dL[i]);
            out[2 * (done + i) + 1] = to_i16(dR[i]);
        }
        done += n;
    }
}
