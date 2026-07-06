#ifndef FAM_SHIMMER_H
#define FAM_SHIMMER_H

/*
 * shimmer.{h,c} — octave-up pitch-shift regeneration around the hall
 * (r18.99).
 *
 * PRINCIPLE learned from the Sonicware Liven Ambient Ø architecture
 * (every layer has a reverb send AND a shimmer send — the shimmer wash is
 * its signature "heaven" register) and from the classic Eno/Lanois studio
 * shimmer (pitch shifter inside the reverb regeneration; Valhalla-style
 * plugins made it famous). No code copied — the loop is reinvented here:
 *
 *   send ──► REVERB ──► wet ──► (output)
 *              ▲                  │
 *              └── LP ◄─ +12 st ◄─┘   × amount
 *
 * The reverb's own output is pitch-shifted one octave up and re-enters
 * the reverb input; every pass climbs another octave and gets darker
 * (low-pass in the loop) → an upward-blooming halo instead of a static
 * layer. Constitution §2 forbids DAUER-shimmer: amount is a macro (menu
 * slot), per-world defaults stay modest, and 0 is bit-exact off.
 *
 * Shifter: dual-tap Doppler reader on a ring (two crossfaded grains, one
 * octave = read rate 2×) — the standard cheap real-time shifter.
 */

void shimmer_init(void);

/* Macro 0..1 → internal loop gain (clamped stable). 0 = hard bypass. */
void shimmer_set_amount(float v01);

/* Call BEFORE reverb_render: adds the shifted return into the send bus. */
void shimmer_feed_add(float *send_L, float *send_R, int frames);

/* Call AFTER reverb_render: captures the wet output into the shifter. */
void shimmer_capture(const float *wet_L, const float *wet_R, int frames);

#endif
