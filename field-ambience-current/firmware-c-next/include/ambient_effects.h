#ifndef SANCTE_AMBIENT_EFFECTS_H
#define SANCTE_AMBIENT_EFFECTS_H

/*
 * Original, allocation-free effects engine for the AMBIENT instrument.
 *
 * Product target:
 * - C11, STM32H743, 44.1 kHz stereo, arbitrary render block sizes.
 * - No allocation, locks, I/O, or unbounded work in the render path.
 * - Caller-owned state and caller-owned hot delay memory.
 * - Parameter targets are smoothed inside the audio path.
 *
 * The large arena must live in internal CPU-accessible SRAM. It is touched on
 * every sample and is therefore not appropriate for serial external memory.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMBIENT_FX_SAMPLE_RATE                 44100u
#define AMBIENT_FX_STATE_BYTES                  4096u
#define AMBIENT_FX_DEFAULT_MAX_DELAY_MS          750u
#define AMBIENT_FX_DEFAULT_BLUR_BUFFER_MS        220u
#define AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES 245760u
#define AMBIENT_FX_FDN_LINES                       8u

typedef enum AmbientFxMode {
    AMBIENT_FX_BYPASS = 0,
    AMBIENT_FX_DARK_REVERB,
    AMBIENT_FX_PING_PONG_DELAY,
    AMBIENT_FX_CHORUS_DETUNE,
    AMBIENT_FX_TAPE_AGE,
    AMBIENT_FX_REVERSE_SWELL,
    AMBIENT_FX_SHIMMER_REVERB,
    AMBIENT_FX_BLUR,
    AMBIENT_FX_DREAM_CHAIN,
    AMBIENT_FX_MODE_COUNT
} AmbientFxMode;

typedef enum AmbientFxWorld {
    AMBIENT_FX_TOKYO_CITY = 0,
    AMBIENT_FX_CRYSTAL_COAST,
    AMBIENT_FX_MIDNIGHT_DRIVE,
    AMBIENT_FX_AFTER_HOURS,
    AMBIENT_FX_WORLD_COUNT
} AmbientFxWorld;

typedef struct AmbientFxConfig {
    uint32_t sample_rate;
    uint16_t max_delay_ms;
    uint16_t blur_buffer_ms;
    uint32_t seed;
} AmbientFxConfig;

/*
 * Product-language controls. Every public value is clamped to a safe range.
 * The UI writes targets; rendering glides to them without zipper noise.
 */
typedef struct AmbientFxParameters {
    float space;         /* room scale / decay: 0 intimate .. 1 long hall */
    float atmosphere;    /* global spatial send: 0 dry .. 1 enveloping */
    float echo;          /* ping-pong amount and safe feedback */
    float motion;        /* chorus depth and slow modulation */
    float age;           /* wow, flutter, bandwidth loss, saturation, hiss */
    float shimmer;       /* restrained octave regeneration */
    float blur;          /* overlapping time-grain smear */
    float width;         /* stereo field */
    float tone;          /* 0 dark .. 1 open */
    float level;         /* final trim */
    float delay_seconds; /* world timing; clamped to the configured arena */
} AmbientFxParameters;

/* Fixed-size, correctly aligned storage for the private control state. */
typedef union AmbientFxStorage {
    max_align_t align;
    unsigned char bytes[AMBIENT_FX_STATE_BYTES];
} AmbientFxStorage;

typedef struct AmbientFx AmbientFx;

AmbientFxConfig ambient_fx_default_config(void);
AmbientFxParameters ambient_fx_world_parameters(AmbientFxWorld world);
const char *ambient_fx_mode_name(AmbientFxMode mode);
const char *ambient_fx_world_name(AmbientFxWorld world);

/* Exact caller-owned arena requirement for a configuration. */
size_t ambient_fx_memory_required(const AmbientFxConfig *config);
size_t ambient_fx_state_bytes(void);

/*
 * Initializes state in storage and partitions arena. Returns NULL when any
 * pointer/configuration is invalid or arena is too small. The function may be
 * called outside the audio callback; render functions never allocate.
 */
AmbientFx *ambient_fx_init(AmbientFxStorage *storage,
                           void *arena,
                           size_t arena_bytes,
                           const AmbientFxConfig *config);

void ambient_fx_reset(AmbientFx *fx);
void ambient_fx_set_mode(AmbientFx *fx, AmbientFxMode mode);
void ambient_fx_set_parameters(AmbientFx *fx, AmbientFxParameters parameters);
void ambient_fx_set_world(AmbientFx *fx, AmbientFxWorld world);

AmbientFxMode ambient_fx_mode(const AmbientFx *fx);
AmbientFxParameters ambient_fx_parameters(const AmbientFx *fx);
size_t ambient_fx_latency_frames(const AmbientFx *fx, AmbientFxMode mode);

/* In-place stereo processing. Float samples use the normal -1..+1 range. */
void ambient_fx_process_f32(AmbientFx *fx,
                            float *interleaved_stereo,
                            size_t frames);
void ambient_fx_process_i16(AmbientFx *fx,
                            int16_t *interleaved_stereo,
                            size_t frames);

/*
 * Live reverse gesture for notes known in advance by the generative composer.
 * Call this lead_seconds before the scheduled note. It creates a rising,
 * spectrally opening pre-tail without delaying the dry instrument.
 */
void ambient_fx_trigger_reverse_swell(AmbientFx *fx,
                                      float frequency_hz,
                                      float level_0_1,
                                      float lead_seconds);
bool ambient_fx_reverse_swell_active(const AmbientFx *fx);

/*
 * True reverse reverb for host rendering or an explicitly delayed look-ahead
 * path. Not for the DMA callback: runtime is proportional to the whole clip.
 *
 * output_frames must be at least input_frames + tail_frames. Input and output
 * must not overlap. The dry phrase is placed at tail_frames; the reversed wet
 * field rises before it. Returns frames written, or 0 on invalid input.
 */
size_t ambient_fx_reverse_reverb_offline_f32(AmbientFx *fx,
                                             const float *input_stereo,
                                             size_t input_frames,
                                             float *output_stereo,
                                             size_t output_frames,
                                             size_t tail_frames,
                                             float dry_gain,
                                             float wet_gain);

#ifdef __cplusplus
}
#endif

#endif
