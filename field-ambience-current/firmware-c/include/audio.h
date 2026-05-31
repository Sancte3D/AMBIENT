#ifndef FAM_AUDIO_H
#define FAM_AUDIO_H

/*
 * Native I²S audio output for Field Ambience.
 *
 * Step 5: produces a continuous test sine on the PCM5102A via I²S, driven
 * by a PIO state machine + DMA double-buffer. Implements the SPEC v0.6 §8
 * pop-suppression power sequence around the PAM8403 (/SHDN, /MUTE) and
 * PCM5102A (XSMT, via MCP-GPA5) so the speakers come up silently.
 *
 * Later steps replace the test sine with the famPadCore + famSubBass + …
 * voice rendering — same DMA pump, different fill callback.
 *
 * Pin map (per the v0.9 reassignment in NATIVE_PORT_PLAN.md):
 *   GP0  : I²S BCK  → PCM5102A pin 13
 *   GP1  : I²S LRCK → PCM5102A pin 15
 *   GP4  : I²S DIN  → PCM5102A pin 14
 *   GP27 : PAM8403 /SHDN  (HIGH = chip awake)
 *   GP28 : PAM8403 /MUTE  (HIGH = un-muted)
 *   (MCP GPA5 = PCM XSMT, driven via mcp_set_xsmt())
 */

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_SAMPLE_RATE_HZ  44100
#define AUDIO_BUFFER_FRAMES   256          /* per buffer */
#define AUDIO_NUM_BUFFERS     2            /* ping-pong */

/* Initialise pins + PIO + DMA, kick off the audio pump streaming silence,
 * then run the SPEC §8 amp/DAC un-mute sequence (≥50 ms rail settle, then
 * /SHDN HIGH, ≥50 ms settle, then /MUTE HIGH + PCM_XSMT HIGH together).
 *
 * Once this returns the speakers are un-muted and the engine is producing
 * whatever audio_render_callback() writes — by default a 440 Hz test sine
 * at -20 dB FS. */
void audio_init(void);

/* Mute the output without freeing the pipeline (drives /MUTE LOW + XSMT LOW
 * with no /SHDN change). Call before tearing things down or switching modes. */
void audio_mute(void);

/* Step-5 sandbox controls: change the test sine frequency and amplitude
 * (0.0..1.0 of full scale, clamped). Hooks for the encoders in later steps. */
void audio_set_test_freq(float hz);
void audio_set_test_amp(float amp_0_1);

#endif
