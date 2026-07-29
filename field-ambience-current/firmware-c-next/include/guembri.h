/*
 * guembri.h — plucked low-lute voice (Desert), r19.56 prototype.
 *
 * A guembri / sintir (North-African bass lute): a PLUCKED low string with the
 * characteristic bridge BUZZ (the metal-ring rattle), a warm resonant body that
 * darkens as the note decays, dry and percussive. Deliberately a DIFFERENT
 * synthesis family from the sustained voices (bowed/horn/choir): this one is a
 * decaying pluck, low register, with a short buzzy attack.
 *
 * One guembri_note() = one pluck (fast attack, exponential decay). Hot-path
 * safe (alias-free saw, LUT sine, no per-sample transcendental).
 */
#ifndef GUEMBRI_H
#define GUEMBRI_H

void guembri_init(void);
void guembri_note(float freq_hz, float amp);
int  guembri_active_count(void);
void guembri_render_mix(float *dry_L, float *dry_R,
                        float *send_L, float *send_R,
                        int frames, float send_amount);

#endif /* GUEMBRI_H */
