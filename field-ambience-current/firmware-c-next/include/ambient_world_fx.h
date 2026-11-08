#ifndef FAM_AMBIENT_WORLD_FX_H
#define FAM_AMBIENT_WORLD_FX_H
/* Instrument Worlds (FJORD, MEADOW, GLACIER): a sculpting stage on the
 * instrument bus, after the Torso S-4 idea of a chain behind every sound.
 *   Tape:      wow and flutter (a short modulated delay), warm asymmetric
 *              saturation and a bandwidth roll-off. No hiss is added.
 *   Resonator: four sympathetic strings tuned to the key (tonic, fifth,
 *              octave and the World's third, else sixth or fourth), excited
 *              by the bus above 200 Hz: the notes ring on in tune.
 * Delay, grains and the hall follow in the shared music effects. Main picks
 * a preset and publishes the string tuning; Audio ramps every parameter and
 * dips the strings out and in around a retune. The classic Worlds never
 * enter the stage, so their output stays bit for bit what it was.
 * Single Main writer, single audio reader; one read attempt per block, no
 * libm, heap or unbounded work in the audio path. All storage is internal
 * .bss (about 6 KB). */
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "dsp.h"

enum { WFX_TAPE=256, WFX_STRINGS=4, WFX_COMB=256, WFX_PRESETS=4 };
typedef struct {
    float wow_hz,wow;          /* slow pitch drift: rate, depth in samples */
    float flutter_hz,flutter;  /* fast pitch shake: rate, depth in samples */
    float drive,bias;          /* saturation input gain (1 = none), asymmetry */
    float tone_hz;             /* tape bandwidth (one pole) */
    float strings;             /* resonator return */
    float t60,damp;            /* string decay in seconds, loop damping 0..1/4 */
    float trim;                /* World loudness, matched to the classic Worlds */
} wfx_preset_t;
/* 0 = bypass (the classic Worlds), then FJORD, MEADOW, GLACIER. Pitch depth
 * of wow: 2*pi*rate*depth/SR, about 3 cents in FJORD, 7 in MEADOW, 2.5 in
 * GLACIER. The bus peaks near .15, so a gain of 3-5 drives them to .45-.75
 * of the saturation knee: third harmonic about -30 dB on peaks, the bias
 * adds a second; the kalimba gets the most and the darkest tape. */
static const wfx_preset_t WFX_PRESET[WFX_PRESETS]={
    {0,0,0,0,1,0,20000,0,1,.2f,1},
    /* FJORD   */ {.30f,40,5.5f,.5f,3.0f,.010f,9000,.25f,3.5f,.20f,1},
    /* MEADOW  */ {.55f,52,6.8f,1.2f,5.0f,.012f,6500,.30f,2.5f,.24f,1},
    /* GLACIER */ {.22f,45,0,0,2.2f,.006f,12000,.45f,5.0f,.12f,1},
};
static _Atomic uint32_t wfx_sel;                    /* preset index */
static _Atomic uint32_t wfx_tone_bits;              /* tape one-pole coefficient */
static _Atomic uint32_t wfx_tune_version;           /* odd while Main writes */
static _Atomic uint32_t wfx_tune_bits[2*WFX_STRINGS]; /* period, feedback */
static struct {
    float tape[2][WFX_TAPE],comb[WFX_STRINGS][WFX_COMB];
    float period[WFX_STRINGS],feedback[WFX_STRINGS],input[WFX_STRINGS],z1[WFX_STRINGS],z2[WFX_STRINGS];
    float wow,wow_hz,flutter,flutter_hz,drive,bias,tone_a,strings,damp,trim; /* ramped */
    float wow_ph,flutter_ph,lp[2],hp,mix,gate;
    unsigned at,cat,version;
    bool tuned,active;
} wfx;
static inline uint32_t wfx_bits(float v) { uint32_t b; memcpy(&b,&v,4); return b; }
static inline float wfx_value(uint32_t b) { float v; memcpy(&v,&b,4); return v; }
/* e^x for Main-side coefficients without libm (x <= 0): halve until small,
 * seven Taylor terms, square back. Relative error below 1e-7 for x >= -8. */
static float wfx_exp(float x) {
    int k=0; while(x<-.25f && k<8) { x*=.5f; ++k; }
    float y=1.0f+x*(1.0f+x*(.5f+x*(1.0f/6+x*(1.0f/24+x*(1.0f/120+x*(1.0f/720))))));
    while(k-->0) y*=y;
    return y;
}

