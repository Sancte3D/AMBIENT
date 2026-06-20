/*
 * render_cells_ab.c — A/B audition: single-note cells (HEUTE) vs chord cells
 * (HiChord-Modell) on the SAME phrase, same V1 pad voice.
 *
 * Part A "single" : each cell press sounds ONE note — brain_cell_root(cell) —
 *                   exactly what engine_cell_sample() does today.
 * Part B "chords" : each cell press sounds the FULL voiced chord for that
 *                   degree — brain_chord() — and the chord COLOUR (vibe family:
 *                   add9 → maj7 → min11 → sus2) sweeps across the phrase, the
 *                   way one encoder bound to brain_set_vibe() would feel live.
 *
 * Both parts use the identical degree sequence + timing + pad profile, so the
 * only variable the listener hears is "single note" vs "in-key chord + colour".
 *
 * No engine, no reverb, no firmware change — pure offline demo built from the
 * existing brain + pad sources.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_cells_ab.c \
 *      src/dsp.c src/pad.c src/brain.c -lm -o /tmp/render_cells_ab
 *   /tmp/render_cells_ab /tmp/cells_
 */

#include "pad.h"
#include "brain.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR          44100
#define BLOCK       256
#define PART_SECS   26
#define HOLD_S      4.8f    /* each chord/note rings this long (presses 4 s apart) */

static void put_u32(FILE *f,uint32_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);fputc((v>>16)&0xff,f);fputc((v>>24)&0xff,f);}
static void put_u16(FILE *f,uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void wav_header(FILE *f,uint32_t nframes){
    uint32_t data=nframes*4u;
    fwrite("RIFF",1,4,f);put_u32(f,36+data);fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f);put_u32(f,16);put_u16(f,1);put_u16(f,2);
    put_u32(f,SR);put_u32(f,SR*4u);put_u16(f,4);put_u16(f,16);
    fwrite("data",1,4,f);put_u32(f,data);
}

/* The phrase: 6 cell presses, ~4 s apart, degrees within the 5-cell range
 * (1..5). A slow ambient loop in C-dorian (brain guarantees consonance). */
typedef struct { float t; int degree; int vibe; } press_t;
static const press_t SEQ[] = {
    /* t      degree  vibe (Part B colour: 0 add9, 1 maj7, 2 min11, 3 sus2) */
    {  0.5f,  1,      0 },
    {  4.5f,  5,      0 },
    {  8.5f,  4,      1 },
    { 12.5f,  3,      1 },
    { 16.5f,  2,      2 },
    { 20.5f,  1,      3 },
};
#define N_SEQ (sizeof SEQ / sizeof SEQ[0])

/* note-off scheduler (one entry per voice) */
typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[PAD_MAX*2]; static int npend = 0;
static void sched_off(float t, uint8_t src){
    if (npend < (int)(sizeof pend/sizeof pend[0])) { pend[npend].t=t; pend[npend].src=src; npend++; }
}

static void render_part(int chords, const char *out_path, FILE *combined_f) {
    brain_init();
    brain_set_key(60);          /* C4 */
    brain_set_mode(1);          /* dorian — floaty minor, not church-ionian */
    pad_init();
    pad_set_profile(1);         /* Felt — the cell-tap voice */
    npend = 0;

    FILE *f = fopen(out_path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", out_path); return; }
    wav_header(f, (uint32_t)PART_SECS * SR);

    int16_t buf[BLOCK*2];
    uint32_t total = (uint32_t)PART_SECS * SR, frame = 0;
    size_t si = 0;
    int peak = 0; double sumsq = 0; long sumN = 0;

    while (frame < total) {
        float t = (float)frame / SR;

        while (si < N_SEQ && SEQ[si].t <= t) {
            const press_t *p = &SEQ[si];
            int bank = (int)(si % 2) * 6;          /* two 6-voice banks → clean overlap */
            if (chords) {
                brain_set_vibe(p->vibe);            /* live colour, like the encoder */
                int notes[BRAIN_MAX_CHORD];
                int n = brain_chord(p->degree, notes, BRAIN_MAX_CHORD);
                float per = 0.13f / sqrtf((float)(n > 0 ? n : 1));   /* equal-power */
                for (int j = 0; j < n; ++j) {
                    uint8_t src = (uint8_t)(bank + j);
                    pad_note_on(src, dsp_midi_to_hz((float)notes[j]), per);
                    sched_off(t + HOLD_S, src);
                }
            } else {
                int midi = brain_cell_root(p->degree - 1);   /* cell = degree-1 */
                uint8_t src = (uint8_t)bank;
                pad_note_on(src, dsp_midi_to_hz((float)midi), 0.15f);
                sched_off(t + HOLD_S, src);
            }
            ++si;
        }
        for (int i = 0; i < npend; ) {
            if (pend[i].t <= t) { pad_note_off(pend[i].src); pend[i] = pend[--npend]; }
            else ++i;
        }

        int n = (int)((total - frame) < BLOCK ? (total - frame) : BLOCK);
        pad_render(buf, n);
        for (int i = 0; i < n*2; ++i) {
            int a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > peak) peak = a;
            double s = buf[i] / 32768.0; sumsq += s*s; ++sumN;
        }
        fwrite(buf, sizeof(int16_t), (size_t)n*2, f);
        if (combined_f) fwrite(buf, sizeof(int16_t), (size_t)n*2, combined_f);
        frame += (uint32_t)n;
    }
    fclose(f);
    double rms = sqrt(sumsq / (double)sumN);
    printf("  %-7s → %s : peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           chords ? "chords" : "single", out_path, peak,
           20.0*log10((double)(peak?peak:1)/32767.0), rms, 20.0*log10(rms+1e-12));
}

int main(int argc, char **argv) {
    const char *prefix = (argc > 1) ? argv[1] : "/tmp/cells_";
    dsp_init();

    char a_path[512], b_path[512], ab_path[512];
    snprintf(a_path,  sizeof a_path,  "%sA_single.wav", prefix);
    snprintf(b_path,  sizeof b_path,  "%sB_chords.wav", prefix);
    snprintf(ab_path, sizeof ab_path, "%sAB.wav",       prefix);

    FILE *cf = fopen(ab_path, "wb");
    if (!cf) { fprintf(stderr, "cannot open %s\n", ab_path); return 1; }
    wav_header(cf, (uint32_t)PART_SECS * SR * 2);   /* A then B back-to-back */

    printf("A/B cells render (same phrase, C-dorian, Felt voice):\n");
    render_part(0, a_path, cf);   /* Part A — single note per cell (today) */
    render_part(1, b_path, cf);   /* Part B — full chord + colour sweep    */
    fclose(cf);
    printf("Combined A→B: %s (%d s)\n", ab_path, PART_SECS * 2);
    return 0;
}
