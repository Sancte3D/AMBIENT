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
#include <stdbool.h>

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

/* r19.21 — Encoder-Push-Aktionen (knobs.c). Alle halten params.c als
 * einzige Quelle der Wahrheit: Drehen an DRIVE/VOLUME hebt Bypass/Mute
 * automatisch auf (params_encoder), damit der Knopf den Wert "nimmt". */
int  params_toggle_drive_bypass(void);  /* Rueckgabe: 1 = jetzt Bypass    */
bool params_drive_bypassed(void);
void params_reset_drive(void);          /* Default 15 %, Bypass aus       */
void params_set_bright_neutral(void);   /* Kurz-Druck: 0 Hz               */
void params_reset_bright(void);         /* Lang-Druck: Default (= 0 Hz)   */
int  params_toggle_mute(void);          /* Rueckgabe: 1 = jetzt stumm     */
/* r19.22 Scenes-Recall: Drive/Brightness setzen (Volume bewusst NICHT —
 * Lautstaerke springt beim Recall nie). Hebt einen Drive-Bypass auf. */
void params_apply_scene(int drive_pct, float bright_hz);
bool params_muted(void);

#endif
