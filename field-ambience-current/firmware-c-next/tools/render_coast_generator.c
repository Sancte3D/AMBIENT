/* Actual Product engine audition. Main ticks on a fixed 10-ms grid; the
 * renderer never calls a source directly or changes Product SHAPE limits.
 * Stop after the first shared destination has held for 6s; releases end
 * naturally, with no Clear, output fade, archive layer or extra effect. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
enum { SR=44100,SECONDS=70,MAX_BLOCK=512,TICK=441 };
static FILE *trace;
static uint32_t at,goal_ms;
static int starts,lower[4],upper[3],nl,nu,anchor,max_sources;
static void hook(int on,uint8_t owner,float hz,float v) {
    fprintf(trace,"%u,%d,%u,%.7f,%.5f\n",at,on,owner,(double)hz,(double)v);
    if(on<=0)return;
    int midi=(int)lroundf(69+12*log2f(hz/440));++starts;
    if(owner==6) {assert(nl<4);lower[nl++]=midi;if(nl==4)goal_ms=at*1000u/SR;}
    else if(owner==7) {assert(nu<3);upper[nu++]=midi;}
    else {assert(owner==15);++anchor;}
}
static void u16(FILE *f,uint16_t x) {fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x) {u16(f,x&65535);u16(f,x>>16);}
static void header(FILE *f) {
    uint32_t n=SR*SECONDS*4;
    fwrite("RIFF",1,4,f);u32(f,36+n);fwrite("WAVEfmt ",1,8,f);u32(f,16);
    u16(f,1);u16(f,2);u32(f,SR);u32(f,SR*4);u16(f,4);u16(f,16);
    fwrite("data",1,4,f);u32(f,n);
}
int main(int argc,char **argv) {
    if(argc!=4)return 2;
    int block=atoi(argv[2]);bool room=!strcmp(argv[1],"room");
    if(block<1 || block>MAX_BLOCK || (!room && strcmp(argv[1],"dry")))return 2;
    FILE *out=fopen(argv[3],"wb");if(!out)return 2;header(out);
    char path[1024];if(snprintf(path,sizeof path,"%s.events.csv",argv[3])>=(int)sizeof path)return 2;
    trace=fopen(path,"w");if(!trace)return 2;
    fprintf(trace,"ack_frame,on,owner,hz,velocity\n");
    engine_init();engine_set_note_hook(hook);engine_set_gen_seed(1234);
    engine_set_room(.24f);engine_set_nature(0);engine_set_fx_mode(room ? 1 : 0);
    engine_set_generative(true,-1);
    int16_t pcm[MAX_BLOCK*2];bool stopped=false;int peak=0;
    for(at=0;at<SR*SECONDS;) {
        if(at%TICK==0) {
            uint32_t ms=at*1000u/SR;
            engine_generative_tick(ms);
            if(goal_ms && ms>=goal_ms+6000u && !stopped) {
                engine_set_generative(false,-1);stopped=true;
            }
            /* Queries reap tickets/history, so they are Main work too.
             * Keep them on the control grid instead of each audio chunk. */
            int sources=engine_active_voices();assert(sources<=3);
            if(sources>max_sources)max_sources=sources;
        }
        uint32_t end=(at/TICK+1)*TICK;if(end>SR*SECONDS)end=SR*SECONDS;
        int n=(int)(end-at);if(n>block)n=block;
        engine_render(pcm,n);
        for(int i=0;i<n*2;++i) {
            int a=pcm[i]<0 ? -pcm[i] : pcm[i];if(a>peak)peak=a;
            u16(out,(uint16_t)pcm[i]);
        }
        at+=(uint32_t)n;
    }
    engine_generative_tick(SECONDS*1000);
    assert(stopped && starts==8 && nl==4 && nu==3 && anchor==1 && max_sources==3);
    assert(lower[0]==50 && lower[1]==57 && lower[2]==59 && lower[3]==62);
    assert(upper[0]==74 && upper[1]==71 && upper[2]==66);
    assert(engine_active_voices()==0 && engine_generative_episode_count()==1);
    assert(engine_nonfinite_samples()==0 && engine_output_limited_samples()==0);
    fprintf(stderr,"COAST ENGINE mode=%s frames=%u starts=%d max_sources=%d final_sources=%d meeting_ms=%u peak=%.8f\n",
        argv[1],at,starts,max_sources,engine_active_voices(),goal_ms,(double)peak/32768);
    fclose(trace);return fclose(out) ? 2 : 0;
}
