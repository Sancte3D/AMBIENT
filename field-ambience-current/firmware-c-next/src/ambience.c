/*
 * ambience.c — per-world atmospheric layer (ADR-0017 Phase 2a..d).
 *
 * Generators run inside engine_render() between texture and bass:
 *   • WIND    — broadband pink air, irregular gusts and smaller eddies.
 *               No periodic filter sweep or pitched whistles.
 *   • RAIN    — Moss Fields only (world 3). Pink-BP "sshhh" + pool of 12
 *               noise-burst drops at 1.5..4.5 kHz, 15..40 ms decay.
 *   • WAVES   — Open Sea only (world 1). Asymmetric envelope
 *               (1.2..2 s attack, 5..9 s decay, 1..4 s gap). LP'd brown
 *               body + HF pink-BP splash gated to crest. r19.47: softened to
 *               a gentle Mediterranean lap (warmer wash, quieter break).
 *   • SEA HUM — Open Sea only (r19.47). Warm, wide, non-tonal low bed
 *               (brown → resonant ~160 Hz SVF) that breathes with a slow
 *               ground-swell — the body of the sea under the surf.
 *   • VINYL   — After Hours only (world 3). Hi-pass noise crackle +
 *               sparse sharp pops every ~0.02..0.08 s + slow LP'd brown
 *               rumble (distant city through walls).
 *
 *   • FJORD   — Fjords only (world 2, r19.54). Dark low water murmur + sparse
 *               deep drips against rock — cold, still, vertical.
 *   • DESERT  — Desert only (world 4, r19.54). Faint high-mid heat haze +
 *               very sparse dry sand grains — mostly stillness.
 *
 * All generators lifted near-verbatim from tools/render_worlds.c so the
 * on-device sound matches the audition tools.
 */

#include "ambience.h"
#include "dsp.h"
#include <stdint.h>
#include <math.h>

#define SR            ((float)DSP_SAMPLE_RATE_HZ)
#define SILENCE_EPS   1.0e-5f

/* World index keys per-world dispatch. Index meaning matches worlds.c:
 *   0 = Tokyo City, 1 = Crystal Coast, 2 = Midnight Drive, 3 = After Hours. */
/* r19.44: landscape worlds (order matches worlds.c). Environmental textures
 * are gated by index so each landscape gets its matching support layer. */
#define WORLD_ALPS     0
#define WORLD_OPENSEA  1
#define WORLD_FJORDS   2
#define WORLD_MOSS     3
#define WORLD_DESERT   4

static int   world_i = 0;
static float level_cur = 0.0f, level_tgt = 0.0f;

/* ===========================================================================
 * Wind — universal, runs for every world.
 *
 * Irregular weather and turbulence on separate time scales. Broad lowpass
 * colour follows pressure; no resonant pipe model or deterministic LFO.
 * =========================================================================== */

/* Broadband wind: independent weather and turbulence clocks, no cyclic sweep
 * or whistle resonators. Fixed state, two low-Q filters instead of four SVFs.
 * Control noise never consumes the audio PRNG: block size cannot change weather. */
static uint32_t wnd_rng_L, wnd_rng_R, wnd_weather;
static dsp_svf_t wnd_lpL, wnd_lpR;
static float wnd_pink_L_b0, wnd_pink_L_b1, wnd_pink_L_b2;
static float wnd_pink_R_b0, wnd_pink_R_b1, wnd_pink_R_b2;
static float wnd_gust_env, wnd_gust_tgt, wnd_slew;
static float wnd_eddy, wnd_eddy_tgt, wnd_dcL, wnd_dcR;
static int wnd_gust_until, wnd_eddy_until;
static uint32_t wnd_ctrl;

