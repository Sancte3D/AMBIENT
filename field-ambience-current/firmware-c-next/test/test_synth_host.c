/*
 * test_synth_host.c — the swappable synth-engine host + the ACID engine.
 * Engine-level: bounded / idle-silent / decays / accent-louder. Host-level:
 * select + active name, render is audible & non-clipping, panic silences.
 */
#include "v2/synth_host.h"
#include "v2/synth_engine.h"
#include "dsp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int checks = 0, fails = 0;
#define CHECK(c, ...) do { ++checks; if(!(c)){ ++fails; \
    fprintf(stderr,"FAIL %s:%d  ",__FILE__,__LINE__); \
    fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\n"); } } while(0)

#define SR        44100
enum { N = 256 };
#define SR_BLOCKS (SR / N)

extern const synth_engine_t engine_acid;   /* test the engine directly (float) */

static float dL[N], dR[N], sL[N], sR[N];
static void clr(void){ memset(dL,0,sizeof dL); memset(dR,0,sizeof dR);
                       memset(sL,0,sizeof sL); memset(sR,0,sizeof sR); }

/* ---- engine-level (no reverb / no limiter) ---- */

static void test_engine_bounded_clean(void){
    engine_acid.init();
    engine_acid.note_on(45, 1.0f);              /* A2, accented */
    float peak = 0.0f; int bad = 0;
    for (int s=0;s<5;++s) for (int b=0;b<SR_BLOCKS;++b){
        clr(); engine_acid.render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){
            if (isnan(dL[i])||isinf(dL[i])) ++bad;
            float ax=fabsf(dL[i]); if(ax>peak)peak=ax;
            CHECK(dL[i]==dR[i], "L/R must match (mono)");
        }
    }
    CHECK(bad==0, "%d NaN/Inf", bad);
    CHECK(peak < 1.2f, "diverged: peak %.3f", peak);
    CHECK(peak > 0.05f, "silent while held: peak %.3f", peak);
    printf("  engine bounded: 5 s held, peak %.3f, %d NaN\n", peak, bad);
}

static void test_engine_idle_silent(void){
    engine_acid.init();
    float peak=0.0f;
    for (int b=0;b<SR_BLOCKS;++b){ clr(); engine_acid.render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){ float a=fabsf(dL[i]); if(a>peak)peak=a; } }
    CHECK(peak==0.0f, "idle not silent: %.6f", peak);
    printf("  engine idle silent: %.6f\n", peak);
}

static void test_engine_release_decays(void){
    engine_acid.init();
    engine_acid.note_on(45, 0.5f);
    for (int b=0;b<SR_BLOCKS/2;++b){ clr(); engine_acid.render_mix(dL,dR,sL,sR,N); }
    engine_acid.note_off();
    float tail=0.0f;
    for (int s=0;s<2;++s) for (int b=0;b<SR_BLOCKS;++b){
        clr(); engine_acid.render_mix(dL,dR,sL,sR,N);
        if (s<1) continue;                       /* measure the 2nd second */
        for (int i=0;i<N;++i){ float a=fabsf(dL[i]); if(a>tail)tail=a; }
    }
    CHECK(tail < 0.02f, "did not decay after note_off: tail %.4f", tail);
    printf("  engine release tail = %.4f\n", tail);
}

static double rms_first(float vel, double secs){
    engine_acid.init();
    engine_acid.note_on(45, vel);
    double sum=0; long n=0; int blocks=(int)(secs*SR_BLOCKS);
    for (int b=0;b<blocks;++b){ clr(); engine_acid.render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){ sum+=(double)dL[i]*dL[i]; ++n; } }
    return sqrt(sum/(n?n:1));
}
static void test_engine_accent_louder(void){
    double soft = rms_first(0.0f, 0.20);
    double acc  = rms_first(1.0f, 0.20);
    CHECK(acc > soft*1.10, "accent (%.4f) not louder than soft (%.4f)", acc, soft);
    printf("  accent louder: soft=%.4f accent=%.4f (%.2fx)\n", soft, acc, acc/soft);
}

/* ---- host-level (full path → int16) ---- */

