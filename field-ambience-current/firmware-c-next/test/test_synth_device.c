/*
 * test_synth_device.c — r19.16 SYNTH mode through the DEVICE path.
 *
 * Registers the real synth_host as the engine's V2 backend (exactly like
 * main_h743 does) and drives everything through the public engine API:
 * engine_set_synth / engine_note_on / engine_render. Verifies:
 *   - without a backend, set_synth(>0) is a no-op (bench/host safety),
 *   - the ambient→V2 switch has NO discontinuity (max inter-sample step
 *     across the whole crossfade stays in normal-audio range),
 *   - every V2 core is playable via a cell note-on and stays bounded,
 *   - note_off decays, switch back to ambient restores the pad path.
 */
#include "engine.h"
#include "dsp.h"
#include "v2/synth_host.h"
#include "ambient_effects.h"
#include "bowed.h"
#include "horn.h"
#include "choir.h"
#include "harmony.h"
#include "composer.h"
#include "tuning.h"
#include "shape.h"
#include "brain.h"
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks = 0;
#define CHECK(c) do { assert(c); ++checks; } while (0)

#define BLK 512

static void be_select  (int id)              { synth_host_select((synth_id_t)id); }
static void be_note_on (int midi, float vel) { synth_host_note_on(midi, vel); }
static void be_note_off(void)                { synth_host_note_off(); }
static void be_panic   (void)                { synth_host_panic(); }
static void be_render  (int16_t *b, int n)   { synth_host_render(b, n); }
static void be_param(int slot, float value) { synth_host_set_param((synth_param_t)slot, value); }
static const engine_synth_backend_t BE = {
    be_select, be_note_on, be_note_off, be_panic, be_render, synth_host_render_mix, be_param,
    synth_host_note_on_hz, synth_host_set_macro, synth_host_retune_hz
};

static int peak_of(const int16_t *b, int frames) {
    int p = 0;
    for (int i = 0; i < frames * 2; ++i) {
        int a = b[i] < 0 ? -b[i] : b[i];
        if (a > p) p = a;
    }
    return p;
}

static double level_after(int blocks) {
    int16_t b[BLK*2]; double energy=0;
    for(int i=0;i<blocks;++i) {
        engine_render(b,BLK);
        if(i>=blocks/2) for(int n=0;n<BLK*2;++n) energy+=(double)b[n]*b[n];
    }
    return sqrt(energy / ((blocks-blocks/2)*BLK*2));
}

static int observed_note, observed_offs;
static void observe_note(int note,float velocity) { (void)velocity; observed_note=note; }
static void observe_off(void) { ++observed_offs; }
static void test_playability(void) {
    engine_synth_backend_t probe=BE;
    probe.note_on_hz=NULL; probe.note_on=observe_note; probe.note_off=observe_off;
    engine_init(); synth_host_init(); engine_set_synth_backend(&probe); engine_set_synth(3);
    observed_offs=0;
    engine_note_on(0,220,0.2f); engine_note_on(1,330,0.4f);
    engine_note_off(0);
    CHECK(observed_note==64 && observed_offs==0); /* releasing A preserves B */
    engine_note_on(0,220,0.2f); engine_note_off(0);
    CHECK(observed_note==64 && observed_offs==0); /* releasing A resumes B */
    engine_note_off(1); CHECK(observed_offs==1);
    engine_note_off(1); CHECK(observed_offs==1); /* duplicate release harmless */
    engine_set_synth_backend(&BE);

    for(int core=1;core<=6;++core) {
        engine_init(); synth_host_init(); engine_set_synth_backend(&BE);
        engine_set_fx_mode(0); engine_set_synth(core); level_after(30);
        engine_note_on(0,220,0.8f); double loud=level_after(100);
        engine_set_master_volume(0); double silent=level_after(200);
        CHECK(loud>10); CHECK(silent<1.0);
        engine_note_on(0,330,0.9f); CHECK(level_after(80)<1.0);
        double dynamics[2];
        for(int v=0;v<2;++v) {
            engine_init(); synth_host_init(); engine_set_synth_backend(&BE);
            engine_set_fx_mode(0); engine_set_synth(core); level_after(30);
            engine_note_on(0,220,v ? 0.8f : 0.1f); dynamics[v]=level_after(50);
        }
        CHECK(dynamics[1]>dynamics[0]*1.4);
    }
    engine_init(); synth_host_init(); engine_set_synth_backend(&BE);
    engine_set_fx_mode(0); engine_set_synth(3); engine_note_on(0,220,0.8f);
    double full=level_after(200);
    /* Reset chorus/oscillator phase: sequential windows of a moving pad
     * aren't a controlled gain comparison. */
    engine_init(); synth_host_init(); engine_set_synth_backend(&BE);
    engine_set_fx_mode(0); engine_set_synth(3); engine_set_master_volume(0.3f);
    engine_note_on(0,220,0.8f); double half=level_after(200);
    CHECK(half/full>0.49 && half/full<0.51);
    engine_set_synth_param(0,0.0f); double dark=level_after(160);
    engine_set_synth_param(0,1.0f); double bright=level_after(160);
    CHECK(bright>dark*1.1); /* menu-connected cutoff changes the real output */
    engine_set_fx_mode(8); engine_set_age(1); engine_set_atmosphere(1);
    level_after(100); engine_set_master_volume(0);
    CHECK(level_after(200)<1.0); /* mute also owns wet tails and tape noise */
    printf("  shared master: all six cores mute; half volume %.3f; cutoff %.2fx\n",half/full,bright/dark);
}

