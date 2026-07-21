#include "ambient_effects.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t checks;
static uint64_t failures;

#define CHECK(condition, message) do { \
    ++checks; \
    if (!(condition)) { \
        ++failures; \
        fprintf(stderr, "FAIL: %s\n", message); \
    } \
} while (0)

typedef struct Fixture {
    AmbientFxStorage storage;
    AmbientFx *fx;
    uint8_t *allocation;
    uint8_t *arena;
    size_t arena_bytes;
} Fixture;

static void fixture_create(Fixture *fixture, uint32_t seed)
{
    memset(fixture, 0, sizeof(*fixture));
    AmbientFxConfig config = ambient_fx_default_config();
    config.seed = seed;
    fixture->arena_bytes = ambient_fx_memory_required(&config);
    fixture->allocation = (uint8_t *)malloc(fixture->arena_bytes + 64u);
    if (!fixture->allocation) return;
    memset(fixture->allocation, 0xa5, fixture->arena_bytes + 64u);
    fixture->arena = fixture->allocation + 32u;
    fixture->fx = ambient_fx_init(&fixture->storage, fixture->arena,
                                  fixture->arena_bytes, &config);
}

static void fixture_destroy(Fixture *fixture)
{
    free(fixture->allocation);
    memset(fixture, 0, sizeof(*fixture));
}

static void check_guards(const Fixture *fixture)
{
    if (!fixture->allocation) return;
    for (size_t i = 0u; i < 32u; ++i) {
        CHECK(fixture->allocation[i] == 0xa5, "effect arena wrote before its boundary");
        CHECK(fixture->allocation[32u + fixture->arena_bytes + i] == 0xa5,
              "effect arena wrote after its boundary");
    }
}

static void make_source(float *stereo, size_t frames)
{
    float phase_a = 0.0f;
    float phase_b = 0.0f;
    for (size_t i = 0u; i < frames; ++i) {
        float envelope = i < 6000u ? 0.24f : 0.0f;
        stereo[i * 2u] = sinf(phase_a) * envelope;
        stereo[i * 2u + 1u] = sinf(phase_b) * envelope * 0.82f;
        phase_a += 6.28318530718f * 220.0f / (float)AMBIENT_FX_SAMPLE_RATE;
        phase_b += 6.28318530718f * 329.63f / (float)AMBIENT_FX_SAMPLE_RATE;
        if (phase_a >= 6.28318530718f) phase_a -= 6.28318530718f;
        if (phase_b >= 6.28318530718f) phase_b -= 6.28318530718f;
    }
}

static double signal_energy(const float *stereo, size_t first, size_t last)
{
    double energy = 0.0;
    for (size_t i = first; i < last; ++i) {
        energy += fabs((double)stereo[i * 2u]);
        energy += fabs((double)stereo[i * 2u + 1u]);
    }
    return energy;
}

static float signal_peak(const float *stereo, size_t first, size_t last)
{
    float peak = 0.0f;
    for (size_t i = first; i < last; ++i) {
        float left = fabsf(stereo[i * 2u]);
        float right = fabsf(stereo[i * 2u + 1u]);
        if (left > peak) peak = left;
        if (right > peak) peak = right;
    }
    return peak;
}

static void verify_basics(void)
{
    AmbientFxConfig config = ambient_fx_default_config();
    size_t memory = ambient_fx_memory_required(&config);
    CHECK(memory > 180000u, "full effect arena is unexpectedly small");
    CHECK(memory <= AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES,
          "default effects exceed their internal-SRAM arena budget");
    CHECK(ambient_fx_state_bytes() <= AMBIENT_FX_STATE_BYTES,
          "effect control state exceeds fixed storage");

    uint8_t tiny[128];
    AmbientFxStorage storage;
    CHECK(ambient_fx_init(&storage, tiny, sizeof(tiny), &config) == NULL,
          "initialization must reject an undersized arena");

    AmbientFxConfig invalid = config;
    invalid.sample_rate = 48000u;
    CHECK(ambient_fx_memory_required(&invalid) == 0u,
          "fixed-rate engine must reject an accidental sample-rate mismatch");

    for (int mode = 0; mode < AMBIENT_FX_MODE_COUNT; ++mode) {
        CHECK(strcmp(ambient_fx_mode_name((AmbientFxMode)mode), "UNKNOWN") != 0,
              "effect mode needs a stable name");
    }
    for (int world = 0; world < AMBIENT_FX_WORLD_COUNT; ++world) {
        CHECK(strcmp(ambient_fx_world_name((AmbientFxWorld)world), "UNKNOWN") != 0,
              "effect world needs a stable name");
    }
}

