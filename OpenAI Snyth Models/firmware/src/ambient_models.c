#include "ambient_models.h"

#include <math.h>
#include <string.h>

#define AMB_PI 3.14159265358979323846f

typedef struct ModelProfile {
    float attack;
    float release;
    float decay;
    float gain;
    float send;
    uint8_t percussive;
} ModelProfile;

static const char *const k_names[AMBIENT_MODEL_COUNT] = {
    "ACID RAIN", "FM GLASS", "CHORUS MIST", "ION STORM", "GLASS ORBIT",
    "BAMBOO CIRCUIT", "NACRE HORIZON", "TIDEGLASS", "LUMEN SWARM",
    "HOLLOW CHOIR"
};

static const char *const k_slugs[AMBIENT_MODEL_COUNT] = {
    "acid-rain", "fm-glass", "chorus-mist", "ion-storm", "glass-orbit",
    "bamboo-circuit", "nacre-horizon", "tideglass", "lumen-swarm",
    "hollow-choir"
};

static const ModelProfile k_profile[AMBIENT_MODEL_COUNT] = {
    {0.08000f, 0.00300f, 0.99978f, 0.58f, 0.58f, 1},
    {0.03500f, 0.00120f, 0.99991f, 0.50f, 0.74f, 1},
    {0.00012f, 0.000025f, 1.00000f, 0.29f, 0.78f, 0},
    {0.00022f, 0.000035f, 1.00000f, 0.32f, 0.70f, 0},
    {0.02500f, 0.00070f, 0.99994f, 0.42f, 0.84f, 1},
    {0.90000f, 0.00800f, 0.99987f, 0.68f, 0.54f, 1},
    {0.00016f, 0.000022f, 1.00000f, 0.30f, 0.82f, 0},
    {0.00020f, 0.000030f, 1.00000f, 0.34f, 0.86f, 0},
    {0.00300f, 0.00016f, 0.999965f, 0.35f, 0.88f, 1},
    {0.00014f, 0.000020f, 1.00000f, 0.27f, 0.90f, 0}
};

static const uint16_t k_fdn_length[4] = {1499u, 1601u, 1777u, 1901u};

_Static_assert(sizeof(AmbientSynth) <= AMBIENT_STATE_BUDGET_BYTES,
               "AmbientSynth exceeds the embedded RAM budget");

