#ifndef FAM_NATURE_H
#define FAM_NATURE_H
void nature_init(void);
void nature_clear(void); /* audio owner, behind zero output */
void nature_set_amount(float value);
void nature_set_world(int world);
void nature_render(float *l,float *r,int frames); /* direct bus, independent of Room */
#endif
