/* Host renderer for the Ambient V3 Worlds (FAM_SOUND_AMBIENT_V3) through the
 * real Product engine. Not a listening acceptance and not a device timing:
 * it writes what the firmware would play and reports the host CPU time.
 *
 *   render_v3_world <out.wav> <world 0..5> <cells|generate|both> <seconds> [block]
 *
 *   cells:    the five Cells one after another, then SHIFT octave, held 4 s each.
 *   generate: GENERATE with DRONE underneath from second 1.
 *   both:     12 s of Cells, then GENERATE with DRONE.
 * `block` is the render partition in frames (default 512, the device block).
 * Prints one line: world, mode, seconds, host CPU ms per second of audio, the
 * worst single block in microseconds, heard note starts and the peak sample. */
#include "engine.h"
#include "engine_product.h"
#include "world_grammar.h"
#include "tuning.h"
#include "dsp.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if defined(__SSE2__)
#include <xmmintrin.h>
#include <pmmintrin.h>
#endif
enum { SR=44100,TICK=441 };
static unsigned starts;
static void hook(int on,uint8_t owner,float hz,float velocity) { (void)owner;(void)hz;(void)velocity; if(on>0) ++starts; }
static void u16(FILE *f,uint16_t x) { fputc(x&255,f); fputc(x>>8,f); }
static void u32(FILE *f,uint32_t x) { u16(f,x&65535); u16(f,(uint16_t)(x>>16)); }
static double now_us(void) { struct timespec t; clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t); return t.tv_sec*1e6+t.tv_nsec/1e3; }
int main(int argc,char **argv) {
    if(argc<5 || argc>6) return 2;
    int world=atoi(argv[2]),seconds=atoi(argv[4]),block=argc==6 ? atoi(argv[5]) : 512;
    bool cells=!strcmp(argv[3],"cells") || !strcmp(argv[3],"both");
    bool gen=!strcmp(argv[3],"generate") || !strcmp(argv[3],"both");
    if(world<0 || world>=AMBIENT_WORLD_COUNT || seconds<1 || seconds>600 || block<1 || block>512 || (!cells && !gen)) return 2;
    /* The H743 runs with flush-to-zero (REALTIME_AUDIO_RULES). An x86 host
     * does not by default, and decaying resonators then cost many times more
     * in denormals. FAM_HOST_FTZ=0 shows the host without it. */
    const char *ftz=getenv("FAM_HOST_FTZ");
#if defined(__SSE2__)
    if(!ftz || strcmp(ftz,"0")) { _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON); _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON); }
#else
    (void)ftz;
#endif
    FILE *out=fopen(argv[1],"wb"); if(!out) return 2;
    uint32_t frames=(uint32_t)seconds*SR,bytes=frames*4u;
    fwrite("RIFF",1,4,out); u32(out,36+bytes); fwrite("WAVEfmt ",1,8,out); u32(out,16);
    u16(out,1); u16(out,2); u32(out,SR); u32(out,SR*4); u16(out,4); u16(out,16); fwrite("data",1,4,out); u32(out,bytes);
    engine_init(); engine_set_world(world); engine_set_gen_seed(1234);
    engine_set_color(.5f); engine_set_master_volume(.75f); engine_set_room(.5f); engine_set_nature(0);
    engine_set_note_hook(hook);
    uint32_t gen_at=cells && gen ? 12000u : 1000u;
    double cpu=0,worst=0; int peak=0;
    for(uint32_t frame=0;frame<frames;) {
        if(frame%TICK==0) {
            uint32_t ms=(uint32_t)((uint64_t)frame*1000u/SR);
            if(cells && (!gen || ms<gen_at)) {
                /* One Cell every 2 s from 0.5 s, held 1.6 s; second round with SHIFT. */
                uint32_t slot=ms/2000u,in=ms%2000u; int cell=(int)(slot%5u); bool upper=(slot/5u)%2u==1u;
                if(in==500u) (void)engine_try_cell_on((uint8_t)cell,tuning_hz((float)engine_product_cell_midi(cell,upper)),.6f);
                if(in==100u && slot>0) engine_note_off((uint8_t)((slot-1u)%5u));
            }
            if(gen && ms==gen_at) { for(int c=0;c<5;++c) engine_note_off((uint8_t)c); engine_set_drone(true); engine_set_generative(true,-1); }
            engine_generative_tick(ms);
        }
        int n=(int)((frame/TICK+1u)*TICK-frame); if(n>block) n=block;
        int16_t pcm[1024];
        double t0=now_us(); engine_render(pcm,n); double dt=now_us()-t0;
        cpu+=dt; if(dt*512.0/n>worst) worst=dt*512.0/n;
        for(int i=0;i<2*n;++i) { u16(out,(uint16_t)pcm[i]); int a=abs(pcm[i]); if(a>peak) peak=a; }
        frame+=(uint32_t)n;
    }
    printf("world=%d mode=%s seconds=%d cpu_ms_per_s=%.2f worst_block_us_scaled512=%.0f starts=%u peak_dbfs=%.1f nonfinite=%u limited=%u\n",
        world,argv[3],seconds,cpu/1000.0/seconds,worst,starts,peak ? 20.0*log10(peak/32768.0) : -99.0,
        (unsigned)engine_nonfinite_samples(),(unsigned)engine_output_limited_samples());
    int error=ferror(out); if(fclose(out)) error=1; return error;
}
