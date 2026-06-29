/*
 * render_acid_dry.c — reference-matching renderer for the ACID RAIN engine.
 *
 * Renders the engine DRY: oscillator → envelope → filter → drive → output.
 * NO reverb, NO delay, NO beauty-guard, NO texture/beat. This is the debug
 * surface for A/B'ing against the Python reference WAV — if it sounds wrong
 * here, the synth core is wrong, not the FX. (The host adds global FX later;
 * see render_synth.c for the wet/product render.)
 *
 * Deterministic: replays the EXACT note sequence extracted from the reference
 * (ambient_world_01_acid_rain_303_inspired.wav): 31 steps, 254.6 ms 8ths
 * (117 BPM), A-minor acid line, accents where the reference accents.
 *
 *   cc -std=c11 -O2 -Iinclude tools/render_acid_dry.c \
 *      src/dsp.c src/dsp_ladder.c src/v2/engines/engine_acid.c -lm -o /tmp/render_acid_dry
 *   /tmp/render_acid_dry /tmp/acid_dry.wav
 */
#include "dsp.h"
#include "v2/synth_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SR     44100
#define BLOCK  256
#define STEP_S 0.25456f       /* 8th note @ 117 BPM (measured from reference) */
#define GATE_S 0.200f         /* note length ~0.2 s */

extern const synth_engine_t engine_acid;

typedef struct { int midi, accent; } ref_step_t;
/* extracted from the reference WAV (pitch + accent per 8th step) */
static const ref_step_t REF[] = {
  { 45,1}, { 45,0}, { 48,0}, { 45,1}, { 52,0}, { 50,1}, { 48,0}, { 45,0},
  { 45,1}, { 48,0}, { 45,1}, { 55,0}, { 52,0}, { 48,1}, { 50,0}, { 45,1},
  { 45,0}, { 45,0}, { 48,0}, { 45,0}, { 52,0}, { 50,0}, { 48,0}, { 45,0},
  { 48,0}, { 48,0}, { 45,0}, { 52,0}, { 40,0}, { 49,0}, { 50,0},
};
#define NSTEPS ((int)(sizeof REF / sizeof REF[0]))

static void u32(FILE *f, uint32_t v){ for(int i=0;i<4;++i) fputc((v>>(8*i))&0xff,f); }
static void u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data = nf * 4u;
    fwrite("RIFF",1,4,f); u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); u32(f,16); u16(f,1); u16(f,2);
    u32(f,SR); u32(f,SR*4u); u16(f,4); u16(f,16);
    fwrite("data",1,4,f); u32(f,data);
}

/* one note event */
typedef struct { double t; int on; int midi; float vel; } ev_t;

int main(int argc, char **argv){
    const char *out = (argc > 1) ? argv[1] : "/tmp/acid_dry.wav";
    dsp_init();
    engine_acid.init();
    /* same voicing as the wet render so A/B is apples-to-apples */
    engine_acid.set_param(SP_A, 0.40f);   /* cutoff base                     */
    engine_acid.set_param(SP_B, 0.50f);   /* resonance — gentler squelch     */
    engine_acid.set_param(SP_C, 0.22f);   /* decay                           */
    engine_acid.set_param(SP_D, 0.40f);   /* drive                           */
    engine_acid.set_param(SP_E, 0.35f);   /* glide                           */
    engine_acid.set_param(SP_F, 0.55f);   /* env amount — narrower sweep      */

    /* deterministic event list */
    static ev_t ev[NSTEPS*2]; int ne = 0;
    for (int i = 0; i < NSTEPS; ++i){
        double t0 = i * STEP_S;
        float vel = REF[i].accent ? 0.95f : 0.45f;
        ev[ne++] = (ev_t){ t0, 1, REF[i].midi, vel };
        ev[ne++] = (ev_t){ t0 + GATE_S, 0, REF[i].midi, 0.0f };
    }

    const uint32_t total = (uint32_t)(NSTEPS * STEP_S * SR) + SR/4; /* +0.25 s tail */
    FILE *f = fopen(out, "wb");
    if (!f){ fprintf(stderr,"open %s\n", out); return 1; }
    wav_header(f, total);

    float dL[BLOCK], dR[BLOCK], sL[BLOCK], sR[BLOCK];
    int16_t o16[BLOCK*2];
    int peak = 0; double sumsq = 0; long ns = 0;
    uint32_t frame = 0; int ei = 0;

    while (frame < total){
        int n = (int)((total - frame) < BLOCK ? (total - frame) : BLOCK);
        double t1 = (double)(frame + n) / SR;
        while (ei < ne && ev[ei].t < t1){
            if (ev[ei].on) engine_acid.note_on(ev[ei].midi, ev[ei].vel);
            else           engine_acid.note_off();
            ++ei;
        }
        memset(dL,0,sizeof(float)*n); memset(dR,0,sizeof(float)*n);
        memset(sL,0,sizeof(float)*n); memset(sR,0,sizeof(float)*n);
        engine_acid.render_mix(dL, dR, sL, sR, n);   /* DRY only; send ignored */
        for (int i=0;i<n;++i){
            float s = dL[i] * 0.9f;
            int li = (int)lrintf(s * 32767.0f);
            if (li >  32767) li =  32767;
            if (li < -32768) li = -32768;
            o16[i*2]=(int16_t)li; o16[i*2+1]=(int16_t)li;
            int a = li<0?-li:li; if (a>peak) peak=a;
            sumsq += (double)li*li; ++ns;
        }
        fwrite(o16, sizeof(int16_t), (size_t)n*2, f);
        frame += (uint32_t)n;
    }
    fclose(f);
    double rms = sqrt(sumsq/(ns?ns:1)) / 32768.0;
    printf("ACID RAIN (DRY) → %s  (%.2fs, peak %d = %.1f dBFS, rms %.3f, mono)\n",
           out, (double)total/SR, peak, 20.0*log10((double)(peak?peak:1)/32767.0), rms);
    printf("  golden reference: 8.00s rms 0.279. DRY = no FX (debug surface).\n");
    return 0;
}
