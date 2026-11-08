/* Real Product chain: COAST A/B, WOODLAND A/B, HIGHLANDS A/B, 12s each.
 * Usage: render_cell_unfold OUT.wav [neutral]. No file gain, AGC or new bed. */
#include "engine.h"
#include "engine_product.h"
#include "controls.h"
#include "cells.h"
#include "dsp.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
int main(int argc,char **argv) {
    if(argc<2 || argc>3)return 2;
    bool neutral=argc==3 && !strcmp(argv[2],"neutral");
    FILE *f=fopen(argv[1],"wb");if(!f)return 2;
    uint32_t bytes=72u*DSP_SAMPLE_RATE_HZ*4;
    fwrite("RIFF",1,4,f);u32(f,36+bytes);fwrite("WAVEfmt ",1,8,f);u32(f,16);
    u16(f,1);u16(f,2);u32(f,DSP_SAMPLE_RATE_HZ);u32(f,DSP_SAMPLE_RATE_HZ*4);
    u16(f,4);u16(f,16);fwrite("data",1,4,f);u32(f,bytes);
    for(int world=0;world<3;++world)for(int b=0;b<2;++b) {
        engine_init();controls_init();engine_set_world(world);
        engine_set_nature(0);engine_set_room(.5f);engine_set_master_volume(.75f);
        controls_modifier(MOD_HOLD,true);
        for(int tick=0;tick<1200;++tick) {
            if(tick==0 || tick==100) {
                int cell=tick ? 2 : 0;
                controls_cell_press((uint8_t)cell,CELL_AMP_MAX*.6f);
#ifdef HAS_CELL_UNFOLD
                if(b && !neutral)controls_cell_physical((uint8_t)cell,true);
#else
                (void)neutral;
#endif
            }
            if(tick==600)controls_cell_release(0);
            if(tick==700)controls_cell_release(2);
            if(tick==900)controls_modifier(MOD_CLEAR,true);
            int16_t pcm[882];engine_render(pcm,441);
            if(tick==101)assert(engine_active_voices()==2);
            for(int i=0;i<882;++i)u16(f,(uint16_t)pcm[i]);
            assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
        }
    }
    int error=ferror(f);if(fclose(f))error=1;
    return error ? 1 : 0;
}
