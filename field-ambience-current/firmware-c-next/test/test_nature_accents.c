/* Real, sample-by-sample Nature. The test hook only observes event starts;
 * it cannot select, accelerate or replace the weather being rendered. */
#include "nature.h"
#include "dsp.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
enum { SR=44100,BLOCK=512,SECONDS=120 };
static unsigned observed_world,events[3][4],wind_types;
static uint32_t shortest=UINT32_MAX,longest;
static double thunder_energy;
static float thunder_peak;
static float l[BLOCK],r[BLOCK];
static float reference[2*SR*SECONDS];
void nature_accent_event(unsigned kind,unsigned profile,uint32_t frames) {
    assert(kind<4);++events[observed_world][kind];
    if(kind==0) {
        assert(profile<3 && frames>SR*4 && frames<SR*26);
        wind_types|=1u<<profile;
        if(frames<shortest)shortest=frames;
        if(frames>longest)longest=frames;
    }
    if(kind==1)assert(observed_world==0 && frames>SR*8 && frames<SR*25);
    if(kind==2)assert(observed_world==1 && frames>=SR*15 && frames<SR*40);
    if(kind==3)assert(observed_world==1 && frames>SR*32 && frames<SR*83);
}
void nature_accent_thunder_sample(float x) {
    assert(observed_world==1 && isfinite(x) && fabsf(x)<.0007f);
    thunder_energy+=(double)x*x;thunder_peak=fmaxf(thunder_peak,fabsf(x));
}
static uint64_t hash(uint64_t h,float x) {uint32_t u;memcpy(&u,&x,4);return (h^u)*UINT64_C(1099511628211);}
static uint64_t capture(int world,uint32_t seed,int cap,int seconds,int report) {
    observed_world=(unsigned)world;nature_init();nature_set_world(world);nature_set_seed(seed);nature_set_amount(1);
    uint64_t h=UINT64_C(1469598103934665603);
    double energy=0,mono=0,mean=0,window=0,minimum=1,maximum=0;float peak=0;int samples=0;
    for(int frame=0;frame<seconds*SR;) {
        int n=seconds*SR-frame;if(n>cap)n=cap;
        memset(l,0,sizeof l);memset(r,0,sizeof r);nature_render(l,r,n);
        for(int i=0;i<n;++i) {
            float L=l[i],R=r[i];assert(isfinite(L)&&isfinite(R));
            int index=2*(frame+i);
            if(report && seed==1234) {reference[index]=L;reference[index+1]=R;}
            if(cap==37) {
                assert(!memcmp(&reference[index],&L,sizeof L));
                assert(!memcmp(&reference[index+1],&R,sizeof R));
            }
            assert(fabsf(L)<.004f && fabsf(R)<.004f);
            h=hash(hash(h,L),R);double e=(double)L*L+(double)R*R;
            energy+=e;window+=e;mean+=L+R;mono+=(double)(L+R)*(L+R)*.5;
            peak=fmaxf(peak,fmaxf(fabsf(L),fabsf(R)));
            if(++samples==SR) {
                if(frame+i>=SR*5) {if(window<minimum)minimum=window;if(window>maximum)maximum=window;}
                window=0;samples=0;
            }
        }frame+=n;
    }
    assert(energy>1e-6 && mono/energy>.65 && fabs(mean/(2*seconds*SR))<3e-5);
    if(seconds>=60)assert(maximum>minimum*9);
    if(report)printf("Nature world=%d seed=%u seconds=%d peak=%.9f rms=%.9f mono=%.6f hash=%016" PRIx64 "\n",
                     world,seed,seconds,(double)peak,sqrt(energy/(2*seconds*SR)),mono/energy,h);
    return h;
}
int main(void) {
    dsp_init();
    for(int world=0;world<3;++world) {
        uint64_t a=capture(world,1234,512,SECONDS,1);
        assert(a==capture(world,1234,37,SECONDS,0));
        assert(a!=capture(world,42,512,SECONDS,1));
        assert(events[world][0]>0);
        if(world==0)assert(events[world][1]>0 && !events[world][2] && !events[world][3]);
        if(world==1)assert(events[world][2]>0 && events[world][3]>0 && !events[world][1]);
        if(world==2)assert(!events[world][1] && !events[world][2] && !events[world][3]);
    }
    assert(wind_types==7 && longest>shortest+SR*3 && thunder_energy>1e-5 && thunder_peak>1e-6f);
    /* Cold idle neither emits nor advances a hidden weather timeline. */
    observed_world=0;nature_init();nature_set_seed(1234);
    for(int j=0;j<1000;++j) {
        memset(l,0,sizeof l);memset(r,0,sizeof r);nature_render(l,r,BLOCK);
        for(int i=0;i<BLOCK;++i)assert(l[i]==0 && r[i]==0);
    }
    nature_set_amount(1);uint64_t h=UINT64_C(1469598103934665603);
    for(int f=0;f<SR*8;) {
        int n=SR*8-f;if(n>BLOCK)n=BLOCK;memset(l,0,sizeof l);memset(r,0,sizeof r);nature_render(l,r,n);
        for(int i=0;i<n;++i)h=hash(hash(h,l[i]),r[i]);
        f+=n;
    }
    assert(h==capture(0,1234,512,8,0));
    nature_set_amount(0);
    for(int f=0;f<SR*31;f+=BLOCK) {memset(l,0,sizeof l);memset(r,0,sizeof r);nature_render(l,r,BLOCK);}
    for(int i=0;i<BLOCK;++i)assert(l[i]==0 && r[i]==0);
    nature_clear();nature_render(l,r,BLOCK);
    for(int i=0;i<BLOCK;++i)assert(l[i]==0 && r[i]==0);
    printf("Nature accents PASS: 18 minutes actual float weather, three wind profiles, variable intervals %u..%u samples, sea/rain/distant thunder (actual peak %.9f), bounded level, seed/37-512/cold-idle/off/Clear\n",shortest,longest,(double)thunder_peak);
}
