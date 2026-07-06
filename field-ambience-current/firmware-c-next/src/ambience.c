/*
 * ambience.c — per-world atmospheric layer (ADR-0017 Phase 2a..d).
 *
 * Generators run inside engine_render() between texture and bass:
 *   • WIND    — universal, every world. Pink-BP swept 350..900 Hz / 14 s
 *               + random gust envelopes.
 *   • RAIN    — Tokyo only (world 0). Pink-BP "sshhh" + pool of 12
 *               noise-burst drops at 1.5..4.5 kHz, 15..40 ms decay.
 *   • WAVES   — Crystal Coast only (world 1). Asymmetric envelope
 *               (1.2..2 s attack, 5..9 s decay, 1..4 s gap). LP'd brown
 *               body + HF pink-BP splash gated to crest.
 *   • VINYL   — After Hours only (world 3). Hi-pass noise crackle +
 *               sparse sharp pops every ~0.02..0.08 s + slow LP'd brown
 *               rumble (distant city through walls).
 *
 * Midnight Drive (world 2) intentionally gets only WIND today — the
 * highway feel comes from wind sweep; distant-traffic generator can be
 * added later without changing the API.
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
#define LEVEL_COEF    0.04f       /* per-block one-pole, ~50 ms */

/* World index keys per-world dispatch. Index meaning matches worlds.c:
 *   0 = Tokyo City, 1 = Crystal Coast, 2 = Midnight Drive, 3 = After Hours. */
#define WORLD_TOKYO   0
#define WORLD_COAST   1
#define WORLD_HOURS   3

static int   world_i = 0;
static float level_cur = 0.0f, level_tgt = 0.0f;

/* ===========================================================================
 * Wind — universal, runs for every world.
 *
 * r18.97 (user: "echter simulierter Wind wäre 100x geiler — nicht als
 * Rauschen, sondern als realistischer Sound"): the old wind never stopped —
 * a 0.40 gust floor meant a constant band-passed noise carpet under every
 * world. Real wind is intermittent. Three perceptual cues (principles from
 * the standard procedural-audio treatment of wind — Farnell's "Designing
 * Sound" analysis — studied and reinvented, no code copied):
 *   1. GUSTS with asymmetric slew: intensity glides up fast (~1.5 s) and
 *      dies slowly (~4 s), separated by long, nearly silent lulls;
 *   2. the spectral centre RISES with intensity — strong wind is brighter;
 *   3. strong gusts grow narrow WHISTLES (wires/edges resonating) that
 *      wander slowly in pitch.
 * The squared gate makes lulls truly quiet — the silence between gusts is
 * what makes it read as weather instead of noise.
 * =========================================================================== */

static uint32_t wnd_rng_L = 0xACE12345u, wnd_rng_R = 0x7B19F88Au;
static dsp_svf_t wnd_bpL, wnd_bpR;
static dsp_svf_t wnd_whL, wnd_whR;          /* gust whistles (high-Q) */
static float wnd_lfo = 0.0f;
static float wnd_pink_L_b0 = 0, wnd_pink_L_b1 = 0, wnd_pink_L_b2 = 0;
static float wnd_pink_R_b0 = 0, wnd_pink_R_b1 = 0, wnd_pink_R_b2 = 0;
static float wnd_gust_env  = 0.10f;         /* glides toward wnd_gust_tgt */
static float wnd_gust_tgt  = 0.60f;
static int   wnd_gust_until = 0;
static float wnd_wh_fc  = 700.0f;           /* whistle centre, wanders */
static float wnd_wh_tgt = 700.0f;
static int      wnd_wh_until = 0;           /* counted in control ticks */
static uint32_t wnd_ctrl = 0;               /* ÷16 control-rate divider */

static inline float wnd_white(uint32_t *r) {
    *r = (*r) * 1664525u + 1013904223u;
    return (float)((int32_t)*r) * (1.0f / 2147483648.0f);
}

