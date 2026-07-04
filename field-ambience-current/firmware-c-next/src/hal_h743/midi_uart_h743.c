/*
 * midi_uart_h743.c — USART2 TRS-MIDI Out (r18.86, Step 13.3 — real driver).
 *
 * Replaces the RP2040 PIO-UART hack (src/hal_pico/midi_pio.c) with a native
 * STM32 USART. Implements the device seam from include/midi.h
 * (midi_hw_init / midi_hw_pump); the message logic + ring buffer in
 * src/midi.c is hardware-independent and host-tested.
 *
 * Pin map (SPEC §5.7 / r18.14, hardware fully stuffed since r18.67):
 *   PD5 (pin 86) = USART2_TX (AF7) → 220 Ω → TRS-Tip   (MIDI data)
 *                                  +3V3 → 220 Ω → TRS-Ring
 *                                  GND → Sleeve
 *   3.3 V levels — MMA-CA-033 (2020) explicitly allows 3.3 V for Type A.
 *   No optocoupler needed (that rule applies to MIDI IN only; we ship OUT).
 *
 * Baud: USART2 kernel ← rcc_pclk1 = 120 MHz (D2CCIP2R USART234578SEL reset
 * default). 120 MHz / 31250 = 3840 exactly — zero baud-rate error.
 *
 * Pump model: midi_hw_pump() moves at most one byte per call from the
 * midi.c FIFO into TDR, only when the transmit register is empty — it
 * never blocks. Worst-case MIDI OUT traffic (~300 B/s) is three orders of
 * magnitude under the 3125 B/s line rate, so no DMA needed.
 *
 * NOTE: firmware-side MIDI stays DEFERRED (ADR-0004 / r18.30) — main_h743.c
 * keeps the midi_hw_init() call commented out. This driver is complete so
 * reactivation is a one-line change.
 */

#include "midi.h"
#include "h743_hal.h"

static UART_HandleTypeDef s_huart2;
static bool s_ready = false;

void HAL_UART_MspInit(UART_HandleTypeDef *h) {
    if (h->Instance != USART2) return;
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_5;              /* PD5 = USART2_TX */
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_LOW;     /* 31250 baud — slow edges, low EMI */
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &g);
}

void midi_hw_init(void) {
    s_huart2.Instance                    = USART2;
    s_huart2.Init.BaudRate               = 31250;
    s_huart2.Init.WordLength             = UART_WORDLENGTH_8B;
    s_huart2.Init.StopBits               = UART_STOPBITS_1;
    s_huart2.Init.Parity                 = UART_PARITY_NONE;
    s_huart2.Init.Mode                   = UART_MODE_TX;
    s_huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    s_huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
    s_huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    s_huart2.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    s_huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&s_huart2) != HAL_OK) Error_Handler();
    s_ready = true;
}

/* Drain the midi.c FIFO into the UART, one byte per empty-TDR check.
 * Called from the main loop; never blocks. */
void midi_hw_pump(void) {
    if (!s_ready) return;
    while (__HAL_UART_GET_FLAG(&s_huart2, UART_FLAG_TXE)) {
        int b = midi_pop_byte();
        if (b < 0) return;                 /* FIFO empty */
        s_huart2.Instance->TDR = (uint8_t)b;
    }
}
