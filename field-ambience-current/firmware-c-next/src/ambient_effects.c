#include "ambient_effects.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#define FX_PI 3.14159265358979323846f
#define FX_TWO_PI 6.28318530717958647692f
#define FX_REVERB_MOD_MARGIN 24u

typedef struct FxStereoRing {
    int16_t *left;
    int16_t *right;
    uint32_t capacity;
    uint32_t write;
} FxStereoRing;

typedef struct FxFdnLine {
    int16_t *data;
    uint32_t capacity;
    uint32_t write;
    float base_delay;
    float damping;
    float feedback;
    float feedback_target;
} FxFdnLine;

typedef struct FxBlurGrain {
    float phase;
    float delay_frames;
} FxBlurGrain;

typedef struct FxReverseSwell {
    uint32_t remaining;
    uint32_t total;
    float frequency;
    float level;
    float phase;
    float filter_l;
    float filter_r;
} FxReverseSwell;

struct AmbientFx {
    AmbientFxConfig config;
    AmbientFxParameters current;
    AmbientFxParameters target;
    int16_t *arena;
    size_t arena_samples;

    FxStereoRing tape;
    FxStereoRing chorus;
    FxStereoRing delay;
    FxStereoRing blur;
    FxFdnLine fdn[AMBIENT_FX_FDN_LINES];
    int16_t *pitch;
    uint32_t pitch_capacity;
    uint32_t pitch_write;

    float tape_wow_phase;
    float tape_flutter_phase;
    float tape_hum_phase;
    float tape_drift;
    float tape_lp_l;
    float tape_lp_r;

    float chorus_phase_l;
    float chorus_phase_r;

    float delay_lp_l;
    float delay_lp_r;

    float reverb_hp_x_l;
    float reverb_hp_x_r;
    float reverb_hp_y_l;
    float reverb_hp_y_r;
    float reverb_mod_phase_l;
    float reverb_mod_phase_r;
    float reverb_wet_l;
    float reverb_wet_r;
    float shimmer_hp_x;
    float shimmer_hp_y;
    float shimmer_return;
    float pitch_phase;
    uint32_t reverb_control_counter;

    FxBlurGrain grains[2];
    float blur_lp_l;
    float blur_lp_r;

    FxReverseSwell reverse;

    float dc_x_l;
    float dc_x_r;
    float dc_y_l;
    float dc_y_r;

    uint32_t random_state;
    uint32_t initial_seed;
    uint32_t transition_position;
    uint32_t transition_length;
    AmbientFxMode mode;
    AmbientFxMode target_mode;
    uint8_t transition_switched;
    uint8_t initialized;
};

_Static_assert(sizeof(struct AmbientFx) <= AMBIENT_FX_STATE_BYTES,
               "AMBIENT_FX_STATE_BYTES is too small for AmbientFx");

static const uint16_t k_fdn_delays_44100[AMBIENT_FX_FDN_LINES] = {
    1117u, 1277u, 1429u, 1601u, 1789u, 1999u, 2203u, 2423u
};

static const char *const k_mode_names[AMBIENT_FX_MODE_COUNT] = {
    "BYPASS",
    "DARK REVERB",
    "PING-PONG DELAY",
    "CHORUS + DETUNE",
    "TAPE AGE",
    "REVERSE SWELL",
    "SHIMMER REVERB",
    "BLUR",
    "DREAM CHAIN"
};

static const char *const k_world_names[AMBIENT_FX_WORLD_COUNT] = {
    "TOKYO CITY", "CRYSTAL COAST", "MIDNIGHT DRIVE", "AFTER HOURS"
};