static inline float wnd_white(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}
static inline float wnd_random(void) { return 0.5f + 0.5f * wnd_white(&wnd_weather); }
static inline float wnd_pink(uint32_t *rng, float *b0, float *b1, float *b2) {
    float w = wnd_white(rng);
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}
static void wind_reset(void) {
    wnd_rng_L=0xACE12345u; wnd_rng_R=0x7B19F88Au; wnd_weather=0x91BC24E3u;
    dsp_svf_reset(&wnd_lpL); dsp_svf_reset(&wnd_lpR);
    wnd_gust_env=0.10f; wnd_gust_tgt=0.60f; wnd_slew=1.5e-5f;
    wnd_gust_until=(int)(SR*3.0f); wnd_eddy_until=0; wnd_ctrl=0;
    wnd_eddy=wnd_eddy_tgt=wnd_dcL=wnd_dcR=0.0f;
    wnd_pink_L_b0=wnd_pink_L_b1=wnd_pink_L_b2=0.0f;
    wnd_pink_R_b0=wnd_pink_R_b1=wnd_pink_R_b2=0.0f;
}
static inline void wind_tick(float *outL, float *outR) {
    if (--wnd_gust_until<=0) {
        float r=wnd_random();
        wnd_gust_tgt = r<0.40f ? 0.02f+r*0.20f : 0.30f+wnd_random()*0.65f;
        /* Wide irregular intervals, including long calms; random rise/fall. */
        wnd_gust_until=(int)(SR*(2.0f+wnd_random()*15.0f));
        wnd_slew=1.0f/(SR*(wnd_gust_tgt>wnd_gust_env ?
                         0.6f+wnd_random()*2.4f : 1.2f+wnd_random()*3.0f));
    }
    if (--wnd_eddy_until<=0) {
        wnd_eddy_tgt=wnd_random()*2.0f-1.0f;
        wnd_eddy_until=(int)(SR*(0.18f+wnd_random()*1.9f));
    }
    wnd_gust_env+=wnd_slew*(wnd_gust_tgt-wnd_gust_env);
    wnd_eddy+=0.00012f*(wnd_eddy_tgt-wnd_eddy);
    float gust=wnd_gust_env*wnd_gust_env*(0.85f+0.15f*wnd_eddy);
    if ((wnd_ctrl++ & 63u)==0) {
        float fc=650.0f+wnd_gust_env*1900.0f+wnd_eddy*180.0f;
        dsp_svf_set(&wnd_lpL,fc,0.707f);
        dsp_svf_set(&wnd_lpR,fc*1.08f,0.707f);
    }
    float pL=wnd_pink(&wnd_rng_L,&wnd_pink_L_b0,&wnd_pink_L_b1,&wnd_pink_L_b2);
    float pR=wnd_pink(&wnd_rng_R,&wnd_pink_R_b0,&wnd_pink_R_b1,&wnd_pink_R_b2);
    float L=dsp_svf_lp(&wnd_lpL,pL), R=dsp_svf_lp(&wnd_lpR,pR);
    /* Remove infra/low rumble without a resonant bandpass centre. */
    wnd_dcL+=0.009f*(L-wnd_dcL); wnd_dcR+=0.009f*(R-wnd_dcR);
    *outL+=(L-wnd_dcL)*gust*0.70f;
    *outR+=(R-wnd_dcR)*gust*0.70f;
}

/* ===========================================================================
 * Rain (Phase 2b) — Tokyo only.
 *
 * Background "sshhh": pink noise through wide BP around 2.5 kHz. Foreground:
 * pool of up to 12 noise-burst DROPS through resonant BP at 1.5..4.5 kHz,
 * Q=4, 15..40 ms exponential decay. Drops are scheduled 30..180 ms apart.
 * =========================================================================== */

#define RAIN_MAX_DROPS 12

typedef struct {
    int       active;
    float     env;
    float     decay;
    dsp_svf_t bp;
} rain_drop_t;

static rain_drop_t rain_drops[RAIN_MAX_DROPS];
static uint32_t    rain_rng = 0x5417F00Du;
static dsp_svf_t   rain_bg_bpL, rain_bg_bpR;
static float       rain_pink_b0L = 0, rain_pink_b1L = 0, rain_pink_b2L = 0;
static float       rain_pink_b0R = 0, rain_pink_b1R = 0, rain_pink_b2R = 0;
static int         rain_until_next = 0;
static float rain_activity=0.5f, rain_target=0.5f;
static int rain_weather_until=0;
static uint32_t rain_weather_rng=0xFA831290u;

static inline float rain_white(void) {
    rain_rng = rain_rng * 1664525u + 1013904223u;
    return (float)((int32_t)rain_rng) * (1.0f / 2147483648.0f);
}

static inline float rain_pink(float *b0, float *b1, float *b2) {
    float w = rain_white();
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}

static void rain_reset(void) {
    for (int d = 0; d < RAIN_MAX_DROPS; ++d) {
        rain_drops[d].active = 0;
        rain_drops[d].env    = 0.0f;
        rain_drops[d].decay  = 0.0f;
        dsp_svf_reset(&rain_drops[d].bp);
    }
    dsp_svf_reset(&rain_bg_bpL);
    dsp_svf_reset(&rain_bg_bpR);
    /* r18.92: 2.5 kHz was the phone-speaker sizzle zone — drop the shh a
     * fifth darker and softer-Q so it reads as distance, not dirt. */
    dsp_svf_set(&rain_bg_bpL, 1900.0f, 1.2f);
    dsp_svf_set(&rain_bg_bpR, 2050.0f, 1.2f);   /* slight L/R offset */
    rain_pink_b0L = rain_pink_b1L = rain_pink_b2L = 0.0f;
    rain_pink_b0R = rain_pink_b1R = rain_pink_b2R = 0.0f;
    rain_until_next = (int)(SR * 0.04f);
    rain_activity=rain_target=0.5f; rain_weather_until=0; rain_weather_rng=0xFA831290u;
}

