# Resource and quality review

## Enforced budgets

| Resource | Limit | Enforcement |
|---|---:|---|
| active audio context | 32,768 bytes | C11 static assertion + host test |
| effects control state | 4,096 bytes | opaque storage assertion + host test |
| effects hot arena | 245,760 bytes | queried fixed layout + guarded host test |
| visual state | 2,048 bytes | C11 static assertion + host test |
| packed framebuffer | 27,200 bytes | API constant + host test |
| animated RGB565 palette LUT | 32 bytes | fixed 16-entry table + host test |
| additional framebuffer | 0 bytes | direct packed rendering |
| audio allocation in render | 0 | fixed state and stack-only block output |

The audio context shares one chorus memory, one four-line spatial network, and
four compact feedback-string buffers across all models. Loading another model
reinitializes that same memory instead of keeping ten engines resident.

The effects engine is a separate master-processing option. Its measured private
state is 680 bytes and its default complete arena requirement is 214,489 bytes,
including worst-case start-alignment allowance. That arena replaces the
current product's old effect-delay regions; it must not be added while the old
echo, blur, reverb, tape, and shimmer histories remain resident. All histories
are signed 16-bit, while accumulators and filters remain floating point.

## Tests supplied

- deterministic output for an identical seed;
- exact output regardless of render block partitioning;
- silence before note input;
- signal, stereo, and clipping-margin checks for every model;
- control/event stress across every delay-line wrap point;
- bit-exact effects bypass and exact effects block-partition invariance;
- guarded effects arena with undersized-arena rejection;
- all nine processed effect modes under finite, bounded, energy, and stereo
  checks;
- filtered ping-pong timing, long-tail reverb survival, and true reverse
  pre-tail alignment checks;
- randomized effects-parameter and mode-transition stress;
- framebuffer guard checks for every visual;
- animated/non-empty framebuffer checks;
- host render benchmark for all eighteen visual modes;
- twelve animated palette identity, range, and RGB565 uniqueness checks;
- tone-14 neon-chroma checks at quiet and peak palette phases;
- GIF saturation and midpoint-to-PNG colour-fidelity gates;
- 26 motion GIF checks for geometry, exact frame count, animation, chroma,
  distinct first frames, and bounded loop seams;
- AddressSanitizer and UndefinedBehaviorSanitizer build;
- source-origin audit for media and external implementation references;
- SHA-256 source manifest.

## What still requires target hardware

Host speed is a regression indicator, not an STM32H743 proof. Before merging
into the product firmware, capture worst-case cycles in the real DMA callback
with ten active voices, maximum SPACE, display updates, storage traffic, and
the rest of the product mix enabled. The acceptance gate should be based on the
existing audio profiler and include a multi-hour underrun soak.

## Highest-value improvements under the small-capacity constraint

1. Keep only four to six simultaneous voices for the densest models on target;
   the spatial field supplies perceived size more cheaply than extra voices.
2. Replace host `sqrtf` panning with a 65-entry equal-power table if target
   profiling shows it is material.
3. Render visuals at 24 fps and reuse the product's control-rate band followers.
4. Give the feedback field two target-tuned damping profiles rather than adding
   a second reverb.
5. Add a hard voice-count telemetry counter so real songs reveal whether ten
   voices are ever perceptually necessary.

Large sample libraries, convolution, per-model reverbs, a second framebuffer,
and an FFT dedicated to visuals are intentionally excluded.

The complete effects path likewise excludes convolution, FFT processing,
dynamic allocation, locks, and external serial-RAM access in the callback. The
host benchmark measured DREAM CHAIN at roughly 95× real time, but only target
DWT cycle measurement can establish the STM32H743 release margin.

The thirteen additional motion systems increase flash/code size and the test
surface, but not visual-state RAM, framebuffer RAM, SPI payload size, or palette
LUT size. The host run measured the slowest renderer at roughly 59 microseconds
per frame; use this only as a regression baseline until the target cycle count
is captured.

The saturation-preserving palette morph increased the `-Os` host object text
from 1,423 to 2,375 bytes (**+952 bytes**) while object data remained 576 bytes.
Target linking may differ, but this bounds the order of magnitude: the neon pass
costs roughly one kilobyte of flash and zero additional state RAM.

## Cinematic master-loop capacity

The ten high-quality loops are offline art masters. Their 2× working canvas,
NumPy arrays, Pillow layers, and Gaussian bloom exist only on the development
host and therefore do not alter the enforced audio, visual-state, or packed
framebuffer budgets above.

The generated GIFs total roughly 9 MB and range from about 316 KB to 1.6 MB per
loop. Shipping all of them in internal MCU flash would contradict the small
capacity goal. The current recommendation is to test the loops on the physical
panel, choose three to five, and then select one measured playback path:

- external-flash scanline streaming;
- one 54,400-byte 8-bit indexed frame plus a per-loop palette;
- or a strict 27,200-byte procedural 4-bit port using position-derived hue.

The third path preserves the existing framebuffer budget but is not expected
to be pixel-identical to the masters. `HIGH_QUALITY_VISUALS.md` records this
distinction so review GIF quality is never mistaken for completed target
firmware capability.
