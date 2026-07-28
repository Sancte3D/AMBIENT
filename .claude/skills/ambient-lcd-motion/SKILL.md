---
name: ambient-lcd-motion
description: Design, implement, profile, or review smooth animation on AMBIENT's ST7789 LCD, including easing, fixed-step state updates, frame pacing, dirty rectangles, DMA scheduling, interruption, tearing avoidance, and audio-safe performance budgets.
---

# AMBIENT LCD Motion

Optimize for consistent motion and uninterrupted audio, not for the highest nominal frame rate.

## Budget before building

1. Read `../ambient-ui-lcd-smt/references/project-baseline.md`.
2. Read the current panel dimensions, SPI clock, display driver, memory map, and audio IRQ rules.
3. Compute bytes and wire time for every animated region:

   `bytes = width × height × 2`

   `wire_seconds = bytes × 8 / spi_hz`

4. Reserve margin for window commands, DMA setup, cache maintenance, rendering, interrupts, and region coalescing.
5. Pick a target cadence the real path can hold at p99. Stable 30 fps is better than oscillating between 60 and 20 fps.

Use `.claude/skills/ambient-lcd-motion/scripts/frame_budget.py` for repeatable transfer calculations:

```bash
python3 .claude/skills/ambient-lcd-motion/scripts/frame_budget.py \
  --spi-mhz 30 --fps 60 --overhead-percent 15 \
  --region full=320x170 --region strip=320x40 --region tile=80x80
```

At the current 30 MHz SPI:

- full 320 × 170 RGB565 frame: about 29.0 ms before overhead;
- 320 × 40 strip: about 6.83 ms;
- 80 × 80 tile: about 3.41 ms.

Do not promise 60 fps full-screen animation on this bus.

## Motion architecture

- Advance animation state from monotonic elapsed time, not from rendered-frame count.
- Clamp extreme elapsed-time jumps after pause or debugger stop.
- Separate input/state update, rasterization, dirty-region collection, and asynchronous flush.
- Keep at most one display transfer owner.
- Coalesce overlapping dirty rectangles, but do not expand small changes into full-screen refreshes without measurement.
- Budget source storage as well as transfer buffers. A crossfade needs simultaneous access to old and new pixels; two DMA line buffers do not provide those two source frames.
- If two source surfaces do not fit, re-render deterministic old/new content per row or region, retain only the minimum old-state snapshot, or choose a wipe/dissolve that needs one source. Measure the added CPU cost.
- Retarget active animations from their current value when new input arrives; do not queue stale transitions.
- Keep essential feedback visible even when optional frames are dropped.
- Avoid heap allocation, blocking waits, logging, filesystem access, and unbounded loops in frame-critical paths.

## Motion language

Use motion to show:

- continuity between values or screens;
- cause and effect from a hardware control;
- entering or leaving a temporary mode;
- progress only when real progress exists.

Starting ranges, to be tuned on hardware:

| Motion | Duration | Curve |
|---|---:|---|
| Encoder value settle | 80–140 ms | ease-out cubic |
| Focus/selection shift | 120–180 ms | ease-out cubic |
| Overlay enter/exit | 140–220 ms | ease-out / ease-in |
| World transition | 300–600 ms | staged crossfade or wipe in bounded regions |

Avoid elastic overshoot for precise parameter values. Use springs only with bounded, deterministic integration and a clear semantic reason.

## H7 and audio safety

- Keep display DMA below audio DMA priority.
- Use only DMA-accessible memory for DMA1/DMA2 transfers.
- Align cache-maintained buffers to the H7 D-cache line and clean exact expanded ranges before DMA reads.
- Do not add a full RGB565 double buffer without a current linker-map budget.
- Preserve the current row-pipelined line-buffer approach unless measurement proves another design better.
- Prevent framebuffer mutation races while a region is being converted or transferred.

## Prove smoothness

Measure:

- update and render CPU time;
- submitted-region area and bytes;
- DMA busy time;
- achieved frame interval p50, p95, and p99;
- dropped/coalesced frames;
- audio underruns and worst audio ISR latency;
- visible tearing and input-to-feedback latency on the real panel.

Run rapid encoder input, repeated world switching, maximum audio load, low-power/backlight cases, and a multi-hour soak.
