#ifndef FAM_TEXTURE_H
#define FAM_TEXTURE_H

/*
 * famTexture — Step 10. The non-melodic noise bed under everything.
 * Ported from the Web-Audio `ensureTexture`.
 *
 * One always-present voice (not per-note):
 *   - rumble : brown noise → lowpass 220 Hz             (gain 0.35)
 *   - breath : brown noise → bandpass 600 Hz Q1.6, centre swept by a
 *              0.052 Hz sine ±260 Hz, amplitude pulsed by a 0.04 Hz sine
 *              (gain 0.5 ±0.22)
 *   - both   → mix → warm lowpass 4500 Hz → slow amp → dry + reverb send
 *
 * Stereo enhancement over the mono webapp: L and R run independent brown-
 * noise streams through the same (control-rate-shared) filter settings, so
 * the bed is wide and decorrelated rather than a single mono channel.
 *
 * Amount 0..1 maps to the bed's target amplitude (×0.12, as in the webapp).
 * At amount 0 the module early-outs and costs almost nothing — it boots at
 * 0 so the SPEC §8 pop-suppressed power-up still streams true silence.
 */

#include <stdint.h>

void texture_init(void);

/* Set the bed amount 0..1 (smoothed internally with a slow ~2 s glide so the
 * bed blooms in rather than snapping). */
void texture_set_amount(float amount_0_1);

/* ADD the texture's stereo output into the dry buffers and a `send_amount`-
 * scaled copy into the reverb-send buffers (float, separate-channel, length
 * `frames`). Caller clears the buffers before the block's first add.
 * Early-outs to a no-op while the bed is effectively silent. */
void texture_render_mix(float *dry_L, float *dry_R,
                        float *send_L, float *send_R,
                        int frames, float send_amount);

#endif