static float clampf(float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static uint32_t clampu32(uint32_t x, uint32_t lo, uint32_t hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static float finite_audio(float x)
{
    if (!(x == x)) {
        return 0.0f;
    }
    return clampf(x, -8.0f, 8.0f);
}

static int16_t float_to_i16(float x)
{
    x = clampf(x, -1.0f, 1.0f);
    return (int16_t)lrintf(x * 32767.0f);
}

static float i16_to_float(int16_t x)
{
    return (float)x * (1.0f / 32768.0f);
}

static float soft_clip(float x)
{
    x = clampf(x, -3.0f, 3.0f);
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

/* Phase is guaranteed to be inside 0..1 by advance_phase(). */
static float fast_sine(float phase)
{
    float x = phase * 2.0f - 1.0f;
    float y = 4.0f * x * (1.0f - fabsf(x));
    return y + 0.225f * (y * fabsf(y) - y);
}

static void advance_phase(float *phase, float increment)
{
    float p = *phase + increment;
    while (p >= 1.0f) p -= 1.0f;
    while (p < 0.0f) p += 1.0f;
    *phase = p;
}

static float smooth_window(float phase)
{
    return 4.0f * phase * (1.0f - phase);
}

static float one_pole_coefficient(float cutoff_hz, float sample_rate)
{
    float x = FX_TWO_PI * clampf(cutoff_hz, 1.0f, sample_rate * 0.45f) / sample_rate;
    return x / (1.0f + x);
}

static float exp_negative_approx(float x)
{
    x = clampf(x, 0.0f, 4.0f);
    float x2 = x * x;
    float x3 = x2 * x;
    return 1.0f / (1.0f + x + 0.48f * x2 + 0.235f * x3);
}

static uint32_t random_u32(AmbientFx *fx)
{
    uint32_t x = fx->random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    fx->random_state = x ? x : 0x9e3779b9u;
    return fx->random_state;
}

static float random_bipolar(AmbientFx *fx)
{
    return (float)((int32_t)(random_u32(fx) >> 9) - 4194304) *
           (1.0f / 4194304.0f);
}

static float read_mono_ring(const int16_t *data,
                            uint32_t capacity,
                            uint32_t write,
                            float delay_frames)
{
    float position = (float)write - delay_frames;
    while (position < 0.0f) position += (float)capacity;
    while (position >= (float)capacity) position -= (float)capacity;
    uint32_t i0 = (uint32_t)position;
    uint32_t i1 = i0 + 1u;
    if (i1 >= capacity) i1 = 0u;
    float fraction = position - (float)i0;
    float a = i16_to_float(data[i0]);
    float b = i16_to_float(data[i1]);
    return a + (b - a) * fraction;
}

static float read_stereo_ring(const FxStereoRing *ring,
                              unsigned channel,
                              float delay_frames)
{
    const int16_t *data = channel ? ring->right : ring->left;
    return read_mono_ring(data, ring->capacity, ring->write, delay_frames);
}

static void write_stereo_ring(FxStereoRing *ring, float left, float right)
{
    ring->left[ring->write] = float_to_i16(left);
    ring->right[ring->write] = float_to_i16(right);
    ++ring->write;
    if (ring->write >= ring->capacity) ring->write = 0u;
}

static uint32_t frames_from_ms(uint32_t sample_rate, uint32_t milliseconds)
{
    uint64_t frames = ((uint64_t)sample_rate * milliseconds + 999u) / 1000u;
    if (frames > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)frames;
}

static bool config_valid(const AmbientFxConfig *config)
{
    return config &&
           config->sample_rate == AMBIENT_FX_SAMPLE_RATE &&
           config->max_delay_ms >= 100u &&
           config->max_delay_ms <= 2000u &&
           config->blur_buffer_ms >= 80u &&
           config->blur_buffer_ms <= 500u;
}

AmbientFxConfig ambient_fx_default_config(void)
{
    AmbientFxConfig config;
    config.sample_rate = AMBIENT_FX_SAMPLE_RATE;
    config.max_delay_ms = AMBIENT_FX_DEFAULT_MAX_DELAY_MS;
    config.blur_buffer_ms = AMBIENT_FX_DEFAULT_BLUR_BUFFER_MS;
    config.seed = 0x7f4a7c15u;
    return config;
}

AmbientFxParameters ambient_fx_world_parameters(AmbientFxWorld world)
{
    static const AmbientFxParameters worlds[AMBIENT_FX_WORLD_COUNT] = {
        /* space atm  echo motion age shimmer blur width tone level delay */
        {0.62f, 0.54f, 0.38f, 0.32f, 0.38f, 0.05f, 0.12f, 0.78f, 0.34f, 0.84f, 0.430f},
        {0.82f, 0.68f, 0.34f, 0.46f, 0.10f, 0.18f, 0.22f, 0.94f, 0.72f, 0.82f, 0.570f},
        {0.56f, 0.50f, 0.52f, 0.40f, 0.58f, 0.04f, 0.08f, 0.88f, 0.30f, 0.86f, 0.360f},
        {0.88f, 0.72f, 0.44f, 0.30f, 0.66f, 0.07f, 0.18f, 0.84f, 0.26f, 0.80f, 0.500f}
    };
    if ((unsigned)world >= AMBIENT_FX_WORLD_COUNT) {
        world = AMBIENT_FX_TOKYO_CITY;
    }
    return worlds[world];
}

const char *ambient_fx_mode_name(AmbientFxMode mode)
{
    return (unsigned)mode < AMBIENT_FX_MODE_COUNT ? k_mode_names[mode] : "UNKNOWN";
}

const char *ambient_fx_world_name(AmbientFxWorld world)
{
    return (unsigned)world < AMBIENT_FX_WORLD_COUNT ? k_world_names[world] : "UNKNOWN";
}

static size_t arena_sample_count(const AmbientFxConfig *config)
{
    uint32_t sr = config->sample_rate;
    size_t count = 0u;
    count += (size_t)(frames_from_ms(sr, 14u) + 8u) * 2u; /* tape */
    count += (size_t)(frames_from_ms(sr, 40u) + 8u) * 2u; /* chorus */
    count += (size_t)(frames_from_ms(sr, config->max_delay_ms) + 8u) * 2u;
    count += (size_t)(frames_from_ms(sr, config->blur_buffer_ms) + 8u) * 2u;
    count += (size_t)(frames_from_ms(sr, 64u) + 8u);      /* pitch shifter */
    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        uint32_t scaled = (uint32_t)(((uint64_t)k_fdn_delays_44100[i] * sr + 22050u) / 44100u);
        count += (size_t)scaled + FX_REVERB_MOD_MARGIN;
    }
    return count;
}

size_t ambient_fx_memory_required(const AmbientFxConfig *config)
{
    if (!config_valid(config)) return 0u;
    return arena_sample_count(config) * sizeof(int16_t) + 3u;
}

size_t ambient_fx_state_bytes(void)
{
    return sizeof(struct AmbientFx);
}

static int16_t *take_samples(int16_t **cursor, size_t *remaining, size_t count)
{
    if (*remaining < count) return NULL;
    int16_t *result = *cursor;
    *cursor += count;
    *remaining -= count;
    return result;
}

static bool bind_stereo_ring(FxStereoRing *ring,
                             int16_t **cursor,
                             size_t *remaining,
                             uint32_t capacity)
{
    ring->left = take_samples(cursor, remaining, capacity);
    ring->right = take_samples(cursor, remaining, capacity);
    ring->capacity = capacity;
    ring->write = 0u;
    return ring->left && ring->right;
}

static AmbientFxParameters sanitize_parameters(const AmbientFx *fx,
                                                AmbientFxParameters p)
{
    p.space = clampf(p.space, 0.0f, 1.0f);
    p.atmosphere = clampf(p.atmosphere, 0.0f, 1.0f);
    p.echo = clampf(p.echo, 0.0f, 1.0f);
    p.motion = clampf(p.motion, 0.0f, 1.0f);
    p.age = clampf(p.age, 0.0f, 1.0f);
    p.shimmer = clampf(p.shimmer, 0.0f, 1.0f);
    p.blur = clampf(p.blur, 0.0f, 1.0f);
    p.width = clampf(p.width, 0.0f, 1.0f);
    p.tone = clampf(p.tone, 0.0f, 1.0f);
    p.level = clampf(p.level, 0.0f, 1.0f);
    float maximum = (float)(fx->delay.capacity - 4u) / (float)fx->config.sample_rate;
    p.delay_seconds = clampf(p.delay_seconds, 0.050f, maximum);
    return p;
}

AmbientFx *ambient_fx_init(AmbientFxStorage *storage,
                           void *arena,
                           size_t arena_bytes,
                           const AmbientFxConfig *config)
{
    if (!storage || !arena || !config_valid(config)) return NULL;
    size_t required = ambient_fx_memory_required(config);
    if (required == 0u || arena_bytes < required) return NULL;

    memset(storage->bytes, 0, sizeof(storage->bytes));
    AmbientFx *fx = (AmbientFx *)(void *)storage->bytes;
    fx->config = *config;
    fx->initial_seed = config->seed ? config->seed : 0x7f4a7c15u;

    uintptr_t raw = (uintptr_t)arena;
    uintptr_t aligned = (raw + 3u) & ~(uintptr_t)3u;
    size_t skipped = (size_t)(aligned - raw);
    if (arena_bytes < skipped) return NULL;
    int16_t *cursor = (int16_t *)(void *)aligned;
    size_t remaining = (arena_bytes - skipped) / sizeof(int16_t);
    fx->arena = cursor;
    fx->arena_samples = arena_sample_count(config);

    uint32_t sr = config->sample_rate;
    uint32_t tape_capacity = frames_from_ms(sr, 14u) + 8u;
    uint32_t chorus_capacity = frames_from_ms(sr, 40u) + 8u;
    uint32_t delay_capacity = frames_from_ms(sr, config->max_delay_ms) + 8u;
    uint32_t blur_capacity = frames_from_ms(sr, config->blur_buffer_ms) + 8u;
    fx->pitch_capacity = frames_from_ms(sr, 64u) + 8u;

    if (!bind_stereo_ring(&fx->tape, &cursor, &remaining, tape_capacity) ||
        !bind_stereo_ring(&fx->chorus, &cursor, &remaining, chorus_capacity) ||
        !bind_stereo_ring(&fx->delay, &cursor, &remaining, delay_capacity) ||
        !bind_stereo_ring(&fx->blur, &cursor, &remaining, blur_capacity)) {
        return NULL;
    }
    fx->pitch = take_samples(&cursor, &remaining, fx->pitch_capacity);
    if (!fx->pitch) return NULL;

    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        uint32_t base = (uint32_t)(((uint64_t)k_fdn_delays_44100[i] * sr + 22050u) / 44100u);
        fx->fdn[i].capacity = base + FX_REVERB_MOD_MARGIN;
        fx->fdn[i].base_delay = (float)base;
        fx->fdn[i].data = take_samples(&cursor, &remaining, fx->fdn[i].capacity);
        if (!fx->fdn[i].data) return NULL;
    }

    fx->initialized = 1u;
    ambient_fx_reset(fx);
    return fx;
}

static void reset_reverb(AmbientFx *fx)
{
    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        memset(fx->fdn[i].data, 0, (size_t)fx->fdn[i].capacity * sizeof(int16_t));
        fx->fdn[i].write = 0u;
        fx->fdn[i].damping = 0.0f;
        fx->fdn[i].feedback = 0.75f;
        fx->fdn[i].feedback_target = 0.75f;
    }
    memset(fx->pitch, 0, (size_t)fx->pitch_capacity * sizeof(int16_t));
    fx->pitch_write = 0u;
    fx->pitch_phase = 0.0f;
    fx->shimmer_return = 0.0f;
    fx->shimmer_hp_x = 0.0f;
    fx->shimmer_hp_y = 0.0f;
    fx->reverb_hp_x_l = 0.0f;
    fx->reverb_hp_x_r = 0.0f;
    fx->reverb_hp_y_l = 0.0f;
    fx->reverb_hp_y_r = 0.0f;
    fx->reverb_mod_phase_l = 0.0f;
    fx->reverb_mod_phase_r = 0.37f;
    fx->reverb_wet_l = 0.0f;
    fx->reverb_wet_r = 0.0f;
    fx->reverb_control_counter = 0u;
}

