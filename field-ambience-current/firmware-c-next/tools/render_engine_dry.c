/*
 * render_engine_dry.c — DRY reference-matching renderer for the chorus / ion /
 * glass / bamboo engines (acid + fm have their own dry renderers). Renders the
 * selected engine with NO FX, replaying the exact note sequence extracted from
 * each reference WAV. Debug surface for A/B; render_synth.c is the wet path.
 *
 *   cc -std=c11 -O2 -Iinclude tools/render_engine_dry.c src/dsp.c \
 *      src/v2/engines/engine_chorus_mist.c src/v2/engines/engine_ion_storm.c \
 *      src/v2/engines/engine_glass_orbit.c src/v2/engines/engine_bamboo_circuit.c \
 *      -lm -o /tmp/render_engine_dry
 *   /tmp/render_engine_dry chorus|ion|glass|bamboo out.wav
 */
#include "dsp.h"
#include "v2/synth_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SR    44100
#define BLOCK 256

extern const synth_engine_t engine_chorus_mist, engine_ion_storm,
                            engine_glass_orbit, engine_bamboo_circuit;

typedef struct { float t; int midi; } note_t;

/* extracted sequences (onset time, pitch) */
static const note_t ION[] = {
  {0.006f,33},{0.401f,33},{0.517f,33},{0.627f,33},{0.743f,33},{0.853f,33},{0.975f,45},
  {2.003f,36},{2.351f,36},{2.467f,36},{2.577f,36},{2.688f,36},{2.966f,36},
  {4.011f,81},{4.139f,43},{4.470f,55},{4.586f,52},{4.795f,81},{4.899f,43},{5.021f,43},
  {6.008f,33},{6.142f,33},{6.397f,33},{6.513f,33},{6.629f,33},{6.745f,33},{6.873f,33} };
static const note_t WT[] = {
  {0.877f,40},{1.010f,40},{1.863f,40},{2.270f,38},{3.077f,35},{3.233f,35},{3.379f,35},
  {3.524f,35},{3.680f,35},{3.802f,35},{3.947f,35},{4.058f,35},{4.185f,35},{4.296f,35},
  {4.412f,35},{4.528f,35},{4.656f,35},{4.772f,35},{4.882f,35},{4.998f,35},{5.108f,35},
  {5.370f,35},{5.950f,40},{6.658f,35},{6.821f,35},{6.966f,47},{7.129f,47},{7.250f,47},{7.355f,35},{7.517f,35} };
static const note_t LPG[] = {
  {0.192f,45},{0.615f,52},{1.033f,48},{1.184f,48},{1.457f,55},{1.875f,50},{2.293f,57},
  {2.717f,53},{3.135f,60},{3.547f,45},{3.791f,38},{3.901f,52},{4.040f,52},{4.255f,48},
  {4.394f,48},{4.516f,48},{4.690f,55},{4.801f,43},{5.045f,38},{5.236f,50},{5.654f,57},
  {6.072f,53},{6.496f,60},{6.664f,38},{6.914f,45},{7.332f,52},{7.755f,48} };

static void u32(FILE *f, uint32_t v){ for(int i=0;i<4;++i) fputc((v>>(8*i))&0xff,f); }
static void u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data = nf * 4u;
    fwrite("RIFF",1,4,f); u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); u32(f,16); u16(f,1); u16(f,2);
    u32(f,SR); u32(f,SR*4u); u16(f,4); u16(f,16);
    fwrite("data",1,4,f); u32(f,data);
}

typedef struct { double t; int on; int midi; float vel; } ev_t;

