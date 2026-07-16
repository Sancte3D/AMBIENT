#include "ambient_models.h"
#include "ambient_palettes.h"
#include "ambient_visuals.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
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

static uint32_t hash_bytes(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < length; ++i) {
        h = (h ^ bytes[i]) * 16777619u;
    }
    return h;
}

static void seed_chord(AmbientSynth *s)
{
    ambient_synth_note_on(s, 110.0f, 0.62f, -0.65f);
    ambient_synth_note_on(s, 164.81f, 0.55f, -0.18f);
    ambient_synth_note_on(s, 246.94f, 0.48f, 0.26f);
    ambient_synth_note_on(s, 329.63f, 0.42f, 0.67f);
}

static void verify_model(AmbientModel model)
{
    AmbientSynth silent;
    ambient_synth_init(&silent, model, 100u + (uint32_t)model);
    int16_t silence[512 * 2];
    memset(silence, 0x7f, sizeof(silence));
    ambient_synth_render(&silent, silence, 512u);
    for (unsigned i = 0; i < 1024u; ++i) CHECK(silence[i] == 0, "fresh synth must be silent");

    AmbientSynth a, b;
    ambient_synth_init(&a, model, 0x12340000u + (uint32_t)model);
    ambient_synth_init(&b, model, 0x12340000u + (uint32_t)model);
    seed_chord(&a);
    seed_chord(&b);
    int16_t large[4096 * 2];
    int16_t chunked[4096 * 2];
    ambient_synth_render(&a, large, 4096u);
    for (unsigned offset = 0; offset < 4096u; offset += 127u) {
        unsigned n = 4096u - offset;
        if (n > 127u) n = 127u;
        ambient_synth_render(&b, chunked + offset * 2u, n);
    }
    CHECK(memcmp(large, chunked, sizeof(large)) == 0, "render must be block-size invariant");
    CHECK(hash_bytes(&a, sizeof(a)) == hash_bytes(&b, sizeof(b)), "state must be deterministic");

    uint64_t energy = 0u;
    uint64_t side_energy = 0u;
    int peak = 0;
    for (unsigned i = 0; i < 4096u; ++i) {
        int l = large[i * 2u];
        int r = large[i * 2u + 1u];
        int al = l < 0 ? -l : l;
        int ar = r < 0 ? -r : r;
        if (al > peak) peak = al;
        if (ar > peak) peak = ar;
        energy += (uint64_t)al + (uint64_t)ar;
        int side = l - r;
        side_energy += (uint64_t)(side < 0 ? -side : side);
        CHECK(al <= 32767 && ar <= 32767, "samples must stay in int16 range");
    }
    CHECK(energy > 100000u, "model output energy is unexpectedly low");
    CHECK(side_energy > 10000u, "model must create a stereo field");
    CHECK(peak < 32767, "model must preserve a clipping margin");

    /* Control and event stress: long enough to exercise all delay wrap points. */
    int16_t stress[257 * 2];
    uint32_t rng = 0x9e3779b9u + (uint32_t)model;
    for (unsigned block = 0; block < 420u; ++block) {
        rng = rng * 1664525u + 1013904223u;
        if ((block % 13u) == 0u) {
            float f = 45.0f + (float)(rng % 3000u) * 0.5f;
            ambient_synth_note_on(&a, f, 0.2f + (float)(rng & 255u) / 320.0f,
                                  (float)((int)(rng >> 24) - 128) / 128.0f);
        }
        AmbientControls c = ambient_model_default_controls(model);
        c.color = (float)(rng & 255u) / 255.0f;
        c.motion = (float)((rng >> 8) & 255u) / 255.0f;
        c.space = (float)((rng >> 16) & 255u) / 255.0f;
        c.texture = (float)((rng >> 24) & 255u) / 255.0f;
        ambient_synth_set_controls(&a, c);
        ambient_synth_render(&a, stress, 257u);
        for (unsigned i = 0; i < 514u; ++i) {
            int sample = stress[i];
            CHECK(sample >= -32767 && sample <= 32767, "stress sample out of range");
        }
    }
}

