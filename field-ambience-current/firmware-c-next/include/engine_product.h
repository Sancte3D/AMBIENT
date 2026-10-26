#ifndef FAM_ENGINE_PRODUCT_H
#define FAM_ENGINE_PRODUCT_H
#include <stdbool.h>
#include <stdint.h>
void engine_set_activity(float value);
void engine_set_color(float value);
void engine_set_room(float value);
void engine_set_nature(float value);
/* Physical Note-mode edge after admission; never inferred from replay/HOLD. */
void engine_set_cell_held(uint8_t source,bool down);
/* Opt-in Model-1 hearing candidate. Main-only; applies to future notes.
 * Default OFF; not a new Scene/UI/MIDI parameter. Active snapshots stay intact. */
void engine_set_world_character(bool enabled);
/* Main-only Model-1 WOODLAND body hearing candidate; future notes only.
 * Default OFF, independent of World character; no Scene/UI/MIDI migration. */
void engine_set_woodland_body(bool enabled);
/* Main-only harmonic modal replacement, future notes. Default OFF. */
void engine_set_woodland_resonator(bool enabled);
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
#ifdef FAM_SOUND_FINISH_CANDIDATE
/* Independent Main targets; no Scene/UI mapping is implied. */
void engine_set_sound_effects(float delay,float grain,bool freeze,float drive,float crush,float phaser,bool plate);
/* Playback owners 8/14 stay separate from live/shift/drone/Generate. */
bool engine_gesture_note(uint8_t cell,bool upper,bool pressed,float velocity,uint32_t now_ms);
#endif
/* Soft 40 ms whole-chain silence, preserves targets and Generate intent.
 * Old sources/room are cleared behind zero; resume creates only fresh events. */
void engine_set_muted(bool on);
bool engine_muted(void);
#endif
