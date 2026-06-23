#ifndef FAM_BLUR_H
#define FAM_BLUR_H

/*
 * Blur — granular smear (Reddit "Texture / Smear / Cloud" macro).
 *
 * Captures the current dry-bus signal into a ~200 ms ring buffer and
 * re-emits it as a cloud of overlapping short grains. Each grain has
 * its own position in the source buffer, pitch (playback rate), pan,
 * and Hann-style envelope.
 *
 * One macro 0..1 hides:
 *   • grain density   — 6 grains/s (subtle) … 35 grains/s (cloud)
 *   • grain size      — 30 ms      … 90 ms
 *   • pitch jitter    — ±0 cents   … ±150 cents (per grain)
 *   • position scatter — ±20 ms    … ±90 ms (jitter around recent input)
 *   • wet mix         — 0          … 0.7
 *
 * Architecture inspired by Dumumub Granular (JUCE) and the Mutable
 * Instruments Beads / Clouds family — algorithm-only reference; our
 * implementation is from-scratch C, libm-free.
 *
 * CPU budget: 16 grains × per-sample (read + linear-interp + envelope
 * + 2 pan multiplies) ≈ 64 mul-adds per sample = ~0.6 % of an H743
 * at 480 MHz. Comfortable.
 *
 * Buffer footprint: 200 ms × 44.1 kHz × stereo × float = ~70 KB BSS.
 * Fits AXI-SRAM with plenty of room.
 */

#include <stdint.h>

void blur_init(void);

/* Set the macro 0..1. Smoothed per-block. 0 = wet=0 + grain scheduling
 * paused (cheap silence). */
void blur_set_amount(float amount_0_1);

/* Read the current dry-bus signal AND add the blurred-cloud version back
 * into dry. send_amount routes a copy of the cloud into the reverb send. */
void blur_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount);

#endif
