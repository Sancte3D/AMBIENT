# AMBIENT effects engine

This module implements the complete ambient, dreamy, and nostalgic processing
set as deterministic C11 DSP for the STM32H743 product path. It is not a plug-in
wrapper and contains no host-only DSP dependency.

## Fixed target contract

| Item | Contract |
|---|---|
| sample rate | 44,100 Hz, fixed and checked at initialization |
| channels | stereo |
| product callback | arbitrary frame count; current product block is 512 frames |
| dry-path latency | zero frames in every real-time mode |
| allocation in render | none |
| locks, I/O, logging in render | none |
| parameter changes | clamped and sample-smoothed |
| state | caller-owned `AmbientFxStorage` |
| delay memory | caller-owned, internal CPU-accessible SRAM |

At 44.1 kHz, the current 512-frame DMA half-buffer is about 11.61 ms. The
module is block-size invariant, but the full product still needs the existing
DWT deadline profiler on real STM32H743 silicon.

## Signal flow

The individual modes expose one algorithm at a time. `AMBIENT_FX_DREAM_CHAIN`
uses this restrained order:

```text
input
  -> tape age
  -> chorus / detune
  -> optional two-grain blur
  -> filtered cross-feedback echo
  -> dark eight-line FDN with restrained octave regeneration
  -> DC blocker
  -> bounded soft limiter
  -> output
```

Delay and reverb are not inserted as fully wet serial processors. Their returns
are mixed around a direct path, so sparse playing remains articulate and every
live mode reports zero dry-path latency.

## Implemented effects

### Long dark reverb

- Eight delay lines with mutually different lengths.
- Energy-preserving Householder feedback matrix.
- Line-specific feedback derived from a 1.65–7.5 second RT60 range.
- Input high-pass around 175 Hz, so sub and bass do not make the tank unstable.
- Tone-dependent damping from roughly 2.1 to 7.3 kHz.
- Sub-sample slow modulation reduces static metallic ringing.
- Stereo injection and output signs create width without a second reverb.

`space` controls decay and room scale. `atmosphere` controls the audible return.
Keeping these separate prevents “bigger room” from automatically meaning
“completely washed out.”

### Filtered ping-pong delay

- Fractional read position.
- Cross-feedback: left returns to right and right returns to left.
- Repeats become darker through a 1.7–6.4 kHz one-pole filter.
- Feedback is limited to a maximum of 0.56.
- Default world timings: 360, 430, 500, and 570 ms.

Delay-time movement is deliberately much slower than wet/feedback movement.
Changing it live therefore produces a restrained tape-like bend instead of a
hard pointer jump.

### Slow chorus and detune

- Left base delay: 18 ms, up to ±5.5 ms modulation.
- Right base delay: 22 ms, up to ±6.5 ms modulation.
- Independent rates around 0.17 and 0.23 Hz.
- Fractional linear reads and a retained direct path.
- `motion` controls depth; `width` controls cross-channel spread.

### Tape age

- Slow wow, faster flutter, and a bounded random drift component.
- Causal variable delay with about 3.6 ms center delay.
- Progressive high-frequency loss.
- Bounded nonlinear saturation.
- Small stereo collapse, deterministic hiss, and very low 50 Hz hum.
- The direct path remains present, so the effect does not add instrument
  latency even at high `age` values.

### Reverse processing

Two different implementations are required because time direction matters.

1. `ambient_fx_trigger_reverse_swell()` is the live product implementation.
   The generative composer calls it before a note that is already scheduled.
   It creates a rising, spectrally opening pre-tail and does not delay playing.
2. `ambient_fx_reverse_reverb_offline_f32()` is the true algorithm: reverse the
   clip, run the reverb, reverse the wet result, and place the dry event after
   the chosen look-ahead tail. It is intended for host rendering or a path that
   explicitly accepts seconds of latency.

A true reverse reverb cannot precede an unpredictable human key press without
delaying that key press by the complete look-ahead window. The API keeps that
causality explicit instead of hiding a large latency.

### Shimmer reverb

- Dual overlapping variable-delay heads shift the wet field one octave up.
- The shifted signal is high-passed near 480 Hz.
- Only the wet field regenerates into the FDN.
- The public control is squared and capped at 0.22 regeneration gain.
- The dry path is never pitch-shifted.

The cap is intentional. The product sound becomes generic and harmonically
crowded when shimmer is treated as the main body instead of a quiet halo.

### Blur