static void test_voice_gates(void) {
    void (*init[])(void)={bowed_init,horn_init,choir_init};
    void (*on[])(int,float,float)={bowed_note_on,horn_note_on,choir_note_on};
    void (*off[])(int)={bowed_note_off,horn_note_off,choir_note_off};
    int (*count[])(void)={bowed_active_count,horn_active_count,choir_active_count};
    void (*render[])(float*,float*,float*,float*,int,float)={bowed_render_mix,horn_render_mix,choir_render_mix};
    float l[BLK],r[BLK],sl[BLK],sr[BLK];
    engine_init();
    for(int v=0;v<3;++v) {
        init[v](); on[v](0,220,0.4f);
        for(int b=0;b<700;++b) {
            memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
            render[v](l,r,sl,sr,BLK,0.5f);
        }
        CHECK(count[v]()==1); /* >8 s: no fixed-duration cutoff */
        off[v](1); CHECK(count[v]()==1); /* source ownership */
        off[v](0);
        for(int b=0;b<1600;++b) {
            memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
            render[v](l,r,sl,sr,BLK,0.5f);
        }
        CHECK(count[v]()==0);
    }
}

static double send_tail(int mode,float send) {
    static AmbientFxStorage storage;
    static unsigned char arena[AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES] __attribute__((aligned(32)));
    AmbientFxConfig cfg=ambient_fx_default_config();
    AmbientFx *fx=ambient_fx_init(&storage,arena,sizeof arena,&cfg); CHECK(fx!=NULL);
    AmbientFxParameters p=ambient_fx_world_parameters(0);
    p.space=0.8f;p.atmosphere=1;p.echo=0.8f;p.age=0;p.motion=0;p.blur=0;p.shimmer=0.3f;
    ambient_fx_set_parameters(fx,p);ambient_fx_set_mode(fx,mode);
    float d[BLK*2],s[BLK*2]; double e=0;
    for(int b=0;b<200;++b) {
        memset(d,0,sizeof d);memset(s,0,sizeof s);
        if(b==80) for(int n=0;n<BLK;++n) { /* one short, identical musical excitation */
            d[2*n]=d[2*n+1]=0.2f*sinf(n*6.2831853f*440/44100);
            s[2*n]=s[2*n+1]=d[2*n]*send;
        }
        ambient_fx_process_buses_f32(fx,d,s,BLK);
        if(b>81) for(int n=0;n<BLK*2;++n) e+=d[n]*d[n];
    }
    return e;
}

static void test_sends(void) {
    int spatial[]={1,2,6,8};
    for(int i=0;i<4;++i) {
        double dry=send_tail(spatial[i],0),wet=send_tail(spatial[i],1);
        CHECK(dry<1e-10); CHECK(wet>1e-5);
    }
}