void ambient_fx_reset(AmbientFx *fx)
{
    if (!fx || !fx->initialized) return;
    memset(fx->arena, 0, fx->arena_samples * sizeof(int16_t));
    fx->tape.write = 0u;
    fx->chorus.write = 0u;
    fx->delay.write = 0u;
    fx->blur.write = 0u;
    fx->pitch_write = 0u;

    fx->tape_wow_phase = 0.0f;
    fx->tape_flutter_phase = 0.19f;
    fx->tape_hum_phase = 0.0f;
    fx->tape_drift = 0.0f;
    fx->tape_lp_l = 0.0f;
    fx->tape_lp_r = 0.0f;
    fx->chorus_phase_l = 0.0f;
    fx->chorus_phase_r = 0.31f;
    fx->delay_lp_l = 0.0f;
    fx->delay_lp_r = 0.0f;
    fx->blur_lp_l = 0.0f;
    fx->blur_lp_r = 0.0f;
    fx->grains[0].phase = 0.0f;
    fx->grains[1].phase = 0.5f;
    fx->grains[0].delay_frames = (float)frames_from_ms(fx->config.sample_rate, 74u);
    fx->grains[1].delay_frames = (float)frames_from_ms(fx->config.sample_rate, 151u);
    memset(&fx->reverse, 0, sizeof(fx->reverse));
    fx->dc_x_l = fx->dc_x_r = 0.0f;
    fx->dc_y_l = fx->dc_y_r = 0.0f;
    fx->random_state = fx->initial_seed;
    fx->current = ambient_fx_world_parameters(AMBIENT_FX_TOKYO_CITY);
    fx->target = fx->current;
    fx->mode = AMBIENT_FX_BYPASS;
    fx->target_mode = AMBIENT_FX_BYPASS;
    fx->transition_position = 0u;
    fx->transition_length = 0u;
    fx->transition_switched = 0u;
    reset_reverb(fx);
}

