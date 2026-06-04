#ifndef FAM_PAD_H
#define FAM_PAD_H

/*
 * famPadCore — the warm detuned-saw pad that is the instrument's core voice
 * (Step 9 of the native port). Ported from the Web-Audio reference
 * `_makePadVoice` in field_ambience_webapp.html.
 *
 * Per pad voice (one per cell tap / chord tone):
 *   - two detuned "sides" (±detune cents) for chorus width
 *   - each side = 3 sawtooths (×1, ×~1.0, ×0.5) + 2 squares (crossfaded in
 *     by voiceMix), summed
 *   - each side through its own 2-pole lowpass (TPT SVF) whose cutoff is
 *     swept by a slow LFO + a filter ADSR + a shared brightness offset
 *   - Haas micro-delay (8/14 ms) + opposing pan → a wide, decorrelated stereo
 *     image
 *   - one shared amp envelope (linear bloom attack, exponential release)
 *
 * Drop-in compatible with the Step-7 voice-pool API (note_on / note_off /
 * render) so main.c only swaps which renderer it registers. Unlike the
 * Step-7 pool, pad_render writes a genuine stereo field (L ≠ R).
 *
 * The harmonic side (which chord, which voicing) is still the Step-7
 * placeholder in main.c; Step 9 is purely the timbre. The harmonic brain
 * (Step 12) will drive real chords through this same note_on.
 */

#include <stdint.h>
#include <stdbool.h>

#define PAD_MAX 8       /* 5 cells + headroom for chord spawns later */

/* Build the pool. Call once after dsp_init(). */
void pad_init(void);

/* Start (or re-trigger) the pad voice owned by `source` at `freq_hz`, peak
 * amplitude `amp` (0..1, before the soft-clipped master). Re-triggering an
 * existing source re-blooms it rather than stacking a second voice. */
void pad_note_on(uint8_t source, float freq_hz, float amp);

/* Release the voice owned by `source` into its exponential tail. */
void pad_note_off(uint8_t source);

/* Release every voice. */
void pad_all_off(void);

/* Shared brightness offset added to every voice's filter cutoff, in Hz
 * (the BRIGHT encoder will drive this later; default 0). Smoothed internally
 * so turning it does not zipper. */
void pad_set_brightness(float hz);

/* Global pad-voice timbre, applied to ALL voices at once and smoothed so the
 * whole stack glides into the new sound together (no old/new timbre clashing).
 * 0 = warm (pure saw), ~0.6 = strings, ~1.2 = brass — webapp PAD_VOICE_MIXES. */
void pad_set_voice_mix(float vmix);

/* Render `frames` interleaved stereo int16 samples (L,R,L,R,…), summing all
 * active pad voices through a soft-clipped master. Audio-context safe:
 * allocation-free and bounded. Standalone form: no reverb, no engine bus.
 * Step 11 replaced this as the live renderer with engine_render(); kept here
 * for fall-back use and existing unit tests. */
void pad_render(int16_t *buf, int frames);

/* Step 11 mix-bus form: ADDS the pad's stereo output into the dry buffer and
 * a `send_amount`-scaled copy into the reverb-send buffer. All four buffers
 * are float, separate-channel, length `frames`. Caller must clear the
 * buffers before the first add of a block. */
void pad_render_mix(float *dry_L, float *dry_R,
                    float *send_L, float *send_R,
                    int frames, float send_amount);

/* Voices not idle — for the OLED / debug. */
int pad_active_count(void);

#endif
