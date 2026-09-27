/* Actual dry PCM: stable onset pitch, soft attack, body and bounded controls. */
#include "v2/synth_engine.h"
#include "dsp.h"
#include "shape.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const synth_engine_t engine_ion_storm;
#define SR 44100
static float x[SR], l[256],r[256],sl[256],sr[256];
static void render(int frames,int save) {
    for(int at=0;at<frames;at+=256) {
        int n=frames-at<256?frames-at:256;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        engine_ion_storm.render_mix(l,r,sl,sr,n);
        for(int i=0;i<n;++i) {
            assert(isfinite(l[i]) && fabsf(l[i])<1.2f);
            if(save)x[at+i]=l[i];
        }
    }
}
static double energy(int a,int b) {
    double e=0;for(int i=a;i<b;++i)e+=(double)x[i]*x[i];return e/(b-a);
}
static double onset_pitch(void) {
    double first=0,last=0;int count=0;
    for(int i=512;i<3500;++i) if(x[i-1]<=0 && x[i]>0) {
        double t=i-1-x[i-1]/(x[i]-x[i-1]);
        if(!count) first=t;
        last=t;++count;
    }
    assert(count>10);
    return SR*(count-1)/(last-first);
}
int main(void) {
    dsp_init();shape_init();
    for(int pitch=0;pitch<2;++pitch) {
        engine_ion_storm.init();
        engine_ion_storm.set_param(SP_B,0); /* unison for the pitch probe */
        engine_ion_storm.set_param(SP_C,0);
        engine_ion_storm.set_param(SP_D,0);
        engine_ion_storm.note_on(pitch?81:69,1);
        render(SR,1);
        double cents=1200*log2(onset_pitch()/(pitch?880:440));
        printf("Storm early onset %d Hz: %.3f cents\n",pitch?880:440,cents);
        fflush(stdout);
        assert(fabs(cents)<3);
        assert(energy(0,441)<energy(3000,4000)*.15);
    }
    double held=energy(30000,40000);
    engine_ion_storm.note_off();render(SR,1);
    assert(energy(20000,22000)>held*.005); /* usable release, not a stab */
    render(4*SR,0);render(SR,1);
    assert(energy(0,SR)<1e-8);
    for(int reg=0;reg<3;++reg)for(int setting=0;setting<3;++setting) {
        engine_ion_storm.init();
        for(int p=0;p<6;++p)engine_ion_storm.set_param((synth_param_t)p,setting*.5f);
        engine_ion_storm.note_on(36+reg*24,.7f);render(SR,0);
    }
    puts("Storm role regression PASS: onset, attack, release, nine range probes");
    return 0;
}
