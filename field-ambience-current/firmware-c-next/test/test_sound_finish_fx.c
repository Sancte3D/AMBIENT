#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../src/ambient_room.c"
static float l[512],r[512],sl[512],sr[512];
static float observed_peak,last;
static void run(unsigned frames,bool tone) {
    static unsigned phase;
    while(frames) {
        int n=frames>512 ? 512 : (int)frames;
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        if(tone)for(int i=0;i<n;++i)sl[i]=sr[i]=.1f*dsp_sin((float)(phase++%44100)*196/44100);
        ambient_room_process(l,r,sl,sr,n);last=0;
        for(int i=0;i<n;++i) {
            assert(isfinite(l[i]) && isfinite(r[i]));
            float a=fmaxf(fabsf(l[i]),fabsf(r[i]));if(a>observed_peak)observed_peak=a;if(a>last)last=a;
        }
        frames-=(unsigned)n;
    }
}
static float partition_a[2*3*44100],partition_b[2*3*44100];
static void partition(int cap,float *out) {
    unsigned at_frame=0;const unsigned edits[4]={1003,7319,11617,30001};int edit=0;
    ambient_room_init();ambient_room_set(1);
    while(at_frame<3u*44100) {
        if(edit<4 && at_frame==edits[edit]) {
            ambient_room_finish_set(.6f,.4f,false,.1f,.1f,.2f,(edit&1)!=0);++edit;
        }
        unsigned n=3u*44100-at_frame;if(n>(unsigned)cap)n=(unsigned)cap;
        if(edit<4 && n>edits[edit]-at_frame)n=edits[edit]-at_frame;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        for(unsigned i=0;i<n;++i)sl[i]=sr[i]=.1f*dsp_sin((float)((at_frame+i)%44100)*196/44100);
        ambient_room_process(l,r,sl,sr,(int)n);
        for(unsigned i=0;i<n;++i) {out[2*(at_frame+i)]=l[i];out[2*(at_frame+i)+1]=r[i];}
        at_frame+=n;
    }
}
int main(void) {
    dsp_init();assert(ambient_room_storage_bytes()<288u*1024);
    partition(37,partition_a);partition(512,partition_b);
    assert(!memcmp(partition_a,partition_b,sizeof partition_a));
    for(int plate=0;plate<2;++plate)for(int macro=0;macro<2;++macro) {
        ambient_room_init();ambient_room_set(1);observed_peak=last=0;
        ambient_room_finish_set((float)macro,(float)macro,false,(float)macro,(float)macro,(float)macro,plate);
        run(44100,false);run(2*44100,true);run(14*44100,false);
        assert(last<1e-5f && observed_peak>.001f && observed_peak<.6f);
        printf("Effects model %d macro %d peak %.6f residual14s %.9f; actual stored bytes %zu\n",plate,macro,observed_peak,last,ambient_room_storage_bytes());
    }
    ambient_room_init();ambient_room_finish_set(0,1,false,0,0,0,true);run(3*44100,true);
    ambient_room_finish_set(0,1,true,0,0,0,true);run(20*44100,false);
    assert(last>1e-4f && ambient_room_retains_history());
    ambient_room_enable(false);run(44100,false);
    assert(last==0 && sf.cold && !ambient_room_retains_history());
    ambient_room_enable(true);run(44100,false);assert(last==0); /* no revival */
    uint32_t before[5];for(int i=0;i<5;++i)before[i]=atomic_load(&sf_target[i]);
    ambient_room_finish_set(NAN,1,true,1,1,1,false);
    for(int i=0;i<5;++i)assert(before[i]==atomic_load(&sf_target[i]));
    ambient_room_clear();run(1000,false);assert(last==0);
    puts("Effects: finite stereo, both rooms, max macros, bounded real tail, held Freeze, Dry no-revival, rejected targets and Clear pass");
}
