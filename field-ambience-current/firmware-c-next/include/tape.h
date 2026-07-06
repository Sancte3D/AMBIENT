#ifndef FAM_TAPE_H
#define FAM_TAPE_H

/*
 * Tape character — hiss + warm saturation (ADR-0017 Phase 3).
 *
 * Two short stages that sit at the end of the engine's master path,
 * giving the device a consistent "tape / vinyl / dreamy" colour. Both
 * lifted verbatim from tools/render_dreamy_warm.c (the user-approved
 * "DAS IST ES" reference render).
 *
 *   • hiss        — decorrelated white noise added to the stereo bus.
 *                   Subtle by design (default amp 0.005 ≈ −46 dBFS):
 *                   you don't *hear* it directly, it just adds tape
 *                   colour to whatever is playing.
 *   • saturation  — tanh-based soft drive with implicit −2 dB makeup.
 *                   Default drive 1.10 = barely-there warmth that
 *                   rounds peaks and adds even-harmonic colour.
 *
 * Both are stage operations: hiss ADDS into the bus, saturation
 * REPLACES samples in place. Order in engine.c master stage:
 *     hiss_render_add(L, R, n)  →  saturation_process(L, R, n)
 * (saturation last so the noise also gets the warmth).
 */

void tape_init(void);

/* hiss amplitude in linear units (≈ peak deviation). 0 = silent (still
 * called but adds nothing). Default 0.005 ≈ −46 dBFS. */
void tape_set_hiss_amount(float amp);

/* drive into the tanh. 1.0 = transparent below the saturating knee
 * (but always with the −2 dB scaling). Default 1.10. */
void tape_set_saturation_drive(float drive);

/* Add decorrelated white-noise hiss into L/R. */
void tape_hiss_render_add(float *L, float *R, int frames);

/* r18.89 — vinyl crackle intensity 0..1 (0 = off, the boot default). Driven
 * by the AGE macro together with hiss + saturation: sparse dust ticks through
 * a 2.6 kHz resonator + a faint chaotic fry bed. */
void tape_set_crackle(float v01);
void tape_crackle_render_add(float *L, float *R, int frames);

/* r18.92 — feed the dry-bus block peak so hiss/crackle DUCK in silence
 * (fast attack, ~600 ms release, floored at −10.5/−6 dB). Call once per
 * rendered block, before tape_hiss_render_add. */
void tape_set_program_level(float block_peak);

/* Apply tanh-saturation in place. */
void tape_saturation_process(float *L, float *R, int frames);

/* r18.99 — WOW & FLUTTER: tape pitch instability on the master bus.
 * Depth 0..1 (AGE macro drives it, square curve). depth 0 = true bypass
 * (bit-exact reference); engage/release crossfades over one block. */
void tape_set_wow_depth(float v01);
void tape_wow_process(float *L, float *R, int frames);

#endif
