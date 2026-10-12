/* Actual Product PCM: prepared, held and releasing notes keep captured Release. */
#include "engine.h"
#include "engine_product.h"
#include "dsp.h"
#include "shape.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,FRAMES=SR*6 };
static int16_t reference[FRAMES*2],candidate[FRAMES*2],buffer[1024];
static int ons,offs;
static void hook(int on,uint8_t owner,float hz,float velocity) {
    assert(owner==0 && hz==220);(void)velocity;if(on)++ons;else ++offs;
}
static void capture(int world,float target,int when,int block,int16_t *out) {
    engine_init();engine_set_world(world);engine_set_attack(.5f);engine_set_release(.5f);
    engine_set_color(.5f);engine_set_room(.5f);engine_set_nature(0);engine_set_master_volume(.6f);
    for(int i=0;i<SR;) {int n=SR-i;if(n>512)n=512;engine_render(buffer,n);i+=n;}
    ons=offs=0;engine_set_note_hook(hook);
    if(when<0)engine_set_release(target);
    assert(engine_try_note_on(0,220,.7f));
    for(int at=0;at<FRAMES;) {
        if(at==when) {int before=ons;engine_set_release(target);assert(ons==before);}
        if(at==3*SR)engine_note_off(0);
        int edge=(at/SR+1)*SR,n=block;if(n>edge-at)n=edge-at;
        engine_render(out+2*at,n);at+=n;assert(engine_active_voices()<=1);
    }
    assert(ons==1 && offs==1 && shape_attack_scale()==1);
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
int main(void) {
    int livecases=0,clamps=0,blocks=0;const int when[]={0,SR,4*SR};
    for(int w=0;w<3;++w) {
        capture(w,.5f,-1,512,reference);
        for(int i=0;i<3;++i)for(int v=0;v<2;++v) {
            capture(w,(float)v,when[i],64,candidate);assert(!memcmp(reference,candidate,sizeof reference));++livecases;
        }
        const float invalid[]={NAN,INFINITY,-INFINITY};
        for(unsigned i=0;i<sizeof invalid/sizeof *invalid;++i) {
            capture(w,invalid[i],-1,64,candidate);assert(!memcmp(reference,candidate,sizeof reference));
        }
        for(int v=0;v<3;++v) {
            capture(w,.5f*v,-1,512,reference);capture(w,.5f*v,-1,64,candidate);
            assert(!memcmp(reference,candidate,sizeof reference));++blocks;
        }
        for(int v=0;v<2;++v) {
            capture(w,(float)v,-1,512,reference);capture(w,v ? 2 : -1,-1,64,candidate);
            assert(!memcmp(reference,candidate,sizeof reference));++clamps;
        }
    }
    printf("RELEASE OWNER PASS: %d prepared/held/releasing edits PCM-exact for6s, %d future-note64/512 cases, "
        "%d clamped cases, invalid inputs ignored, Attack unchanged; setter starts no note\n",livecases,blocks,clamps);
}
