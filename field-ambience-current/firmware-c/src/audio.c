/*
 * Native I²S audio output — PIO + DMA double-buffer.
 *
 * Step 5 design notes:
 *
 * - PIO0 SM0 runs the audio_i2s program (compiled by pico_generate_pio_header
 *   from src/audio_i2s.pio). Output base = GP4 (DIN). Sideset base = GP0
 *   (BCK), sideset+1 = GP1 (LRCK). Clock divider sized for 64 PIO cycles per
 *   stereo frame at 44.1 kHz → 2.8224 MHz target.
 *
 * - One DMA channel pushes 32-bit words from a ping-pong of two int16_t
 *   buffers into pio0->txf[0]. Each transfer = AUDIO_BUFFER_FRAMES words
 *   (each word = stereo frame). On completion the IRQ swaps to the other
 *   buffer (immediate restart) and refills the just-finished one with
 *   freshly-rendered samples.
 *
 * - SPEC v0.6 §8 power sequence is enforced in audio_init(). The PIO is
 *   started with a silence buffer BEFORE /SHDN goes HIGH, so the first
 *   I²S frames hit a DAC that is still soft-muted via XSMT — no plop.
 *
 * Later steps replace fill_test_sine() with the real voice/reverb chain.
 */

#include "audio.h"
#include "mcp23017.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include <math.h>
#include <string.h>

#include "audio_i2s.pio.h"   /* generated from audio_i2s.pio at build */

/* ---- Pin map (v0.9 reassignment vs SPEC v0.6 §5) ---- */
#define PIN_I2S_BCK    0
#define PIN_I2S_LRCK   1     /* MUST be BCK+1 (sideset bit 1) */
#define PIN_I2S_DIN    4
#define PIN_AMP_nSHDN  27    /* HIGH = chip awake */
#define PIN_AMP_nMUTE  28    /* HIGH = un-muted */

#define AUDIO_PIO      pio0
#define AUDIO_SM       0

/* ---- Ping-pong buffers. Interleaved int16_t L,R per frame. ---- */
static int16_t audio_bufs[AUDIO_NUM_BUFFERS][AUDIO_BUFFER_FRAMES * 2];
static volatile uint8_t playing_idx = 0;   /* which buffer DMA is reading */

static int dma_chan = -1;

/* ---- Test sine state (Step 5 placeholder for the real engine) ---- */
static volatile float test_freq_hz = 440.0f;
static volatile float test_amp_0_1 = 0.10f;   /* -20 dB FS — safe for the 23 dB PAM8403 gain */
static float phase = 0.0f;

static void fill_test_sine(int16_t *buf, int frames) {
    float freq = test_freq_hz;
    float amp  = test_amp_0_1;
    if (amp > 1.0f) amp = 1.0f;
    if (amp < 0.0f) amp = 0.0f;
    float scale = amp * 32767.0f;
    float phase_inc = 2.0f * (float)M_PI * freq / (float)AUDIO_SAMPLE_RATE_HZ;
    float p = phase;
    for (int i = 0; i < frames; ++i) {
        int16_t s = (int16_t)(sinf(p) * scale);
        buf[i * 2 + 0] = s;   /* L */
        buf[i * 2 + 1] = s;   /* R (mono test) */
        p += phase_inc;
        if (p >= 2.0f * (float)M_PI) p -= 2.0f * (float)M_PI;
    }
    phase = p;
}

/* ---- DMA completion IRQ ---- */
static void __isr __not_in_flash_func(audio_dma_irq)(void) {
    if (dma_irqn_get_channel_status(0, dma_chan)) {
        dma_irqn_acknowledge_channel(0, dma_chan);

        /* The buffer that just finished is `playing_idx`. Swap to the other
         * one and rearm DMA for it (no manual restart latency — the trigger
         * write does it). Then refill the buffer we just freed. */
        uint8_t finished = playing_idx;
        uint8_t next     = (uint8_t)(playing_idx ^ 1);
        playing_idx = next;
        dma_channel_set_read_addr(dma_chan, audio_bufs[next], true);
        fill_test_sine(audio_bufs[finished], AUDIO_BUFFER_FRAMES);
    }
}

