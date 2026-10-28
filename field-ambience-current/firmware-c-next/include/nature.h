#ifndef FAM_NATURE_H
#define FAM_NATURE_H
#include <stdint.h>
void nature_init(void);
void nature_clear(void); /* audio owner, behind zero output */
void nature_set_amount(float value);
void nature_set_world(int world);
void nature_set_seed(uint32_t seed);
/* Direct bus, independent of Room; zero is optional silence. In the complete
 * sound candidate: COAST surf, WOODLAND rain/distant thunder, HIGHLANDS breeze.
 * Quiet irregular wind belongs to each positive-Nature scene. */
void nature_render(float *l,float *r,int frames);
#ifdef FAM_SOUND_FINISH_CANDIDATE
/* Audio-owned foreground reference, before music FX. reference_gain converts
 * the supplied send back to Dry. Weather/RNG are unchanged; only its return
 * is reduced while music is present, with a slow recovery in real silence. */
void nature_render_music(float *l,float *r,const float *music_l,const float *music_r,
                         float reference_gain,int frames);
#endif
#endif
