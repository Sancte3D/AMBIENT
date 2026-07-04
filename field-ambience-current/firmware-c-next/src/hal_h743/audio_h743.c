/*
 * audio_h743.c — SAI1 Block A + DMA audio pump (r18.86, Step 13.3 — the
 * skeleton is now the real driver).
 *
 * Clocking: SAI1 kernel ← PLL3P = 11.289609 MHz (system_clock_h743.c).
 * With the master divider at 256×FS this yields FS = 44.1 kHz exactly and
 * BCK = 64 × FS = 2.8224 MHz for the 2 × 32-bit-slot I²S frame. No MCLK
 * pin — the PCM5102A runs 3-wire (SCK pin tied to GND, internal PLL).
 *
 * Pins (AF6, NETS-verified r18.83): PE4 SAI1_FS_A → LRCK (DAC 15),
 * PE5 SAI1_SCK_A → BCK (DAC 13), PE6 SAI1_SD_A → DIN (DAC 14).
 * Amp control: PB14 = AMP_nSHDN, PB15 = AMP_nMUTE (hardware pulldowns keep
 * both LOW through boot — SPEC §8.3 pop-suppression).
 *
 * Pump: one ping-pong buffer (2 × AUDIO_BUFFER_FRAMES stereo frames),
 * circular DMA (DMA1 Stream 0, request SAI1_A). Half-transfer IRQ refills
 * the first half while the second streams, transfer-complete refills the
 * second. The buffer lives in AXI SRAM (D1 — DMA1 cannot reach DTCM,
 * ADR-0015 D3; the linker script puts .bss there) and is 32-byte aligned
 * so the D-cache clean after each fill hits exact lines.
 *
 * IRQ budget: engine_render(512 frames) must finish inside 11.6 ms. The
 * DMA IRQ runs at priority 5 (below nothing audio-critical, above the
 * main loop); Step 13.5 profiles the real headroom on hardware.
 */

#include "audio.h"
#include "h743_hal.h"
#include "mcp23017.h"
#include <string.h>

SAI_HandleTypeDef h743_hsai1a;
DMA_HandleTypeDef h743_hdma_sai;

/* ---- Internal state ---- */
static audio_render_fn s_renderer = NULL;
static float s_test_freq = 440.0f;
static float s_test_amp  = 0.10f;
static float s_test_phase = 0.0f;

/* Ping-pong DMA buffer — interleaved stereo int16. Lives in .bss → RAM_D1
 * (AXI) per the linker script; 32-byte aligned for exact cache-line ops. */
__attribute__((aligned(32)))
static int16_t s_buffer[AUDIO_BUFFER_FRAMES * 2 * AUDIO_NUM_BUFFERS];

/* ---- test-sine fallback (identical contract to the Pico pump) ---- */
static void fill_test_sine(int16_t *block, int frames) {
    const float step = 6.2831853f * s_test_freq / (float)AUDIO_SAMPLE_RATE_HZ;
    for (int n = 0; n < frames; ++n) {
        /* cheap parabolic sine — this is only the bring-up beep */
        float x = s_test_phase;
        float sx = x < 3.14159265f
                 ? 4.0f * x * (3.14159265f - x) / 9.8696044f
                 : -4.0f * (x - 3.14159265f) * (6.2831853f - x) / 9.8696044f;
        int16_t v = (int16_t)(sx * s_test_amp * 32767.0f);
        block[2 * n + 0] = v;
        block[2 * n + 1] = v;
        s_test_phase += step;
        if (s_test_phase >= 6.2831853f) s_test_phase -= 6.2831853f;
    }
}

static void fill_half(int half) {
    int16_t *block = &s_buffer[half * AUDIO_BUFFER_FRAMES * 2];
    if (s_renderer) s_renderer(block, AUDIO_BUFFER_FRAMES);
    else            fill_test_sine(block, AUDIO_BUFFER_FRAMES);
    /* CPU wrote through the D-cache — clean the lines so the DMA (which
     * reads straight from AXI SRAM) sees the fresh samples. Address and
     * size are 32-byte aligned by construction. */
    SCB_CleanDCache_by_Addr((uint32_t *)block,
                            AUDIO_BUFFER_FRAMES * 2 * (int)sizeof(int16_t));
}

/* HAL SAI DMA callbacks (dispatched from DMA1_Stream0_IRQHandler). */
void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *h) {
    (void)h; fill_half(0);
}
void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *h) {
    (void)h; fill_half(1);
}