static int16_t obuf[N*2];
static int host_peak_over(double secs){
    int peak=0; int blocks=(int)(secs*SR_BLOCKS);
    for (int b=0;b<blocks;++b){ synth_host_render(obuf,N);
        for (int i=0;i<N;++i){ int v=obuf[i*2]; int a=v<0?-v:v; if(a>peak)peak=a; } }
    return peak;
}
static void test_host_select(void){
    synth_host_init();
    CHECK(synth_host_active()==SYNTH_ACID, "default engine not ACID");
    CHECK(strcmp(synth_host_active_name(),"ACID RAIN")==0, "name=%s", synth_host_active_name());
    synth_host_select((synth_id_t)999);          /* invalid → unchanged */
    CHECK(synth_host_active()==SYNTH_ACID, "invalid select changed engine");
    synth_host_select(SYNTH_FM_GLASS);           /* swap the sound-core */
    CHECK(synth_host_active()==SYNTH_FM_GLASS, "select FM_GLASS failed");
    CHECK(strcmp(synth_host_active_name(),"FM GLASS")==0, "name=%s", synth_host_active_name());
    synth_host_select(SYNTH_ACID);
    CHECK(synth_host_active()==SYNTH_ACID, "select back to ACID failed");
    printf("  host select: ACID <-> FM GLASS ok\n");
}

/* the FM engine must be bounded, idle-silent and decay too */
static void test_fm_glass_engine(void){
    extern const synth_engine_t engine_fm_glass;
    engine_fm_glass.init();
    float peak=0.0f; int bad=0;
    engine_fm_glass.note_on(45, 0.9f);
    for (int s=0;s<3;++s) for (int b=0;b<SR_BLOCKS;++b){
        clr(); engine_fm_glass.render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){ if(isnan(dL[i])||isinf(dL[i]))++bad;
            float a=fabsf(dL[i]); if(a>peak)peak=a; CHECK(dL[i]==dR[i],"L/R"); } }
    CHECK(bad==0 && peak<1.2f && peak>0.05f, "fm bounded/audible: peak=%.3f bad=%d", peak, bad);
    /* idle silent */
    engine_fm_glass.init(); float ip=0.0f;
    for (int b=0;b<SR_BLOCKS;++b){ clr(); engine_fm_glass.render_mix(dL,dR,sL,sR,N);
        for (int i=0;i<N;++i){ float a=fabsf(dL[i]); if(a>ip)ip=a; } }
    CHECK(ip==0.0f, "fm idle not silent: %.6f", ip);
    printf("  fm_glass: held peak=%.3f, idle=%.6f, %d NaN\n", peak, ip, bad);
}
static void test_host_render_bounded(void){
    synth_host_init();
    int idle = host_peak_over(0.10);
    CHECK(idle==0, "host idle not silent: %d", idle);
    synth_host_note_on(45, 1.0f);
    int peak = host_peak_over(1.0);
    CHECK(peak > 1000,  "host render inaudible: %d", peak);
    CHECK(peak <= 32767,"host render clipped past full-scale: %d", peak);
    printf("  host render: idle=%d, note peak=%d\n", idle, peak);
}
static void test_host_panic(void){
    synth_host_init();
    synth_host_note_on(45, 1.0f);
    (void)host_peak_over(0.2);
    synth_host_panic();
    int tail = host_peak_over(1.0);    /* dry gone instantly; reverb tail allowed to ring out */
    CHECK(tail < 20000, "panic left it loud: %d", tail);
    printf("  host panic tail=%d\n", tail);
}

/* every registered engine must be audible, non-clipping, NaN-free, and go
 * silent after panic — exercised through the host (full path → int16). */
static void test_all_engines(void){
    for (int id = 0; id < SYNTH_COUNT; ++id){
        synth_host_init();
        synth_host_select((synth_id_t)id);
        CHECK(synth_host_active()==(synth_id_t)id, "select %d failed", id);
        const char *nm = synth_host_active_name();
        synth_host_note_on(45, 0.9f);
        int peak = host_peak_over(1.0);
        CHECK(peak > 800,   "%s inaudible: %d", nm, peak);
        CHECK(peak <= 32767,"%s clipped past full-scale: %d", nm, peak);
        synth_host_panic();
        int tail = host_peak_over(1.2);   /* dry gone; only reverb tail rings */
        CHECK(tail < 22000, "%s loud after panic: %d", nm, tail);
        printf("  %-15s note peak=%5d  post-panic=%5d\n", nm, peak, tail);
    }
}

int main(void){
    dsp_init();
    printf("== synth_host + 6 engines ==\n");
    test_engine_bounded_clean();
    test_engine_idle_silent();
    test_engine_release_decays();
    test_engine_accent_louder();
    test_fm_glass_engine();
    test_all_engines();
    test_host_select();
    test_host_render_bounded();
    test_host_panic();
    printf("\n%d checks, %d failures\n", checks, fails);
    return fails ? 1 : 0;
}
