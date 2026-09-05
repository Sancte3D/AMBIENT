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
    be_select, be_note_on, be_note_off, be_panic, be_render, synth_host_render_mix, be_param
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
    probe.note_on=observe_note; probe.note_off=observe_off;
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

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    static int16_t buf[BLK * 2];

    dsp_init();
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
    printf("synth_device: %d checks, 0 failures\n", checks);
    return 0;
}
