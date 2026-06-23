#ifndef FAM_ECHO_H
#define FAM_ECHO_H

/*
 * Echo — tape-style stereo delay (ADR-0017 Phase ✦ post-Phase-4).
 *
 * One macro ("Echo") hides 4 internal params:
 *   • time        — 100..600 ms delay
 *   • feedback    — 0..0.85 (capped < 1 for stability)
 *   • wet         — 0..0.75 mix
 *   • tone        — 1-pole lowpass in the FEEDBACK path (0..1 dark..bright).
 *                   Each repeat gets a bit darker → the classic tape echo
 *                   character (Signalsmith reverb-example-code + DaisySP
 *                   `Tone` reference; SuperCollider CombC + LPF analog).
 *
 * Lifted in concept from the textbook circular-buffer delay line; the
 * tape-darkening lowpass-in-feedback trick is what makes a digital delay
 * sound like a vintage tape unit instead of a clinical sampler.
 *
 * Buffer per channel = ECHO_MAX_TIME_S × DSP_SAMPLE_RATE_HZ floats. At
 * 600 ms × 44.1 kHz = 26 460 samples × 4 B = 103 KB total stereo. Fits in
 * AXI-SRAM (H743) with room to spare.
 *
 * License note: no GPL-linked code is included — Signalsmith's example
 * code is referenced for design, SuperCollider's algorithms are public
 * DSP knowledge, our implementation is from-scratch C.
 */

#include <stdint.h>

void echo_init(void);

/* Set the macro 0..1. Smoothed per-block. Maps:
 *   echo=0           → silent (wet=0, feedback=0)
 *   echo=0.3..0.5    → noticeable echo, lively repeats
 *   echo=0.7..1.0    → ambient wash, dark hypnotic feedback
 * Out-of-range is clamped. */
void echo_set_amount(float amount_0_1);

/* Add the delayed signal into the engine bus. The DRY input (the current
 * pad+ambience+bass+drone mix) is read from dry_L/dry_R and re-injected
 * via the delay line; the wet output is summed back into the same
 * dry_L/dry_R. send_amount routes a copy into the reverb-send bus so the
 * echoed signal can also pick up room (set 0 to keep echo dry). */
void echo_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R,
                     int frames, float send_amount);

#endif
