/*
 * render_synth_samples.c — kurze Demo je V2-Sound-Core (Acid, FM Glass,
 * Chorus Mist, Ion Storm, Glass Orbit, Bamboo Circuit) durch die ECHTE
 * synth_host-Note-API — der Pfad, den das Geraet im SYNTH-Menue nutzt.
 * Ein WAV (44.1k/16-bit/stereo) mit allen 6 Cores nacheinander, je Core
 * leicht auf einen Zielpegel getrimmt. Argument 1 = Ausgabepfad.
 *
 * WICHTIG: dsp_init() zuerst (fuellt die Sinus-LUT — sonst sind die
 * dsp_sin-basierten Cores FM Glass + Bamboo stumm).
 * Kompile: siehe tools/render_synth_samples.sh
 */
#include "v2/synth_host.h"
#include "dsp.h"
#include "audio.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SR 44100
#define BLK AUDIO_BUFFER_FRAMES
#define TARGET_PEAK 20000.0f

static int16_t *g_buf; static size_t g_cap, g_len;
static void push(const int16_t *s,int fr){size_t need=g_len+(size_t)fr*2;
    if(need>g_cap){g_cap=need*2;g_buf=realloc(g_buf,g_cap*sizeof*g_buf);}
    memcpy(g_buf+g_len,s,(size_t)fr*2*sizeof*s);g_len=need;}
static void render_ms(int ms){static int16_t b[BLK*2];int t=SR*ms/1000,d=0;
    while(d<t){int n=(t-d<BLK)?t-d:BLK;synth_host_render(b,n);push(b,n);d+=n;}}
static void phrase(const int *m,const int *t,int n){
    for(int i=0;i<n;i++){synth_host_note_on(m[i],0.9f);render_ms(t[i]);synth_host_note_off();render_ms(150);}
    render_ms(550);}
static void normalize_from(size_t from){int pk=1;
    for(size_t i=from;i<g_len;i++){int a=g_buf[i]<0?-g_buf[i]:g_buf[i];if(a>pk)pk=a;}
    if(pk<40)return;float g=TARGET_PEAK/(float)pk;if(g>4.0f)g=4.0f;if(g<0.5f)g=0.5f;
    for(size_t i=from;i<g_len;i++){float v=(float)g_buf[i]*g;
        g_buf[i]=(int16_t)(v>32767?32767:v<-32768?-32768:v);}}
static void wav_write(const char*p){FILE*f=fopen(p,"wb");if(!f){perror("fopen");exit(1);}
    uint32_t fr=(uint32_t)(g_len/2),data=fr*4u,riff=36u+data,sr=SR,br=sr*4u;
    uint16_t bs=4,bps=16,ch=2,fmt=1;uint32_t fs=16;
    fwrite("RIFF",1,4,f);fwrite(&riff,4,1,f);fwrite("WAVE",1,4,f);fwrite("fmt ",1,4,f);
    fwrite(&fs,4,1,f);fwrite(&fmt,2,1,f);fwrite(&ch,2,1,f);fwrite(&sr,4,1,f);fwrite(&br,4,1,f);
    fwrite(&bs,2,1,f);fwrite(&bps,2,1,f);fwrite("data",1,4,f);fwrite(&data,4,1,f);
    fwrite(g_buf,sizeof*g_buf,g_len,f);fclose(f);}

int main(int argc,char**argv){
    const char*path=argc>1?argv[1]:"/tmp/field_synths.wav";
    dsp_init();                                  /* Sinus-LUT — Pflicht! */
    synth_host_init(); synth_host_set_reverb(0.42f,0.18f);
    const synth_id_t order[6]={SYNTH_ACID,SYNTH_FM_GLASS,SYNTH_CHORUS_MIST,
        SYNTH_ION_STORM,SYNTH_GLASS_ORBIT,SYNTH_BAMBOO_CIRCUIT};
    int acid_n[]={45,45,52,45,57,45,48,45},  acid_t[]={150,150,150,150,150,150,150,150};
    int glass_n[]={60,64,67,72,67,60},       glass_t[]={430,430,430,620,430,760};
    int mist_n[]={48,52,55,59},              mist_t[]={1100,1100,1100,1400};
    int storm_n[]={45,45,52,50},             storm_t[]={520,520,700,900};
    int orbit_n[]={52,59,55,64},             orbit_t[]={1200,1200,1200,1500};
    int bamboo_n[]={48,55,52,59,48,57},      bamboo_t[]={280,280,280,280,280,440};
    for(int i=0;i<6;i++){
        synth_host_select(order[i]);
        fprintf(stderr,"  core %d: %s\n",i,synth_host_active_name());
        render_ms(1400);                         /* Vorlauf: alten Reverb abklingen */
        size_t from=g_len;
        switch(order[i]){
            case SYNTH_ACID:           phrase(acid_n,acid_t,8); break;
            case SYNTH_FM_GLASS:       phrase(glass_n,glass_t,6); break;
            case SYNTH_CHORUS_MIST:    phrase(mist_n,mist_t,4); break;
            case SYNTH_ION_STORM:      phrase(storm_n,storm_t,4); break;
            case SYNTH_GLASS_ORBIT:    phrase(orbit_n,orbit_t,4); break;
            case SYNTH_BAMBOO_CIRCUIT: phrase(bamboo_n,bamboo_t,6); break;
        }
        synth_host_panic(); render_ms(650); normalize_from(from);
    }
    wav_write(path);
    fprintf(stderr,"wrote %s (%.1fs)\n",path,(double)(g_len/2)/SR);
    free(g_buf);return 0;
}
