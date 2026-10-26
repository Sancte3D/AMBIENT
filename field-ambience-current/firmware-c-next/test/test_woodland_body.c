/* Real private string states plus spectral/stability properties of its body. */
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
        for(int i=0;i<n;++i) assert(isfinite(l[i]) && isfinite(r[i]));
    }
}
static void init(bool body) {
    pluck_init();shape_init();pluck_set_ambient(true);
    pluck_set_character(42,true);pluck_set_body(body);
}
static double response(float fundamental,float ratio) {
    woodland_body_t b;woodland_body_prepare(&b,fundamental,true);
    double input=0,output=0;
    for(int i=0;i<2*DSP_SAMPLE_RATE_HZ;++i) {
        float x=.2f*sinf(6.283185307179586f*(fundamental*ratio)*(float)i/DSP_SAMPLE_RATE_HZ);
        float y=woodland_body_step(&b,x);assert(isfinite(y) && fabsf(y)<.24f);
        if(i>DSP_SAMPLE_RATE_HZ) {input+=(double)x*x;output+=(double)y*y;}
    }
    return sqrt(output/input);
}
int main(void) {
    dsp_init();
    for(int f=0;f<3;++f) {
        float hz=(float[]){60,196,470}[f];
        double fundamental=response(hz,1),second=response(hz,2),third=response(hz,3);
        assert(fundamental>.99 && fundamental<1.01);
        assert(second>1.09 && second<1.12 && third>1.05 && third<1.08);
    }
    woodland_body_t b;
    woodland_body_prepare(&b,8000,true);assert(b.modes==1);
    woodland_body_prepare(&b,10000,true);assert(b.modes==0);
    woodland_body_prepare(&b,NAN,true);assert(b.modes==0);
    woodland_body_prepare(&b,196,false);woodland_body_t bypass=b;
    assert(woodland_body_step(&b,.125f)==.125f && !memcmp(&b,&bypass,sizeof b));
    woodland_body_prepare(&b,60,true);
    (void)woodland_body_step(&b,1);
    for(int i=0;i<2*DSP_SAMPLE_RATE_HZ;++i) {
        float y=woodland_body_step(&b,0);assert(isfinite(y) && fabsf(y)<.02f);
    }
    assert(fabsf(b.y1[0])+fabsf(b.y1[1])<1e-10f);
    for(int color=0;color<3;++color) {
        init(false);pluck_set_color(.5f*color);
        assert(pluck_note_on(0,196,.2f));assert(pluck_note_on(1,246.94165f,.2f));
        render(8*DSP_SAMPLE_RATE_HZ);pluck_voice_t old=v[0],neighbour=v[1];
        init(true);pluck_set_color(.5f*color);assert(pluck_note_on(0,196,.2f));
        pluck_set_body(false);assert(pluck_note_on(1,246.94165f,.2f));
        render(8*DSP_SAMPLE_RATE_HZ);
        assert(!memcmp(&neighbour,&v[1],sizeof neighbour));
        assert(!memcmp(old.buf,v[0].buf,sizeof old.buf));
        assert(old.N==v[0].N && old.rho==v[0].rho && old.damp==v[0].damp);
        assert(old.env==v[0].env && old.active==v[0].active);
        assert(old.attack_phase==v[0].attack_phase && v[0].body.modes==3);
        pluck_voice_t held=v[0];pluck_set_body(true);pluck_set_body(false);
        assert(!memcmp(&held,&v[0],sizeof held));
        assert(!pluck_note_on(0,196,.2f));assert(!pluck_note_on(2,NAN,.2f));
        assert(!memcmp(&held,&v[0],sizeof held));
        pluck_note_off(0);render(3*DSP_SAMPLE_RATE_HZ);assert(!v[0].active);
        pluck_set_body(true);assert(pluck_note_on(0,196,.2f));
        assert(!v[0].body.x1 && !v[0].body.x2);
        for(int m=0;m<2;++m)assert(!v[0].body.y1[m] && !v[0].body.y2[m]);
        pluck_note_off(0);assert(!v[0].active); /* cancelled preparation is silent */
    }
    /* Natural source retirement occurs at the exact same sample. */
    unsigned retired[2];
    for(int on=0;on<2;++on) {
        init(on!=0);assert(pluck_note_on(0,196,.2f));unsigned n=0;
        while(v[0].active && n<120u*DSP_SAMPLE_RATE_HZ) {render(1);++n;}
        assert(!v[0].active);retired[on]=n;
    }
    assert(retired[0]==retired[1]);
    printf("WOODLAND body: harmonic response, stable silent tail, exact bypass, unchanged neighbour/string/retirement (%u frames), future snapshots, reuse and cancelled starts pass\n",retired[0]);
}
