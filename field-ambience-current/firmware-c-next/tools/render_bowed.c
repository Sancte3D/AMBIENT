/*
 * render_bowed.c — audition the bowed lyra voice (r19.46) playing a slow
 * Open-Sea (D mixolydian) phrase with overlapping bow strokes, through a
 * little reverb for space. Standalone — judges the VOICE before integration.
 * Build: tools/render_bowed.sh
 */
#include "bowed.h"
#include "reverb.h"
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SR    44100
#define BLOCK 256
#define SECS  42

static void pu32(FILE *f, uint32_t v){ for(int i=0;i<4;++i){fputc(v&0xff,f);v>>=8;} }
static void pu16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void hdr(FILE *f, uint32_t n){ uint32_t d=n*4u;
    fwrite("RIFF",1,4,f);pu32(f,36+d);fwrite("WAVE",1,4,f);fwrite("fmt ",1,4,f);
    pu32(f,16);pu16(f,1);pu16(f,2);pu32(f,SR);pu32(f,SR*4u);pu16(f,4);pu16(f,16);
    fwrite("data",1,4,f);pu32(f,d); }

int main(int argc, char **argv){
    const char *out = argc>1?argv[1]:"/tmp/bowed.wav";
    dsp_init(); bowed_init(); bowed_set_colour(0);   /* Open Sea */
    reverb_init(); reverb_set(0.80f, 0.42f);

    /* slow overlapping phrase, D mixolydian-ish, spread across octaves */
    static const int notes[] = { 50, 57, 62, 66, 69, 62, 64, 57, 72, 69, 62, 57 };
    static const float amps[] = {0.5f,0.42f,0.55f,0.5f,0.48f,0.5f,0.46f,0.44f,0.4f,0.5f,0.52f,0.45f};
    const int NN = (int)(sizeof(notes)/sizeof(notes[0]));

    FILE *f = fopen(out, "wb");
    uint32_t total = (uint32_t)SECS * SR;
    hdr(f, total);

    float dryL[BLOCK], dryR[BLOCK], sendL[BLOCK], sendR[BLOCK], wetL[BLOCK], wetR[BLOCK];
    int next_note = 0, note_i = 0;
    int16_t buf[BLOCK*2];
    uint32_t done = 0, blkctr = 0;
    const int NOTE_BLOCKS = (int)(3.1f * SR / BLOCK);   /* new stroke ~every 3.1 s */

    while (done < total){
        memset(dryL,0,sizeof dryL); memset(dryR,0,sizeof dryR);
        memset(sendL,0,sizeof sendL); memset(sendR,0,sizeof sendR);

        if ((int)blkctr >= next_note && note_i < NN){
            bowed_note(dsp_midi_to_hz((float)notes[note_i]), amps[note_i]);
            note_i++;
            next_note += NOTE_BLOCKS;
        }

        bowed_render_mix(dryL, dryR, sendL, sendR, BLOCK, 0.55f);
        reverb_render(sendL, sendR, wetL, wetR, BLOCK);

        for (int n=0;n<BLOCK;++n){
            float L = dryL[n] + wetL[n]*0.6f;
            float R = dryR[n] + wetR[n]*0.6f;
            if (L> 1.0f)L=1.0f; if(L<-1.0f)L=-1.0f;
            if (R> 1.0f)R=1.0f; if(R<-1.0f)R=-1.0f;
            buf[n*2]=(int16_t)(L*30000.0f); buf[n*2+1]=(int16_t)(R*30000.0f);
        }
        fwrite(buf,sizeof(int16_t),BLOCK*2,f);
        done += BLOCK; blkctr++;
    }
    fclose(f);
    printf("wrote %s (%d s bowed lyra, Open Sea)\n", out, SECS);
    return 0;
}
