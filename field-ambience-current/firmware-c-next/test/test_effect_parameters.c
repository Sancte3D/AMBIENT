/* Actual FX module, including read heads, lazy Clear and both stereo paths. */
#include "../src/ambient_room.c"
#include <assert.h>
#include <stdio.h>
static float l[512],r[512],sl[512],sr[512],a[2*6*44100],b[2*6*44100];
static void parameters(unsigned variant) {
    sound_parameters_t p=sound_parameters_default();
    p.delay=.8f;p.grain=.7f;p.drive=.3f;p.phaser=.8f;
    p.delay_left_ms=(variant&1) ? 700 : 40;p.delay_right_ms=(variant&1) ? 113 : 620;
    p.feedback=(variant&1) ? .5f : 0;p.grain_ms=(variant&1) ? 260 : 40;
    p.phaser_hz=(variant&1) ? .5f : .01f;
    assert(ambient_room_finish_apply(&p));
}
static void partition(int cap,float *out) {
    static const unsigned edits[]={1003,1021,4777,7319,11617,30001,88201};
    unsigned edit=0,frame=0;ambient_room_init();ambient_room_set(.7f);
    while(frame<6u*44100) {
        if(edit<7 && frame==edits[edit]) {parameters(edit);++edit;}
        unsigned n=6u*44100-frame;if(n>(unsigned)cap)n=(unsigned)cap;
        if(edit<7 && n>edits[edit]-frame)n=edits[edit]-frame;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        for(unsigned i=0;i<n;++i) {
            sl[i]=.1f*dsp_sin((float)((frame+i)%44100)*196/44100);
            sr[i]=.08f*dsp_sin((float)((frame+i)%44100)*293.6648f/44100);
        }
        ambient_room_process(l,r,sl,sr,(int)n);
        for(unsigned i=0;i<n;++i) {out[2*(frame+i)]=l[i];out[2*(frame+i)+1]=r[i];}
        frame+=n;
    }
}
static void impulse(void) {
    sf_init();sound_parameters_t p=sound_parameters_default();
    p.delay=1;p.grain=p.drive=p.crush=p.phaser=p.feedback=0;p.plate=false;
    p.delay_left_ms=210;p.delay_right_ms=700;assert(sf_apply_parameters(&p));sf_accept();
    int first[2]={-1,-1};
    for(int i=0;i<33000;++i) {
        float left=0,right=0,in=i==0 ? 1 : 0;
        sf_process(&left,&right,&in,&in,1,1,0,true,false);
        if(left!=0 && first[0]<0)first[0]=i;
        if(right!=0 && first[1]<0)first[1]=i;
    }
    assert(first[0]==9261 && first[1]==30870);
    /* The buffer still contains old samples. Clear must hide them without
     * a 256KiB IRQ memset, including short delays after a longer old time. */
    sf_clear();p.delay_left_ms=p.delay_right_ms=40;assert(sf_apply_parameters(&p));sf_accept();
    for(unsigned i=0;i<2u*SF_DELAY;++i) {
        float left=0,right=0,in=0;sf_process(&left,&right,&in,&in,1,1,0,true,false);
        assert(left==0 && right==0);
    }
}
static void codecs(void) {
    sound_parameters_t p=sound_parameters_default(),q,saved;
    p.delay_left_ms=620;p.delay_right_ms=490;p.feedback=.47f;p.grain_ms=240;p.phaser_hz=.19f;
    p.freeze=true;p.plate=false;uint8_t bytes[SOUND_PARAMETERS_BYTES],copy[SOUND_PARAMETERS_BYTES];
    assert(sound_parameters_encode(&p,bytes) && sound_parameters_decode(bytes,&q));
    assert(sound_parameters_encode(&q,copy) && !memcmp(bytes,copy,sizeof bytes));
    assert(q.delay_left_ms==p.delay_left_ms && q.feedback==p.feedback && q.freeze && !q.plate);
    saved=q;
    for(unsigned i=0;i<sizeof bytes;++i) {
        bytes[i]^=1;assert(!sound_parameters_decode(bytes,&q));bytes[i]^=1;
        assert(q.delay_left_ms==saved.delay_left_ms && q.feedback==saved.feedback);
    }
    /* Even correctly checksummed nonfinite/out-of-range packets fail closed. */
    sound_wire_put32(bytes+24,0x7f800000u);sound_wire_put32(bytes+48,sound_wire_crc(bytes,48));
    assert(!sound_parameters_decode(bytes,&q));
    p.delay_left_ms=NAN;assert(!sound_parameters_normalize(&p,&q));
    p=sound_parameters_default();p.feedback=2;p.grain_ms=0;p.phaser_hz=2;
    assert(sound_parameters_normalize(&p,&q) && q.feedback==.5f && q.grain_ms==40 && q.phaser_hz==.5f);
}
static void transaction(void) {
    sf_init();sf_accept();float old[10];memcpy(old,sf_controls,sizeof old);
    unsigned version=atomic_load(&sf_version);atomic_store(&sf_version,version+1);
    for(unsigned i=0;i<10;++i) {
        if(i<5)atomic_store(&sf_target[i],65536);
        else {float v=old[i]*2;uint32_t n;memcpy(&n,&v,4);atomic_store(&sf_detail[i-5],n);}
        atomic_store(&sf_freeze,true);atomic_store(&sf_plate_on,false);sf_accept();
        assert(!memcmp(old,sf_controls,sizeof old) && !sf_is_frozen && sf_accepted_plate);
    }
    atomic_store(&sf_version,version+2);sf_accept();
    for(unsigned i=0;i<10;++i)assert(sf_controls[i]==(i<5 ? 1 : old[i]*2));
    assert(sf_is_frozen && !sf_accepted_plate);
    /* The existing macro setter must retain independent timing/rate values. */
    sound_parameters_t p=sound_parameters_default();p.feedback=.47f;p.grain_ms=240;
    p.delay_left_ms=620;p.phaser_hz=.3f;assert(ambient_room_finish_apply(&p));
    ambient_room_finish_set(.5f,.6f,false,.1f,.2f,.3f,true);
    sound_parameters_t q;ambient_room_finish_get(&q);
    assert(q.delay==.5f && q.feedback==p.feedback && q.grain_ms==p.grain_ms &&
           q.delay_left_ms==p.delay_left_ms && q.phaser_hz==p.phaser_hz);
}
static void longest_tail(void) {
    sf_init();sound_parameters_t p=sound_parameters_default();
    p.delay=p.grain=1;p.delay_left_ms=p.delay_right_ms=700;p.feedback=.5f;p.grain_ms=260;
    p.drive=p.crush=p.phaser=0;assert(sf_apply_parameters(&p));sf_accept();
    float peak=0,last=0;
    for(unsigned i=0;i<16u*44100;++i) {
        float in=i<44100 ? .1f*dsp_sin((float)(i%44100)*196/44100) : 0,left=0,right=0;
        sf_process(&left,&right,&in,&in,1,1,.8f,true,true);
        assert(isfinite(left) && isfinite(right));
        float mag=fmaxf(fabsf(left),fabsf(right));if(mag>peak)peak=mag;
        if(i>=15u*44100 && mag>last)last=mag;
    }
    assert(peak>.01f && peak<.6f && last<1e-5f);
    printf("Longest 700ms/.5 feedback tail: peak %.6f, 14s-after-input residual %.9f\n",peak,last);
}
int main(void) {
    dsp_init();codecs();transaction();impulse();partition(37,a);partition(512,b);
    assert(!memcmp(a,b,sizeof a));float jump=0;
    for(unsigned i=2;i<sizeof a/sizeof *a;++i) {
        assert(isfinite(a[i]));float d=fabsf(a[i]-a[i-2]);if(d>jump)jump=d;
    }
    assert(jump<.025f);longest_tail();
    printf("Effect parameters: exact impulse times, non-Doppler tap fades, grain-length/rate edits, 6s full PCM partitions, max sample step %.6f, codec, lazy Clear and tail bounds pass\n",jump);
}
