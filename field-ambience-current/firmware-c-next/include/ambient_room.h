#ifndef FAM_AMBIENT_ROOM_H
#define FAM_AMBIENT_ROOM_H
#include <stdbool.h>
#include <stddef.h>
void ambient_room_init(void);
void ambient_room_clear(void); /* audio owner, behind zero output */
void ambient_room_set(float amount); /* control owner, finite 0..1 */
void ambient_room_enable(bool enabled);
void ambient_room_process(float *l,float *r,const float *sl,const float *sr,int n);
float ambient_room_tail_seconds(void);
float ambient_room_peak(void); /* audio owner, last block wet peak */
size_t ambient_room_storage_bytes(void);
#endif