static float clampf(float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static uint32_t random_u32(AmbientSynth *s)
{
    uint32_t x = s->random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s->random_state = x ? x : 0x9e3779b9u;
    return s->random_state;
}

static float random_bipolar(AmbientSynth *s)
{
    return (float)((int32_t)(random_u32(s) >> 9) - 4194304) * (1.0f / 4194304.0f);
}

/* Fast, table-free sine approximation. Input and output phases are 0..1. */
static float fast_sine(float phase)
{
    float x = phase - floorf(phase);
    x = x * 2.0f - 1.0f;
    float y = 4.0f * x * (1.0f - fabsf(x));
    return y + 0.225f * (y * fabsf(y) - y);
}

static float fast_triangle(float phase)
{
    float x = phase - floorf(phase);
    return 1.0f - 4.0f * fabsf(x - 0.5f);
}

static float advance(float *phase, float frequency)
{
    float p = *phase + frequency * (1.0f / (float)AMBIENT_SAMPLE_RATE);
    p -= floorf(p);
    *phase = p;
    return p;
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
    x = clampf(x, -1.45f, 1.45f);
    return x * (1.0f - 0.14814815f * x * x);
}

static AmbientControls sanitize_controls(AmbientControls c)
{
    c.color = clampf(c.color, 0.0f, 1.0f);
    c.motion = clampf(c.motion, 0.0f, 1.0f);
    c.space = clampf(c.space, 0.0f, 1.0f);
    c.texture = clampf(c.texture, 0.0f, 1.0f);
    c.width = clampf(c.width, 0.0f, 1.0f);
    c.level = clampf(c.level, 0.0f, 1.0f);
    return c;
}

AmbientControls ambient_model_default_controls(AmbientModel model)
{
    static const AmbientControls defaults[AMBIENT_MODEL_COUNT] = {
        {0.62f, 0.48f, 0.68f, 0.60f, 0.76f, 0.72f},
        {0.73f, 0.30f, 0.82f, 0.46f, 0.86f, 0.68f},
        {0.44f, 0.55f, 0.80f, 0.52f, 0.90f, 0.70f},
        {0.34f, 0.72f, 0.76f, 0.68f, 0.82f, 0.66f},
        {0.78f, 0.70f, 0.88f, 0.44f, 0.94f, 0.66f},
        {0.57f, 0.38f, 0.64f, 0.54f, 0.72f, 0.72f},
        {0.42f, 0.24f, 0.86f, 0.38f, 0.92f, 0.72f},
        {0.66f, 0.58f, 0.90f, 0.55f, 0.96f, 0.67f},
        {0.76f, 0.82f, 0.92f, 0.66f, 1.00f, 0.62f},
        {0.36f, 0.32f, 0.94f, 0.58f, 0.88f, 0.70f}
    };
    if ((unsigned)model >= AMBIENT_MODEL_COUNT) {
        model = AMBIENT_NACRE_HORIZON;
    }
    return defaults[model];
}

const char *ambient_model_name(AmbientModel model)
{
    return (unsigned)model < AMBIENT_MODEL_COUNT ? k_names[model] : "UNKNOWN";
}

const char *ambient_model_slug(AmbientModel model)
{
    return (unsigned)model < AMBIENT_MODEL_COUNT ? k_slugs[model] : "unknown";
}

void ambient_synth_init(AmbientSynth *s, AmbientModel model, uint32_t seed)
{
    if (!s) {
        return;
    }
    if ((unsigned)model >= AMBIENT_MODEL_COUNT) {
        model = AMBIENT_NACRE_HORIZON;
    }
    memset(s, 0, sizeof(*s));
    s->model = (uint8_t)model;
    s->random_state = seed ? seed : 0x6d2b79f5u;
    s->next_note_id = 1u;
    s->controls = ambient_model_default_controls(model);
    s->target = s->controls;
    for (unsigned i = 0; i < 4; ++i) {
        s->background_phase[i] = (float)(random_u32(s) & 0xffffu) / 65536.0f;
    }
}

void ambient_synth_set_model(AmbientSynth *s, AmbientModel model)
{
    if (!s || (unsigned)model >= AMBIENT_MODEL_COUNT) {
        return;
    }
    uint32_t seed = s->random_state ^ 0xa511e9b3u;
    ambient_synth_init(s, model, seed);
}

void ambient_synth_set_controls(AmbientSynth *s, AmbientControls controls)
{
    if (s) {
        s->target = sanitize_controls(controls);
    }
}

static unsigned choose_voice(AmbientSynth *s)
{
    unsigned limit = s->model == AMBIENT_BAMBOO_CIRCUIT ? 4u : AMBIENT_MAX_VOICES;
    unsigned best = 0u;
    float quietest = 1000.0f;
    for (unsigned i = 0; i < limit; ++i) {
        if (!s->voices[i].active) {
            return i;
        }
        float importance = s->voices[i].envelope * s->voices[i].tail;
        if (importance < quietest) {
            quietest = importance;
            best = i;
        }
    }
    return best;
}

static void prepare_pluck(AmbientSynth *s, AmbientVoice *v, unsigned slot)
{
    unsigned n = (unsigned)((float)AMBIENT_SAMPLE_RATE / v->frequency + 0.5f);
    if (n < 18u) n = 18u;
    if (n > 1024u) n = 1024u;
    float last = 0.0f;
    for (unsigned i = 0; i < n; ++i) {
        float raw = random_bipolar(s);
        last += 0.52f * (raw - last);
        s->pluck[slot][i] = float_to_i16(last * 0.76f);
    }
    for (unsigned i = n; i < 1024u; ++i) {
        s->pluck[slot][i] = 0;
    }
    v->pluck_slot = (uint8_t)slot;
    v->pluck_length = (uint16_t)n;
    v->pluck_position = 0u;
}

uint16_t ambient_synth_note_on(AmbientSynth *s, float frequency, float velocity,
                               float pan)
{
    if (!s) {
        return 0u;
    }
    unsigned index = choose_voice(s);
    AmbientVoice *v = &s->voices[index];
    memset(v, 0, sizeof(*v));
    v->frequency = clampf(frequency, 24.0f, 6000.0f);
    v->velocity = clampf(velocity, 0.0f, 1.0f);
    v->pan = clampf(pan, -1.0f, 1.0f);
    v->tail = 1.0f;
    v->gate = 1u;
    v->active = 1u;
    v->note_id = s->next_note_id++;
    if (s->next_note_id == 0u) {
        s->next_note_id = 1u;
    }
    v->phase[0] = (float)(random_u32(s) & 0xffffu) / 65536.0f;
    v->phase[1] = (float)(random_u32(s) & 0xffffu) / 65536.0f;
    v->phase[2] = (float)(random_u32(s) & 0xffffu) / 65536.0f;
    v->phase[3] = (float)(random_u32(s) & 0xffffu) / 65536.0f;
    v->aux[0] = random_bipolar(s);
    v->aux[1] = random_bipolar(s);
    v->aux[2] = random_bipolar(s);
    v->aux[3] = random_bipolar(s);
    if (s->model == AMBIENT_BAMBOO_CIRCUIT) {
        prepare_pluck(s, v, index);
        v->envelope = 1.0f;
    }
    return v->note_id;
}

void ambient_synth_note_off(AmbientSynth *s, uint16_t note_id)
{
    if (!s || note_id == 0u) {
        return;
    }
    for (unsigned i = 0; i < AMBIENT_MAX_VOICES; ++i) {
        if (s->voices[i].active && s->voices[i].note_id == note_id) {
            s->voices[i].gate = 0u;
        }
    }
}

void ambient_synth_all_notes_off(AmbientSynth *s)
{
    if (!s) {
        return;
    }
    for (unsigned i = 0; i < AMBIENT_MAX_VOICES; ++i) {
        s->voices[i].gate = 0u;
    }
}

static float render_acid_rain(AmbientSynth *s, AmbientVoice *v)
{
    float pitch_fall = 1.0f + 1.8f * v->tail * v->tail;
    float p = advance(&v->phase[0], v->frequency * pitch_fall);
    float overtone = fast_sine(advance(&v->phase[1], v->frequency * 2.037f));
    float drop = fast_sine(p) + overtone * (0.24f + 0.22f * s->controls.color);
    float grit = random_bipolar(s) * v->tail * v->tail * 0.18f * s->controls.texture;
    return (drop * 0.72f + grit) * v->tail;
}

static float render_fm_glass(AmbientSynth *s, AmbientVoice *v)
{
    float mod = fast_sine(advance(&v->phase[1], v->frequency * (2.001f + 0.31f * s->controls.texture)));
    float index = (1.4f + 5.8f * s->controls.color) * v->tail;
    advance(&v->phase[0], v->frequency);
    float carrier = fast_sine(v->phase[0] + mod * index * (1.0f / (2.0f * AMB_PI)));
    float halo = fast_sine(advance(&v->phase[2], v->frequency * 3.997f)) * 0.12f;
    return (carrier + halo * v->tail) * v->tail;
}

static float render_chorus_mist(AmbientSynth *s, AmbientVoice *v)
{
    float detune = 0.0012f + 0.0060f * s->controls.motion;
    float a = fast_triangle(advance(&v->phase[0], v->frequency));
    float b = fast_sine(advance(&v->phase[1], v->frequency * (1.0f + detune)));
    float c = fast_sine(advance(&v->phase[2], v->frequency * (1.0f - detune * 0.73f)));
    float octave = fast_sine(advance(&v->phase[3], v->frequency * 0.501f));
    return a * 0.30f + b * 0.28f + c * 0.28f + octave * 0.14f;
}

static float render_ion_storm(AmbientSynth *s, AmbientVoice *v)
{
    float sub = fast_triangle(advance(&v->phase[0], v->frequency * 0.5003f));
    float electric = fast_sine(advance(&v->phase[1], v->frequency * 1.503f));
    float drift = fast_sine(advance(&v->phase[2], 0.071f + 0.09f * s->controls.motion));
    float crack = 0.0f;
    uint32_t threshold = (uint32_t)(2.0f + 13.0f * s->controls.texture);
    if ((random_u32(s) & 0xffffu) < threshold) {
        v->aux[3] = random_bipolar(s);
    }
    v->aux[3] *= 0.996f;
    crack = v->aux[3] * 0.32f;
    return sub * 0.58f + electric * (0.18f + 0.12f * drift) + crack;
}

static float render_glass_orbit(AmbientSynth *s, AmbientVoice *v)
{
    float a = fast_sine(advance(&v->phase[0], v->frequency));
    float b = fast_sine(advance(&v->phase[1], v->frequency * 2.713f));
    float c = fast_sine(advance(&v->phase[2], v->frequency * 4.117f));
    float d = fast_sine(advance(&v->phase[3], v->frequency * 6.83f));
    float bright = s->controls.color;
    return (a * 0.58f + b * (0.25f + 0.10f * bright) +
            c * (0.12f + 0.08f * bright) + d * 0.05f) * v->tail;
}

static float render_bamboo(AmbientSynth *s, AmbientVoice *v)
{
    if (v->pluck_length < 2u) {
        return 0.0f;
    }
    unsigned slot = v->pluck_slot & 3u;
    unsigned p = v->pluck_position;
    unsigned n = (p + 1u) % v->pluck_length;
    float a = i16_to_float(s->pluck[slot][p]);
    float b = i16_to_float(s->pluck[slot][n]);
    float loss = 0.9925f + 0.0062f * s->controls.color;
    float next = (a + b) * 0.5f * loss;
    float nonlinear = next - next * next * next * (0.025f + 0.08f * s->controls.texture);
    s->pluck[slot][p] = float_to_i16(nonlinear);
    v->pluck_position = (uint16_t)n;
    return a * v->tail;
}

static float render_nacre(AmbientSynth *s, AmbientVoice *v)
{
    float root = fast_sine(advance(&v->phase[0], v->frequency));
    float fifth = fast_sine(advance(&v->phase[1], v->frequency * 1.5012f));
    float sub = fast_triangle(advance(&v->phase[2], v->frequency * 0.5000f));
    float pearl = fast_sine(advance(&v->phase[3], v->frequency * 2.004f));
    return root * 0.56f + fifth * 0.18f + sub * 0.18f + pearl * (0.04f + 0.08f * s->controls.color);
}

static float render_tideglass(AmbientSynth *s, AmbientVoice *v)
{
    float tide = fast_sine(advance(&v->phase[3], 0.043f + 0.12f * s->controls.motion));
    float ratio = 1.997f + tide * (0.015f + 0.045f * s->controls.texture);
    float mod = fast_sine(advance(&v->phase[1], v->frequency * ratio));
    float index = 0.8f + (2.0f + tide) * s->controls.color;
    advance(&v->phase[0], v->frequency * (1.0f + tide * 0.0015f));
    float water = fast_sine(v->phase[0] + mod * index * (1.0f / (2.0f * AMB_PI)));
    float low = fast_sine(advance(&v->phase[2], v->frequency * 0.5006f));
    return water * 0.70f + low * 0.30f;
}

static float render_lumen(AmbientSynth *s, AmbientVoice *v)
{
    float slow = fast_sine((float)s->sample_clock * (1.0f / AMBIENT_SAMPLE_RATE) *
                           (0.08f + 0.19f * s->controls.motion) + v->aux[0]);
    float a = fast_sine(advance(&v->phase[0], v->frequency * (1.0f + 0.0015f * v->aux[0])));
    float b = fast_sine(advance(&v->phase[1], v->frequency * (1.498f + 0.004f * v->aux[1])));
    float c = fast_sine(advance(&v->phase[2], v->frequency * (2.006f + 0.006f * v->aux[2])));
    float d = fast_sine(advance(&v->phase[3], v->frequency * (3.011f + 0.009f * v->aux[3])));
    float sparkle = 0.08f + 0.16f * (slow * 0.5f + 0.5f) * s->controls.texture;
    return a * 0.46f + b * 0.24f + c * 0.18f + d * sparkle;
}

static float render_choir(AmbientSynth *s, AmbientVoice *v)
{
    float vowel = fast_sine(advance(&v->phase[3], 0.028f + 0.05f * s->controls.motion));
    float fundamental = fast_sine(advance(&v->phase[0], v->frequency));
    float formant_a = fast_sine(advance(&v->phase[1], v->frequency * (2.92f + 0.25f * vowel)));
    float formant_b = fast_sine(advance(&v->phase[2], v->frequency * (5.08f - 0.31f * vowel)));
    float breath = random_bipolar(s) * (0.018f + 0.055f * s->controls.texture);
    return fundamental * 0.52f + formant_a * 0.30f + formant_b * 0.13f + breath;
}

static float render_voice(AmbientSynth *s, AmbientVoice *v, float *pan)
{
    const ModelProfile *profile = &k_profile[s->model];
    float target = v->gate ? 1.0f : 0.0f;
    float coefficient = v->gate ? profile->attack : profile->release;
    v->envelope += (target - v->envelope) * coefficient;
    if (profile->percussive) {
        v->tail *= profile->decay;
        if (v->age > AMBIENT_SAMPLE_RATE / 20u) {
            v->gate = 0u;
        }
    }

    float sample;
    switch ((AmbientModel)s->model) {
    case AMBIENT_ACID_RAIN:       sample = render_acid_rain(s, v); break;
    case AMBIENT_FM_GLASS:        sample = render_fm_glass(s, v); break;
    case AMBIENT_CHORUS_MIST:     sample = render_chorus_mist(s, v); break;
    case AMBIENT_ION_STORM:       sample = render_ion_storm(s, v); break;
    case AMBIENT_GLASS_ORBIT:     sample = render_glass_orbit(s, v); break;
    case AMBIENT_BAMBOO_CIRCUIT:  sample = render_bamboo(s, v); break;
    case AMBIENT_NACRE_HORIZON:   sample = render_nacre(s, v); break;
    case AMBIENT_TIDEGLASS:       sample = render_tideglass(s, v); break;
    case AMBIENT_LUMEN_SWARM:     sample = render_lumen(s, v); break;
    case AMBIENT_HOLLOW_CHOIR:    sample = render_choir(s, v); break;
    default:                       sample = 0.0f; break;
    }

    float orbit = 0.0f;
    if (s->model == AMBIENT_GLASS_ORBIT || s->model == AMBIENT_LUMEN_SWARM) {
        orbit = fast_sine((float)v->age * (1.0f / AMBIENT_SAMPLE_RATE) *
                          (0.035f + 0.16f * s->controls.motion) + v->aux[1]) * 0.74f;
    }
    *pan = clampf(v->pan + orbit * s->controls.width, -1.0f, 1.0f);

    float cutoff = 0.015f + 0.38f * s->controls.color * s->controls.color;
    v->filter += (sample - v->filter) * cutoff;
    float colored = v->filter + (sample - v->filter) * s->controls.color * 0.35f;
    float output = colored * v->envelope * v->tail * v->velocity * profile->gain;
    ++v->age;
    if ((v->envelope < 0.00008f && !v->gate) || v->tail < 0.00006f) {
        v->active = 0u;
    }
    return output;
}

static void smooth_controls(AmbientSynth *s)
{
#define SMOOTH_CONTROL(field) \
    s->controls.field += (s->target.field - s->controls.field) * 0.00065f
    SMOOTH_CONTROL(color);
    SMOOTH_CONTROL(motion);
    SMOOTH_CONTROL(space);
    SMOOTH_CONTROL(texture);
    SMOOTH_CONTROL(width);
    SMOOTH_CONTROL(level);
#undef SMOOTH_CONTROL
}

static void chorus(AmbientSynth *s, float dry_l, float dry_r, float *out_l, float *out_r)
{
    unsigned write = s->chorus_position & 511u;
    float sweep = fast_sine(s->lfo_phase);
    unsigned offset_l = (unsigned)(86.0f + (34.0f + 72.0f * s->controls.motion) * (sweep * 0.5f + 0.5f));
    unsigned offset_r = (unsigned)(103.0f + (38.0f + 63.0f * s->controls.motion) * (-sweep * 0.5f + 0.5f));
    float delayed_l = i16_to_float(s->chorus[0][(write - offset_l) & 511u]);
    float delayed_r = i16_to_float(s->chorus[1][(write - offset_r) & 511u]);
    s->chorus[0][write] = float_to_i16(dry_l);
    s->chorus[1][write] = float_to_i16(dry_r);
    s->chorus_position = (write + 1u) & 511u;
    float amount = 0.04f + 0.20f * s->controls.motion;
    *out_l = dry_l + (delayed_r - dry_l) * amount * s->controls.width;
    *out_r = dry_r + (delayed_l - dry_r) * amount * s->controls.width;
}

static void spatial_fdn(AmbientSynth *s, float in_l, float in_r, float *wet_l, float *wet_r)
{
    float line[4];
    for (unsigned i = 0; i < 4u; ++i) {
        unsigned p = s->fdn_position[i];
        float x = i16_to_float(s->fdn[i][p]);
        float damp = 0.08f + 0.36f * (1.0f - s->controls.color);
        s->fdn_damping[i] += (x - s->fdn_damping[i]) * damp;
        line[i] = s->fdn_damping[i];
    }

    float h0 = line[0] + line[1] + line[2] + line[3];
    float h1 = line[0] - line[1] + line[2] - line[3];
    float h2 = line[0] + line[1] - line[2] - line[3];
    float h3 = line[0] - line[1] - line[2] + line[3];
    float matrix[4] = {h0 * 0.5f, h1 * 0.5f, h2 * 0.5f, h3 * 0.5f};
    float input[4] = {in_l, in_r, (in_l + in_r) * 0.707f, (in_l - in_r) * 0.707f};
    float feedback = 0.72f + 0.255f * s->controls.space;

    for (unsigned i = 0; i < 4u; ++i) {
        unsigned p = s->fdn_position[i];
        float write = input[i] * 0.30f + matrix[(i + 1u) & 3u] * feedback;
        s->fdn[i][p] = float_to_i16(write);
        p++;
        if (p >= k_fdn_length[i]) p = 0u;
        s->fdn_position[i] = p;
    }
    *wet_l = (line[0] + line[2] - line[3]) * 0.36f;
    *wet_r = (line[1] - line[2] + line[3]) * 0.36f;
}

void ambient_synth_render(AmbientSynth *s, int16_t *out, size_t frames)
{
    if (!s || !out) {
        return;
    }
    const ModelProfile *profile = &k_profile[s->model];
    for (size_t frame = 0; frame < frames; ++frame) {
        smooth_controls(s);
        s->lfo_phase += (0.027f + 0.083f * s->controls.motion) *
                        (1.0f / (float)AMBIENT_SAMPLE_RATE);
        s->lfo_phase -= floorf(s->lfo_phase);

        float dry_l = 0.0f;
        float dry_r = 0.0f;
        for (unsigned i = 0; i < AMBIENT_MAX_VOICES; ++i) {
            AmbientVoice *v = &s->voices[i];
            if (!v->active) continue;
            float pan;
            float sample = render_voice(s, v, &pan);
            float left_gain = sqrtf(0.5f * (1.0f - pan));
            float right_gain = sqrtf(0.5f * (1.0f + pan));
            dry_l += sample * left_gain;
            dry_r += sample * right_gain;
        }

        float wide_l, wide_r;
        chorus(s, dry_l, dry_r, &wide_l, &wide_r);
        float wet_l, wet_r;
        spatial_fdn(s, wide_l * profile->send, wide_r * profile->send, &wet_l, &wet_r);
        float wet_amount = 0.16f + 0.53f * s->controls.space;
        float mixed_l = wide_l * (1.0f - wet_amount * 0.36f) + wet_l * wet_amount;
        float mixed_r = wide_r * (1.0f - wet_amount * 0.36f) + wet_r * wet_amount;

        /* DC blocker keeps asymmetric algorithmic texture away from the DAC. */
        s->dc_l = mixed_l - s->dc_prev_l + 0.995f * s->dc_l;
        s->dc_r = mixed_r - s->dc_prev_r + 0.995f * s->dc_r;
        s->dc_prev_l = mixed_l;
        s->dc_prev_r = mixed_r;
        float level = s->controls.level * 0.86f;
        out[frame * 2u] = float_to_i16(soft_clip(s->dc_l * level));
        out[frame * 2u + 1u] = float_to_i16(soft_clip(s->dc_r * level));
        ++s->sample_clock;
    }
}

size_t ambient_synth_state_bytes(void)
{
    return sizeof(AmbientSynth);
}