/* ---- PIO + DMA setup ---- */
static void pio_dma_init(void) {
    /* Load the program and configure pins. */
    uint offset = pio_add_program(AUDIO_PIO, &audio_i2s_program);

    pio_sm_config c = audio_i2s_program_get_default_config(offset);
    sm_config_set_out_pins(&c, PIN_I2S_DIN, 1);
    sm_config_set_sideset_pins(&c, PIN_I2S_BCK);                    /* +1 = LRCK */
    sm_config_set_out_shift(&c, false /* MSB-first */, true /* autopull */, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);                   /* deeper TX FIFO */

    /* Clock divider: each bit takes 2 PIO cycles. 16-bit stereo = 32 bits
     * = 64 PIO cycles per frame. Target frame rate = AUDIO_SAMPLE_RATE_HZ. */
    float sys_hz = (float)clock_get_hz(clk_sys);
    float pio_clk = (float)AUDIO_SAMPLE_RATE_HZ * 64.0f;
    sm_config_set_clkdiv(&c, sys_hz / pio_clk);

    /* Pin direction + PIO function ownership. */
    pio_gpio_init(AUDIO_PIO, PIN_I2S_DIN);
    pio_gpio_init(AUDIO_PIO, PIN_I2S_BCK);
    pio_gpio_init(AUDIO_PIO, PIN_I2S_LRCK);
    pio_sm_set_consecutive_pindirs(AUDIO_PIO, AUDIO_SM, PIN_I2S_DIN,  1, true);
    pio_sm_set_consecutive_pindirs(AUDIO_PIO, AUDIO_SM, PIN_I2S_BCK,  2, true);

    pio_sm_init(AUDIO_PIO, AUDIO_SM, offset + audio_i2s_offset_entry_point, &c);

    /* DMA channel — 32-bit transfers, source incrementing, destination = TX
     * FIFO, pacing by PIO TX DREQ. AUDIO_BUFFER_FRAMES transfers per buffer
     * (one transfer = one stereo frame = 4 bytes = one 32-bit word). */
    dma_chan = (int)dma_claim_unused_channel(true);
    dma_channel_config dmacfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&dmacfg, DMA_SIZE_32);
    channel_config_set_read_increment(&dmacfg, true);
    channel_config_set_write_increment(&dmacfg, false);
    channel_config_set_dreq(&dmacfg, pio_get_dreq(AUDIO_PIO, AUDIO_SM, true /* TX */));

    dma_channel_configure(
        dma_chan, &dmacfg,
        &AUDIO_PIO->txf[AUDIO_SM],          /* dest */
        audio_bufs[0],                       /* src  — start with buf 0 */
        AUDIO_BUFFER_FRAMES,                 /* count (32-bit words) */
        false                                /* don't start yet */
    );

    dma_irqn_set_channel_enabled(0, dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, audio_dma_irq);
    irq_set_enabled(DMA_IRQ_0, true);
}

/* ---- Public API ---- */

void audio_init(void) {
    /* 1) Boot-safe defaults on the amp control pins. R_SHDN_PD + R_MUTE_PD
     *    pull-downs hold them LOW already; we drive them LOW explicitly. */
    gpio_init(PIN_AMP_nSHDN); gpio_set_dir(PIN_AMP_nSHDN, GPIO_OUT); gpio_put(PIN_AMP_nSHDN, 0);
    gpio_init(PIN_AMP_nMUTE); gpio_set_dir(PIN_AMP_nMUTE, GPIO_OUT); gpio_put(PIN_AMP_nMUTE, 0);
    mcp_set_xsmt(false);   /* PCM5102A soft-mute on */

    /* 2) Bring up PIO + DMA with a silent first buffer so the DAC never
     *    sees garbage at startup. */
    memset(audio_bufs, 0, sizeof audio_bufs);
    pio_dma_init();
    playing_idx = 0;
    pio_sm_set_enabled(AUDIO_PIO, AUDIO_SM, true);
    dma_channel_start(dma_chan);

    /* 3) SPEC v0.6 §8 power sequence — silent throughout. */
    sleep_ms(50);                              /* rails settle */
    gpio_put(PIN_AMP_nSHDN, 1);                /* amp chip wakes */
    sleep_ms(50);                              /* internal refs settle */
    gpio_put(PIN_AMP_nMUTE, 1);                /* amp un-mutes */
    mcp_set_xsmt(true);                        /* DAC un-soft-mutes */

    /* 4) Hand the buffers over to the test-sine fill. Until the IRQ first
     *    refills one, we pre-fill both so the second flip already has data. */
    fill_test_sine(audio_bufs[0], AUDIO_BUFFER_FRAMES);
    fill_test_sine(audio_bufs[1], AUDIO_BUFFER_FRAMES);
}

void audio_mute(void) {
    /* Reverse order of un-mute: XSMT off (DAC), then /MUTE off (amp).
     * /SHDN stays HIGH — this is a "pause", not a teardown. */
    mcp_set_xsmt(false);
    gpio_put(PIN_AMP_nMUTE, 0);
}

void audio_set_test_freq(float hz) {
    if (hz < 20.0f)    hz = 20.0f;
    if (hz > 8000.0f)  hz = 8000.0f;
    test_freq_hz = hz;
}

void audio_set_test_amp(float amp_0_1) {
    if (amp_0_1 < 0.0f) amp_0_1 = 0.0f;
    if (amp_0_1 > 1.0f) amp_0_1 = 1.0f;
    test_amp_0_1 = amp_0_1;
}
