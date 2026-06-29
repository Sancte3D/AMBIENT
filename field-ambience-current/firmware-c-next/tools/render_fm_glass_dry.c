/*
 * render_fm_glass_dry.c — reference-matching renderer for the FM GLASS engine.
 *
 * Renders engine_fm_glass DRY (no reverb/limiter/texture), replaying the EXACT
 * free-timed note sequence extracted from the reference WAV
 * (ambient_world_02_fm_glass_station_dx7_inspired.wav): 23 notes, low register
 * (E2-A2). Debug surface for A/B; render_synth.c is the wet/product path.
 *
 *   cc -std=c11 -O2 -Iinclude tools/render_fm_glass_dry.c \
 *      src/dsp.c src/v2/engines/engine_fm_glass.c -lm -o /tmp/render_fm_glass_dry
 *   /tmp/render_fm_glass_dry /tmp/fm_glass_dry.wav
 */
#include "dsp.h"
#include "v2/synth_engine.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SR     44100
#define BLOCK  256

extern const synth_engine_t engine_fm_glass;

typedef struct { float t; int midi; } fm_note_t;
/* extracted from the reference WAV (onset time + pitch) */
static const fm_note_t SEQ[] = {
  {0.000f,45}, {0.163f,45}, {0.325f,45}, {0.749f,43}, {1.498f,40},
  {1.997f,43}, {2.119f,43}, {2.241f,43}, {2.374f,43}, {2.995f,45},
  {3.750f,43}, {4.000f,40}, {4.122f,40}, {4.243f,40}, {4.365f,40},
  {4.499f,40}, {4.714f,40}, {5.248f,41}, {5.997f,41}, {6.124f,41},
  {6.252f,41}, {6.409f,45}, {6.745f,43},
};
#define NSEQ ((int)(sizeof SEQ / sizeof SEQ[0]))
#define TOTAL_S 8.0f
#define GATE_MAX 0.45f          /* glass rings; cap the gate so repeats retrigger */

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
    const char *out = (argc > 1) ? argv[1] : "/tmp/fm_glass_dry.wav";
    dsp_init();
    engine_fm_glass.init();
    engine_fm_glass.set_param(SP_A, 0.78f);   /* brightness (index peak)     */
    engine_fm_glass.set_param(SP_B, 0.40f);   /* ratio → 3 (harmonics higher)*/
    engine_fm_glass.set_param(SP_C, 0.55f);   /* decay — let brightness ring */
    engine_fm_glass.set_param(SP_D, 0.78f);   /* tone — brighter lowpass     */
    engine_fm_glass.set_param(SP_E, 0.20f);   /* glide                       */
    engine_fm_glass.set_param(SP_F, 0.70f);   /* body — sustained harmonics  */

    /* build events: gate each note until the next onset, capped at GATE_MAX */
    static ev_t ev[NSEQ*2]; int ne = 0;
    for (int i = 0; i < NSEQ; ++i){
        float t0 = SEQ[i].t;
        float nxt = (i+1 < NSEQ) ? SEQ[i+1].t : TOTAL_S;
        float gate = nxt - t0; if (gate > GATE_MAX) gate = GATE_MAX;
        ev[ne++] = (ev_t){ t0, 1, SEQ[i].midi, 0.85f };
        ev[ne++] = (ev_t){ t0 + gate, 0, SEQ[i].midi, 0.0f };
    }

    const uint32_t total = (uint32_t)(TOTAL_S * SR) + SR/2;   /* +0.5 s ring-out */
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
            if (ev[ei].on) engine_fm_glass.note_on(ev[ei].midi, ev[ei].vel);
            else           engine_fm_glass.note_off();
            ++ei;
        }
        memset(dL,0,sizeof(float)*n); memset(dR,0,sizeof(float)*n);
        memset(sL,0,sizeof(float)*n); memset(sR,0,sizeof(float)*n);
        engine_fm_glass.render_mix(dL, dR, sL, sR, n);
        for (int i=0;i<n;++i){
            float s = dL[i] * 0.5f;   /* the reference is soft (rms 0.11) */
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
    printf("FM GLASS (DRY) → %s  (%.2fs, peak %d = %.1f dBFS, rms %.3f, mono)\n",
           out, (double)total/SR, peak, 20.0*log10((double)(peak?peak:1)/32767.0), rms);
    printf("  golden reference: 8.00s rms 0.114 (soft). DRY = no FX.\n");
    return 0;
}
