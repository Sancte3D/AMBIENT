/* Product long-string Color contracts on actual float PCM. */
#include "pluck.h"
#include "dsp.h"
#include "shape.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100, FRAMES=SR*4 };
static float reference[4][FRAMES], candidate[4][FRAMES];
static void capture(float x[4][FRAMES],float color,int block,int live,int ambient) {
    memset(x,0,sizeof reference);pluck_init();pluck_set_ambient(ambient);
    pluck_set_damp(.025f-.020f*.5f);pluck_set_color(color);
    assert(pluck_note_on(6,220,.4f));assert(!pluck_note_on(6,220,.4f));
    for(int at=0;at<FRAMES;) {
        if(live && at==SR)pluck_set_color(0);
        if(live && at==2*SR)pluck_set_color(1);
        if(at==3*SR)pluck_note_off(6);
        int n=block,edge=(at/SR+1)*SR;if(n>edge-at)n=edge-at;
        pluck_render_mix(x[0]+at,x[1]+at,x[2]+at,x[3]+at,n);at+=n;
    }
    for(int k=0;k<4;++k)for(int i=0;i<FRAMES;++i)assert(isfinite(x[k][i]) && fabsf(x[k][i])<1);
}
static double mode(float x[4][FRAMES],double frequency) {
    double re=0,im=0;
    for(int n=SR;n<2*SR;++n) {
        double phase=6.283185307179586*frequency*n/SR;
        double window=.5-.5*cos(6.283185307179586*(n-SR)/(SR-1));
        double sample=(x[0][n]+x[1][n])*window;
        re+=sample*cos(phase);im+=sample*sin(phase);
    }
    return re*re+im*im;
}
static int natural_end(float color) {
    pluck_init();pluck_set_ambient(true);pluck_set_damp(.025f-.020f*.5f);pluck_set_color(color);
    assert(pluck_note_on(6,110,.4f));
    int at=0;float x[4][512];
    while(pluck_active_count() && at<SR*48) {
        memset(x,0,sizeof x);pluck_render_mix(x[0],x[1],x[2],x[3],512);at+=512;
    }
    assert(!pluck_active_count() && at>SR*10);return at;
}
int main(void) {
    dsp_init();shape_set_attack(.5f);shape_set_release(.5f);
    for(int ambient=0;ambient<2;++ambient) {
        capture(reference,.5f,512,0,ambient);
        const float invalid[]={NAN,INFINITY,-INFINITY};
        for(unsigned i=0;i<sizeof invalid/sizeof *invalid;++i) {
            capture(candidate,invalid[i],64,0,ambient);assert(!memcmp(reference,candidate,sizeof reference));
        }
        for(int live=0;live<2;++live)for(int i=0;i<3;++i) {
            float c=.5f*i;capture(reference,c,512,live,ambient);capture(candidate,c,64,live,ambient);
            assert(!memcmp(reference,candidate,sizeof reference));
        }
    }
    for(int i=0;i<2;++i) {
        capture(reference,(float)i,512,0,1);capture(candidate,i ? 2 : -1,512,0,1);
        assert(!memcmp(reference,candidate,sizeof reference));
    }
    capture(reference,0,512,0,0);capture(candidate,1,512,0,0);
    assert(!memcmp(reference,candidate,sizeof reference)); /* historical short source */
    capture(reference,0,512,0,1);capture(candidate,1,512,0,1);
    double dark=mode(reference,440)/mode(reference,220),bright=mode(candidate,440)/mode(candidate,220);
    assert(bright>dark*1.5); /* spectral ratio, not loudness or phase alone */
    int ends[3];for(int i=0;i<3;++i)ends[i]=natural_end(.5f*i);
    assert(ends[0]==ends[1] && ends[1]==ends[2]);
    printf("pluck Color PASS: full float64/512, invalid/clamped inputs, historical short source, "
        "upper/fundamental %.5f -> %.5f, same natural retirement %.3fs\n",dark,bright,ends[1]/(double)SR);
}
