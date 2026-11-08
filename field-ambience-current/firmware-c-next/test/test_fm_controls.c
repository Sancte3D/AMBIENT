/* Rendered FM control regressions; no internal state inspection. */
#include "v2/synth_engine.h"
#include "dsp.h"
#include "shape.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const synth_engine_t engine_fm_glass;
#define SR 44100
#define N 8820
static float capture[N], l[256],r[256],sl[256],sr[256];
static void render(int frames, int save) {
    for(int at=0;at<frames;at+=256) {
        int n=frames-at<256?frames-at:256;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        engine_fm_glass.render_mix(l,r,sl,sr,n);
        for(int i=0;i<n;++i) {
            assert(isfinite(l[i]) && fabsf(l[i])<1.2f);
            if(save) capture[at+i]=l[i];
        }
    }
}
static double power(double hz) {
    double re=0,im=0;
    for(int i=0;i<N;++i) {
        double w=.5-.5*cos(6.283185307179586*i/(N-1));
        double a=6.283185307179586*hz*i/SR;
        re+=capture[i]*w*cos(a);im+=capture[i]*w*sin(a);
    }
    return re*re+im*im;
}
static double third_ratio(void) {
    render(3*SR,0);render(N,1);
    double f=dsp_midi_to_hz(60),fund=power(f);
    assert(fund>1);
    return power(3*f)/fund;
}
int main(void) {
    dsp_init();shape_init();engine_fm_glass.init();
    engine_fm_glass.set_param(SP_A,0);
    engine_fm_glass.set_param(SP_B,.2f); /* integer ratio 2 */
    engine_fm_glass.set_param(SP_F,1);
    engine_fm_glass.note_on(60,.9f);
    double low=third_ratio();
    printf("FM minimum Index / maximum Body: third/fundamental %.6f\n",low);
    assert(low<.025); /* Body must not bypass minimum FM Index. */
    engine_fm_glass.set_param(SP_A,.25f); /* same still-held note */
    double raised=third_ratio();
    printf("FM live Index: third/fundamental %.6f\n",raised);
    assert(raised>low*3 && raised<.6);
    engine_fm_glass.set_param(SP_F,0);
    double empty=third_ratio();
    assert(empty<low*.1); /* Body zero approaches a sine, without reattack. */
    engine_fm_glass.note_off();render(4*SR,0);render(N,1);
    double peak=0;for(int i=0;i<N;++i) if(fabs(capture[i])>peak)peak=fabs(capture[i]);
    assert(peak<.0001);
    puts("FM control regression PASS: bounded, live Index, Body coupling, release");
    return 0;
}