void ambient_fx_set_mode(AmbientFx *fx, AmbientFxMode mode)
{
    if (!fx || !fx->initialized || (unsigned)mode >= AMBIENT_FX_MODE_COUNT) return;
    if (mode == fx->target_mode && fx->transition_length != 0u) return;
    if (mode == fx->mode && fx->transition_length == 0u) {
        fx->target_mode = mode;
        return;
    }
    fx->target_mode = mode;
    fx->transition_length = clampu32(fx->config.sample_rate / 100u, 64u, 2048u);
    fx->transition_position = 0u;
    fx->transition_switched = 0u;
}

void ambient_fx_set_parameters(AmbientFx *fx, AmbientFxParameters parameters)
{
    if (!fx || !fx->initialized) return;
    fx->target = sanitize_parameters(fx, parameters);
}

void ambient_fx_set_world(AmbientFx *fx, AmbientFxWorld world)
{
    if (!fx || (unsigned)world >= AMBIENT_FX_WORLD_COUNT) return;
    ambient_fx_set_parameters(fx, ambient_fx_world_parameters(world));
}

AmbientFxMode ambient_fx_mode(const AmbientFx *fx)
{
    return fx && fx->initialized ? fx->mode : AMBIENT_FX_BYPASS;
}

AmbientFxParameters ambient_fx_parameters(const AmbientFx *fx)
{
    if (fx && fx->initialized) return fx->target;
    return ambient_fx_world_parameters(AMBIENT_FX_TOKYO_CITY);
}

size_t ambient_fx_latency_frames(const AmbientFx *fx, AmbientFxMode mode)
{
    (void)fx;
    (void)mode;
    /* Every real-time mode retains a direct path; only wet returns are late. */
    return 0u;
}

static void smooth_parameters(AmbientFx *fx)
{
#define SMOOTH(field, coefficient) \
    fx->current.field += (fx->target.field - fx->current.field) * (coefficient)
    SMOOTH(space, 0.00070f);
    SMOOTH(atmosphere, 0.00070f);
    SMOOTH(echo, 0.00055f);
    SMOOTH(motion, 0.00055f);
    SMOOTH(age, 0.00045f);
    SMOOTH(shimmer, 0.00045f);
    SMOOTH(blur, 0.00045f);
    SMOOTH(width, 0.00070f);
    SMOOTH(tone, 0.00055f);
    SMOOTH(level, 0.00120f);
    SMOOTH(delay_seconds, 0.00008f);
#undef SMOOTH
}

static void process_tape(AmbientFx *fx, float in_l, float in_r,
                         float *out_l, float *out_r)
{
    float age = fx->current.age;
    float sr = (float)fx->config.sample_rate;
    advance_phase(&fx->tape_wow_phase, (0.31f + 0.10f * fx->current.motion) / sr);
    advance_phase(&fx->tape_flutter_phase, (5.70f + 0.80f * fx->current.motion) / sr);
    advance_phase(&fx->tape_hum_phase, 50.0f / sr);

    float random = random_bipolar(fx);
    fx->tape_drift += (random - fx->tape_drift) * 0.000075f;
    float modulation_ms = 3.60f + age * (
        1.45f * fast_sine(fx->tape_wow_phase) +
        0.20f * fast_sine(fx->tape_flutter_phase) +
        0.16f * fx->tape_drift);
    float delay_frames = clampf(modulation_ms * 0.001f * sr,
                                1.0f, (float)fx->tape.capacity - 2.0f);
    float delayed_l = read_stereo_ring(&fx->tape, 0u, delay_frames);
    float delayed_r = read_stereo_ring(&fx->tape, 1u, delay_frames + 0.37f);
    write_stereo_ring(&fx->tape, in_l, in_r);

    float cutoff = 11500.0f - age * 6500.0f;
    float coefficient = one_pole_coefficient(cutoff, sr);
    fx->tape_lp_l += (delayed_l - fx->tape_lp_l) * coefficient;
    fx->tape_lp_r += (delayed_r - fx->tape_lp_r) * coefficient;
    float crossfeed = 0.14f * age;
    float colored_l = fx->tape_lp_l * (1.0f - crossfeed) + fx->tape_lp_r * crossfeed;
    float colored_r = fx->tape_lp_r * (1.0f - crossfeed) + fx->tape_lp_l * crossfeed;

    float drive = 1.0f + 1.45f * age;
    colored_l = soft_clip(colored_l * drive) * (1.0f - 0.12f * age);
    colored_r = soft_clip(colored_r * drive) * (1.0f - 0.12f * age);
    float hiss = random_bipolar(fx) * (0.00145f * age * age);
    float hum = fast_sine(fx->tape_hum_phase) * (0.00032f * age * age);
    colored_l += hiss + hum;
    colored_r += hiss * 0.91f + hum;

    float mix = 0.88f * age;
    *out_l = in_l * (1.0f - mix) + colored_l * mix;
    *out_r = in_r * (1.0f - mix) + colored_r * mix;
}