static inline void rain_tick(float *outL, float *outR) {
    /* Separate weather clock: uneven showers and rests, not steady hiss. */
    if (--rain_weather_until<=0) {
        rain_weather_rng=rain_weather_rng*1664525u+1013904223u;
        float r=(float)(rain_weather_rng>>8)/16777216.0f;
        rain_target=r*r;
        rain_weather_rng=rain_weather_rng*1664525u+1013904223u;
        rain_weather_until=(int)(SR*(4.0f+14.0f*(float)(rain_weather_rng>>8)/16777216.0f));
    }
    rain_activity+=0.000015f*(rain_target-rain_activity);
    /* background sshhh */
    float pL = rain_pink(&rain_pink_b0L, &rain_pink_b1L, &rain_pink_b2L);
    float pR = rain_pink(&rain_pink_b0R, &rain_pink_b1R, &rain_pink_b2R);
    /* r18.97: the bg shh was the loudest stationary noise left after the
     * hiss/PADsynth fixes — the DROPS carry the rain image, the wash only
     * glues them. 0.45 → 0.18. */
    float bgL = dsp_svf_bp(&rain_bg_bpL, pL) * 0.18f * rain_activity;
    float bgR = dsp_svf_bp(&rain_bg_bpR, pR) * 0.18f * rain_activity;

    /* schedule a new drop */
    if (--rain_until_next <= 0) {
        for (int d = 0; d < RAIN_MAX_DROPS; ++d) {
            if (!rain_drops[d].active) {
                rain_drops[d].active = 1;
                rain_drops[d].env    = 0.4f + (rain_white() * 0.5f + 0.5f) * 0.35f;
                float fc   = 1500.0f + (rain_white() * 0.5f + 0.5f) * 3000.0f;
                float dec  = 0.015f + (rain_white() * 0.5f + 0.5f) * 0.025f;
                dsp_svf_set(&rain_drops[d].bp, fc, 1.4f);
                rain_drops[d].decay = expf(-1.0f / (dec * SR));
                break;
            }
        }
        float interval = 0.060f + (rain_white()*0.5f+0.5f)*(0.20f+1.8f*(1.0f-rain_activity));
        rain_until_next = (int)(interval * SR);
    }

    /* sum active drops; spread across stereo by index parity */
    float dropL = 0.0f, dropR = 0.0f;
    for (int d = 0; d < RAIN_MAX_DROPS; ++d) {
        if (!rain_drops[d].active) continue;
        float src = rain_white();
        float out = dsp_svf_bp(&rain_drops[d].bp, src) * rain_drops[d].env;
        if (d & 1) { dropR += out; dropL += out * 0.4f; }
        else       { dropL += out; dropR += out * 0.4f; }
        rain_drops[d].env *= rain_drops[d].decay;
        if (rain_drops[d].env < 0.001f) rain_drops[d].active = 0;
    }

    *outL += bgL + dropL * 0.36f;
    *outR += bgR + dropR * 0.36f;
}

/* ===========================================================================
 * Waves (Phase 2c, rebuilt r18.97) — Crystal Coast only.
 *
 * The old version was an envelope on filtered noise — it pulsed, but it
 * didn't read as WATER. Three cues make surf real (principles from field
 * recordings + the procedural-audio literature, reinvented here):
 *   1. the BODY brightens as the wave builds: LP cutoff follows the
 *      envelope 150 → 420 Hz (a swell is dull far off, present up close);
 *   2. the WASH darkens as the water recedes: after the break, the splash
 *      band-pass centre FALLS 2.6 kHz → 500 Hz across the decay — the
 *      signature "shhh → shhoo" of water draining through sand;
 *   3. the BREAK throws SPRAY: a short (0.35 s) burst of granular dust
 *      pings through a 3 kHz resonator right at the attack→decay corner,
 *      like droplets on rock.
 * The envelope gate is SQUARED so the gaps between waves are properly
 * quiet. Asymmetric timing stays: 1.2..2 s build, 5..9 s wash, 1..4 s gap.
 * =========================================================================== */

