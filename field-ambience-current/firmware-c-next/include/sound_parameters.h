#ifndef FAM_SOUND_PARAMETERS_H
#define FAM_SOUND_PARAMETERS_H
#include <stdbool.h>
/* Main-owned musical controls, independent of physical assignments. */
typedef struct {
    float delay,grain,drive,crush,phaser;
    bool freeze,plate;
} sound_parameters_t;
bool engine_apply_sound_parameters(const sound_parameters_t *parameters);
void engine_get_sound_parameters(sound_parameters_t *parameters);
#endif