static void setup_core(int core) {
    engine_init(); synth_host_init(); engine_set_synth_backend(&BE);
    engine_set_tuning(1); engine_set_key(60); engine_set_fx_mode(0); engine_set_synth(core);
    /* Narrow unison/chorus to isolate Mist's fundamental in the pitch test. */
    if(core==3) { engine_set_synth_param(1,0); engine_set_synth_param(2,0); }
    level_after(80);
}
static void capture(int16_t *out,int frames) {
    for(int i=0;i<frames;i+=BLK) engine_render(out+2*i,frames-i<BLK?frames-i:BLK);
}
static double energy(const int16_t *x,int frames) {
    double e=0; for(int i=0;i<frames*2;++i) e+=(double)x[i]*x[i]; return e;
}
static double spectral_power(const int16_t *x,int frames,double hz) {
    double re=0,im=0;
    for(int i=0;i<frames;++i) {
        double w=0.5-0.5*cos(6.283185307179586*i/(frames-1));
        double a=6.283185307179586*hz*i/44100.0;
        re+=x[2*i]*w*cos(a); im+=x[2*i]*w*sin(a);
    }
    return re*re+im*im;
}
static void test_pitch_and_controls(void) {
    enum {N=44100*3}; static int16_t ref[N*2],changed[N*2];
    for(int core=1;core<=6;++core) {
        setup_core(core);float hz=tuning_hz(64);engine_note_on(0,hz,0.65f);capture(ref,N);
        double pure=spectral_power(ref,N,hz),rounded=spectral_power(ref,N,dsp_midi_to_hz(64));
        printf("  core %d Just/rounded spectral energy %.2fx\n",core,pure/(rounded+1)); CHECK(pure>rounded*2);
        for(int macro=0;macro<4;++macro) {
            setup_core(core);
            if(macro==0) engine_set_brightness(800);
            if(macro==1) engine_set_resonance(0.85f);
            if(macro==2) engine_set_sweep(1);
            if(macro==3) engine_set_envmod(1);
            engine_note_on(0,hz,0.65f);capture(changed,N);double diff=0;
            for(int i=0;i<N*2;++i) {double d=(double)changed[i]-ref[i];diff+=d*d;}
            CHECK(diff>energy(ref,N)*0.0001);CHECK(peak_of(changed,N)<32767);
        }
        double onset[2],tail[2];
        for(int slow=0;slow<2;++slow) {
            setup_core(core);engine_set_attack(slow?1:0);engine_set_release(0.5f);
            engine_note_on(0,hz,0.65f);capture(changed,882);onset[slow]=energy(changed,882);
            setup_core(core);engine_set_attack(0.5f);engine_set_release(slow?1:0);
            engine_note_on(0,hz,0.65f);capture(changed,44100);engine_note_off(0);
            capture(changed,N);tail[slow]=energy(changed,N);
        }
        CHECK(onset[0]>onset[1]*1.2);CHECK(tail[1]>tail[0]*1.2);
    }
    engine_set_tuning(0);
}
static int retune_calls, attack_calls;
static float retuned_hz, attack_hz;
static void observe_retune(float hz) { ++retune_calls; retuned_hz=hz; }
static void observe_attack_hz(float hz,float vel) {
    (void)vel; ++attack_calls; attack_hz=hz;
}
static void test_live_tuning(void) {
    /* Non-numeric press order: tuning must not reorder last-note priority,
     * emit note-on/off, or forget a covered held source's new pitch. */
    engine_synth_backend_t probe=BE;
    probe.retune_hz=observe_retune; probe.note_on_hz=observe_attack_hz;
    setup_core(1); engine_set_synth_backend(&probe);
    retune_calls=attack_calls=0;
    engine_note_on(13,tuning_hz(64),0.4f);
    engine_note_on(0,tuning_hz(67),0.7f);
    engine_set_tuning(0);
    CHECK(attack_calls==2); CHECK(retune_calls==1);
    CHECK(fabsf(retuned_hz-dsp_midi_to_hz(67))<0.01f);
    engine_set_tuning(0); CHECK(retune_calls==1);
    engine_note_off(0);
    CHECK(attack_calls==3); CHECK(fabsf(attack_hz-dsp_midi_to_hz(64))<0.01f);
    engine_set_tuning(1);
    CHECK(retune_calls==2); CHECK(attack_calls==3);
    CHECK(fabsf(retuned_hz-tuning_hz(64))<0.01f);
    engine_note_off(13); engine_set_tuning(0); CHECK(retune_calls==2);

    /* Actual device PCM, both directions on all six native cores. */
    enum { N=44100*2 }; static int16_t samples[N*2];
    for(int core=1;core<=6;++core) for(int target=0;target<=1;++target) {
        setup_core(core); engine_set_release(1);
        engine_set_tuning(!target); float old=tuning_hz(64);
        engine_note_on(0,old,0.65f); capture(samples,22050);
        engine_set_tuning(target); float hz=tuning_hz(64);
        capture(samples,22050); /* settle the existing native glide */
        capture(samples,N);
        double now=spectral_power(samples,N,hz), prior=spectral_power(samples,N,old);
        printf("  core %d live tuning %d: target/old %.2fx\n",core,target,now/(prior+1));
        CHECK(now>prior*2); CHECK(peak_of(samples,N)<32767);
    }
}