/* ---- MspInit: pins, clocks, DMA plumbing ---- */
void HAL_SAI_MspInit(SAI_HandleTypeDef *h) {
    if (h->Instance != SAI1_Block_A) return;
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_SAI1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;   /* FS, SCK, SD */
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;    /* 2.8 MHz BCK — low slew = low EMI */
    g.Alternate = GPIO_AF6_SAI1;
    HAL_GPIO_Init(GPIOE, &g);

    h743_hdma_sai.Instance                 = DMA1_Stream0;
    h743_hdma_sai.Init.Request             = DMA_REQUEST_SAI1_A;
    h743_hdma_sai.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    h743_hdma_sai.Init.PeriphInc           = DMA_PINC_DISABLE;
    h743_hdma_sai.Init.MemInc              = DMA_MINC_ENABLE;
    h743_hdma_sai.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    h743_hdma_sai.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    h743_hdma_sai.Init.Mode                = DMA_CIRCULAR;
    h743_hdma_sai.Init.Priority            = DMA_PRIORITY_HIGH;
    h743_hdma_sai.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    h743_hdma_sai.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_HALFFULL;
    h743_hdma_sai.Init.MemBurst            = DMA_MBURST_SINGLE;
    h743_hdma_sai.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&h743_hdma_sai) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(h, hdmatx, h743_hdma_sai);

    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

/* ---- Public API (include/audio.h) ---- */

void audio_init(void) {
    /* Amp control pins LOW first (they already are, via R_SHDN_PD/R_MUTE_PD
     * — drive them explicitly anyway before anything clocks). */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_14 | GPIO_PIN_15;       /* nSHDN, nMUTE */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

    /* SAI1-A: master TX, I²S, 16-bit data in 32-bit slots, FS from the
     * 256×FS master divider (kernel 11.289609 MHz → FS 44.1 kHz). */
    memset(s_buffer, 0, sizeof s_buffer);
    SCB_CleanDCache_by_Addr((uint32_t *)s_buffer, sizeof s_buffer);

    h743_hsai1a.Instance            = SAI1_Block_A;
    h743_hsai1a.Init.AudioMode      = SAI_MODEMASTER_TX;
    h743_hsai1a.Init.Synchro        = SAI_ASYNCHRONOUS;
    h743_hsai1a.Init.OutputDrive    = SAI_OUTPUTDRIVE_ENABLE;
    h743_hsai1a.Init.NoDivider      = SAI_MASTERDIVIDER_ENABLE;
    h743_hsai1a.Init.FIFOThreshold  = SAI_FIFOTHRESHOLD_EMPTY;
    h743_hsai1a.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
    h743_hsai1a.Init.SynchroExt     = SAI_SYNCEXT_DISABLE;
    h743_hsai1a.Init.MonoStereoMode = SAI_STEREOMODE;
    h743_hsai1a.Init.CompandingMode = SAI_NOCOMPANDING;
    h743_hsai1a.Init.TriState       = SAI_OUTPUT_NOTRELEASED;
    h743_hsai1a.Init.MckOutput      = SAI_MCK_OUTPUT_DISABLE;
    if (HAL_SAI_InitProtocol(&h743_hsai1a, SAI_I2S_STANDARD,
                             SAI_PROTOCOL_DATASIZE_16BITEXTENDED, 2) != HAL_OK)
        Error_Handler();

    /* Kick the circular pump on the whole ping-pong buffer (streams the
     * zeroed silence until the first half-complete IRQ). */
    if (HAL_SAI_Transmit_DMA(&h743_hsai1a, (uint8_t *)s_buffer,
                             (uint16_t)(AUDIO_BUFFER_FRAMES * 2 *
                                        AUDIO_NUM_BUFFERS)) != HAL_OK)
        Error_Handler();

    /* SPEC §8.3 pop-suppression: rails settle → amp wakes → un-mute. */
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);   /* /SHDN high  */
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);   /* /MUTE high  */
    mcp_set_xsmt(true);                                    /* DAC un-mute */
}

void audio_mute(void) {
    /* Silence without stopping the pump (no click on resume). */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET); /* /MUTE low   */
    mcp_set_xsmt(false);
}

void audio_set_test_freq(float hz) { s_test_freq = hz; }
void audio_set_test_amp(float amp) {
    if (amp < 0.0f) amp = 0.0f;
    if (amp > 1.0f) amp = 1.0f;
    s_test_amp = amp;
}

void audio_set_renderer(audio_render_fn fn) { s_renderer = fn; }

/* Retained for the bench/test seam: the DMA callbacks go through this so a
 * future host-side pump test can call it directly. */
void audio_h743_fill_block(int16_t *block, int frames) {
    if (s_renderer) { s_renderer(block, frames); return; }
    fill_test_sine(block, frames);
}
