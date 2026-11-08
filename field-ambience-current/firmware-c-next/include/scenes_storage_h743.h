#ifndef FAM_SCENES_STORAGE_H743_H
#define FAM_SCENES_STORAGE_H743_H
#include <stdbool.h>
/* Session candidate only. Call after scenes_init, before starting audio.
 * Boot recovery reads only; subsequent writes use scenes_save_step in Main. */
bool scenes_storage_h743_bind(void);
#endif