static void process_chorus(AmbientFx *fx, float in_l, float in_r,
                           float *out_l, float *out_r)
{
    float sr = (float)fx->config.sample_rate;
    float motion = fx->current.motion;
    advance_phase(&fx->chorus_phase_l, (0.17f + 0.035f * motion) / sr);
    advance_phase(&fx->chorus_phase_r, (0.23f + 0.041f * motion) / sr);
    float delay_l = (18.0f + 5.5f * motion * fast_sine(fx->chorus_phase_l)) * 0.001f * sr;
    float delay_r = (22.0f + 6.5f * motion * fast_sine(fx->chorus_phase_r)) * 0.001f * sr;
    float wet_l = read_stereo_ring(&fx->chorus, 0u, delay_l);
    float wet_r = read_stereo_ring(&fx->chorus, 1u, delay_r);
    write_stereo_ring(&fx->chorus, in_l, in_r);

    float width = fx->current.width;
    float field_l = wet_l * (1.0f - 0.55f * width) + wet_r * (0.55f * width);
    float field_r = wet_r * (1.0f - 0.55f * width) + wet_l * (0.55f * width);
    float mix = 0.55f * motion;
    *out_l = in_l * (1.0f - 0.38f * mix) + field_l * (0.72f * mix);
    *out_r = in_r * (1.0f - 0.38f * mix) + field_r * (0.72f * mix);
}

static void process_delay_wet(AmbientFx *fx, float in_l, float in_r,
                              float *wet_l, float *wet_r)
{
    float sr = (float)fx->config.sample_rate;
    float delay_frames = clampf(fx->current.delay_seconds * sr,
                                2.0f, (float)fx->delay.capacity - 2.0f);
    float read_l = read_stereo_ring(&fx->delay, 0u, delay_frames);
    float read_r = read_stereo_ring(&fx->delay, 1u, delay_frames + 0.31f);
    float cutoff = 1700.0f + 4700.0f * fx->current.tone;
    float coefficient = one_pole_coefficient(cutoff, sr);
    fx->delay_lp_l += (read_l - fx->delay_lp_l) * coefficient;
    fx->delay_lp_r += (read_r - fx->delay_lp_r) * coefficient;

    float echo = fx->current.echo;
    float feedback = echo * (0.22f + 0.34f * echo);
    float write_l = soft_clip(in_l + fx->delay_lp_r * feedback);
    float write_r = soft_clip(in_r + fx->delay_lp_l * feedback);
    write_stereo_ring(&fx->delay, write_l, write_r);
    *wet_l = fx->delay_lp_l;
    *wet_r = fx->delay_lp_r;
}

static float process_pitch_octave(AmbientFx *fx, float input)
{
    fx->pitch[fx->pitch_write] = float_to_i16(input);
    float sr = (float)fx->config.sample_rate;
    float window = clampf(sr * 0.045f, 256.0f, (float)fx->pitch_capacity - 48.0f);
    float phase_a = fx->pitch_phase;
    float phase_b = phase_a + 0.5f;
    if (phase_b >= 1.0f) phase_b -= 1.0f;
    float delay_a = 32.0f + (1.0f - phase_a) * window;
    float delay_b = 32.0f + (1.0f - phase_b) * window;
    float gain_a = smooth_window(phase_a);
    float gain_b = smooth_window(phase_b);
    float a = read_mono_ring(fx->pitch, fx->pitch_capacity, fx->pitch_write, delay_a);
    float b = read_mono_ring(fx->pitch, fx->pitch_capacity, fx->pitch_write, delay_b);
    float norm = 1.0f / (gain_a + gain_b + 0.0001f);
    float output = (a * gain_a + b * gain_b) * norm;

    ++fx->pitch_write;
    if (fx->pitch_write >= fx->pitch_capacity) fx->pitch_write = 0u;
    advance_phase(&fx->pitch_phase, 1.0f / window);
    return output;
}

static void update_reverb_targets(AmbientFx *fx)
{
    float space = fx->current.space;
    float rt60 = 1.65f + 5.85f * space * space;
    float sr = (float)fx->config.sample_rate;
    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        float seconds = fx->fdn[i].base_delay / sr;
        float exponent = 6.9077553f * seconds / rt60;
        fx->fdn[i].feedback_target = clampf(exp_negative_approx(exponent), 0.65f, 0.992f);
    }
}