static void verify_bypass(void)
{
    Fixture fixture;
    fixture_create(&fixture, 0x11111111u);
    CHECK(fixture.fx != NULL, "fixture initialization");
    if (!fixture.fx) return;
    int16_t block[512u * 2u];
    for (unsigned i = 0u; i < 1024u; ++i) block[i] = (int16_t)(i * 31u - 15000u);
    int16_t original[512u * 2u];
    memcpy(original, block, sizeof(block));
    ambient_fx_process_i16(fixture.fx, block, 512u);
    CHECK(memcmp(block, original, sizeof(block)) == 0, "bypass must be bit-exact");
    CHECK(ambient_fx_latency_frames(fixture.fx, AMBIENT_FX_DREAM_CHAIN) == 0u,
          "real-time chain must preserve a zero-latency dry path");
    check_guards(&fixture);
    fixture_destroy(&fixture);
}

static void verify_mode(AmbientFxMode mode)
{
    const size_t frames = 32768u;
    Fixture fixture;
    fixture_create(&fixture, 0x22220000u + (uint32_t)mode);
    float *audio = (float *)calloc(frames * 2u, sizeof(float));
    CHECK(fixture.fx && audio, "mode fixture allocation");
    if (!fixture.fx || !audio) {
        free(audio);
        fixture_destroy(&fixture);
        return;
    }
    make_source(audio, frames);
    ambient_fx_set_world(fixture.fx, AMBIENT_FX_CRYSTAL_COAST);
    ambient_fx_set_mode(fixture.fx, mode);
    if (mode == AMBIENT_FX_REVERSE_SWELL || mode == AMBIENT_FX_DREAM_CHAIN) {
        ambient_fx_trigger_reverse_swell(fixture.fx, 220.0f, 0.6f, 0.35f);
    }
    ambient_fx_process_f32(fixture.fx, audio, frames);

    double energy = signal_energy(audio, 0u, frames);
    double side = 0.0;
    float peak = 0.0f;
    for (size_t i = 0u; i < frames; ++i) {
        float left = audio[i * 2u];
        float right = audio[i * 2u + 1u];
        CHECK(isfinite(left) && isfinite(right), "effect generated a non-finite sample");
        float al = fabsf(left);
        float ar = fabsf(right);
        if (al > peak) peak = al;
        if (ar > peak) peak = ar;
        side += fabs((double)left - (double)right);
    }
    CHECK(energy > 100.0, "effect output energy is unexpectedly low");
    CHECK(side > 1.0, "effect output must retain or create stereo information");
    CHECK(peak <= 1.0001f, "effect output escaped the final limiter");
    check_guards(&fixture);
    free(audio);
    fixture_destroy(&fixture);
}