/* 3-pole pink-noise filter (Paul Kellet approximation). */
static inline float wnd_pink(uint32_t *rng, float *b0, float *b1, float *b2) {
    float w = wnd_white(rng);
    *b0 = 0.99765f * (*b0) + w * 0.0990460f;
    *b1 = 0.96300f * (*b1) + w * 0.2965164f;
    *b2 = 0.57000f * (*b2) + w * 1.0526913f;
    return (*b0 + *b1 + *b2 + w * 0.1848f) * 0.18f;
}

static void wind_reset(void) {
    dsp_svf_reset(&wnd_bpL);
    dsp_svf_reset(&wnd_bpR);
    dsp_svf_reset(&wnd_whL);
    dsp_svf_reset(&wnd_whR);
    wnd_lfo        = 0.0f;
    /* Boot INTO a rising gust so the layer is audible right away; the
     * first scheduled retarget (3 s in) may then drop into a lull. */
    wnd_gust_env   = 0.10f;
    wnd_gust_tgt   = 0.60f;
    wnd_gust_until = (int)(SR * 3.0f);
    wnd_wh_fc = wnd_wh_tgt = 700.0f;
    wnd_wh_until   = 0;
    wnd_ctrl       = 0;
    wnd_pink_L_b0 = wnd_pink_L_b1 = wnd_pink_L_b2 = 0.0f;
    wnd_pink_R_b0 = wnd_pink_R_b1 = wnd_pink_R_b2 = 0.0f;
}

/* Add one sample of wind to the (outL, outR) accumulators. Caller owns the
 * level + send routing. */
static inline void wind_tick(float *outL, float *outR) {
    /* Gust scheduler: pick a new intensity target every 3..9 s. 35 % of
     * the picks are LULLS (0.03..0.23) — real wind spends much of its
     * time nearly silent, and that silence is what sells the gusts. */
    if (--wnd_gust_until <= 0) {
        float r = wnd_white(&wnd_rng_L) * 0.5f + 0.5f;
        if (r < 0.35f)
            wnd_gust_tgt = 0.03f + r * (0.20f / 0.35f);
        else
            wnd_gust_tgt = 0.45f + (r - 0.35f) * (0.60f / 0.65f);
        float rr = wnd_white(&wnd_rng_R) * 0.5f + 0.5f;
        wnd_gust_until = (int)(SR * (3.0f + rr * 6.0f));
    }
    /* Asymmetric glide: rise ~1.5 s, fall ~4 s — wind builds faster
     * than it dies. */
    float coef = (wnd_gust_tgt > wnd_gust_env) ? 1.5e-5f : 5.7e-6f;
    wnd_gust_env += coef * (wnd_gust_tgt - wnd_gust_env);
    /* Squared gate: lulls are ~silent, gusts keep their drama. The tiny
     * floor keeps the layer from switching hard in/out. */
    float gust = wnd_gust_env * wnd_gust_env + 0.015f;

    /* Control-rate (÷16) filter moves: the body centre follows the slow
     * 14 s sweep AND the gust intensity (cue 2); the whistle wanders. */
    if ((wnd_ctrl++ & 15u) == 0) {
        wnd_lfo += 16.0f * (1.0f / 14.0f) / SR;
        if (wnd_lfo >= 1.0f) wnd_lfo -= 1.0f;
        float s = dsp_sin(wnd_lfo);
        float centre = 450.0f + s * 180.0f + wnd_gust_env * 500.0f;
        dsp_svf_set(&wnd_bpL, centre,         1.8f);
        dsp_svf_set(&wnd_bpR, centre * 1.07f, 1.8f);

        if (--wnd_wh_until <= 0) {
            wnd_wh_tgt   = 400.0f + (wnd_white(&wnd_rng_R) * 0.5f + 0.5f) * 700.0f;
            wnd_wh_until = (int)((SR / 16.0f) * 10.0f);
        }
        wnd_wh_fc += 0.002f * (wnd_wh_tgt - wnd_wh_fc);
        dsp_svf_set(&wnd_whL, wnd_wh_fc,          18.0f);
        dsp_svf_set(&wnd_whR, wnd_wh_fc * 1.013f, 18.0f);
    }

    float pL = wnd_pink(&wnd_rng_L, &wnd_pink_L_b0, &wnd_pink_L_b1, &wnd_pink_L_b2);
    float pR = wnd_pink(&wnd_rng_R, &wnd_pink_R_b0, &wnd_pink_R_b1, &wnd_pink_R_b2);
    float L = dsp_svf_bp(&wnd_bpL, pL) * gust;
    float R = dsp_svf_bp(&wnd_bpR, pR) * gust;

    /* Whistle only in strong gusts — the "singing wires" cue (3). */
    if (wnd_gust_env > 0.6f) {
        float wg = (wnd_gust_env - 0.6f) * 0.38f;
        L += dsp_svf_bp(&wnd_whL, pL) * wg;
        R += dsp_svf_bp(&wnd_whR, pR) * wg;
    }
    *outL += L;
    *outR += R;
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
}