static void process_reverb_wet(AmbientFx *fx, float in_l, float in_r,
                               float shimmer_amount,
                               float *wet_l, float *wet_r)
{
    float sr = (float)fx->config.sample_rate;
    float highpass_r = clampf(1.0f - FX_TWO_PI * 175.0f / sr, 0.90f, 0.995f);
    float hp_l = in_l - fx->reverb_hp_x_l + highpass_r * fx->reverb_hp_y_l;
    float hp_r = in_r - fx->reverb_hp_x_r + highpass_r * fx->reverb_hp_y_r;
    fx->reverb_hp_x_l = in_l;
    fx->reverb_hp_x_r = in_r;
    fx->reverb_hp_y_l = hp_l;
    fx->reverb_hp_y_r = hp_r;

    advance_phase(&fx->reverb_mod_phase_l,
                  (0.071f + 0.047f * fx->current.motion) / sr);
    advance_phase(&fx->reverb_mod_phase_r,
                  (0.109f + 0.061f * fx->current.motion) / sr);
    float mod_l = fast_sine(fx->reverb_mod_phase_l);
    float mod_r = fast_sine(fx->reverb_mod_phase_r);
    float modulation_depth = 0.35f + 3.20f * fx->current.motion;
    float damping_coefficient = one_pole_coefficient(
        2100.0f + 5200.0f * fx->current.tone, sr);

    float line[AMBIENT_FX_FDN_LINES];
    float sum = 0.0f;
    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        float modulation = (i & 1u ? mod_r : mod_l) * modulation_depth;
        if (i & 2u) modulation = -modulation;
        float delay = fx->fdn[i].base_delay + modulation;
        float value = read_mono_ring(fx->fdn[i].data, fx->fdn[i].capacity,
                                     fx->fdn[i].write, delay);
        fx->fdn[i].damping += (value - fx->fdn[i].damping) * damping_coefficient;
        line[i] = fx->fdn[i].damping;
        sum += line[i];
    }

    if ((fx->reverb_control_counter++ & 31u) == 0u) {
        update_reverb_targets(fx);
    }

    static const float inject_l[AMBIENT_FX_FDN_LINES] = {
        1.0f, -1.0f, 0.72f, -0.72f, 0.45f, -0.45f, 0.88f, -0.88f
    };
    static const float inject_r[AMBIENT_FX_FDN_LINES] = {
        0.45f, 0.88f, -1.0f, -0.72f, 1.0f, 0.72f, -0.45f, -0.88f
    };
    static const float shimmer_sign[AMBIENT_FX_FDN_LINES] = {
        1.0f, -1.0f, -1.0f, 1.0f, 0.72f, -0.72f, 0.72f, -0.72f
    };

    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        /* Eight-line Householder reflection: energy-preserving in exact math. */
        float matrix = line[i] - 0.25f * sum;
        fx->fdn[i].feedback +=
            (fx->fdn[i].feedback_target - fx->fdn[i].feedback) * 0.0025f;
        float input = (hp_l * inject_l[i] + hp_r * inject_r[i]) * 0.105f;
        input += fx->shimmer_return * shimmer_sign[i] * 0.080f;
        float write = soft_clip(input + matrix * fx->fdn[i].feedback);
        fx->fdn[i].data[fx->fdn[i].write] = float_to_i16(write);
        ++fx->fdn[i].write;
        if (fx->fdn[i].write >= fx->fdn[i].capacity) fx->fdn[i].write = 0u;
    }

    float left = (line[0] - line[1] + line[2] + line[4] - line[6] + line[7]) * 0.205f;
    float right = (line[1] + line[3] - line[4] + line[5] + line[6] - line[7]) * 0.205f;
    float width = fx->current.width;
    float mid = 0.5f * (left + right);
    left = mid + (left - mid) * (0.25f + 0.95f * width);
    right = mid + (right - mid) * (0.25f + 0.95f * width);

    float pitch_input = 0.5f * (left + right);
    float shimmer_hp_r = clampf(1.0f - FX_TWO_PI * 480.0f / sr, 0.85f, 0.99f);
    float high = pitch_input - fx->shimmer_hp_x + shimmer_hp_r * fx->shimmer_hp_y;
    fx->shimmer_hp_x = pitch_input;
    fx->shimmer_hp_y = high;
    float shifted = process_pitch_octave(fx, high);
    float shimmer_gain = 0.22f * shimmer_amount * shimmer_amount;
    fx->shimmer_return = soft_clip(shifted * shimmer_gain);

    fx->reverb_wet_l = left;
    fx->reverb_wet_r = right;
    *wet_l = left;
    *wet_r = right;
}

static void choose_new_grain_delay(AmbientFx *fx, FxBlurGrain *grain)
{
    float sr = (float)fx->config.sample_rate;
    float minimum = 0.055f * sr;
    float maximum = (float)fx->blur.capacity - 8.0f;
    float random = 0.5f * (random_bipolar(fx) + 1.0f);
    grain->delay_frames = minimum + random * (maximum - minimum);
}

static void process_blur(AmbientFx *fx, float in_l, float in_r,
                         float *out_l, float *out_r)
{
    float sr = (float)fx->config.sample_rate;
    float grain_frames = clampf(sr * (0.055f + 0.075f * fx->current.blur),
                                128.0f, (float)fx->blur.capacity * 0.55f);
    float wet_l = 0.0f;
    float wet_r = 0.0f;
    float weight = 0.0f;

    for (unsigned i = 0u; i < 2u; ++i) {
        FxBlurGrain *grain = &fx->grains[i];
        float window = smooth_window(grain->phase);
        float travel = grain->phase * grain_frames * (0.18f + 0.22f * fx->current.motion);
        float delay = clampf(grain->delay_frames - travel,
                             8.0f, (float)fx->blur.capacity - 2.0f);
        float left = read_stereo_ring(&fx->blur, 0u, delay);
        float right = read_stereo_ring(&fx->blur, 1u, delay + (i ? 0.61f : 0.17f));
        wet_l += left * window;
        wet_r += right * window;
        weight += window;
        grain->phase += 1.0f / grain_frames;
        if (grain->phase >= 1.0f) {
            grain->phase -= 1.0f;
            choose_new_grain_delay(fx, grain);
        }
    }
    write_stereo_ring(&fx->blur, in_l, in_r);
    float norm = 1.0f / (weight + 0.0001f);
    wet_l *= norm;
    wet_r *= norm;
    float coefficient = one_pole_coefficient(2600.0f + 3400.0f * fx->current.tone, sr);
    fx->blur_lp_l += (wet_l - fx->blur_lp_l) * coefficient;
    fx->blur_lp_r += (wet_r - fx->blur_lp_r) * coefficient;
    float mix = 0.78f * fx->current.blur;
    *out_l = in_l * (1.0f - 0.58f * mix) + fx->blur_lp_l * (0.78f * mix);
    *out_r = in_r * (1.0f - 0.58f * mix) + fx->blur_lp_r * (0.78f * mix);
}

