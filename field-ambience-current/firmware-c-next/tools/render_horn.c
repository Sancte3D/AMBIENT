/*
 * render_horn.c — audition the alphorn/brass voice (r19.48) playing a slow
 * Alps (G lydian) call with overlapping blown notes, through a little reverb
 * for the valley. Standalone — judges the VOICE before integration.
 * Build: tools/render_horn.sh
 */
#include "horn.h"
#include "reverb.h"
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SR    44100
#define BLOCK 256
#define SECS  40

static void pu32(FILE *f, uint32_t v){ for(int i=0;i<4;++i){fputc(v&0xff,f);v>>=8;} }
static void pu16(FILE *f, uint16_t v){ fputc(v&0xff,f); fputc((v>>8)&0xff,f); }
static void hdr(FILE *f, uint32_t n){ uint32_t d=n*4u;
    fwrite("RIFF",1,4,f);pu32(f,36+d);fwrite("WAVE",1,4,f);fwrite("fmt ",1,4,f);
    pu32(f,16);pu16(f,1);pu16(f,2);pu32(f,SR);pu32(f,SR*4u);pu16(f,4);pu16(f,16);
    fwrite("data",1,4,f);pu32(f,d); }

int main(int argc, char **argv){
    const char *out = argc>1?argv[1]:"/tmp/horn.wav";
    dsp_init(); horn_init();
    reverb_init(); reverb_set(0.86f, 0.30f);   /* long valley, brighter */

    /* slow overlapping alpine call, G lydian, spread across octaves */
    static const int notes[] = { 55, 62, 59, 67, 66, 62, 64, 59, 71, 67, 62, 55 };
    static const float amps[] = {0.5f,0.5f,0.46f,0.52f,0.48f,0.5f,0.46f,0.44f,0.42f,0.5f,0.5f,0.5f};
    const int NN = (int)(sizeof(notes)/sizeof(notes[0]));

    FILE *f = fopen(out, "wb");
    uint32_t total = (uint32_t)SECS * SR;
    hdr(f, total);

    float dryL[BLOCK], dryR[BLOCK], sendL[BLOCK], sendR[BLOCK], wetL[BLOCK], wetR[BLOCK];
    int next_note = 0, note_i = 0;
    int16_t buf[BLOCK*2];
    uint32_t done = 0, blkctr = 0;
    const int NOTE_BLOCKS = (int)(3.0f * SR / BLOCK);   /* new call ~every 3.0 s */

    while (done < total){
        memset(dryL,0,sizeof dryL); memset(dryR,0,sizeof dryR);
        memset(sendL,0,sizeof sendL); memset(sendR,0,sizeof sendR);

        if ((int)blkctr >= next_note && note_i < NN){
            horn_note(dsp_midi_to_hz((float)notes[note_i]), amps[note_i]);
            note_i++;
            next_note += NOTE_BLOCKS;
        }

        horn_render_mix(dryL, dryR, sendL, sendR, BLOCK, 0.5f);
        reverb_render(sendL, sendR, wetL, wetR, BLOCK);

        for (int n=0;n<BLOCK;++n){
            float L = dryL[n] + wetL[n]*0.55f;
            float R = dryR[n] + wetR[n]*0.55f;
            if (L> 1.0f)L=1.0f; if(L<-1.0f)L=-1.0f;
            if (R> 1.0f)R=1.0f; if(R<-1.0f)R=-1.0f;
            buf[n*2]=(int16_t)(L*30000.0f); buf[n*2+1]=(int16_t)(R*30000.0f);
        }
        fwrite(buf,sizeof(int16_t),BLOCK*2,f);
        done += BLOCK; blkctr++;
    }
    fclose(f);
    printf("wrote %s (%d s alphorn, Alps)\n", out, SECS);
    return 0;
}
