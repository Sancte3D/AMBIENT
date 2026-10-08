/* Musical direction sketch, not the autonomous Product generator.
 * Real unchanged Product Bowed/Room DSP; explicit composed score.
 * A shorter direct SHAPE release is diagnostic and not a new product default. */
#include "bowed.h"
#include "ambient_room.h"
#include "shape.h"
#include "dsp.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
enum { SR=44100, SECONDS=27, MAX_BLOCK=512 };
typedef struct { float seconds; int on,owner,midi; float velocity,attack; } event_t;
static const event_t score[]={
    {0.0f,1,0,50,.62f,.52f}, {0.6f,1,1,57,.48f,.35f},
    {1.2f,1,2,66,.54f,.45f}, /* D major: D3 A3 F#4 */
    {4.0f,0,0,50,0,0}, {4.5f,0,1,57,0,0},
    {6.8f,1,0,54,.48f,.50f}, {7.3f,1,1,59,.58f,.45f},
    {8.0f,0,2,66,0,0}, /* retain F# while bass/register move */
    {10.0f,0,0,54,0,0}, {10.7f,1,2,62,.58f,.45f}, /* B minor/F# */
    {12.7f,1,0,55,.55f,.52f}, /* G major: G3 B3 D4 */
    {14.0f,0,0,55,0,0}, {14.4f,0,1,59,0,0},
    {16.7f,1,0,57,.54f,.50f}, {17.1f,1,1,64,.40f,.35f}, /* A sus4 */
    {20.0f,0,2,62,0,0},
    {22.7f,1,2,61,.54f,.42f}, /* A major: D4 resolves to C#4 */
    {23.5f,0,0,57,0,0}, {23.8f,0,1,64,0,0}, {24.0f,0,2,61,0,0}
};
static void u16(FILE *f,uint16_t x){fputc(x&255,f);fputc(x>>8,f);}
static void u32(FILE *f,uint32_t x){u16(f,x&65535);u16(f,x>>16);}
static void header(FILE *f){uint32_t n=SR*SECONDS*4;
    fwrite("RIFF",1,4,f);u32(f,36+n);fwrite("WAVEfmt ",1,8,f);u32(f,16);
    u16(f,1);u16(f,2);u32(f,SR);u32(f,SR*4);u16(f,4);u16(f,16);
    fwrite("data",1,4,f);u32(f,n);
}
int main(int argc,char **argv){
    if(argc!=4)return 2;
    bool room=!strcmp(argv[1],"room");
    if(!room && strcmp(argv[1],"dry"))return 2;
    int block=atoi(argv[2]);if(block<1 || block>MAX_BLOCK)return 2;
    FILE *out=fopen(argv[3],"wb");if(!out)return 2;header(out);
    char path[1024];if(snprintf(path,sizeof path,"%s.events.csv",argv[3])>=(int)sizeof path)return 2;
    FILE *trace=fopen(path,"w");if(!trace)return 2;
    fprintf(trace,"frame,on,owner,midi,velocity,attack,direct_release\n");
    dsp_init();shape_init();bowed_init();ambient_room_init();
    ambient_room_set(room ? .24f : 0);ambient_room_enable(room);
    float l[MAX_BLOCK],r[MAX_BLOCK],sl[MAX_BLOCK],sr[MAX_BLOCK];
    /* Settle the existing room controls with silence, before the score. */
    for(int k=0;k<16;++k){
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        ambient_room_process(l,r,sl,sr,MAX_BLOCK);
    }
    size_t event=0,events=sizeof score/sizeof score[0];
    uint32_t frame=0;int starts=0,max_sources=0;float dc_l=0,dc_r=0,gate=0,peak=0;
    while(frame<SR*SECONDS){
        while(event<events && frame==(uint32_t)lroundf(score[event].seconds*SR)){
            const event_t *e=&score[event++];
            if(e->on){
                shape_set_attack(e->attack);shape_set_release(0);
                assert(bowed_try_note_on(e->owner,dsp_midi_to_hz((float)e->midi),.5f*e->velocity));
                ++starts;
            } else bowed_note_off(e->owner);
            fprintf(trace,"%u,%d,%d,%d,%.3f,%.3f,0\n",frame,e->on,e->owner,e->midi,e->velocity,e->attack);
        }
        int sources=bowed_active_count();if(sources>max_sources)max_sources=sources;
        assert(sources<=3);
        uint32_t boundary=event<events ? (uint32_t)lroundf(score[event].seconds*SR) : SR*SECONDS;
        int n=(int)(boundary-frame);if(n>block)n=block;assert(n>0);
        memset(l,0,n*sizeof(float));memset(r,0,n*sizeof(float));
        memset(sl,0,n*sizeof(float));memset(sr,0,n*sizeof(float));
        bowed_render_mix(l,r,sl,sr,n,.35f);
        ambient_room_process(l,r,sl,sr,n);
        for(int i=0;i<n;++i){
            dc_l+=.00228f*(l[i]-dc_l);dc_r+=.00228f*(r[i]-dc_r);
            gate=fminf(1,gate+1.0f/(.040f*SR));
            float x=(l[i]-dc_l)*.6f*gate,y=(r[i]-dc_r)*.6f*gate;
            assert(isfinite(x)&&isfinite(y)&&fabsf(x)<1&&fabsf(y)<1);
            peak=fmaxf(peak,fmaxf(fabsf(x),fabsf(y)));
            u16(out,(uint16_t)(int16_t)lroundf(x*32767));
            u16(out,(uint16_t)(int16_t)lroundf(y*32767));
        }
        frame+=(uint32_t)n;
    }
    assert(event==events && starts==10 && max_sources==3 && bowed_active_count()==0);
    fprintf(stderr,"ENSEMBLE mode=%s frames=%u starts=%d max_sources=%d final_sources=%d peak=%.8f\n",
        argv[1],frame,starts,max_sources,bowed_active_count(),(double)peak);
    fclose(trace);return fclose(out) ? 2 : 0;
}
