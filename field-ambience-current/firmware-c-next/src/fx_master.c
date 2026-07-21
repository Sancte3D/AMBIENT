/*
 * fx_master.c — see fx_master.h. This translation unit deliberately owns
 * every large effects buffer: on the H743 the linker places this object's
 * .bss into RAM_D2 (stm32h743_flash.ld .d2_bss), reclaiming the region the
 * legacy echo (207 KB) + blur (69 KB) delay lines used to occupy. Confirm
 * placement in the .map file after any change here (INTEGRATION_TASK.md).
 */
#include "fx_master.h"
#include "ambient_effects.h"
#include "audio.h"

#include <stddef.h>

/* Storage + hot arena. The budget constant is the engine's documented upper
 * bound (240 KB); the actual requirement for the default config is 214,489 B
 * and is asserted by ambient_fx_init returning non-NULL. 32-byte alignment
 * satisfies the engine's max_align_t needs with cache-line margin. */
static AmbientFxStorage s_storage;
static unsigned char s_arena[AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES]
    __attribute__((aligned(32)));
static float s_buf[AUDIO_BUFFER_FRAMES * 2];   /* interleave scratch */

static AmbientFx           *s_fx = 0;
static AmbientFxParameters  s_params;
static bool                 s_ok = false;

void fx_master_init(void) {
    AmbientFxConfig cfg = ambient_fx_default_config();
    s_fx = ambient_fx_init(&s_storage, s_arena, sizeof s_arena, &cfg);
    if (!s_fx) { s_ok = false; return; }           /* fail closed: dry */
    s_params = ambient_fx_world_parameters(AMBIENT_FX_TOKYO_CITY);
    ambient_fx_set_parameters(s_fx, s_params);
    ambient_fx_set_mode(s_fx, AMBIENT_FX_DREAM_CHAIN);
    s_ok = true;
}

bool fx_master_ok(void) { return s_ok; }

void fx_master_process(float *outL, float *outR, int frames) {
    if (!s_ok || frames <= 0) return;              /* dry pass-through */
    if (frames > AUDIO_BUFFER_FRAMES) frames = AUDIO_BUFFER_FRAMES;
    for (int n = 0; n < frames; ++n) {
        s_buf[2 * n]     = outL[n];
        s_buf[2 * n + 1] = outR[n];
    }
    ambient_fx_process_f32(s_fx, s_buf, (size_t)frames);
    for (int n = 0; n < frames; ++n) {
        outL[n] = s_buf[2 * n];
        outR[n] = s_buf[2 * n + 1];
    }
}

static float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

static void push(void) {
    if (s_ok) ambient_fx_set_parameters(s_fx, s_params);
}

void fx_master_set_space(float v)      { s_params.space      = clamp01(v); push(); }
void fx_master_set_atmosphere(float v) { s_params.atmosphere = clamp01(v); push(); }
void fx_master_set_echo(float v)       { s_params.echo       = clamp01(v); push(); }
void fx_master_set_motion(float v)     { s_params.motion     = clamp01(v); push(); }
void fx_master_set_age(float v)        { s_params.age        = clamp01(v); push(); }
void fx_master_set_shimmer(float v)    { s_params.shimmer    = clamp01(v); push(); }
void fx_master_set_blur(float v)       { s_params.blur       = clamp01(v); push(); }
void fx_master_set_tone(float v)       { s_params.tone       = clamp01(v); push(); }

void fx_master_set_world(int idx) {
    if (!s_ok) return;
    if (idx < 0 || idx >= (int)AMBIENT_FX_WORLD_COUNT) idx = 0;
    AmbientFxWorld w = (AmbientFxWorld)idx;
    /* Engine voicing first, then the parameter set for that world. The
     * product's world load pushes its own macro values right after this
     * (menu load_world_preset), overwriting the user-facing fields — same
     * ordering as a manual world change. width/tone/level/delay keep the
     * engine's curated per-world values. */
    ambient_fx_set_world(s_fx, w);
    s_params = ambient_fx_world_parameters(w);
    push();
}

void fx_master_set_mode(int idx) {
    if (!s_ok) return;
    if (idx < 0 || idx >= (int)AMBIENT_FX_MODE_COUNT) return;
    ambient_fx_set_mode(s_fx, (AmbientFxMode)idx);
}

int fx_master_mode(void) {
    return s_ok ? (int)ambient_fx_mode(s_fx) : 0;
}

const char *fx_master_mode_name(int idx) {
    return ambient_fx_mode_name((AmbientFxMode)idx);
}

int fx_master_mode_count(void) { return (int)AMBIENT_FX_MODE_COUNT; }

void fx_master_trigger_swell(float freq_hz, float level_0_1,
                             float lead_seconds) {
    if (s_ok) ambient_fx_trigger_reverse_swell(s_fx, freq_hz, level_0_1,
                                               lead_seconds);
}
