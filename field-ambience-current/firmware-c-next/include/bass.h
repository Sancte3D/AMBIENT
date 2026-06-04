#ifndef FAM_BASS_H
#define FAM_BASS_H

/*
 * famSubBass + famDeepBass — Step 8. The two-layer bass under the chord root.
 * Ported from the webapp `_makeSubBass` / `_makeDeepBass`.
 *
 *   famSubBass : pure low sine + tri@2× (0.08) → lowpass 90 Hz, very slow
 *                3 s bloom, 0.04 Hz breath LFO on amplitude. Tuned 2 octaves
 *                below the chord root. Reverb send 0.03.
 *   famDeepBass: sine + tri@2× (0.06) → tanh saturation → highpass 50 →
 *                lowpass 350 (Q 1.8), 2.5 s bloom. Tuned 1 octave below the
 *                root. Reverb send 0.08.
 *
 * Both are single mono voices (bass is non-directional — written equally to
 * L and R). They follow the lowest currently-held note: `bass_note` glides
 * the pitch (legato portamento) while held and only blooms from idle on the
 * first note; `bass_release` fades both tails out.
 *
 * IMPORTANT on the 380 Hz onboard speaker: famSubBass (LP 90) is entirely
 * below what the driver reproduces, and the lower half of famDeepBass too —
 * onboard, this layer is heard mostly through its reverb send (warmth in the
 * tail) and over Line-Out. That's expected (SPEC §8 r14); the layer still
 * matters for the full mix and external monitoring.
 */

#include <stdint.h>
#include <stdbool.h>

void bass_init(void);

/* Depth 0..1 → both layers' peak amplitude (sub 0.06..0.20, deep 0.04..0.16,
 * matching the webapp's depth linlin). Smoothed via the envelopes. */
void bass_set_depth(float depth_0_1);

/* Follow the chord root: `lowest_freq_hz` is the lowest held note. From idle
 * this blooms the bass in; while already sounding it glides the pitch. Sub is
 * placed 2 octaves below, deep 1 octave below. */
void bass_note(float lowest_freq_hz);

/* Release both layers into their exponential tails (all notes lifted). */
void bass_release(void);

/* ADD the bass into the dry buffers (mono → both channels) and each layer's
 * own send (sub 0.03, deep 0.08) into the reverb-send buffers. */
void bass_render_mix(float *dry_L, float *dry_R,
                     float *send_L, float *send_R, int frames);

/* True while either layer is still producing sound (incl. release tail). */
bool bass_active(void);

#endif