void ambient_fx_trigger_reverse_swell(AmbientFx *fx,
                                      float frequency_hz,
                                      float level_0_1,
                                      float lead_seconds)
{
    if (!fx || !fx->initialized) return;
    float seconds = clampf(lead_seconds, 0.20f, 4.0f);
    uint32_t samples = (uint32_t)(seconds * (float)fx->config.sample_rate);
    fx->reverse.total = samples > 0u ? samples : 1u;
    fx->reverse.remaining = fx->reverse.total;
    fx->reverse.frequency = clampf(frequency_hz, 40.0f, 2400.0f);
    fx->reverse.level = clampf(level_0_1, 0.0f, 1.0f);
    fx->reverse.phase = 0.0f;
    fx->reverse.filter_l = 0.0f;
    fx->reverse.filter_r = 0.0f;
}

bool ambient_fx_reverse_swell_active(const AmbientFx *fx)
{
    return fx && fx->initialized && fx->reverse.remaining != 0u;
}

static void process_reverse_swell(AmbientFx *fx, float *left, float *right)
{
    *left = 0.0f;
    *right = 0.0f;
    FxReverseSwell *swell = &fx->reverse;
    if (swell->remaining == 0u || swell->total == 0u) return;

    float progress = 1.0f - (float)swell->remaining / (float)swell->total;
    float rise = progress * progress * (3.0f - 2.0f * progress);
    float end_distance = 1.0f - progress;
    float end_fade = clampf(end_distance / 0.015f, 0.0f, 1.0f);
    float envelope = rise * end_fade * swell->level;
    advance_phase(&swell->phase, swell->frequency / (float)fx->config.sample_rate);
    float pitched = fast_sine(swell->phase);
    float noise_l = random_bipolar(fx);
    float noise_r = random_bipolar(fx);
    float coefficient = 0.018f + 0.23f * progress * progress;
    float source_l = noise_l * 0.62f + pitched * 0.38f;
    float source_r = noise_r * 0.62f - pitched * 0.30f;
    swell->filter_l += (source_l - swell->filter_l) * coefficient;
    swell->filter_r += (source_r - swell->filter_r) * coefficient;
    *left = swell->filter_l * envelope * 0.42f;
    *right = swell->filter_r * envelope * 0.42f;
    --swell->remaining;
}

static float transition_gain(AmbientFx *fx)
{
    if (fx->transition_length == 0u) return 1.0f;
    uint32_t half = fx->transition_length / 2u;
    uint32_t position = fx->transition_position;
    float gain;
    if (position < half) {
        gain = 1.0f - (float)position / (float)(half ? half : 1u);
    } else {
        if (!fx->transition_switched) {
            fx->mode = fx->target_mode;
            fx->transition_switched = 1u;
        }
        gain = (float)(position - half) /
               (float)((fx->transition_length - half) ? (fx->transition_length - half) : 1u);
    }
    ++fx->transition_position;
    if (fx->transition_position >= fx->transition_length) {
        fx->mode = fx->target_mode;
        fx->transition_length = 0u;
        fx->transition_position = 0u;
        fx->transition_switched = 0u;
        gain = 1.0f;
    }
    return clampf(gain, 0.0f, 1.0f);
}

static void process_frame(AmbientFx *fx, float in_l, float in_r,
                          float *out_l, float *out_r)
{
    smooth_parameters(fx);
    float swell_l, swell_r;
    process_reverse_swell(fx, &swell_l, &swell_r);
    AmbientFxMode mode = fx->mode;
    float left = in_l;
    float right = in_r;

    if (mode == AMBIENT_FX_DARK_REVERB) {
        float wet_l, wet_r;
        process_reverb_wet(fx, in_l, in_r, 0.0f, &wet_l, &wet_r);
        float wet = fx->current.atmosphere * (0.16f + 0.54f * fx->current.space);
        left = in_l * (1.0f - 0.24f * wet) + wet_l * wet;
        right = in_r * (1.0f - 0.24f * wet) + wet_r * wet;
    } else if (mode == AMBIENT_FX_PING_PONG_DELAY) {
        float wet_l, wet_r;
        process_delay_wet(fx, in_l, in_r, &wet_l, &wet_r);
        float wet = 0.62f * fx->current.echo;
        left = in_l * (1.0f - 0.14f * wet) + wet_l * wet;
        right = in_r * (1.0f - 0.14f * wet) + wet_r * wet;
    } else if (mode == AMBIENT_FX_CHORUS_DETUNE) {
        process_chorus(fx, in_l, in_r, &left, &right);
    } else if (mode == AMBIENT_FX_TAPE_AGE) {
        process_tape(fx, in_l, in_r, &left, &right);
    } else if (mode == AMBIENT_FX_REVERSE_SWELL) {
        float wet_l, wet_r;
        float source_l = in_l + swell_l;
        float source_r = in_r + swell_r;
        process_reverb_wet(fx, source_l, source_r, 0.0f, &wet_l, &wet_r);
        float wet = 0.48f * fx->current.atmosphere;
        left = source_l + wet_l * wet;
        right = source_r + wet_r * wet;
    } else if (mode == AMBIENT_FX_SHIMMER_REVERB) {
        float wet_l, wet_r;
        process_reverb_wet(fx, in_l, in_r, fx->current.shimmer, &wet_l, &wet_r);
        float wet = fx->current.atmosphere * (0.18f + 0.55f * fx->current.space);
        left = in_l * (1.0f - 0.24f * wet) + wet_l * wet;
        right = in_r * (1.0f - 0.24f * wet) + wet_r * wet;
    } else if (mode == AMBIENT_FX_BLUR) {
        process_blur(fx, in_l, in_r, &left, &right);
    } else if (mode == AMBIENT_FX_DREAM_CHAIN) {
        float tape_l, tape_r;
        float chorus_l, chorus_r;
        float blur_l, blur_r;
        float echo_l, echo_r;
        float wet_l, wet_r;
        process_tape(fx, in_l, in_r, &tape_l, &tape_r);
        process_chorus(fx, tape_l, tape_r, &chorus_l, &chorus_r);
        process_blur(fx, chorus_l, chorus_r, &blur_l, &blur_r);
        process_delay_wet(fx, blur_l, blur_r, &echo_l, &echo_r);
        float echo_mix = 0.48f * fx->current.echo;
        float spatial_l = blur_l + echo_l * echo_mix;
        float spatial_r = blur_r + echo_r * echo_mix;
        spatial_l += swell_l;
        spatial_r += swell_r;
        process_reverb_wet(fx, spatial_l, spatial_r, fx->current.shimmer,
                           &wet_l, &wet_r);
        float wet = fx->current.atmosphere * (0.13f + 0.52f * fx->current.space);
        left = spatial_l * (1.0f - 0.22f * wet) + wet_l * wet;
        right = spatial_r * (1.0f - 0.22f * wet) + wet_r * wet;
    }

    float mode_gain = transition_gain(fx);
    left *= mode_gain;
    right *= mode_gain;

    /* Master DC blocker and bounded soft limiter. */
    float dc_l = left - fx->dc_x_l + 0.995f * fx->dc_y_l;
    float dc_r = right - fx->dc_x_r + 0.995f * fx->dc_y_r;
    fx->dc_x_l = left;
    fx->dc_x_r = right;
    fx->dc_y_l = dc_l;
    fx->dc_y_r = dc_r;
    float level = 0.92f * fx->current.level;
    *out_l = soft_clip(dc_l * level);
    *out_r = soft_clip(dc_r * level);
}

