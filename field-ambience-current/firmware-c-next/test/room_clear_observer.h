#ifndef FAM_TEST_ROOM_CLEAR_OBSERVER_H
#define FAM_TEST_ROOM_CLEAR_OBSERVER_H
/* Include libc before the macro: fortified/redirected declarations must keep
 * their original name. Only explicit Room calls use the test observer. */
#include <string.h>
extern void *room_clear_observe_memset(void *,int,size_t);
#define memset room_clear_observe_memset
#endif
