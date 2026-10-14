#ifndef FAM_SCENES_SAVE_ASYNC_INTERNAL_H
#define FAM_SCENES_SAVE_ASYNC_INTERNAL_H
#include "scenes_save_async.h"
/* Private bridge: scenes.c owns the exact SCN7 ABI and its immutable pending
 * snapshot. Only the prepared controller may capture/publish this buffer. */
const void *scenes_save_snapshot(int slot,unsigned *length);
void scenes_save_publish(int slot,uint32_t now_ms);
#endif
