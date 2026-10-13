/* SD35: simultaneous controls on a physically prepared/held/releasing source. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,FRAMES=SR*6 };
static int16_t reference[2*FRAMES],candidate[2*FRAMES],buffer[1024];
static int ons,offs;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(owner==0 && hz==220);(void)velocity;if(on)++ons;else ++offs;
}
static void targets(float x,bool all) {
    int before=ons,off_before=offs;
    engine_set_activity(x);engine_set_attack(x);engine_set_release(x);
    if(all) {engine_set_color(x);engine_set_room(x);engine_set_nature(x);engine_set_master_volume(x);}
    assert(ons==before && offs==off_before);
}
static void capture(int world,float x,int when,int block,int16_t *out,bool invalid,bool retired) {
    engine_init();engine_set_world(world);targets(.5f,true);engine_set_nature(.5f);
    /* settle room/volume in silence; Nature is enabled only at the note */
    engine_set_nature(0);
    for(int i=0;i<SR;) {int n=SR-i;if(n>512)n=512;engine_render(buffer,n);i+=n;}
    ons=offs=0;engine_set_note_hook(hook);engine_set_nature(.5f);
    if(when<0)targets(x,true);
    assert(engine_try_note_on(0,220,.7f));
    for(int at=0;at<FRAMES;) {
        if(at==when)targets(x,false);
        if(at==3*SR)engine_note_off(0);
        if(invalid) {targets(NAN,true);targets(INFINITY,true);targets(-INFINITY,true);}
        if(retired) {
            engine_set_drive(1);engine_set_resonance(1);engine_set_age(1);engine_set_echo(1);
            engine_set_blur(1);engine_set_shimmer(1);engine_set_sweep(1);engine_set_texture(1);
        }
        int edge=(at/SR+1)*SR,n=block;if(n>edge-at)n=edge-at;
        engine_render(out+2*at,n);at+=n;assert(engine_active_voices()<=1);
    }
    assert(ons==1 && offs==1 && !engine_nonfinite_samples() && !engine_output_limited_samples());
}
int main(void) {
    const int when[]={0,SR,4*SR};int livecases=0,blocks=0,clamps=0,invariants=0;
    for(int world=0;world<3;++world) {
        capture(world,.5f,-1,512,reference,false,false);
        for(int t=0;t<3;++t)for(int v=0;v<2;++v) {
            capture(world,(float)v,when[t],64,candidate,false,false);
            assert(!memcmp(reference,candidate,sizeof reference));++livecases;
        }
        for(int v=0;v<2;++v) {
            capture(world,(float)v,-1,512,reference,false,false);
            capture(world,(float)v,-1,64,candidate,false,false);
            assert(!memcmp(reference,candidate,sizeof reference));++blocks;
            capture(world,v ? 2 : -1,-1,64,candidate,false,false);
            assert(!memcmp(reference,candidate,sizeof reference));++clamps;
            capture(world,(float)v,-1,64,candidate,true,true);
            assert(!memcmp(reference,candidate,sizeof reference));++invariants;
        }
    }
    printf("COMBINATION OWNER PASS: %d simultaneous Activity/Attack/Release edits on prepared/held/releasing notes "
        "preserve6s complete PCM; %d all-control endpoint64/512 cases, %d clamped cases, "
        "%d invalid/retired-API invariance cases; setters start/stop no note\n",livecases,blocks,clamps,invariants);
}
