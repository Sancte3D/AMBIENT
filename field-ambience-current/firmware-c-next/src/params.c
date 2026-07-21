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
/* r19.21 — Push-Zustaende (knobs.c). Bypass/Mute lassen den gemerkten
 * Wert stehen und schalten nur die Engine-Seite; Drehen hebt sie auf. */
static bool  s_drive_byp;
static bool  s_muted;

#define DRIVE_DEFAULT 0.15f   /* engine_init-Referenz (gentle warmth) */

static void apply_drive(void) {
    float d = s_drive_byp ? 0.0f : s_drive;
    engine_set_drive(d);
    engine_set_reverb_drive(0.10f + 0.45f * d);
}
static void apply_volume(void) {
    engine_set_master_volume(s_muted ? 0.0f : s_volume);
}

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
    s_volume = 0.30f;   /* r19.20: SPEC boot rule — max 30 % at power-on
                         * (headphone-safe since the r19.19 TPA6132A2; the
                         * old 0.60 predates the phones jack). The device
                         * boot additionally starts hard-muted and fades in
                         * (engine_boot_mute + this target). */
    s_drive_byp = false;
    s_muted     = false;
    /* r18.89: DRIVE = master drive stage + a slaved touch of reverb-input
     * drive (the space growls along, ~half a knob behind). Before this the
     * encoder ONLY drove the reverb input — nearly inaudible on dry sounds. */
    apply_drive();
    engine_set_brightness(s_bright);
    apply_volume();
}

void params_encoder(uint8_t enc_id, int delta, uint32_t now_ms) {
    if (delta == 0) return;
    int dir = delta > 0 ? 1 : -1;
    int n   = delta > 0 ? delta : -delta;
    int ticks = n * accel_mul(enc_id, now_ms);   /* accelerated detent count */

    switch (enc_id) {
        case PARAM_ENC_DRIVE:
            s_drive_byp = false;               /* r19.21: der Knopf nimmt den Wert */
            s_drive = clampf(s_drive + dir * ticks * 0.01f, 0.0f, 1.0f);
            apply_drive();
            break;
        case PARAM_ENC_BRIGHT:
            s_bright = clampf(s_bright + dir * ticks * BRIGHT_STEP_HZ,
                              BRIGHT_MIN_HZ, BRIGHT_MAX_HZ);
            engine_set_brightness(s_bright);
            break;
        case PARAM_ENC_VOLUME:
            s_muted = false;                   /* r19.21: Drehen entstummt */
            s_volume = clampf(s_volume + dir * ticks * 0.01f, 0.0f, 1.0f);
            apply_volume();
            break;
        case PARAM_ENC_DISPLAY:    /* menu owns this encoder — ignore here */
        default:
            break;
    }
}

/* ---- r19.21: Push-Aktionen (knobs.c) ------------------------------------ */
int  params_toggle_drive_bypass(void) {
    s_drive_byp = !s_drive_byp;
    apply_drive();
    return s_drive_byp ? 1 : 0;
}
bool params_drive_bypassed(void) { return s_drive_byp; }
void params_reset_drive(void) {
    s_drive_byp = false;
    s_drive = DRIVE_DEFAULT;
    apply_drive();
}
void params_set_bright_neutral(void) { s_bright = 0.0f; engine_set_brightness(0.0f); }
void params_reset_bright(void)       { params_set_bright_neutral(); }
int  params_toggle_mute(void) {
    s_muted = !s_muted;
    apply_volume();
    return s_muted ? 1 : 0;
}
bool params_muted(void) { return s_muted; }
void params_apply_scene(int drive_pct, float bright_hz) {
    s_drive_byp = false;
    s_drive  = clampf((float)drive_pct / 100.0f, 0.0f, 1.0f);
    s_bright = clampf(bright_hz, BRIGHT_MIN_HZ, BRIGHT_MAX_HZ);
    apply_drive();
    engine_set_brightness(s_bright);
}

int   params_drive_pct(void)  { return (int)(s_drive  * 100.0f + 0.5f); }
float params_bright_hz(void)  { return s_bright; }
int   params_volume_pct(void) { return (int)(s_volume * 100.0f + 0.5f); }