/* Main: the World's preset (0 for the classic Worlds). */
static void wfx_set_preset(int preset) {
    if(preset<0 || preset>=WFX_PRESETS) preset=0;
    float a=1.0f-wfx_exp(-6.2831853f*WFX_PRESET[preset].tone_hz/(float)DSP_SAMPLE_RATE_HZ);
    atomic_store_explicit(&wfx_tone_bits,wfx_bits(preset ? a : 1.0f),memory_order_release);
    atomic_store_explicit(&wfx_sel,(uint32_t)preset,memory_order_release);
}
/* Main: tune the four strings (Hz) for the preset's decay. Coefficients are
 * computed here, never in the audio path. */
static void wfx_tune(const float hz[WFX_STRINGS],float t60) {
    uint32_t next[2*WFX_STRINGS];
    for(int k=0;k<WFX_STRINGS;++k) {
        float h=dsp_clampf(hz[k],180.0f,1200.0f),period=(float)DSP_SAMPLE_RATE_HZ/h;
        next[k]=wfx_bits(period);
        next[WFX_STRINGS+k]=wfx_bits(wfx_exp(-6.9077553f*period/((float)DSP_SAMPLE_RATE_HZ*dsp_clampf(t60,.5f,8.0f))));
    }
    uint32_t v=atomic_load_explicit(&wfx_tune_version,memory_order_seq_cst);
    atomic_store_explicit(&wfx_tune_version,v+1u,memory_order_seq_cst);
    for(int k=0;k<2*WFX_STRINGS;++k) atomic_store_explicit(&wfx_tune_bits[k],next[k],memory_order_seq_cst);
    atomic_store_explicit(&wfx_tune_version,v+2u,memory_order_seq_cst);
}
/* Audio, behind zero output (engine init and Clear). */
static void wfx_reset_audio(void) {
    memset(&wfx,0,sizeof wfx);
    wfx.tone_a=1.0f; wfx.damp=.2f; wfx.drive=1.0f; wfx.trim=1.0f; wfx.version=~0u;
}
/* One attempt to adopt a whole published tuning. */
static bool wfx_take_tuning(void) {
    uint32_t v=atomic_load_explicit(&wfx_tune_version,memory_order_seq_cst);
    if(v&1u || v==wfx.version) return false;
    float p[WFX_STRINGS],g[WFX_STRINGS];
    for(int k=0;k<WFX_STRINGS;++k) {
        p[k]=wfx_value(atomic_load_explicit(&wfx_tune_bits[k],memory_order_seq_cst));
        g[k]=wfx_value(atomic_load_explicit(&wfx_tune_bits[WFX_STRINGS+k],memory_order_seq_cst));
    }
    if(v!=atomic_load_explicit(&wfx_tune_version,memory_order_seq_cst)) return false;
    for(int k=0;k<WFX_STRINGS;++k) {
        if(!(p[k]>=2 && p[k]<=WFX_COMB-4) || !(g[k]>=0 && g[k]<1)) return false;
    }
    for(int k=0;k<WFX_STRINGS;++k) {
        wfx.period[k]=p[k]; wfx.feedback[k]=g[k];
        /* A steady tone at the string's pitch rings back at its own level. */
        wfx.input[k]=1.0f-g[k];
    }
    memset(wfx.comb,0,sizeof wfx.comb); memset(wfx.z1,0,sizeof wfx.z1); memset(wfx.z2,0,sizeof wfx.z2);
    wfx.version=v; wfx.tuned=true;
    return true;
}
static inline float wfx_sat(float u) {
    u=dsp_clampf(u,-3.0f,3.0f); float q=u*u;
    return u*(27.0f+q)/(27.0f+9.0f*q); /* tanh, Pade 3/2 */
}
static void wfx_process(float *l,float *r,int n) {
    int sel=(int)atomic_load_explicit(&wfx_sel,memory_order_acquire);
    if(sel<0 || sel>=WFX_PRESETS) sel=0;
    if(!sel && !wfx.active) return;          /* classic Worlds: untouched */
    if(sel) wfx.active=true;
    const wfx_preset_t *p=&WFX_PRESET[sel];
    /* Strings: a newer tuning dips them out (50 ms), swaps at silence and
     * brings them back (300 ms). */
    uint32_t pending=atomic_load_explicit(&wfx_tune_version,memory_order_acquire);
    bool retune=pending!=wfx.version && !(pending&1u);
    if(retune && (wfx.gate<=0 || !wfx.tuned)) { if(wfx_take_tuning()) retune=false; }
    const float gate_step=retune ? -1.0f/(.050f*DSP_SAMPLE_RATE_HZ) : 1.0f/(.300f*DSP_SAMPLE_RATE_HZ);
    const float mix_step=1.0f/(.150f*DSP_SAMPLE_RATE_HZ),glide=1.0f/(.400f*DSP_SAMPLE_RATE_HZ);
    const float mix_target=sel ? 1.0f : 0.0f;
    float tone=wfx_value(atomic_load_explicit(&wfx_tone_bits,memory_order_acquire));
    if(!(tone>0 && tone<=1)) tone=1.0f;
    for(int i=0;i<n;++i) {
        /* Ramps: the preset's values glide; the stage fades in and out. */
        wfx.mix=dsp_clampf(wfx.mix+(mix_target>wfx.mix ? mix_step : -mix_step),0,1);
        wfx.wow+=(p->wow-wfx.wow)*glide; wfx.wow_hz+=(p->wow_hz-wfx.wow_hz)*glide;
        wfx.flutter+=(p->flutter-wfx.flutter)*glide; wfx.flutter_hz+=(p->flutter_hz-wfx.flutter_hz)*glide;
        wfx.drive+=(p->drive-wfx.drive)*glide; wfx.bias+=(p->bias-wfx.bias)*glide;
        wfx.strings+=(p->strings-wfx.strings)*glide;
        wfx.tone_a+=(tone-wfx.tone_a)*glide;
        wfx.damp+=(p->damp-wfx.damp)*glide;
        wfx.trim+=(p->trim-wfx.trim)*glide;
        wfx.gate=dsp_clampf(wfx.gate+gate_step,0,1);
        /* Tape: modulated read behind the write head, 64 samples of headroom. */
        wfx.wow_ph+=wfx.wow_hz*(1.0f/DSP_SAMPLE_RATE_HZ); if(wfx.wow_ph>=1.0f) wfx.wow_ph-=1.0f;
        wfx.flutter_ph+=wfx.flutter_hz*(1.0f/DSP_SAMPLE_RATE_HZ); if(wfx.flutter_ph>=1.0f) wfx.flutter_ph-=1.0f;
        float lag=96.0f+wfx.wow*dsp_sin(wfx.wow_ph)+wfx.flutter*dsp_sin(wfx.flutter_ph);
        float pos=(float)wfx.at-lag; if(pos<0) pos+=(float)WFX_TAPE;
        int i0=(int)pos; float fr=pos-(float)i0; int i1=(i0+1)&(WFX_TAPE-1);
        float g=wfx.drive,ig=1.0f/g,b=wfx.bias,zero=wfx_sat(g*b);
        float in[2]={l[i],r[i]},out[2];
        for(int c=0;c<2;++c) {
            wfx.tape[c][wfx.at]=in[c];
            float x=wfx.tape[c][i0]+fr*(wfx.tape[c][i1]-wfx.tape[c][i0]);
            x=(wfx_sat(g*(x+b))-zero)*ig;
            wfx.lp[c]+=wfx.tone_a*(x-wfx.lp[c]);
            out[c]=in[c]+wfx.mix*(wfx.trim*wfx.lp[c]-in[c]);
        }
        wfx.at=(wfx.at+1u)&(WFX_TAPE-1);
        /* Strings: excited above ~200 Hz, FIR loop damping with exactly one
         * sample of delay, so each string rings at its tuned pitch. */
        if(wfx.tuned) {
            float mono=.5f*(out[0]+out[1]);
            wfx.hp+=.0281f*(mono-wfx.hp);
            float drive_in=(mono-wfx.hp)*wfx.gate*(sel ? 1.0f : 0.0f);
            static const float pan_l[WFX_STRINGS]={.62f,.38f,.78f,.22f};
            float sl=0,sr=0,a=wfx.damp;
            for(int k=0;k<WFX_STRINGS;++k) {
                float rp=(float)wfx.cat-(wfx.period[k]-1.0f); if(rp<0) rp+=(float)WFX_COMB;
                int r0=(int)rp; float f=rp-(float)r0; int r1=(r0+1)&(WFX_COMB-1);
                float y=wfx.comb[k][r0]+f*(wfx.comb[k][r1]-wfx.comb[k][r0]);
                float d=a*(y+wfx.z2[k])+(1.0f-2.0f*a)*wfx.z1[k];
                wfx.z2[k]=wfx.z1[k]; wfx.z1[k]=y;
                wfx.comb[k][wfx.cat]=wfx.input[k]*drive_in+wfx.feedback[k]*d;
                sl+=pan_l[k]*y; sr+=(1.0f-pan_l[k])*y;
            }
            wfx.cat=(wfx.cat+1u)&(WFX_COMB-1);
            float s=wfx.strings*wfx.gate*wfx.trim;
            out[0]+=s*sl; out[1]+=s*sr;
        }
        l[i]=out[0]; r[i]=out[1];
    }
    /* Leave the stage only once it has faded and the strings are silent. */
    if(!sel && wfx.mix<=0 && wfx.strings<1e-5f) { wfx_reset_audio(); }
}
#endif