static uint32_t  wv_rng = 0xBADCAFE1u;
static float     wv_brnL = 0.0f, wv_brnR = 0.0f;
static float     wv_pink_b0L = 0, wv_pink_b1L = 0, wv_pink_b2L = 0;
static float     wv_pink_b0R = 0, wv_pink_b1R = 0, wv_pink_b2R = 0;
static dsp_svf_t wv_lpL, wv_lpR, wv_splashL, wv_splashR;
static dsp_svf_t wv_sprayL, wv_sprayR;
static int       wv_state = 0;           /* 0 idle, 1 attack, 2 decay */
static int       wv_until_next = 0;
static int       wv_phase_samples = 0;
static int       wv_phase_len = 1;
static float     wv_env = 0.0f;
static int       wv_spray_left = 0;      /* samples of spray remaining */
static uint32_t  wv_ctrl = 0;            /* ÷16 control-rate divider */

static inline float wv_white(void) {
    wv_rng = wv_rng * 1664525u + 1013904223u;
    return (float)((int32_t)wv_rng) * (1.0f / 2147483648.0f);
}

static inline float wv_pink(float *b0, float *b1, float *b2) {
    float w = wv_white();
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}

static void waves_reset(void) {
    dsp_svf_reset(&wv_lpL);      dsp_svf_set(&wv_lpL,      150.0f, 0.7f);
    dsp_svf_reset(&wv_lpR);      dsp_svf_set(&wv_lpR,      150.0f, 0.7f);
    dsp_svf_reset(&wv_splashL);  dsp_svf_set(&wv_splashL, 2600.0f, 1.4f);
    dsp_svf_reset(&wv_splashR);  dsp_svf_set(&wv_splashR, 2810.0f, 1.4f);
    dsp_svf_reset(&wv_sprayL);   dsp_svf_set(&wv_sprayL,  3000.0f, 2.0f);
    dsp_svf_reset(&wv_sprayR);   dsp_svf_set(&wv_sprayR,  3240.0f, 2.0f);
    wv_brnL = wv_brnR = 0.0f;
    wv_pink_b0L = wv_pink_b1L = wv_pink_b2L = 0.0f;
    wv_pink_b0R = wv_pink_b1R = wv_pink_b2R = 0.0f;
    wv_state = 0;
    wv_phase_samples = 0;
    wv_phase_len = 1;
    wv_env = 0.0f;
    wv_spray_left = 0;
    wv_ctrl = 0;
    wv_until_next = (int)(SR * 2.0f);
}

static inline void waves_tick(float *outL, float *outR) {
    /* Envelope state machine: idle → attack → decay → idle, then a gap. */
    if (wv_state == 0) {
        if (--wv_until_next <= 0) {
            wv_state = 1; wv_phase_samples = 0;
            wv_phase_len = (int)(SR * (1.2f + (wv_white() * 0.5f + 0.5f) * 0.8f));
        }
    } else if (wv_state == 1) {
        wv_env = (float)wv_phase_samples / (float)wv_phase_len;
        if (++wv_phase_samples >= wv_phase_len) {
            wv_state = 2; wv_phase_samples = 0;
            wv_phase_len = (int)(SR * (5.0f + (wv_white() * 0.5f + 0.5f) * 4.0f));
            wv_spray_left = (int)(SR * 0.35f);   /* the break throws spray */
        }
    } else {
        wv_env = 1.0f - (float)wv_phase_samples / (float)wv_phase_len;
        if (++wv_phase_samples >= wv_phase_len) {
            wv_state = 0; wv_env = 0.0f;
            wv_until_next = (int)(SR * (1.0f + (wv_white() * 0.5f + 0.5f) * 3.0f));
        }
    }

    /* Control-rate (÷16) filter moves — cues 1 and 2. */
    if ((wv_ctrl++ & 15u) == 0) {
        float body_fc = 150.0f + wv_env * 270.0f;
        dsp_svf_set(&wv_lpL, body_fc, 0.7f);
        dsp_svf_set(&wv_lpR, body_fc, 0.7f);
        float rec     = (wv_state == 2) ? wv_env : 1.0f;  /* receding water */
        /* r19.47: warmer Mediterranean wash — cap the splash brightness lower
         * (was 500..2600 Hz) so Open Sea laps gently instead of hissing. */
        float wash_fc = 420.0f + rec * 1150.0f;
        dsp_svf_set(&wv_splashL, wash_fc,         1.4f);
        dsp_svf_set(&wv_splashR, wash_fc * 1.08f, 1.4f);
    }

    /* body: brown noise → env-following LP, SQUARED gate (quiet gaps) */
    float gate = wv_env * wv_env;
    wv_brnL = wv_brnL * 0.998f + wv_white() * 0.02f;
    wv_brnR = wv_brnR * 0.998f + wv_white() * 0.02f;
    float bodyL = dsp_svf_lp(&wv_lpL, wv_brnL) * gate;
    float bodyR = dsp_svf_lp(&wv_lpR, wv_brnR) * gate;

    /* wash: crest-gated on the way up, receding wash on the way down */
    float wash = (wv_state == 2)
                   ? gate * 0.55f
                   : ((wv_env > 0.8f) ? (wv_env - 0.8f) * 2.4f : 0.0f);
    float pL = wv_pink(&wv_pink_b0L, &wv_pink_b1L, &wv_pink_b2L);
    float pR = wv_pink(&wv_pink_b0R, &wv_pink_b1R, &wv_pink_b2R);
    float splashL = dsp_svf_bp(&wv_splashL, pL) * wash;
    float splashR = dsp_svf_bp(&wv_splashR, pR) * wash;

    /* spray: granular dust burst at the break (~130 grains/s, 0.35 s,
     * linear fade-out so the resonator never cuts off mid-ring) */
    float sprayL = 0.0f, sprayR = 0.0f;
    if (wv_spray_left > 0) {
        --wv_spray_left;
        float a  = (float)wv_spray_left * (1.0f / (SR * 0.35f));
        float dL = ((wv_white() * 0.5f + 0.5f) < 130.0f / SR) ? wv_white() * 2.0f : 0.0f;
        float dR = ((wv_white() * 0.5f + 0.5f) < 130.0f / SR) ? wv_white() * 2.0f : 0.0f;
        sprayL = dsp_svf_bp(&wv_sprayL, dL) * a;
        sprayR = dsp_svf_bp(&wv_sprayR, dR) * a;
    }

    /* body 1.35 (was 1.8): the env-following LP passes more energy at the
     * crest than the old fixed 400 Hz LP — 1.8 peaked past full scale.
     * r19.47: soften the surf for a gentle Mediterranean lap — splash 0.5→0.38
     * and the spray "crash" 0.6→0.22. The warm body + the new sea hum carry
     * Open Sea now, not a bright break. */
    *outL += bodyL * 1.35f + splashL * 0.38f + sprayL * 0.22f;
    *outR += bodyR * 1.35f + splashR * 0.38f + sprayR * 0.22f;
}