static inline void rain_tick(float *outL, float *outR) {
    /* background sshhh */
    float pL = rain_pink(&rain_pink_b0L, &rain_pink_b1L, &rain_pink_b2L);
    float pR = rain_pink(&rain_pink_b0R, &rain_pink_b1R, &rain_pink_b2R);
    /* r18.97: the bg shh was the loudest stationary noise left after the
     * hiss/PADsynth fixes — the DROPS carry the rain image, the wash only
     * glues them. 0.45 → 0.18. */
    float bgL = dsp_svf_bp(&rain_bg_bpL, pL) * 0.18f;
    float bgR = dsp_svf_bp(&rain_bg_bpR, pR) * 0.18f;

    /* schedule a new drop */
    if (--rain_until_next <= 0) {
        for (int d = 0; d < RAIN_MAX_DROPS; ++d) {
            if (!rain_drops[d].active) {
                rain_drops[d].active = 1;
                rain_drops[d].env    = 0.4f + (rain_white() * 0.5f + 0.5f) * 0.35f;
                float fc   = 1500.0f + (rain_white() * 0.5f + 0.5f) * 3000.0f;
                float dec  = 0.015f + (rain_white() * 0.5f + 0.5f) * 0.025f;
                dsp_svf_set(&rain_drops[d].bp, fc, 4.0f);
                rain_drops[d].decay = expf(-1.0f / (dec * SR));
                break;
            }
        }
        float interval = 0.030f + (rain_white() * 0.5f + 0.5f) * 0.150f;
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
        float wash_fc = 500.0f + rec * 2100.0f;
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
     * crest than the old fixed 400 Hz LP — 1.8 peaked past full scale */
    *outL += bodyL * 1.35f + splashL * 0.5f + sprayL * 0.6f;
    *outR += bodyR * 1.35f + splashR * 0.5f + sprayR * 0.6f;
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
    /* Smooth target → current at block rate so macro changes don't zipper. */
    level_cur += LEVEL_COEF * (level_tgt - level_cur);

    if (level_cur < SILENCE_EPS && level_tgt < SILENCE_EPS) return;

    const int do_rain  = (world_i == WORLD_TOKYO);
    const int do_waves = (world_i == WORLD_COAST);
    const int do_vinyl = (world_i == WORLD_HOURS);

    /* r18.92 (user: "Grundrauschen zu praesent/zu dirty"): the macro used
     * to apply LINEARLY — ATMOS 0.35 already put a constant −36 dBFS noise
     * carpet under everything. Square-law with a 0.85 ceiling: the lower
     * half of the knob is a whisper (0.35 → −49 dBFS), full knob keeps its
     * drama. The atmosphere must sit BEHIND the music, never beside it. */
    const float lvl = level_cur * level_cur * 0.85f;

    for (int n = 0; n < frames; ++n) {
        float L = 0.0f, R = 0.0f;
        wind_tick(&L, &R);
        if (do_rain)  rain_tick(&L, &R);
        if (do_waves) waves_tick(&L, &R);
        if (do_vinyl) vinyl_tick(&L, &R);

        float outL = L * lvl;
        float outR = R * lvl;
        dry_L[n]  += outL;
        dry_R[n]  += outR;
        send_L[n] += outL * send_amount;
        send_R[n] += outR * send_amount;
    }
}
