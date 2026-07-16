# Firmware integration

This package intentionally changes no existing firmware folder. Integrate it
only after review by adding the two implementation files to the product build
and adapting the existing engine callbacks.

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

## Display integration

Feed already-smoothed 0–1 audio features to `ambient_visual_render` at 24–30
fps. The function writes an entire packed frame and clips every primitive to
the display bounds. If the product reserves a status strip, apply a clip
rectangle in the product adapter or offset the visual region before merge.

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
- display render does not run in the audio interrupt;
- final speaker/DAC path is checked for peak and DC on hardware.
