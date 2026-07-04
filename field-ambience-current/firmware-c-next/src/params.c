/*
 * params.c — encoder → engine parameter bindings. See params.h.
 *
 * Acceleration tiers are the SPEC §5 / bench-tool ACCEL_TIERS, applied
 * per-encoder (each knob keeps its own last-event timestamp).
 */

#include "params.h"
#include "engine.h"

/* Tunables ---------------------------------------------------------------- */
#define BRIGHT_MIN_HZ  (-600.0f)   /* darkest pad cutoff offset */
#define BRIGHT_MAX_HZ  ( 800.0f)   /* brightest */
#define BRIGHT_STEP_HZ ( 20.0f)    /* per detent, before acceleration */
/* drive/volume move 1 %/detent before acceleration */

/* Per-encoder acceleration state (index by enc id 1..4). */
typedef struct { uint32_t last_ms; int first; } accel_t;
static accel_t s_acc[PARAM_ENC_VOLUME + 1];

/* Current values. */
static float s_drive;    /* 0..1 */
static float s_bright;   /* Hz */
static float s_volume;   /* 0..1 */

static int accel_mul(uint8_t id, uint32_t now) {
    static const struct { uint32_t max_dt; int mul; } T[] = {
        { 28, 8 }, { 60, 5 }, { 120, 3 }, { 240, 2 },
    };
    accel_t *a = &s_acc[id];
    uint32_t dt = a->first ? 0xFFFFFFFFu : (now - a->last_ms);
    a->last_ms = now; a->first = 0;
    int mul = 1;
    for (unsigned i = 0; i < sizeof T / sizeof T[0]; ++i)
        if (dt < T[i].max_dt) { mul = T[i].mul; break; }
    return mul;
}

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void params_init(void) {
    for (unsigned i = 0; i < sizeof s_acc / sizeof s_acc[0]; ++i) {
        s_acc[i].last_ms = 0; s_acc[i].first = 1;
    }
    /* Match engine_init() defaults so the readout is truthful at boot. */
    s_drive  = 0.15f;   /* gentle warmth by default */
    s_bright = 0.0f;    /* pad brightness offset default */
    s_volume = 0.60f;   /* engine master-volume default */
    /* r18.89: DRIVE = master drive stage + a slaved touch of reverb-input
     * drive (the space growls along, ~half a knob behind). Before this the
     * encoder ONLY drove the reverb input — nearly inaudible on dry sounds. */
    engine_set_drive(s_drive);
    engine_set_reverb_drive(0.10f + 0.45f * s_drive);
    engine_set_brightness(s_bright);
    engine_set_master_volume(s_volume);
}

void params_encoder(uint8_t enc_id, int delta, uint32_t now_ms) {
    if (delta == 0) return;
    int dir = delta > 0 ? 1 : -1;
    int n   = delta > 0 ? delta : -delta;
    int ticks = n * accel_mul(enc_id, now_ms);   /* accelerated detent count */

    switch (enc_id) {
        case PARAM_ENC_DRIVE:
            s_drive = clampf(s_drive + dir * ticks * 0.01f, 0.0f, 1.0f);
            engine_set_drive(s_drive);
            engine_set_reverb_drive(0.10f + 0.45f * s_drive);
            break;
        case PARAM_ENC_BRIGHT:
            s_bright = clampf(s_bright + dir * ticks * BRIGHT_STEP_HZ,
                              BRIGHT_MIN_HZ, BRIGHT_MAX_HZ);
            engine_set_brightness(s_bright);
            break;
        case PARAM_ENC_VOLUME:
            s_volume = clampf(s_volume + dir * ticks * 0.01f, 0.0f, 1.0f);
            engine_set_master_volume(s_volume);
            break;
        case PARAM_ENC_DISPLAY:    /* menu owns this encoder — ignore here */
        default:
            break;
    }
}

int   params_drive_pct(void)  { return (int)(s_drive  * 100.0f + 0.5f); }
float params_bright_hz(void)  { return s_bright; }
int   params_volume_pct(void) { return (int)(s_volume * 100.0f + 0.5f); }
