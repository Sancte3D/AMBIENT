# Firmware integration

This package intentionally changes no existing firmware folder. Integrate it
only after review by adding the selected synthesis, visual, and effects source
files to the product build and adapting the existing engine callbacks.

## Minimal adapter sketch

```c
#include "ambient_models.h"

static AmbientSynth g_ambient;

void product_ambient_init(void) {
    ambient_synth_init(&g_ambient, AMBIENT_NACRE_HORIZON, device_unique_seed());
}

void product_ambient_note_on(uint8_t cell, float hz, float velocity) {
    float pan = ((float)cell - 2.0f) * 0.32f;
    note_ids[cell] = ambient_synth_note_on(&g_ambient, hz, velocity, pan);
}

void product_ambient_note_off(uint8_t cell) {
    ambient_synth_note_off(&g_ambient, note_ids[cell]);
}

void product_ambient_render(int16_t *stereo, int frames) {
    ambient_synth_render(&g_ambient, stereo, (size_t)frames);
}
```

The actual product should route notes through its existing harmonic brain and
mix this output before the final product limiter. Do not run two master spatial
processors in series by default; either set this module's SPACE lower or bypass
the older wet path for this sound family.

## Parameter mapping

Suggested mapping to the current product language:

| Product control | Model macro |
|---|---|
| brightness / mood | `color` |
| motion | `motion` |
| space | `space` |
| texture / age | `texture` |
| stereo option | `width` |
| per-engine trim | `level` |

Write targets from the UI/control thread. The audio callback reads and smooths
them without allocation. If the target compiler does not guarantee atomic
32-bit float stores, place the assignments behind the firmware's existing
control mailbox.

## Master-effects integration

The complete implementation and parameter contract are in
[`EFFECTS_ENGINE.md`](EFFECTS_ENGINE.md). The minimum product shape is:

```c
#include "ambient_effects.h"

static AmbientFxStorage g_fx_storage;
__attribute__((aligned(32)))
static uint8_t g_fx_arena[AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES];
static AmbientFx *g_fx;

void product_fx_init(void) {
    AmbientFxConfig config = ambient_fx_default_config();
    g_fx = ambient_fx_init(&g_fx_storage, g_fx_arena, sizeof(g_fx_arena),
                           &config);
    if (g_fx != NULL) {
        ambient_fx_set_world(g_fx, AMBIENT_FX_TOKYO_CITY);
        ambient_fx_set_mode(g_fx, AMBIENT_FX_DREAM_CHAIN);
    }
}

void product_fx_render_i16(int16_t *stereo, size_t frames) {
    if (g_fx != NULL) ambient_fx_process_i16(g_fx, stereo, frames);
}
```

Prefer `ambient_fx_process_f32()` on the internal floating mix immediately
before the one final product limiter. The signed-16 path is provided for the
current interleaved SAI-buffer boundary.

Do not keep the current product's full echo, blur, reverb, tape, and shimmer
buffers resident beside this engine. Release or alias those old regions and
place the effects arena deliberately in internal SRAM; it is hot per-sample
memory and is not suitable for QSPI PSRAM. The default arena requires 214,489
bytes including alignment allowance. Check the final `.map` file before target
testing.

Map the existing Space, Atmosphere, Echo, Motion, Age, Shimmer, and Blur menu
items directly to the matching `AmbientFxParameters` fields. Apply world
selection with `ambient_fx_set_world()` and explicit effect-page selection with
`ambient_fx_set_mode()`. Both parameter and mode transitions are smoothed in
the render path.

For a generative event, call `ambient_fx_trigger_reverse_swell()` at the same
time that the composer schedules the future note. A true reverse tail before an
unpredictable live key press is possible only by delaying that note; the
offline/look-ahead API makes this latency explicit.

## Display integration

Feed already-smoothed 0–1 audio features to `ambient_visual_render` at 24–30
fps. The function writes an entire packed frame and clips every primitive to
the display bounds. If the product reserves a status strip, apply a clip
rectangle in the product adapter or offset the visual region before merge.
`AmbientVisual` exposes eighteen modes; indexes 0–4 are the foundational set
and indexes 5–17 are the newer motion studies documented in
`MOTION_STUDY_CATALOG.md`.

For colour, allocate one `uint16_t palette_lut[16]` in the display driver and
call `ambient_palette_build_rgb565(selected_palette, phase, palette_lut)` at UI
rate. During LCD row conversion, map each nibble through that LUT. This replaces
the existing accent-grey LUT lookup and consumes 32 bytes without changing the
framebuffer or SPI transfer size.

## Target acceptance gates

- no DMA underruns during a two-hour maximum-load soak;
- no output discontinuity when macros move from 0 to 1;
- model switching occurs while muted or under a short fade;
- stack and static RAM map remain within the hardware release budget;
- effects arena replaces the old spatial buffers and resides in internal SRAM;
- worst-case DREAM CHAIN remains below 60 percent of one DMA-half deadline;
- display render does not run in the audio interrupt;
- final speaker/DAC path is checked for peak and DC on hardware.