static void verify_block_invariance(void)
{
    const size_t frames = 24000u;
    Fixture a;
    Fixture b;
    fixture_create(&a, 0x13579bdfu);
    fixture_create(&b, 0x13579bdfu);
    float *large = (float *)malloc(frames * 2u * sizeof(float));
    float *chunked = (float *)malloc(frames * 2u * sizeof(float));
    CHECK(a.fx && b.fx && large && chunked, "block invariance allocation");
    if (!a.fx || !b.fx || !large || !chunked) goto cleanup;
    make_source(large, frames);
    memcpy(chunked, large, frames * 2u * sizeof(float));
    ambient_fx_set_world(a.fx, AMBIENT_FX_AFTER_HOURS);
    ambient_fx_set_world(b.fx, AMBIENT_FX_AFTER_HOURS);
    ambient_fx_set_mode(a.fx, AMBIENT_FX_DREAM_CHAIN);
    ambient_fx_set_mode(b.fx, AMBIENT_FX_DREAM_CHAIN);
    ambient_fx_trigger_reverse_swell(a.fx, 164.81f, 0.4f, 0.30f);
    ambient_fx_trigger_reverse_swell(b.fx, 164.81f, 0.4f, 0.30f);
    ambient_fx_process_f32(a.fx, large, frames);
    size_t offset = 0u;
    while (offset < frames) {
        size_t count = frames - offset;
        if (count > 127u) count = 127u;
        ambient_fx_process_f32(b.fx, chunked + offset * 2u, count);
        offset += count;
    }
    int identical = memcmp(large, chunked, frames * 2u * sizeof(float)) == 0;
    if (!identical) {
        for (size_t i = 0u; i < frames * 2u; ++i) {
            if (large[i] != chunked[i]) {
                fprintf(stderr, "first block mismatch: sample=%zu large=%g chunked=%g\n",
                        i, (double)large[i], (double)chunked[i]);
                break;
            }
        }
    }
    CHECK(identical, "effects must be exactly block-size invariant");
    check_guards(&a);
    check_guards(&b);

cleanup:
    free(large);
    free(chunked);
    fixture_destroy(&a);
    fixture_destroy(&b);
}

static void finish_transition(AmbientFx *fx)
{
    float silence[512u * 2u];
    memset(silence, 0, sizeof(silence));
    ambient_fx_process_f32(fx, silence, 512u);
}

static void verify_delay_timing(void)
{
    Fixture fixture;
    fixture_create(&fixture, 0x33333333u);
    const size_t frames = 21000u;
    float *audio = (float *)calloc(frames * 2u, sizeof(float));
    CHECK(fixture.fx && audio, "delay test allocation");
    if (!fixture.fx || !audio) goto cleanup;
    AmbientFxParameters p = ambient_fx_world_parameters(AMBIENT_FX_TOKYO_CITY);
    p.echo = 1.0f;
    p.delay_seconds = 0.430f;
    p.level = 0.8f;
    ambient_fx_set_parameters(fixture.fx, p);
    ambient_fx_set_mode(fixture.fx, AMBIENT_FX_PING_PONG_DELAY);
    finish_transition(fixture.fx);
    audio[0] = 0.8f;
    ambient_fx_process_f32(fixture.fx, audio, frames);
    size_t expected = (size_t)(0.430f * (float)AMBIENT_FX_SAMPLE_RATE);
    double around = signal_energy(audio, expected - 24u, expected + 24u);
    float around_peak = signal_peak(audio, expected - 24u, expected + 24u);
    float early_peak = signal_peak(audio, 256u, expected - 128u);
    CHECK(around > 0.03, "ping-pong echo did not arrive at its configured time");
    CHECK(early_peak < around_peak * 0.35f + 0.001f,
          "delay leaked a false early echo");

cleanup:
    free(audio);
    fixture_destroy(&fixture);
}

static void verify_reverb_tail(void)
{
    Fixture fixture;
    fixture_create(&fixture, 0x44444444u);
    const size_t frames = AMBIENT_FX_SAMPLE_RATE * 2u;
    float *audio = (float *)calloc(frames * 2u, sizeof(float));
    CHECK(fixture.fx && audio, "reverb test allocation");
    if (!fixture.fx || !audio) goto cleanup;
    AmbientFxParameters p = ambient_fx_world_parameters(AMBIENT_FX_AFTER_HOURS);
    p.space = 1.0f;
    p.atmosphere = 1.0f;
    ambient_fx_set_parameters(fixture.fx, p);
    ambient_fx_set_mode(fixture.fx, AMBIENT_FX_DARK_REVERB);
    finish_transition(fixture.fx);
    audio[0] = 0.8f;
    audio[1] = 0.6f;
    ambient_fx_process_f32(fixture.fx, audio, frames);
    CHECK(signal_energy(audio, AMBIENT_FX_SAMPLE_RATE, frames) > 0.20,
          "long reverb tail died before two seconds");
    fixture_destroy(&fixture);
    free(audio);
    return;

cleanup:
    free(audio);
    fixture_destroy(&fixture);
}

