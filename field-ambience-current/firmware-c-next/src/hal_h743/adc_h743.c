/*
 * adc_h743.c — BAT_SENSE battery-voltage ADC (r18.92, the last unwired
 * device input).
 *
 * Hardware (schematic r18.83-verified): VBAT → R_BATA 100k / R_BATB 100k
 * divider → BAT_SENSE = MCU pin 25 = PA3 = ADC12_INP15, with C_BAT_FILT
 * 10 nF as the sample-and-hold reservoir. VBAT = 2 × V(PA3), VREF = 3.3 V.
 *
 * Clocking: ADC kernel ← CLKP = HSE 8 MHz (system_clock_h743.c) — the
 * battery UI needs one reading per second, not speed. The 100k‖100k ≈ 50 kΩ
 * source impedance needs a LONG sampling window: 810.5 ADC clocks at 8 MHz
 * (÷2 presc → 4 MHz) ≈ 200 µs, far beyond the datasheet minimum for high-Z
 * sources, and the 10 nF cap tops the S&H off between reads.
 *
 * Blocking single conversion by design — called at 1 Hz from the UI loop,
 * never from the audio path.
 */

#include "h743_hal.h"

static ADC_HandleTypeDef s_hadc1;
static bool s_ready = false;

void HAL_ADC_MspInit(ADC_HandleTypeDef *h) {
    if (h->Instance != ADC1) return;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC12_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_3;                 /* PA3 = ADC12_INP15 */
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);
}

bool bat_adc_init(void) {
    s_hadc1.Instance                      = ADC1;
    s_hadc1.Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV2;   /* 4 MHz */
    s_hadc1.Init.Resolution               = ADC_RESOLUTION_16B;
    s_hadc1.Init.ScanConvMode             = ADC_SCAN_DISABLE;
    s_hadc1.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;
    s_hadc1.Init.LowPowerAutoWait         = DISABLE;
    s_hadc1.Init.ContinuousConvMode       = DISABLE;
    s_hadc1.Init.NbrOfConversion          = 1;
    s_hadc1.Init.DiscontinuousConvMode    = DISABLE;
    s_hadc1.Init.ExternalTrigConv         = ADC_SOFTWARE_START;
    s_hadc1.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;
    s_hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    s_hadc1.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;
    s_hadc1.Init.LeftBitShift             = ADC_LEFTBITSHIFT_NONE;
    s_hadc1.Init.OversamplingMode         = DISABLE;
    if (HAL_ADC_Init(&s_hadc1) != HAL_OK) return false;

    ADC_ChannelConfTypeDef c = {0};
    c.Channel      = ADC_CHANNEL_15;     /* PA3 single-ended */
    c.Rank         = ADC_REGULAR_RANK_1;
    c.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;   /* high-Z divider source */
    c.SingleDiff   = ADC_SINGLE_ENDED;
    c.OffsetNumber = ADC_OFFSET_NONE;
    c.Offset       = 0;
    if (HAL_ADC_ConfigChannel(&s_hadc1, &c) != HAL_OK) return false;

    if (HAL_ADCEx_Calibration_Start(&s_hadc1, ADC_CALIB_OFFSET_LINEARITY,
                                    ADC_SINGLE_ENDED) != HAL_OK) return false;
    s_ready = true;
    return true;
}

/* One blocking conversion → battery voltage in volts (0 if not ready). */
float bat_adc_read_volts(void) {
    if (!s_ready) return 0.0f;
    if (HAL_ADC_Start(&s_hadc1) != HAL_OK) return 0.0f;
    if (HAL_ADC_PollForConversion(&s_hadc1, 5) != HAL_OK) {
        HAL_ADC_Stop(&s_hadc1);
        return 0.0f;
    }
    uint32_t raw = HAL_ADC_GetValue(&s_hadc1);       /* 16-bit */
    HAL_ADC_Stop(&s_hadc1);
    /* pin volts × 2 (100k/100k divider) */
    return ((float)raw / 65535.0f) * 3.3f * 2.0f;
}
