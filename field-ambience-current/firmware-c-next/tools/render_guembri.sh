#!/usr/bin/env bash
# render_guembri.sh — audition the guembri/sintir low-lute voice (Desert).
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out="${1:-/tmp/guembri.wav}"
cat > /tmp/render_guembri.c <<'EOF'
#include "guembri.h"
#include "reverb.h"
#include "dsp.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define SR 44100
#define BLOCK 256
#define SECS 32
static void pu32(FILE*f,uint32_t v){for(int i=0;i<4;++i){fputc(v&0xff,f);v>>=8;}}
static void pu16(FILE*f,uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void hdr(FILE*f,uint32_t n){uint32_t d=n*4u;fwrite("RIFF",1,4,f);pu32(f,36+d);fwrite("WAVE",1,4,f);fwrite("fmt ",1,4,f);pu32(f,16);pu16(f,1);pu16(f,2);pu32(f,SR);pu32(f,SR*4u);pu16(f,4);pu16(f,16);fwrite("data",1,4,f);pu32(f,d);}
int main(int argc,char**argv){
  const char*out=argc>1?argv[1]:"/tmp/guembri.wav";
  dsp_init(); guembri_init(); reverb_init(); reverb_set(0.55f,0.5f);   /* dry-ish desert */
  /* F-phrygian (Desert) low riff, guembri register */
  static const int notes[]={29,36,41,29,34,36,41,29,36,34,29,41};
  static const float amps[]={.6f,.55f,.5f,.6f,.52f,.55f,.5f,.6f,.55f,.52f,.6f,.5f};
  const int NN=(int)(sizeof(notes)/sizeof(notes[0]));
  FILE*f=fopen(out,"wb"); uint32_t total=(uint32_t)SECS*SR; hdr(f,total);
  float dL[BLOCK],dR[BLOCK],sL[BLOCK],sR[BLOCK],wL[BLOCK],wR[BLOCK]; int16_t buf[BLOCK*2];
  int next=0,ni=0; uint32_t done=0,blk=0; const int NB=(int)(1.5f*SR/BLOCK);   /* plucks every 1.5s */
  while(done<total){
    memset(dL,0,sizeof dL);memset(dR,0,sizeof dR);memset(sL,0,sizeof sL);memset(sR,0,sizeof sR);
    if((int)blk>=next){guembri_note(dsp_midi_to_hz((float)notes[ni%NN]),amps[ni%NN]);ni++;next+=NB;}
    guembri_render_mix(dL,dR,sL,sR,BLOCK,0.35f);
    reverb_render(sL,sR,wL,wR,BLOCK);
    for(int n=0;n<BLOCK;++n){float L=dL[n]+wL[n]*0.4f,R=dR[n]+wR[n]*0.4f;if(L>1)L=1;if(L<-1)L=-1;if(R>1)R=1;if(R<-1)R=-1;buf[n*2]=(int16_t)(L*30000);buf[n*2+1]=(int16_t)(R*30000);}
    fwrite(buf,2,BLOCK*2,f); done+=BLOCK; blk++;
  }
  fclose(f); printf("wrote %s (%d s guembri, Desert)\n",out,SECS); return 0;
}
EOF
cc -std=c11 -O2 -I"$here/include" /tmp/render_guembri.c "$here"/src/guembri.c "$here"/src/dsp.c \
  "$here"/src/reverb.c "$here"/src/reverb_presets.c -lm -o /tmp/render_guembri
/tmp/render_guembri "$out"