void ambient_fx_process_f32(AmbientFx *fx,
                            float *interleaved_stereo,
                            size_t frames)
{
    if (!fx || !fx->initialized || !interleaved_stereo) return;
    if (fx->mode == AMBIENT_FX_BYPASS &&
        fx->target_mode == AMBIENT_FX_BYPASS &&
        fx->transition_length == 0u &&
        fx->reverse.remaining == 0u) {
        return;
    }
    for (size_t i = 0u; i < frames; ++i) {
        float in_l = finite_audio(interleaved_stereo[i * 2u]);
        float in_r = finite_audio(interleaved_stereo[i * 2u + 1u]);
        float out_l, out_r;
        process_frame(fx, in_l, in_r, &out_l, &out_r);
        interleaved_stereo[i * 2u] = out_l;
        interleaved_stereo[i * 2u + 1u] = out_r;
    }
}

void ambient_fx_process_i16(AmbientFx *fx,
                            int16_t *interleaved_stereo,
                            size_t frames)
{
    if (!fx || !fx->initialized || !interleaved_stereo) return;
    if (fx->mode == AMBIENT_FX_BYPASS &&
        fx->target_mode == AMBIENT_FX_BYPASS &&
        fx->transition_length == 0u &&
        fx->reverse.remaining == 0u) {
        return;
    }
    for (size_t i = 0u; i < frames; ++i) {
        float in_l = i16_to_float(interleaved_stereo[i * 2u]);
        float in_r = i16_to_float(interleaved_stereo[i * 2u + 1u]);
        float out_l, out_r;
        process_frame(fx, in_l, in_r, &out_l, &out_r);
        interleaved_stereo[i * 2u] = float_to_i16(out_l);
        interleaved_stereo[i * 2u + 1u] = float_to_i16(out_r);
    }
}

size_t ambient_fx_reverse_reverb_offline_f32(AmbientFx *fx,
                                             const float *input_stereo,
                                             size_t input_frames,
                                             float *output_stereo,
                                             size_t output_frames,
                                             size_t tail_frames,
                                             float dry_gain,
                                             float wet_gain)
{
    if (!fx || !fx->initialized || !input_stereo || !output_stereo ||
        input_stereo == output_stereo || input_frames == 0u ||
        output_frames < input_frames + tail_frames) {
        return 0u;
    }
    size_t total = input_frames + tail_frames;
    memset(output_stereo, 0, total * 2u * sizeof(float));
    reset_reverb(fx);
    fx->current = sanitize_parameters(fx, fx->target);
    update_reverb_targets(fx);
    for (unsigned i = 0u; i < AMBIENT_FX_FDN_LINES; ++i) {
        fx->fdn[i].feedback = fx->fdn[i].feedback_target;
    }

    for (size_t n = 0u; n < total; ++n) {
        float in_l = 0.0f;
        float in_r = 0.0f;
        if (n < input_frames) {
            size_t source = input_frames - 1u - n;
            in_l = finite_audio(input_stereo[source * 2u]);
            in_r = finite_audio(input_stereo[source * 2u + 1u]);
        }
        float wet_l, wet_r;
        process_reverb_wet(fx, in_l, in_r, 0.0f, &wet_l, &wet_r);
        size_t destination = total - 1u - n;
        output_stereo[destination * 2u] = wet_l * wet_gain;
        output_stereo[destination * 2u + 1u] = wet_r * wet_gain;
    }
    for (size_t n = 0u; n < input_frames; ++n) {
        size_t destination = n + tail_frames;
        output_stereo[destination * 2u] += input_stereo[n * 2u] * dry_gain;
        output_stereo[destination * 2u + 1u] += input_stereo[n * 2u + 1u] * dry_gain;
    }
    reset_reverb(fx);
    return total;
}