static int memory_has(int midi) {
    int notes[128],n=engine_sounding_notes(notes,128);
    for(int i=0;i<n;++i) if(notes[i]==midi) return 1;
    return 0;
}
static int auto_checked,mel_prev,mel_run,auto_blocked;
static uint32_t audit_ms,audit_last;
static void audit_onset(int on,uint8_t source,float hz,float amp) {
    (void)amp;if(on!=1 || !(source==8 || source==15 || (source>=5 && source<=7))) return;
    int m=(int)lrintf(69+12*log2f(hz/440));int notes[128],n=engine_sounding_notes(notes,128);
    CHECK(!auto_blocked);CHECK(harmony_in_world(m));CHECK(harmony_collision_ok(m,notes,n));
    if(audit_last) CHECK(audit_ms-audit_last>=1400u);
    audit_last=audit_ms;++auto_checked;
    if(source==15) {
        if(mel_prev) CHECK(abs(m-mel_prev)<=12);
        mel_run=m==mel_prev?mel_run+1:1;mel_prev=m;CHECK(mel_run<=2);
    }
}
static void test_pitch_memory(void) {
    engine_init();engine_set_tuning(0);engine_set_fx_mode(8);
    engine_note_on(0,dsp_midi_to_hz(60),0.2f);engine_note_off(0);
    CHECK(memory_has(60));CHECK(memory_has(36));
    engine_generative_tick(20000);CHECK(memory_has(60));
    engine_generative_tick(90000);CHECK(!memory_has(60));
    int root=brain_get_key();engine_set_drone(true);CHECK(memory_has(root));
    engine_set_key(58);CHECK(memory_has(root)&&memory_has(58));engine_set_drone(false);CHECK(memory_has(58));
    engine_generative_tick(180000);CHECK(!memory_has(58));
    engine_set_release(1);engine_note_on(0,dsp_midi_to_hz(61),0.2f);
    engine_set_release(0);engine_note_off(0);engine_generative_tick(200000);CHECK(memory_has(61));
    auto_checked=0;
    for(int mode=0;mode<6;++mode) for(int seed=1;seed<=3;++seed) {
        engine_init();engine_set_tuning(0);engine_set_key(60);engine_set_mode(mode);
        engine_generative_new_field((uint32_t)seed*1031u);mel_prev=mel_run=0;audit_last=0;auto_blocked=0;
        engine_set_note_hook(audit_onset);engine_set_generative(true,-1);
        for(audit_ms=0;audit_ms<1200000;audit_ms+=250) {
            if(audit_ms==30000) engine_note_on(0,dsp_midi_to_hz(60),0.15f);
            if(audit_ms==100000) {engine_set_user_presence(true);auto_blocked=1;}
            if(audit_ms==110000) engine_set_user_presence(false);
            if(audit_ms==117750) auto_blocked=0; /* 8 s after last occupied tick */
            if(audit_ms==200000) engine_note_off(0);
            if(audit_ms==600000) {engine_set_key(61);mel_prev=mel_run=0;}
            engine_generative_tick(audit_ms);
        }
        engine_set_note_hook(NULL);engine_set_generative(false,-1);
    }
    CHECK(auto_checked>1500);printf("  %d automatic onsets: 6 modes x 3 seeds x 20 minutes\n",auto_checked);
}
static void test_handover(void) {
    void (*init[])(void)={bowed_init,horn_init,choir_init};
    void (*on[])(int,float,float)={bowed_note_on,horn_note_on,choir_note_on};
    void (*render[])(float*,float*,float*,float*,int,float)={bowed_render_mix,horn_render_mix,choir_render_mix};
    float l[BLK],r[BLK],sl[BLK],sr[BLK],baseline[2];shape_init();
    for(int v=0;v<3;++v) for(int steal=0;steal<2;++steal) {
        init[v]();for(int i=0;i<3;++i) on[v](i,220.0f+i*55,0.2f+i*0.1f);
        for(int b=0;b<100;++b) {
            memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
            render[v](l,r,sl,sr,BLK,0.5f);
        }
        if(steal) on[v](3,660,0.5f);
        memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        render[v](l,r,sl,sr,1,0.5f);
        if(!steal) {baseline[0]=l[0];baseline[1]=r[0];}
        else CHECK(l[0]==baseline[0] && r[0]==baseline[1]);
    }
}