static void verify_true_reverse(void)
{
    Fixture fixture;
    fixture_create(&fixture, 0x55555555u);
    const size_t input_frames = 4096u;
    const size_t tail_frames = 12000u;
    const size_t total = input_frames + tail_frames;
    float *input = (float *)calloc(input_frames * 2u, sizeof(float));
    float *output = (float *)calloc(total * 2u, sizeof(float));
    CHECK(fixture.fx && input && output, "reverse test allocation");
    if (!fixture.fx || !input || !output) goto cleanup;
    input[1200u * 2u] = 0.75f;
    input[1200u * 2u + 1u] = 0.55f;
    AmbientFxParameters p = ambient_fx_world_parameters(AMBIENT_FX_AFTER_HOURS);
    p.space = 0.85f;
    p.atmosphere = 0.8f;
    ambient_fx_set_parameters(fixture.fx, p);
    size_t written = ambient_fx_reverse_reverb_offline_f32(
        fixture.fx, input, input_frames, output, total, tail_frames, 0.75f, 0.70f);
    CHECK(written == total, "true reverse renderer returned the wrong frame count");
    size_t dry_arrival = tail_frames + 1200u;
    CHECK(signal_energy(output, 0u, dry_arrival) > 0.01,
          "reverse reverb produced no anticipatory pre-tail");
    CHECK(fabsf(output[dry_arrival * 2u]) > 0.45f,
          "reverse reverb lost the delayed dry event");

cleanup:
    free(input);
    free(output);
    fixture_destroy(&fixture);
}

static void verify_stress(void)
{
    Fixture fixture;
    fixture_create(&fixture, 0x66666666u);
    CHECK(fixture.fx != NULL, "stress fixture allocation");
    if (!fixture.fx) return;
    ambient_fx_set_mode(fixture.fx, AMBIENT_FX_DREAM_CHAIN);
    uint32_t rng = 0x9e3779b9u;
    int16_t block[257u * 2u];
    for (unsigned pass = 0u; pass < 420u; ++pass) {
        rng = rng * 1664525u + 1013904223u;
        AmbientFxParameters p = ambient_fx_world_parameters((AmbientFxWorld)(pass % AMBIENT_FX_WORLD_COUNT));
        p.space = (float)(rng & 255u) / 255.0f;
        p.echo = (float)((rng >> 8) & 255u) / 255.0f;
        p.shimmer = (float)((rng >> 16) & 255u) / 255.0f;
        p.age = (float)((rng >> 24) & 255u) / 255.0f;
        p.blur = (float)((rng >> 4) & 255u) / 255.0f;
        ambient_fx_set_parameters(fixture.fx, p);
        if ((pass % 71u) == 0u) {
            ambient_fx_trigger_reverse_swell(fixture.fx, 110.0f + (float)(rng % 600u),
                                             0.5f, 0.25f);
        }
        for (unsigned i = 0u; i < 514u; ++i) {
            rng = rng * 1664525u + 1013904223u;
            block[i] = (int16_t)(rng >> 18);
        }
        ambient_fx_process_i16(fixture.fx, block, 257u);
        for (unsigned i = 0u; i < 514u; ++i) {
            CHECK(block[i] != INT16_MIN, "stress output hit asymmetric hard clip");
        }
    }
    check_guards(&fixture);
    fixture_destroy(&fixture);
}

int main(void)
{
    verify_basics();
    verify_bypass();
    for (int mode = AMBIENT_FX_DARK_REVERB; mode < AMBIENT_FX_MODE_COUNT; ++mode) {
        verify_mode((AmbientFxMode)mode);
    }
    verify_block_invariance();
    verify_delay_timing();
    verify_reverb_tail();
    verify_true_reverse();
    verify_stress();

    AmbientFxConfig config = ambient_fx_default_config();
    printf("effects verification: %llu checks, %llu failures\n",
           (unsigned long long)checks, (unsigned long long)failures);
    printf("effects memory: state=%zu/%u, default arena=%zu/%u bytes\n",
           ambient_fx_state_bytes(), AMBIENT_FX_STATE_BYTES,
           ambient_fx_memory_required(&config),
           AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES);
    return failures ? 1 : 0;
}
