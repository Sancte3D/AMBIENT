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
#endif
