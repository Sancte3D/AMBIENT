/* Real dry PCM: register consistency, rounded onset, release and limits. */
#include "v2/synth_engine.h"
#include "synth_controls.h"
#include "dsp.h"
#include "shape.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
extern const synth_engine_t engine_acid;
#define SR 44100
static float x[SR],l[256],r[256],sl[256],sr[256];
static void render(int frames,int save) {
    for(int at=0;at<frames;at+=256) {
        int n=frames-at<256?frames-at:256;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        engine_acid.render_mix(l,r,sl,sr,n);
        for(int i=0;i<n;++i) {
            assert(isfinite(l[i]) && fabsf(l[i])<1.2f);
            if(save)x[at+i]=l[i];
        }
    }
}
static double energy(int a,int b) {
    double e=0;for(int i=a;i<b;++i)e+=(double)x[i]*x[i];return e/(b-a);
}
static double partial_power(double hz) {
    double re=0,im=0;
    for(int i=0;i<SR;++i) {
        double w=.5-.5*cos(6.283185307179586*i/(SR-1));
        double ph=6.283185307179586*hz*i/SR;
        re+=x[i]*w*cos(ph);im+=x[i]*w*sin(ph);
    }
    return re*re+im*im;
}
static void setup(void) {
    engine_acid.init();
    for(int p=0;p<6;++p)engine_acid.set_param((synth_param_t)p,synth_control_defaults[0][p]/100.0f);
}
int main(void) {
    dsp_init();shape_init();
    const int notes[]={48,55,60,67,72};
    for(int v=0;v<3;++v) {
        double lo=1e9,hi=0;
        for(int n=0;n<5;++n) {
            setup();engine_acid.note_on((float)notes[n],.2f+v*.3f);
            render(SR,1);
            assert(energy(0,441)<energy(4410,8820)*.1); /* first 10 ms are gentle */
            render(SR,1);double body=energy(0,17640);
            assert(body>1e-5);
            if(notes[n]==60) {
                double hz=dsp_midi_to_hz(60);
                double octave=partial_power(2*hz)/partial_power(hz);
                printf("Dusk octave/root v%d: %.1f dB\n",v,10*log10(octave));
                assert(octave<.16); /* root dominates; reject cancellation/nasal default */
            }
            if(body<lo)lo=body;
            if(body>hi)hi=body;
        }
        printf("Dusk native body spread v%d: %.2f dB\n",v,10*log10(hi/lo));
        assert(hi/lo<2.0); /* <3 dB power spread across C3..C5 */
    }
    double held=energy(0,17640);
    engine_acid.note_off();render(SR,1);
    assert(energy(17640,22050)>held*.005); /* soft but audible release */
    render(3*SR,0);render(SR,1);assert(energy(0,SR)<1e-8);
    /* Relevant interactions: all-dark/all-bright and maximum global colour. */
    for(int note=36;note<=84;note+=24)for(int high=0;high<2;++high) {
        setup();for(int p=0;p<6;++p)engine_acid.set_param((synth_param_t)p,(float)high);
        engine_acid.set_colour(high?8.0f:.35f,(float)high);
        engine_acid.note_on((float)note,1);render(2*SR,0);
        engine_acid.note_off();render(4*SR,0);
    }
    puts("Dusk role regression PASS: register, onset, release and limits");
}