extern const synth_engine_t engine_glass_orbit;
static void test_orbit_fundamental(void) {
    const synth_engine_t *e=&engine_glass_orbit;
    float minimum=1.0f;
    for(int pos=0;pos<=36;++pos) {
        e->init(); e->set_param(SP_A,(float)pos/36.0f); e->set_param(SP_D,0.0f);
        e->note_on(57.0f,0.7f);
        double re=0.0,im=0.0;
        for(int n=0;n<88200;++n) {
            float l=0,r=0,sl=0,sr=0;
            e->render_mix(&l,&r,&sl,&sr,1);
            if(n>=44100) {
                double ph=6.283185307179586*220.0*n/44100.0;
                re+=l*cos(ph); im+=l*sin(ph);
            }
        }
        float fundamental=(float)(2.0*sqrt(re*re+im*im)/44100.0);
        if(fundamental<minimum) minimum=fundamental;
        CHECK(fundamental>0.15f);
    }
    e->panic();
    printf("  Orbit minimum morph fundamental = %.4f\n",minimum);
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    static int16_t buf[BLK * 2];

    dsp_init();
    test_orbit_fundamental();
    engine_init();
    engine_set_world(0);

    /* 1) no backend registered → set_synth(>0) must be a no-op */
    engine_set_synth(3);
    CHECK(engine_synth() == 0);

    /* register the real V2 host, exactly like the product main */
    synth_host_init();
    engine_set_synth_backend(&BE);

    /* 2) ambient plays a pad note (baseline) */
    engine_note_on(0, 261.63f, 0.8f);
    int amb_peak = 0;
    for (int i = 0; i < 20; ++i) { engine_render(buf, BLK); }
    amb_peak = peak_of(buf, BLK);
    CHECK(amb_peak > 500);
    printf("  ambient pad peak = %d\n", amb_peak);

    /* 3) switch to FM GLASS while audio runs: the joined stream across the
     * crossfade must have no hard step (a raw swap would jump >20000). */
    engine_set_synth(2);
    CHECK(engine_synth() == 2);
    int16_t prevL = buf[(BLK - 1) * 2];
    int max_step = 0;
    for (int i = 0; i < 8; ++i) {                 /* covers the 15 ms fade */
        engine_render(buf, BLK);
        int step = abs((int)buf[0] - (int)prevL);
        for (int n = 1; n < BLK; ++n) {
            int s = abs((int)buf[2*n] - (int)buf[2*(n-1)]);
            if (s > step) step = s;
        }
        if (step > max_step) max_step = step;
        prevL = buf[(BLK - 1) * 2];
    }
    printf("  switch max inter-sample step = %d\n", max_step);
    CHECK(max_step < 12000);                      /* no click/pop swap      */

    /* 4) every V2 core is playable through the CELL path and bounded */
    static const char *names[6] =
        { "Acid", "FM Glass", "Mist", "Storm", "Orbit", "Bamboo" };
    for (int core = 1; core <= 6; ++core) {
        engine_set_synth(core);
        for (int i = 0; i < 6; ++i) engine_render(buf, BLK);   /* settle fade */
        engine_note_on(1, 220.0f, 0.9f);          /* cell source 1 → V2      */
        int pk = 0;
        for (int i = 0; i < 12; ++i) {
            engine_render(buf, BLK);
            int p = peak_of(buf, BLK);
            if (p > pk) pk = p;
        }
        printf("  core %-9s peak=%5d\n", names[core-1], pk);
        CHECK(pk > 300);                          /* audible                */
        CHECK(pk <= 32767);                       /* bounded                */
        engine_note_off(1);
        for (int i = 0; i < 30; ++i) engine_render(buf, BLK);
        int tail = peak_of(buf, BLK);
        CHECK(tail < pk);                         /* decays after release   */
    }

    /* 5) back to ambient: pad path works again */
    engine_set_synth(0);
    for (int i = 0; i < 8; ++i) engine_render(buf, BLK);       /* fade out  */
    CHECK(engine_synth() == 0);
    engine_note_on(0, 261.63f, 0.8f);
    int back = 0;
    for (int i = 0; i < 20; ++i) { engine_render(buf, BLK); }
    back = peak_of(buf, BLK);
    printf("  back-to-ambient pad peak = %d\n", back);
    CHECK(back > 500);

    test_playability(); test_voice_gates(); test_sends();
    test_pitch_and_controls(); test_live_tuning(); test_handover(); test_pitch_memory();
    printf("synth_device: %d checks, 0 failures\n", checks);
    return 0;
}