int main(int argc, char **argv){
    if (argc < 2){ fprintf(stderr,"usage: %s chorus|ion|glass|bamboo [out.wav]\n",argv[0]); return 2; }
    const char *which = argv[1];
    const char *out = (argc > 2) ? argv[2] : "/tmp/engine_dry.wav";
    dsp_init();

    const synth_engine_t *e = 0;
    const note_t *seq = 0; int nseq = 0; float gate = 0.25f, vel = 0.85f, scale = 0.7f;
    int held = 0;        /* chorus = one held note */
    float p[6];

    if (!strcmp(which,"chorus")) {
        e=&engine_chorus_mist; held=1; scale=0.8f;
        p[0]=0.30f;p[1]=0.55f;p[2]=0.6f;p[3]=0.35f;p[4]=0.2f;p[5]=0.5f;
    } else if (!strcmp(which,"ion")) {
        e=&engine_ion_storm; seq=ION; nseq=(int)(sizeof ION/sizeof ION[0]); gate=0.26f; scale=0.7f;
        p[0]=0.40f;p[1]=0.55f;p[2]=0.6f;p[3]=0.45f;p[4]=0.2f;p[5]=0.4f;
    } else if (!strcmp(which,"glass")) {
        e=&engine_glass_orbit; seq=WT; nseq=(int)(sizeof WT/sizeof WT[0]); gate=0.45f; scale=0.7f;
        p[0]=0.45f;p[1]=0.68f;p[2]=0.55f;p[3]=0.60f;p[4]=0.2f;p[5]=0.6f;  /* more morph motion */
    } else if (!strcmp(which,"bamboo")) {
        e=&engine_bamboo_circuit; seq=LPG; nseq=(int)(sizeof LPG/sizeof LPG[0]); gate=0.05f; scale=0.85f;
        p[0]=0.60f;p[1]=0.15f;p[2]=0.25f;p[3]=0.30f;p[4]=0.2f;p[5]=0.35f;  /* faster pluck decay */
    } else { fprintf(stderr,"unknown engine '%s'\n",which); return 2; }

    e->init();
    for (int i=0;i<6;++i) e->set_param((synth_param_t)i, p[i]);

    /* build event list */
    static ev_t ev[128]; int ne = 0;
    if (held) {
        ev[ne++]=(ev_t){0.05,1,36,0.8f};         /* C2 pad, held */
        ev[ne++]=(ev_t){7.2,0,36,0.0f};
    } else {
        for (int i=0;i<nseq;++i){
            float t0=seq[i].t;
            float nxt=(i+1<nseq)?seq[i+1].t:8.0f;
            float g=nxt-t0; if (g>gate) g=gate;
            ev[ne++]=(ev_t){t0,1,seq[i].midi,vel};
            ev[ne++]=(ev_t){t0+g,0,seq[i].midi,0.0f};
        }
    }

    const uint32_t total = (uint32_t)(8.0*SR) + SR/2;
    FILE *f=fopen(out,"wb"); if(!f){fprintf(stderr,"open\n");return 1;}
    wav_header(f,total);
    float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK]; int16_t o16[BLOCK*2];
    int peak=0; double sumsq=0; long ns=0; uint32_t frame=0; int ei=0;
    while (frame<total){
        int n=(int)((total-frame)<BLOCK?(total-frame):BLOCK);
        double t1=(double)(frame+n)/SR;
        while (ei<ne && ev[ei].t<t1){
            if (ev[ei].on) e->note_on(ev[ei].midi,ev[ei].vel); else e->note_off();
            ++ei;
        }
        memset(dL,0,sizeof(float)*n);memset(dR,0,sizeof(float)*n);
        memset(sL,0,sizeof(float)*n);memset(sR,0,sizeof(float)*n);
        e->render_mix(dL,dR,sL,sR,n);
        for (int i=0;i<n;++i){
            float l=dL[i]*scale, r=dR[i]*scale;
            int li=(int)lrintf(l*32767.0f), ri=(int)lrintf(r*32767.0f);
            if(li> 32767)li= 32767;
            if(li<-32768)li=-32768;
            if(ri> 32767)ri= 32767;
            if(ri<-32768)ri=-32768;
            o16[i*2]=(int16_t)li;o16[i*2+1]=(int16_t)ri;
            int a=li<0?-li:li; if(a>peak)peak=a; sumsq+=(double)li*li; ++ns;
        }
        fwrite(o16,sizeof(int16_t),(size_t)n*2,f); frame+=(uint32_t)n;
    }
    fclose(f);
    printf("%s (DRY) → %s  peak %d = %.1f dBFS, rms %.3f\n", which, out, peak,
           20.0*log10((double)(peak?peak:1)/32767.0), sqrt(sumsq/(ns?ns:1))/32768.0);
    return 0;
}
