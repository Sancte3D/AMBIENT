#ifndef FAM_PARAMS_H
#define FAM_PARAMS_H

/*
 * params.c — encoder → engine parameter bindings (the four panel knobs).
 *
 * Maps rotary-encoder rotate events to engine setters, with the same
 * velocity-acceleration the bench tool + sim use (SPEC §5): one deliberate
 * detent = a small step, a fast spin multiplies each detent up to ×8 so a
 * full 0–100 % sweep takes well under a turn.
 *
 * Encoder bindings (encoder IDs per src/encoders.c, all ALPS EC11E18244AU):
 *   EN1 "DRIVE"   → engine_set_reverb_drive   (0..100 %)
 *   EN2 "BRIGHT"  → engine_set_brightness     (pad cutoff offset, Hz)
 *   EN3 "DISPLAY" → menu navigation (handled by menu.c, NOT here)
 *   EN4 "VOLUME"  → engine_set_master_volume  (0..100 %)
 *
 * Hardware-independent: the STM32/Pico encoder ISR feeds rotate deltas in via
 * params_encoder(); the value clamping + acceleration + engine calls live
 * here so the host test covers them.
 */

#include <stdint.h>

#define PARAM_ENC_DRIVE   1
#define PARAM_ENC_BRIGHT  2
#define PARAM_ENC_DISPLAY 3
#define PARAM_ENC_VOLUME  4

/* Reset params to engine defaults and push them to the engine. Call once
 * after engine_init(). */
void params_init(void);

/* Feed an encoder rotate event: `delta` = signed detent count this drain
 * (±1 normally, more if a fast spin accumulated several before the main loop
 * read them), `now_ms` = monotonic millisecond timestamp for acceleration.
 * DISPLAY (id 3) is ignored here (menu owns it). */
void params_encoder(uint8_t enc_id, int delta, uint32_t now_ms);

/* Current values — for the menu/UI readout. */
int   params_drive_pct(void);    /* 0..100 */
float params_bright_hz(void);    /* pad brightness offset in Hz */
int   params_volume_pct(void);   /* 0..100 */

#endif