- Two deterministic overlapping time grains.
- Randomized but seeded read offsets inside a bounded 220 ms history.
- Grain travel follows `motion`; mix follows `blur`.
- Tone-dependent low-pass removes clicky grain edges.
- No FFT and no allocation.

This is a temporal cloud, not a spectral-freeze claim.

## Product controls

| Existing product item | API field | Meaning |
|---|---|---|
| Space | `space` | room scale and decay |
| Atmosphere | `atmosphere` | total spatial send |
| Echo | `echo` | delay return and safe feedback |
| Motion | `motion` | chorus, grain, and room motion |
| Age | `age` | tape instability and degradation |
| Shimmer | `shimmer` | octave halo regeneration |
| Blur | `blur` | time-grain smear |
| world descriptor | `delay_seconds`, `tone`, `width` | identity defaults |
| per-engine trim | `level` | final bounded gain |

The four supplied world defaults are `TOKYO CITY`, `CRYSTAL COAST`,
`MIDNIGHT DRIVE`, and `AFTER HOURS`.

## Memory

Default configuration:

| Region | Measured size |
|---|---:|
| private control state | 680 bytes |
| complete hot buffer arena | 214,489 bytes including alignment allowance |
| declared arena budget | 245,760 bytes |

The arena contains compact signed-16-bit histories for tape, chorus, the
750 ms stereo echo, the 220 ms stereo blur, the octave shifter, and eight FDN
lines. Processing and filter state remain float.

The arena is hot: it is read and written on every sample. Place it in internal
SRAM, not serial external PSRAM. The current product already owns large echo,
blur, reverb, tape, and shimmer buffers. This module is therefore a replacement
path, not another full chain to add beside all of them. Reclaim or reuse the old
effect storage before enabling this complete arena; verify the final linker map.

## Minimal standalone adapter

```c
#include "ambient_effects.h"

static AmbientFxStorage g_fx_storage;
__attribute__((aligned(32)))
static uint8_t g_fx_arena[AMBIENT_FX_DEFAULT_ARENA_BUDGET_BYTES];
static AmbientFx *g_fx;

void product_fx_init(void)
{
    AmbientFxConfig config = ambient_fx_default_config();
    g_fx = ambient_fx_init(&g_fx_storage, g_fx_arena, sizeof(g_fx_arena),
                           &config);
    if (!g_fx) {
        /* Fail closed: keep the product on its dry path. */
        return;
    }
    ambient_fx_set_world(g_fx, AMBIENT_FX_TOKYO_CITY);
    ambient_fx_set_mode(g_fx, AMBIENT_FX_DREAM_CHAIN);
}

void product_fx_render(int16_t *interleaved_stereo, size_t frames)
{
    if (g_fx) ambient_fx_process_i16(g_fx, interleaved_stereo, frames);
}
```

The aligned static array lands in the default zero-initialized SRAM section.
For the final product build, give it a deliberate linker section that occupies
memory released by the old global effect buffers. Do not simply add another
214 KB to the current map and assume it fits.

For highest quality, call `ambient_fx_process_f32()` on the internal floating
mix immediately before the one final product limiter. The in-place signed-16
path exists for the current SAI buffer contract and isolated model testing.

## Generative reverse scheduling

```c
/* Composer knows the event will occur 1.4 seconds from now. */
ambient_fx_trigger_reverse_swell(g_fx, next_note_hz, 0.55f, 1.4f);

/* Schedule the actual note for the same lead time. */
composer_schedule_note(next_note_hz, 1.4f);
```

Calling both operations together makes the swell end at the note without
delaying a user-played cell.

## True offline reverse render

```c
size_t written = ambient_fx_reverse_reverb_offline_f32(
    g_fx,
    input, input_frames,
    output, input_frames + tail_frames,
    tail_frames,
    0.70f, 0.72f);
```

Use a separate effect instance or reset the live one afterwards, because the
offline function intentionally consumes and resets its reverb tank.

## Verification

```sh
make verify
make sanitize
make effects-benchmark
make effect-previews
```

The effect test covers:

- exact block-size invariance;
- deterministic seeded modulation and noise;
- guard bytes around the caller arena;
- all modes under finite/peak/stereo checks;
- configured ping-pong arrival time;
- long reverb-tail survival;
- real anticipatory energy from true reverse rendering;
- moving-parameter stress;
- AddressSanitizer and UndefinedBehaviorSanitizer.

Host benchmarking is a regression gate only. Before product integration, run
the existing STM32 DWT profiler with the maximum synth voice count, full
`DREAM CHAIN`, display/LED traffic, and encoder activity. Acceptance remains
zero deadline misses and less than 60 percent peak callback load.