/* ===========================================================================
 * Sea hum (r19.47) — Open Sea only, ON TOP of the (now gentler) waves.
 *
 * The location brief wants Open Sea to read as the warm Mediterranean, not a
 * cold generic beach. The waves alone gave rhythmic surf but no BODY — the
 * feeling of a wide, warm mass of water under everything. This adds that body:
 *   • a warm, wide low bed — brown noise through a resonant low SVF (~160 Hz)
 *     that BREATHES with a very slow swell (~0.05 Hz), so it rises and falls
 *     like a long ground-swell rather than sitting as a static drone;
 *   • fully decorrelated L/R (own noise streams + a slight cutoff offset) so it
 *     opens the stereo field wide;
 *   • deliberately NON-tonal (filtered noise, not an oscillator) so it never
 *     clashes with the musical key — it is the sea's warmth, not a note.
 * =========================================================================== */

static uint32_t  sh_rng_L = 0x1EAF00D5u, sh_rng_R = 0xB16B00B7u;
static float     sh_brnL = 0.0f, sh_brnR = 0.0f;
static dsp_svf_t sh_lpL, sh_lpR;
static float     sh_swell = 0.35f;        /* slow breath envelope 0..1        */
static float     sh_swell_tgt = 0.8f;
static int       sh_swell_until = 0;
static uint32_t  sh_ctrl = 0;             /* ÷16 control-rate divider          */

