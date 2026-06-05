#ifndef FAM_REVERB_H
#define FAM_REVERB_H

/*
 * famReverbMaster — Step 11 of the native port.
 *
 * The Web-Audio reference uses a Convolution reverb (createConvolver) with
 * a generated impulse response. That is impossible on RP2350 (too little
 * SRAM, too few cycles), so this port uses the well-known Schroeder /
 * Freeverb structure: 8 parallel feedback combs + 4 series allpasses per
 * channel, with a small pre-delay and a damping lowpass folded into each
 * comb's feedback path. Result is not bit-identical to the convolution
 * version, but renders the same Sound-Constitution shape — slow, dark, wide,
 * never harsh — and is dirt-cheap on the M33.
 *
 * Stereo: L and R combs/allpasses use slightly different delay lengths
 * (Freeverb's classic +23-sample stereo spread) so the wet field is
 * decorrelated even from a mono input — the dry pad already has its own
 * Haas-width from Step 9.
 *
 * Control surface mirrors the .scd / webapp `computeReverb()` outputs:
 *   size   = comb feedback strength → length of the tail
 *   damp   = HF roll-off inside the comb feedback → darker the higher
 *   wet    = dry+wet master ratio (applied by the engine, not in here)
 *   drive  = pre-reverb tanh shaper amount (matches `driven` stage)
 *
 * Block-call `reverb_render` produces wet-only output; the dry mix is the
 * engine's job (see engine.h). Caller is expected to clear out_L/R before
 * the call — reverb_render WRITES (does not add).
 */

#include <stdint.h>

void reverb_init(void);

/* Set room size 0..1 (→ comb feedback 0.7..0.98) and damping 0..1 (→ comb
 * lowpass coefficient). Smoothed internally (one-pole per block) so a user-
 * facing knob does not zipper. Safe to call from any context. */
void reverb_set(float size_0_1, float damping_0_1);

/* Pre-reverb tanh drive 0..1. Matches the webapp's
 *   driven = tanh(in * (1 + drive*4)) / (1 + drive*0.6)
 * stage. Smoothed like size/damp. */
void reverb_set_drive(float drive_0_1);

/* Process `frames` samples. Reads from in_L/in_R, writes wet output to
 * out_L/out_R (overwrites — does not add). All buffers are float, separate-
 * channel (not interleaved). Audio-context safe, allocation-free, bounded. */
void reverb_render(const float *in_L, const float *in_R,
                   float *out_L,       float *out_R,
                   int frames);

#endif
