/*
 * audio_h743.c — STM32H743 SAI1 audio output skeleton.
 *
 * Implements include/audio.h on the STM32H743VIT6 product target. Replaces
 * the RP2040 PIO+DMA pump in src/hal_pico/audio_pico.c with native SAI1
 * Block A + DMA — the H7 has a dedicated I²S/SAI peripheral with built-in
 * DMA half/full callbacks, no PIO-tricks needed.
 *
 * SAI clock chain (per SPEC v0.7 §5.1):
 *   HSE 8 MHz → PLL3 → PLL3_P @ 11.2896 MHz (12-bit divider, jitter-free)
 *   SAI1 MCK = PLL3_P (or unused — PCM5102A runs 3-wire, no MCLK needed)
 *   SAI1 BCK = MCK / 4  = 2.8224 MHz  (32-bit slot × 2ch × 44.1 kHz)
 *   SAI1 LRCK = BCK / 64 = 44.1 kHz
 *
 * Pin map (DS12110, alt-function AF6, LQFP-100 pin numbers from §5.1):
 *   PE4 (pin 3) = SAI1_FS_A   → PCM5102A LRCK (pin 15)
 *   PE5 (pin 4) = SAI1_SCK_A  → PCM5102A BCK  (pin 13)
 *   PE6 (pin 5) = SAI1_SD_A   → PCM5102A DIN  (pin 14)
 *
 * Amp-control pins (PAM8403 pop-suppression sequence per SPEC §8.3):
 *   PB14 = AMP_nSHDN (LOW at boot, HIGH after rail settle)
 *   PB15 = AMP_nMUTE (LOW at boot, HIGH after /SHDN + 50 ms)
 *   PCM XSMT (PCM5102A pin 17) is driven by MCP23017 GPA5 (mcp_set_xsmt).
 *
 * Step 13.3 (TODO): wire up STM32CubeH7 HAL — SAI1 init, DMA1_Stream0
 * configuration with circular mode + Half-Transfer/Transfer-Complete IRQs,
 * NVIC priorities, audio_render_fn invocation from inside the IRQ handler.
 * Until ST-HAL is integrated this file only defines the API surface so the
 * project structure compiles end-to-end.
 */

#include "audio.h"
#include <string.h>

/* ---- Internal state ---- */
static audio_render_fn s_renderer = NULL;
static int16_t s_buffer[AUDIO_BUFFER_FRAMES * 2 * AUDIO_NUM_BUFFERS];  /* ping-pong */
static float   s_test_freq = 440.0f;
static float   s_test_amp  = 0.10f;

/* ---- Public API (matches include/audio.h) ---- */

void audio_init(void) {
    /* TODO(Step 13.3, STM32CubeH7 HAL):
     *   1. Enable GPIOE/B clocks. Set PE4/PE5/PE6 = AF6 (SAI1_A). Set PB14/PB15
     *      = output, LOW (amp off + muted).
     *   2. Enable SAI1 clock. Configure SAI1 Block A:
     *      - Master Mode TX, I²S Philips standard
     *      - Slot active = both, slot size = 32-bit, data size = 16-bit
     *      - BCK polarity rising edge, FS active low
     *      - Frame length = 64 bits (2 × 32-bit slots)
     *      - PLL3_P as clock source (MX_DMA + SAI clock cfg from CubeMX)
     *   3. Enable DMA1 (or DMA2). Bind DMA stream to SAI1_A TX:
     *      - Circular mode, mem→peri, 16-bit half-word
     *      - Buffer size = AUDIO_BUFFER_FRAMES × 2 × AUDIO_NUM_BUFFERS samples
     *      - Half-Transfer IRQ → fill first half via s_renderer
     *      - Transfer-Complete IRQ → fill second half via s_renderer
     *   4. Enable NVIC for the DMA stream IRQ (priority below audio
     *      processing, above UI).
     *   5. Start SAI + DMA. The pump now runs silence until s_renderer is
     *      set (default = built-in test sine renderer below).
     *   6. SPEC §8.3 pop-suppression:
     *      - wait 50 ms for rails to settle
     *      - drive PB14 (nSHDN) HIGH → PAM8403 wakes up
     *      - wait 50 ms
     *      - drive PB15 (nMUTE) HIGH + mcp_set_xsmt(true) → un-muted
     */
    memset(s_buffer, 0, sizeof s_buffer);
}

void audio_mute(void) {
    /* TODO(Step 13.3):
     *   - Drive PB15 (nMUTE) LOW
     *   - mcp_set_xsmt(false)
     *   - Leave SAI/DMA running so we don't get a click on resume.
     */
}

void audio_set_test_freq(float hz) { s_test_freq = hz; }
void audio_set_test_amp(float amp) {
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;
    s_test_amp = amp;
}

void audio_set_renderer(audio_render_fn fn) { s_renderer = fn; }

/* ---- Block fill (called from DMA half/full IRQ in Step 13.3) ----------------
 * Until then this function exists only to document the interface. It will
 * be called twice per buffer cycle — once at half-transfer (first half),
 * once at transfer-complete (second half). */
void audio_h743_fill_block(int16_t *block, int frames) {
    if (s_renderer) {
        s_renderer(block, frames);
        return;
    }
    /* Default: 440 Hz test sine at -20 dBFS (just enough to confirm the
     * chain is live). Real impl in Step 13.3. */
    (void)block; (void)frames;
}
