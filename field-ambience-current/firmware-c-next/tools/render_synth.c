/*
 * render_synth.c — offline audition of the synth-engine host.
 *
 * Plays an acid line through synth_host (ACID RAIN engine + global reverb +
 * beauty-guard), calibrated to the golden reference: ~117 BPM straight 8ths,
 * bass register A2..D3, with accents and a couple of legato slides so the
 * resonant filter squelch + glide are audible.
 *
 *   cc -std=c11 -O2 -Iinclude tools/render_synth.c \
 *      src/dsp.c src/reverb.c src/v2/beauty_guard.c src/v2/synth_host.c \
 *      src/v2/engines/engine_acid.c -lm -o /tmp/render_synth
 *   /tmp/render_synth /tmp/acid.wav
 */
#include "dsp.h"
#include "v2/synth_host.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR     44100
#define BLOCK  256
#define BPM    117.0f
#define LOOPS  4

static void u32(FILE *f, uint32_t v){ for(int i=0;i<4;++i) fputc((v>>(8*i))&0xff,f); }
static void u16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void wav_header(FILE *f, uint32_t nf){
    uint32_t data = nf * 4u;
    fwrite("RIFF",1,4,f); u32(f,36+data); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); u32(f,16); u16(f,1); u16(f,2);
    u32(f,SR); u32(f,SR*4u); u16(f,4); u16(f,16);
    fwrite("data",1,4,f); u32(f,data);
}

/* 16-step acid line, 8th-note grid. accent: 0..1; slide: this note glides
 * INTO the next one (its note_off is skipped → legato). A minor flavour. */
typedef struct { int midi; float accent; int slide; } step_t;
static const step_t PAT[16] = {
    {45,0.95f,0},{45,0.30f,0},{57,0.95f,0},{45,0.30f,1}, /* A2 A2 A3acc A2(slide) */
    {48,0.30f,0},{45,0.95f,0},{50,0.30f,1},{45,0.30f,0}, /* C3 A2acc D3(slide) A2 */
    {45,0.30f,0},{53,0.95f,0},{45,0.30f,1},{48,0.30f,0}, /* A2 F3acc A2(slide) C3 */
    {45,0.95f,0},{50,0.30f,0},{57,0.95f,1},{48,0.30f,0}, /* A2acc D3 A3acc(slide) C3 */
};

/* a scheduled note event */
typedef struct { double t; int on; int midi; float vel; } ev_t;

int main(int argc, char **argv){
    const char *out = (argc > 1) ? argv[1] : "/tmp/acid.wav";
    dsp_init();
    synth_host_init();
    synth_host_select(SYNTH_ACID);
    synth_host_set_param(SP_A, 0.30f);   /* cutoff base                   */
    synth_host_set_param(SP_B, 0.62f);   /* resonance                     */
    synth_host_set_param(SP_C, 0.24f);   /* decay                         */
    synth_host_set_param(SP_D, 0.45f);   /* drive                         */
    synth_host_set_param(SP_E, 0.35f);   /* glide                         */
    synth_host_set_param(SP_F, 0.65f);   /* env amount                    */
    synth_host_set_reverb(0.45f, 0.22f); /* tasteful space, mostly dry    */
    synth_host_set_master(0.85f);

    /* build the absolute event list */
    const double step = 60.0 / BPM / 2.0;     /* 8th note */
    static ev_t ev[16*LOOPS*2]; int ne = 0;
    for (int L = 0; L < LOOPS; ++L)
        for (int i = 0; i < 16; ++i){
            double t0 = (L*16 + i) * step;
            const step_t *s = &PAT[i];
            float vel = 0.45f + s->accent * 0.55f;
            ev[ne++] = (ev_t){ t0, 1, s->midi, vel };
            if (!s->slide)                       /* gate ~58% unless sliding */
                ev[ne++] = (ev_t){ t0 + step*0.58, 0, s->midi, 0.0f };
        }

    const uint32_t total = (uint32_t)((16*LOOPS) * step * SR) + SR; /* +1s tail */
    FILE *f = fopen(out, "wb");
    if (!f){ fprintf(stderr,"open %s\n", out); return 1; }
    wav_header(f, total);

    int16_t o16[BLOCK*2];
    int peak = 0; double sumsq = 0; long ns = 0;
    uint32_t frame = 0; int ei = 0;

    while (frame < total){
        int n = (int)((total - frame) < BLOCK ? (total - frame) : BLOCK);
        double t1 = (double)(frame + n) / SR;
        while (ei < ne && ev[ei].t < t1){
            if (ev[ei].on) synth_host_note_on(ev[ei].midi, ev[ei].vel);
            else           synth_host_note_off();
            ++ei;
        }
        synth_host_render(o16, n);
        for (int i=0;i<n;++i){
            int li = o16[i*2]; int a = li<0?-li:li; if (a>peak) peak=a;
            sumsq += (double)li*li; ++ns;
        }
        fwrite(o16, sizeof(int16_t), (size_t)n*2, f);
        frame += (uint32_t)n;
    }
    fclose(f);
    double rms = sqrt(sumsq/(ns?ns:1)) / 32768.0;
    printf("ACID RAIN → %s  (%.1fs, peak %d = %.1f dBFS, rms %.3f, stereo)\n",
           out, (double)total/SR, peak, 20.0*log10((double)(peak?peak:1)/32767.0), rms);
    printf("  golden reference rms was 0.279 — compare loudness/character.\n");
    return 0;
}
