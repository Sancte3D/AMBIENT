/*
 * synth_host.c — the swappable sound-core host. See synth_host.h.
 *
 * Signal path (global, identical for every engine):
 *   clear dry/send → active->render_mix(dry, send) → reverb_render(send→wet)
 *   → dry + wet*wet_amp → master → beauty_guard limiter → int16
 *
 * The host is allocation-free and block-chunked internally so a caller can
 * ask for any frame count. It does NOT touch engine_v2 (the FIELD engine).
 */
#include "v2/synth_host.h"
#include "v2/beauty_guard.h"
#include "reverb.h"
#include "dsp.h"
#include <math.h>
#include <string.h>

/* Each engine is a const vtable exported from its own .c file. */
extern const synth_engine_t engine_acid;
extern const synth_engine_t engine_fm_glass;
extern const synth_engine_t engine_chorus_mist;
extern const synth_engine_t engine_ion_storm;
extern const synth_engine_t engine_glass_orbit;
extern const synth_engine_t engine_bamboo_circuit;

static const synth_engine_t *const TABLE[SYNTH_COUNT] = {
    [SYNTH_ACID]           = &engine_acid,
    [SYNTH_FM_GLASS]       = &engine_fm_glass,
    [SYNTH_CHORUS_MIST]    = &engine_chorus_mist,
    [SYNTH_ION_STORM]      = &engine_ion_storm,
    [SYNTH_GLASS_ORBIT]    = &engine_glass_orbit,
    [SYNTH_BAMBOO_CIRCUIT] = &engine_bamboo_circuit,
};

#define HBLOCK 256

static struct {
    const synth_engine_t *active;
    synth_id_t            active_id;
    beauty_guard_t        guard;
    float wet_target, wet_amp;
    float master;
} H;

static float dL[HBLOCK], dR[HBLOCK], sL[HBLOCK], sR[HBLOCK], wL[HBLOCK], wR[HBLOCK];

static int16_t to_i16(float x) {
    if (x >  1.0f) x =  1.0f;
    if (x < -1.0f) x = -1.0f;
    return (int16_t)lrintf(x * 32767.0f);
}

void synth_host_init(void) {
    memset(&H, 0, sizeof H);
    reverb_init();
    reverb_set(0.5f, 0.4f);
    reverb_set_drive(0.10f);
    bg_init(&H.guard);
    H.wet_target = 0.25f;
    H.wet_amp    = 0.0f;
    H.master     = 0.9f;
    for (int i = 0; i < SYNTH_COUNT; ++i)
        if (TABLE[i] && TABLE[i]->init) TABLE[i]->init();
    H.active_id = SYNTH_ACID;
    H.active    = TABLE[SYNTH_ACID];
    if (H.active && H.active->activate) H.active->activate();
}

void synth_host_select(synth_id_t id) {
    if (id < 0 || id >= SYNTH_COUNT || !TABLE[id]) return;
    if (H.active && H.active->deactivate) H.active->deactivate();
    H.active    = TABLE[id];
    H.active_id = id;
    if (H.active->activate) H.active->activate();
}

synth_id_t  synth_host_active(void)      { return H.active_id; }
const char *synth_host_active_name(void) { return H.active ? H.active->name : "-"; }

void synth_host_note_on(int midi, float vel) { if (H.active && H.active->note_on)  H.active->note_on(midi, vel); }
void synth_host_note_off(void)               { if (H.active && H.active->note_off) H.active->note_off(); }
void synth_host_set_param(synth_param_t p, float v) { if (H.active && H.active->set_param) H.active->set_param(p, v); }
void synth_host_panic(void)                  { if (H.active && H.active->panic)    H.active->panic(); }

void synth_host_set_reverb(float size, float wet) {
    reverb_set(dsp_clampf(size, 0.0f, 1.0f), 0.4f);
    H.wet_target = dsp_clampf(wet, 0.0f, 1.0f);
}
void synth_host_set_master(float v) { H.master = dsp_clampf(v, 0.0f, 1.0f); }

void synth_host_render(int16_t *out, int frames) {
    int done = 0;
    while (done < frames) {
        int n = frames - done; if (n > HBLOCK) n = HBLOCK;

        memset(dL, 0, sizeof(float) * n); memset(dR, 0, sizeof(float) * n);
        memset(sL, 0, sizeof(float) * n); memset(sR, 0, sizeof(float) * n);

        if (H.active && H.active->render_mix) H.active->render_mix(dL, dR, sL, sR, n);

        reverb_render(sL, sR, wL, wR, n);

        /* wet amp glides in (~60 ms) so a select() / wet change never zippers */
        float wc = 1.0f - expf(-1.0f / (0.060f * (float)DSP_SAMPLE_RATE_HZ / (float)n));
        H.wet_amp += wc * (H.wet_target - H.wet_amp);

        for (int i = 0; i < n; ++i) {
            dL[i] = (dL[i] + wL[i] * H.wet_amp) * H.master;
            dR[i] = (dR[i] + wR[i] * H.wet_amp) * H.master;
        }

        bg_process(&H.guard, dL, dR, n);

        for (int i = 0; i < n; ++i) {
            out[2 * (done + i) + 0] = to_i16(dL[i]);
            out[2 * (done + i) + 1] = to_i16(dR[i]);
        }
        done += n;
    }
}