static void verify_visual(AmbientVisual visual)
{
    struct Guarded {
        uint8_t before[32];
        uint8_t fb[AMBIENT_DISPLAY_BYTES];
        uint8_t after[32];
    } guarded;
    memset(&guarded, 0xa5, sizeof(guarded));
    AmbientVisualState state;
    ambient_visual_init(&state, visual, 0xbadc0deu + (uint32_t)visual);
    uint32_t first_hash = 0u;
    uint32_t last_hash = 0u;
    uint64_t lit = 0u;
    for (unsigned frame = 0; frame < 180u; ++frame) {
        AmbientVisualInput input = {
            .rms = 0.2f + (float)(frame % 20u) / 30.0f,
            .bass = (float)(frame % 37u) / 36.0f,
            .mid = (float)(frame % 53u) / 52.0f,
            .high = (float)(frame % 29u) / 28.0f,
            .centroid = 0.5f,
            .beat_phase = (float)(frame % 24u) / 24.0f,
            .sound_model = (uint8_t)frame
        };
        ambient_visual_render(&state, guarded.fb, input);
        uint32_t h = hash_bytes(guarded.fb, sizeof(guarded.fb));
        if (frame == 0u) first_hash = h;
        if (frame == 179u) last_hash = h;
        for (unsigned i = 0; i < sizeof(guarded.fb); ++i) {
            if (guarded.fb[i]) ++lit;
        }
    }
    for (unsigned i = 0; i < 32u; ++i) {
        CHECK(guarded.before[i] == 0xa5, "visual wrote before framebuffer");
        CHECK(guarded.after[i] == 0xa5, "visual wrote after framebuffer");
    }
    CHECK(lit > 1000u, "visual is unexpectedly empty");
    CHECK(first_hash != last_hash, "visual must animate");
}

int main(void)
{
    CHECK(ambient_synth_state_bytes() <= AMBIENT_STATE_BUDGET_BYTES, "audio RAM budget");
    CHECK(ambient_visual_state_bytes() <= AMBIENT_VISUAL_STATE_BUDGET_BYTES, "visual RAM budget");
    CHECK(AMBIENT_DISPLAY_BYTES == 27200u, "packed framebuffer size");
    CHECK(AMBIENT_VISUAL_COUNT == 18, "visual model count");
    for (int model = 0; model < AMBIENT_MODEL_COUNT; ++model) {
        CHECK(strcmp(ambient_model_name((AmbientModel)model), "UNKNOWN") != 0, "model name");
        verify_model((AmbientModel)model);
    }
    for (int visual = 0; visual < AMBIENT_VISUAL_COUNT; ++visual) {
        CHECK(strcmp(ambient_visual_name((AmbientVisual)visual), "UNKNOWN") != 0, "visual name");
        CHECK(strcmp(ambient_visual_slug((AmbientVisual)visual), "unknown") != 0, "visual slug");
        for (int earlier = 0; earlier < visual; ++earlier) {
            CHECK(strcmp(ambient_visual_slug((AmbientVisual)visual),
                         ambient_visual_slug((AmbientVisual)earlier)) != 0,
                  "visual slugs must be unique");
        }
        verify_visual((AmbientVisual)visual);
    }
    CHECK(ambient_palette_lut_bytes() == 32u, "palette LUT byte cost");
    for (int palette = 0; palette < AMBIENT_PALETTE_COUNT; ++palette) {
        CHECK(strcmp(ambient_palette_name((AmbientPalette)palette), "UNKNOWN") != 0, "palette name");
        uint16_t quiet[16];
        uint16_t alive[16];
        ambient_palette_build_rgb565((AmbientPalette)palette, 0.0f, quiet);
        ambient_palette_build_rgb565((AmbientPalette)palette, 0.5f, alive);
        CHECK(quiet[0] != quiet[15], "palette needs visible dynamic range");
        CHECK(memcmp(quiet, alive, sizeof(quiet)) != 0, "palette must animate");
        unsigned unique = 1u;
        for (unsigned i = 1; i < 16u; ++i) {
            int seen = 0;
            for (unsigned j = 0; j < i; ++j) seen |= quiet[i] == quiet[j];
            if (!seen) ++unique;
        }
        CHECK(unique >= 12u, "palette loses too many 4-bit colours in RGB565");
        for (int phase_step = 0; phase_step < 2; ++phase_step) {
            float phase = phase_step ? 0.5f : 0.0f;
            unsigned previous_luma = 0u;
            for (unsigned tone = 0; tone < 16u; ++tone) {
                uint8_t r, g, b;
                ambient_palette_rgb((AmbientPalette)palette, (uint8_t)tone,
                                    phase, &r, &g, &b);
                unsigned luma = 54u * r + 183u * g + 19u * b;
                CHECK(luma >= previous_luma, "palette luma must follow nibble order");
                previous_luma = luma;
            }
        }
    }
    printf("verification: %llu checks, %llu failures\n",
           (unsigned long long)checks, (unsigned long long)failures);
    printf("memory: audio=%zu/%u, visual=%zu/%u, framebuffer=%u bytes\n",
           ambient_synth_state_bytes(), AMBIENT_STATE_BUDGET_BYTES,
           ambient_visual_state_bytes(), AMBIENT_VISUAL_STATE_BUDGET_BYTES,
           AMBIENT_DISPLAY_BYTES);
    return failures ? 1 : 0;
}