static inline float sh_white(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

static void seahum_reset(void) {
    dsp_svf_reset(&sh_lpL); dsp_svf_set(&sh_lpL, 160.0f, 1.3f);
    dsp_svf_reset(&sh_lpR); dsp_svf_set(&sh_lpR, 172.0f, 1.3f);   /* wide offset */
    sh_brnL = sh_brnR = 0.0f;
    sh_swell       = 0.35f;
    sh_swell_tgt   = 0.80f;
    sh_swell_until = (int)(SR * 8.0f);
    sh_ctrl        = 0;
}

static inline void seahum_tick(float *outL, float *outR) {
    /* Slow ground-swell: retarget every 8..20 s between 0.35 and 0.95, glide
     * gently toward it — the bed swells and settles under the surf. */
    if (--sh_swell_until <= 0) {
        float r = sh_white(&sh_rng_L) * 0.5f + 0.5f;
        sh_swell_tgt   = 0.04f + r*r * 0.91f;
        float rr = sh_white(&sh_rng_R) * 0.5f + 0.5f;
        sh_swell_until = (int)(SR * (8.0f + rr * 12.0f));
    }
    sh_swell += 6.0e-6f * (sh_swell_tgt - sh_swell);

    /* control-rate cutoff drift — the warm body opens slightly on the swell. */
    if ((sh_ctrl++ & 15u) == 0) {
        float fc = 150.0f + sh_swell * 60.0f;
        dsp_svf_set(&sh_lpL, fc,          1.3f);
        dsp_svf_set(&sh_lpR, fc * 1.075f, 1.3f);
    }

    /* decorrelated brown noise → warm low SVF, scaled by the swell. */
    sh_brnL = sh_brnL * 0.996f + sh_white(&sh_rng_L) * 0.04f;
    sh_brnR = sh_brnR * 0.996f + sh_white(&sh_rng_R) * 0.04f;
    float L = dsp_svf_lp(&sh_lpL, sh_brnL) * sh_swell;
    float R = dsp_svf_lp(&sh_lpR, sh_brnR) * sh_swell;

    *outL += L * 0.85f;
    *outR += R * 0.85f;
}

/* ===========================================================================
 * Fjord water (r19.54) — Fjords only. Deep, cold, still water in a narrow rock
 * channel: a dark low murmur (brown → very low SVF, slow swell) + sparse deep
 * drips/laps against rock (low-mid resonant plonks). Vertical, grounded, cold —
 * the opposite of Open Sea's warm open swell.
 * =========================================================================== */

static uint32_t  fj_rng_L = 0x3C6EF35Fu, fj_rng_R = 0x9E3779B1u;
static float     fj_brnL = 0.0f, fj_brnR = 0.0f;
static dsp_svf_t fj_lpL, fj_lpR;
static float     fj_swell = 0.4f, fj_swell_tgt = 0.7f;
static int       fj_swell_until = 0;
static dsp_svf_t fj_dripbp;
static float     fj_drip_env = 0.0f, fj_drip_decay = 0.0f;
static int       fj_drip_side = 0, fj_until_drip = 0;

static inline float fj_white(uint32_t *r){ *r=(*r)*1664525u+1013904223u; return (float)((int32_t)*r)*(1.0f/2147483648.0f); }

static void fjord_reset(void){
    dsp_svf_reset(&fj_lpL); dsp_svf_set(&fj_lpL, 110.0f, 1.2f);
    dsp_svf_reset(&fj_lpR); dsp_svf_set(&fj_lpR, 118.0f, 1.2f);
    dsp_svf_reset(&fj_dripbp); dsp_svf_set(&fj_dripbp, 380.0f, 1.2f);
    fj_brnL = fj_brnR = 0.0f;
    fj_swell = 0.4f; fj_swell_tgt = 0.7f; fj_swell_until = (int)(SR*8.0f);
    fj_drip_env = 0.0f; fj_drip_decay = 0.0f; fj_until_drip = (int)(SR*2.0f);
}

static inline void fjord_tick(float *outL, float *outR){
    /* slow cold swell */
    if (--fj_swell_until <= 0){
        float r = fj_white(&fj_rng_L)*0.5f+0.5f;
        fj_swell_tgt = 0.04f + r*r*0.81f;
        fj_swell_until = (int)(SR*(9.0f + (fj_white(&fj_rng_R)*0.5f+0.5f)*10.0f));
    }
    fj_swell += 6.0e-6f*(fj_swell_tgt-fj_swell);
    fj_brnL = fj_brnL*0.997f + fj_white(&fj_rng_L)*0.03f;
    fj_brnR = fj_brnR*0.997f + fj_white(&fj_rng_R)*0.03f;
    float L = dsp_svf_lp(&fj_lpL, fj_brnL)*fj_swell;
    float R = dsp_svf_lp(&fj_lpR, fj_brnR)*fj_swell;

    /* sparse deep drips against rock */
    if (--fj_until_drip <= 0){
        fj_drip_env = 0.5f + (fj_white(&fj_rng_L)*0.5f+0.5f)*0.5f;
        float fc = 260.0f + (fj_white(&fj_rng_R)*0.5f+0.5f)*380.0f;
        dsp_svf_set(&fj_dripbp, fc, 1.2f);
        fj_drip_decay = expf(-1.0f/(0.10f*SR));
        fj_drip_side = (fj_white(&fj_rng_L) > 0.0f);
        fj_until_drip = (int)(SR*(3.0f + (fj_white(&fj_rng_R)*0.5f+0.5f)*5.0f));
    }
    if (fj_drip_env > 0.001f){
        float d = dsp_svf_bp(&fj_dripbp, fj_white(&fj_rng_L)) * fj_drip_env;
        fj_drip_env *= fj_drip_decay;
        if (fj_drip_side) R += d*0.7f; else L += d*0.7f;
    }
    *outL += L*0.9f;  *outR += R*0.9f;
}

/* ===========================================================================
 * Desert heat (r19.54) — Desert only. Dry stillness: a faint high-mid HEAT
 * HAZE (thin band of noise wavering slowly, like shimmering air over stone) +
 * very sparse dry SAND grains. Mostly quiet — the desert is the silence.
 * =========================================================================== */

static uint32_t  ds_rng = 0x0DEFACE5u;
static dsp_svf_t ds_hazeL, ds_hazeR;
static float ds_haze_gain=0.0f, ds_haze_target=0.0f;
static int ds_haze_until=0;
static float     ds_tick_env = 0.0f;
static int       ds_until_tick = 0, ds_tick_side = 0;

static inline float ds_white(void){ ds_rng=ds_rng*1664525u+1013904223u; return (float)((int32_t)ds_rng)*(1.0f/2147483648.0f); }

static void desert_reset(void){
    dsp_svf_reset(&ds_hazeL); dsp_svf_set(&ds_hazeL, 1750.0f, 0.707f);
    dsp_svf_reset(&ds_hazeR); dsp_svf_set(&ds_hazeR, 1880.0f, 0.707f);
    ds_haze_gain=ds_haze_target=0.0f; ds_haze_until=0; ds_tick_env = 0.0f; ds_until_tick = (int)(SR*1.5f);
}

static inline void desert_tick(float *outL, float *outR){
    /* heat haze — thin wavering high-mid band, very quiet + wide */
    if (--ds_haze_until<=0) {
        float r=ds_white()*0.5f+0.5f;
        ds_haze_target=r<0.6f ? 0.0f : r*r;
        ds_haze_until=(int)(SR*(3.0f+(ds_white()*0.5f+0.5f)*13.0f));
    }
    ds_haze_gain+=0.00002f*(ds_haze_target-ds_haze_gain);
    float waver=ds_haze_gain;
    float hL = dsp_svf_bp(&ds_hazeL, ds_white()) * waver * 0.10f;
    float hR = dsp_svf_bp(&ds_hazeR, ds_white()) * waver * 0.10f;

    /* sparse dry sand grains */
    if (--ds_until_tick <= 0){
        ds_tick_env = 0.3f + (ds_white()*0.5f+0.5f)*0.4f;
        ds_tick_side = (ds_white() > 0.0f);
        ds_until_tick = (int)(SR*(0.8f + (ds_white()*0.5f+0.5f)*1.8f));
    }
    float tL=0.0f, tR=0.0f;
    if (ds_tick_env > 0.002f){
        float t = ds_white()*ds_tick_env;
        ds_tick_env *= 0.88f;
        if (ds_tick_side) tR = t; else tL = t;
    }
    *outL += hL + tL*0.35f;  *outR += hR + tR*0.35f;
}

/* ===========================================================================
 * Vinyl (Phase 2d, rebuilt r18.98) — After Hours only.
 *
 * r18.98 (user: "am Ende höre ich noch so starkes Rauschen"): the old
 * "crackle" was CONTINUOUS hi-passed white noise at 0.22 — a stationary
 * −56 dBFS HF carpet whenever ATMOS was up in After Hours, fully exposed
 * the moment the music faded (measured at the end of the played-session
 * demo). A real record between the pops is nearly SILENT — its surface
 * noise is dense tiny TICK EVENTS, not a noise bed. Constitution §2.
 *
 * Three sub-layers now:
 *   • ticks:  ~10..50/s single-grain ticks, τ ≈ 1 ms, random side
 *   • pops:   sparse louder bursts, ~50 ms half-life (unchanged)
 *   • rumble: brown-noise integrator — distant city through walls
 * =========================================================================== */

static uint32_t vy_rng = 0xC0DEFEEDu;
static float    vy_rumL = 0.0f, vy_rumR = 0.0f;
static int      vy_until_pop = 0;
static float    vy_pop_env = 0.0f;
static int      vy_until_tick = 0;
static float    vy_tick_env = 0.0f;
static float    vy_tick_sign = 1.0f;
static int      vy_tick_side = 0;

static inline float vy_white(void) {
    vy_rng = vy_rng * 1664525u + 1013904223u;
    return (float)((int32_t)vy_rng) * (1.0f / 2147483648.0f);
}

static void vinyl_reset(void) {
    vy_rumL = vy_rumR = 0.0f;
    vy_until_pop = 800;
    vy_pop_env = 0.0f;
    vy_until_tick = 400;
    vy_tick_env = 0.0f;
    vy_tick_sign = 1.0f;
    vy_tick_side = 0;
}

static inline void vinyl_tick(float *outL, float *outR) {
    /* surface ticks: schedule 20..90 ms apart, each a ~1 ms grain */
    if (--vy_until_tick <= 0) {
        vy_tick_env   = 0.25f + (vy_white() * 0.5f + 0.5f) * 0.75f;
        vy_tick_sign  = (vy_white() > 0.0f) ? 1.0f : -1.0f;
        vy_tick_side  = (vy_white() > 0.0f);
        vy_until_tick = (int)(SR * 0.020f) +
                        (int)((vy_white() * 0.5f + 0.5f) * SR * 0.070f);
    }
    float tickL = 0.0f, tickR = 0.0f;
    if (vy_tick_env > 0.002f) {
        float t = vy_white() * vy_tick_env * vy_tick_sign;
        vy_tick_env *= 0.90f;                 /* τ ≈ 1 ms — a tick, not hiss */
        if (vy_tick_side) tickR = t; else tickL = t;
    }

    /* sparse sharp pops */
    if (--vy_until_pop <= 0) {
        vy_pop_env = 0.5f + (vy_white() * 0.5f + 0.5f) * 0.4f;
        vy_until_pop = 800 + (int)((vy_white() * 0.5f + 0.5f) * 3000.0f);
    }
    float pop = 0.0f;
    if (vy_pop_env > 0.001f) {
        pop = vy_white() * vy_pop_env;
        vy_pop_env *= 0.975f;
    }

    /* slow rumble — distant city / sub */
    vy_rumL = vy_rumL * 0.9985f + vy_white() * 0.0008f;
    vy_rumR = vy_rumR * 0.9985f + vy_white() * 0.0008f;

    *outL += tickL * 0.6f + pop * 0.45f + vy_rumL * 1.5f;
    *outR += tickR * 0.6f - pop * 0.45f + vy_rumR * 1.5f;
}

/* ===========================================================================
 * Public API
 * =========================================================================== */

void ambience_init(void) {
    world_i   = 0;
    level_cur = 0.0f;
    level_tgt = 0.0f;
    wind_reset();
    rain_reset();
    waves_reset();
    seahum_reset();
    fjord_reset();
    desert_reset();
    vinyl_reset();
}

void ambience_set_world(int idx) {
    if (idx < 0) idx = 0;
    /* Upper-bound clamping is the caller's job (worlds_get clamps); we just
     * stash the index for the dispatch below. */
    world_i = idx;
}

void ambience_set_level(float v) {
    if (v < 0.0f) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    level_tgt = v;
}

void ambience_render_mix(float *dry_L, float *dry_R,
                         float *send_L, float *send_R,
                         int frames, float send_amount) {
    /* Sample-rate smoothing preserves timing across all supported blocks. */


    if (level_cur < SILENCE_EPS && level_tgt < SILENCE_EPS) return;

    /* r19.44: RAIN → Moss Fields (damp . fog), WAVES → Open Sea. Alps stays
     * clear air (wind only); Fjords/Desert are wind-only for now (fjord water
     * + desert heat-shimmer textures are a future addition). Vinyl retired —
     * no landscape is a 3am jazz bar. */
    const int do_rain   = (world_i == WORLD_MOSS);
    const int do_waves  = (world_i == WORLD_OPENSEA);
    const int do_fjord  = (world_i == WORLD_FJORDS);   /* r19.54 */
    const int do_desert = (world_i == WORLD_DESERT);   /* r19.54 */
    const int do_vinyl  = 0;

    /* r18.92 (user: "Grundrauschen zu praesent/zu dirty"): the macro used
     * to apply LINEARLY — ATMOS 0.35 already put a constant −36 dBFS noise
     * carpet under everything. Square-law with a 0.85 ceiling: the lower
     * half of the knob is a whisper (0.35 → −49 dBFS), full knob keeps its
     * drama. The atmosphere must sit BEHIND the music, never beside it. */


    for (int n = 0; n < frames; ++n) {
        level_cur += 0.0002834f * (level_tgt-level_cur); /* 80 ms */
        float lvl=level_cur*level_cur*0.85f;
        float L = 0.0f, R = 0.0f;
        wind_tick(&L, &R);
        if (do_rain)   rain_tick(&L, &R);
        if (do_waves)  { waves_tick(&L, &R); seahum_tick(&L, &R); }  /* r19.47 */
        if (do_fjord)  fjord_tick(&L, &R);                           /* r19.54 */
        if (do_desert) desert_tick(&L, &R);                          /* r19.54 */
        if (do_vinyl)  vinyl_tick(&L, &R);

        float outL = L * lvl;
        float outR = R * lvl;
        dry_L[n]  += outL;
        dry_R[n]  += outR;
        send_L[n] += outL * send_amount;
        send_R[n] += outR * send_amount;
    }
}
