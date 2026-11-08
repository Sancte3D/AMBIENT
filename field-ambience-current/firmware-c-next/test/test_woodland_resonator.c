/* The actual modal bank and its real owned source, including fractional Hz. */
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../src/pluck.c"
static float l[512],r[512],sl[512],sr[512];
static void render(int frames) {
    while(frames>0) {
        int n=frames>512 ? 512 : frames;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        pluck_render_mix(l,r,sl,sr,n);frames-=n;
        for(int i=0;i<n;++i)assert(isfinite(l[i]) && isfinite(r[i]));
    }
}
static void init(bool on) {
    pluck_init();shape_init();pluck_set_ambient(true);
    pluck_set_resonator(on);pluck_set_body(true);pluck_set_character(42,true);
}
int main(void) {
    dsp_init();
    assert(sizeof(woodland_resonator_t)<sizeof v[0].buf);
    double worst_cents=0,worst_decay_error=0;
    for(int k=0;k<6;++k)for(int rel=0;rel<3;++rel) {
        float hz=(float[]){60,61.1234f,196,246.94165f,470,8000}[k];
        float scale=(float[]){.25f,1,4}[rel];
        woodland_resonator_t bank;woodland_resonator_prepare(&bank,hz,.22f,.24f,scale);
        assert(bank.modes==(hz==8000 ? 2 : 8));
        /* Isolate the actual fundamental state for pitch/decay measurement,
         * not a second rendering formula. Zero crossings interpolate time. */
        bank.modes=1;double start=hypot(bank.re[0],bank.im[0]);
        double first=-1,last=-1;unsigned crossings=0;float prev=0;
        for(int i=0;i<6*DSP_SAMPLE_RATE_HZ;++i) {
            float y=woodland_resonator_step(&bank);assert(isfinite(y));
            if(i>DSP_SAMPLE_RATE_HZ && prev<=0 && y>0) {
                double t=i-1+(double)(-prev)/(y-prev);
                if(first<0)first=t;
                last=t;++crossings;
            }
            prev=y;
        }
        double measured=(crossings-1)*DSP_SAMPLE_RATE_HZ/(last-first);
        double cents=fabs(1200*log2(measured/hz));
        double t60=-6*log(1000.0)/log(hypot(bank.re[0],bank.im[0])/start);
        double error=fabs(t60/(36*scale)-1);
        assert(cents<.1 && error<.05);
        if(cents>worst_cents)worst_cents=cents;
        if(error>worst_decay_error)worst_decay_error=error;
    }
    for(int k=0;k<3;++k) {
        woodland_resonator_t bank;
        woodland_resonator_prepare(&bank,(float[]){60,196,470}[k],.22f,.24f,4);
        for(int i=0;i<8*DSP_SAMPLE_RATE_HZ;++i) {
            float y=woodland_resonator_step(&bank);assert(isfinite(y) && fabsf(y)<=.2201f);
        }
    }
    woodland_resonator_t invalid;
    woodland_resonator_prepare(&invalid,NAN,.2f,.24f,1);assert(!invalid.modes);
    woodland_resonator_prepare(&invalid,196,INFINITY,.24f,1);assert(!invalid.modes);
    for(int color=0;color<3;++color) {
        init(false);pluck_set_color(color*.5f);assert(pluck_note_on(1,246.94165f,.2f));
        render(8*DSP_SAMPLE_RATE_HZ);pluck_voice_t neighbour=v[0];
        init(false);pluck_set_color(color*.5f);assert(pluck_note_on(1,246.94165f,.2f));
        pluck_set_resonator(true);assert(pluck_note_on(0,196,.2f));
        assert(v[1].resonating && !v[1].body.modes);
        render(8*DSP_SAMPLE_RATE_HZ);
        assert(!memcmp(&neighbour,&v[0],sizeof neighbour));
        assert(v[1].active && v[1].widx==0 && !v[1].y_prev && !v[1].y_prev2);
        pluck_voice_t held=v[1];pluck_set_resonator(false);pluck_set_body(false);
        assert(!memcmp(&held,&v[1],sizeof held));
        assert(!pluck_note_on(0,196,.2f) && !pluck_note_on(2,196,.2f));
        assert(!pluck_note_on(2,NAN,.2f) && !pluck_note_on(2,59,.2f));
        assert(!memcmp(&held,&v[1],sizeof held));
        pluck_quiet_source(0,441);render(441);assert(!v[1].active && v[0].active);
        pluck_set_resonator(true);assert(pluck_note_on(0,196,.2f));
        assert(!v[1].resonator.im[0] && !v[1].resonator.im[7]);
        pluck_note_off(0);assert(!v[1].active); /* unrendered cancellation */
    }
    init(true);assert(pluck_note_on(0,196,.2f));
    render(20*DSP_SAMPLE_RATE_HZ);assert(v[0].active); /* long natural hold */
    unsigned n=20u*DSP_SAMPLE_RATE_HZ;
    while(v[0].active && n<50u*DSP_SAMPLE_RATE_HZ) {render(1);++n;}
    assert(!v[0].active);
    printf("WOODLAND resonator: fractional pitch %.6f cents, decay error %.6f, harmonic/peak bounds, 20s held ring, natural retirement %u frames, no running delay, unchanged neighbour, admission, snapshots, cancellation and reuse pass\n",worst_cents,worst_decay_error,n);
}
