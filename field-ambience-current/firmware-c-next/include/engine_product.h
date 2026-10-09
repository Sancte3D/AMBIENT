#ifndef FAM_ENGINE_PRODUCT_H
#define FAM_ENGINE_PRODUCT_H
#include <stdbool.h>
#include <stdint.h>
void engine_set_activity(float value);
void engine_set_color(float value);
void engine_set_room(float value);
void engine_set_nature(float value);
bool engine_try_note_on(uint8_t source,float actual_hz,float velocity);
int engine_sounding_frequencies(float *out,int max);
uint32_t engine_admission_rejections(void);
uint32_t engine_generative_return_count(void);
uint32_t engine_generative_episode_count(void);
/* Main-only observations of acknowledged phrase development; not controls. */
int engine_generative_phrase_kind(void);
uint32_t engine_generative_memory_return_count(void);
uint32_t engine_output_limited_samples(void);
uint32_t engine_nonfinite_samples(void);
int engine_product_world(void);
int engine_product_collection(void);
/* Bounded lower/upper register choices. Upper is not a promised octave. */
int engine_product_cell_midi(int cell,bool upper);
bool engine_clear_pending(void);
/* Soft 40 ms whole-chain silence, preserves targets and Generate intent.
 * Old sources/room are cleared behind zero; resume creates only fresh events. */
void engine_set_muted(bool on);
bool engine_muted(void);
#endif
