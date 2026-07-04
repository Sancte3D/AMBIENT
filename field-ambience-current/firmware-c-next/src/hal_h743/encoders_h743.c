/*
 * encoders_h743.c — 4× EC11 via TIM hardware-encoder mode (r18.86,
 * Step 13.3 — real driver; replaces the RP2040 1 kHz software quadrature).
 *
 * Encoder map (generator NETS / PINMAP, r18.83-verified):
 *   EN1 "drive"   A/B = PA0/PA1   (TIM2, AF1)  push = PE0
 *   EN2 "bright"  A/B = PC6/PC7   (TIM3, AF2)  push = PE1
 *   EN3 "display" A/B = PD12/PD13 (TIM4, AF2)  push = PE3
 *   EN4 "volume"  A/B = PA8/PA9   (TIM1, AF1)  push = MCP23017 GPB5
 *                                              (fed in by the main loop via
 *                                               enc_set_ext_push)
 *
 * Each timer runs Encoder Mode TI12 (counts both edges of both channels =
 * ×4). The hardware counter can't lose steps; a 1 kHz sample from
 * SysTick_Handler (enc_tick_1khz) diffs the counters, divides by the
 * per-encoder `substeps` (same semantics + values as the bench-proven Pico
 * driver: 2 = smooth half-cycle feel, 4 = one event per detent) and pushes
 * ENC_EVENT_ROTATE into the same lock-free ring. Push switches are
 * debounced 3-of-N at the same tick, PUSH events on both edges — identical
 * contract to src/hal_pico/encoders_pico.c.
 */

#include "encoders.h"
#include "h743_hal.h"

typedef struct {
    uint8_t       id;          /* 1..4 (PARAM_ENC_*)                  */
    TIM_TypeDef  *tim;
    int8_t        dir;         /* +1 / -1 flip if a unit turns backwards */
    int8_t        substeps;    /* quadrature edges per emitted step   */
    GPIO_TypeDef *sw_port;     /* NULL = external (MCP) push          */
    uint16_t      sw_pin;
} enc_def_t;

static const enc_def_t defs[ENC_COUNT] = {
    { 1, TIM2, +1, 2, GPIOE, GPIO_PIN_0 },   /* drive   */
    { 2, TIM3, +1, 2, GPIOE, GPIO_PIN_1 },   /* bright  */
    { 3, TIM4, +1, 4, GPIOE, GPIO_PIN_3 },   /* display — detent feel */
    { 4, TIM1, +1, 2, NULL,  0           },  /* volume  — push via MCP */
};

typedef struct {
    uint16_t prev_cnt;
    int32_t  accum;            /* leftover quadrature edges           */
    uint8_t  sw_stable;        /* debounced level, 1 = released       */
    uint8_t  sw_count;
} enc_hw_state_t;

static TIM_HandleTypeDef  s_htim[ENC_COUNT];
static volatile enc_hw_state_t s_hw[ENC_COUNT];
static volatile int32_t   s_position[ENC_COUNT];
static volatile uint8_t   s_pushed[ENC_COUNT];
/* SysTick (and thus enc_tick_1khz) runs from HAL_Init() on — before
 * enc_init() has enabled the TIM/GPIO clocks. Gate the tick until ready. */
static volatile bool      s_ready = false;

/* --- lock-free SP/SC event ring (same shape as the Pico impl) --- */
#define EVT_RING 32
static volatile enc_event_t evt_ring[EVT_RING];
static volatile uint8_t     evt_head = 0;   /* writer: SysTick ISR */
static volatile uint8_t     evt_tail = 0;   /* reader: main loop   */

static inline void evt_push(enc_event_kind_t k, uint8_t id, int8_t v) {
    uint8_t next = (uint8_t)((evt_head + 1) & (EVT_RING - 1));
    if (next == evt_tail) return;                    /* full → drop */
    evt_ring[evt_head].kind  = k;
    evt_ring[evt_head].id    = id;
    evt_ring[evt_head].value = v;
    evt_head = next;
}

/* --- init --- */

static void enc_gpio_af(GPIO_TypeDef *port, uint16_t pins, uint8_t af) {
    GPIO_InitTypeDef g = {0};
    g.Pin       = pins;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_PULLUP;      /* EC11 commons sit on GND */
    g.Speed     = GPIO_SPEED_FREQ_LOW;
    g.Alternate = af;
    HAL_GPIO_Init(port, &g);
}

static void enc_tim_init(TIM_HandleTypeDef *h, TIM_TypeDef *tim) {
    h->Instance               = tim;
    h->Init.Prescaler         = 0;
    h->Init.CounterMode       = TIM_COUNTERMODE_UP;
    h->Init.Period            = 0xFFFF;
    h->Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    h->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    TIM_Encoder_InitTypeDef e = {0};
    e.EncoderMode  = TIM_ENCODERMODE_TI12;           /* ×4 count      */
    e.IC1Polarity  = TIM_ICPOLARITY_RISING;
    e.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    e.IC1Prescaler = TIM_ICPSC_DIV1;
    e.IC1Filter    = 0x4;                            /* ~8-clk glitch */
    e.IC2Polarity  = TIM_ICPOLARITY_RISING;
    e.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    e.IC2Prescaler = TIM_ICPSC_DIV1;
    e.IC2Filter    = 0x4;
    if (HAL_TIM_Encoder_Init(h, &e) != HAL_OK) Error_Handler();
    if (HAL_TIM_Encoder_Start(h, TIM_CHANNEL_ALL) != HAL_OK) Error_Handler();
}

void enc_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    enc_gpio_af(GPIOA, GPIO_PIN_0 | GPIO_PIN_1, GPIO_AF1_TIM2);   /* EN1 */
    enc_gpio_af(GPIOC, GPIO_PIN_6 | GPIO_PIN_7, GPIO_AF2_TIM3);   /* EN2 */
    enc_gpio_af(GPIOD, GPIO_PIN_12 | GPIO_PIN_13, GPIO_AF2_TIM4); /* EN3 */
    enc_gpio_af(GPIOA, GPIO_PIN_8 | GPIO_PIN_9, GPIO_AF1_TIM1);   /* EN4 */

    /* Push inputs PE0/PE1/PE3 (EN4 push arrives via the MCP expander). */
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &g);

    for (int i = 0; i < ENC_COUNT; ++i) {
        enc_tim_init(&s_htim[i], defs[i].tim);
        s_hw[i].prev_cnt  = (uint16_t)__HAL_TIM_GET_COUNTER(&s_htim[i]);
        s_hw[i].accum     = 0;
        s_hw[i].sw_stable = 1;
        s_hw[i].sw_count  = 0;
        s_position[i]     = 0;
        s_pushed[i]       = 0;
    }
    evt_head = evt_tail = 0;
    s_ready  = true;
}

/* --- 1 kHz service, called from SysTick_Handler (stm32h7xx_it.c) --- */

void enc_tick_1khz(void) {
    if (!s_ready) return;
    for (int i = 0; i < ENC_COUNT; ++i) {
        const enc_def_t *d = &defs[i];
        volatile enc_hw_state_t *s = &s_hw[i];

        /* Rotation: 16-bit wrap-safe delta of the hardware counter. */
        uint16_t cnt = (uint16_t)d->tim->CNT;
        int16_t delta = (int16_t)(cnt - s->prev_cnt);
        s->prev_cnt = cnt;
        if (delta) {
            s->accum += delta;
            while (s->accum >= d->substeps) {
                s->accum -= d->substeps;
                int8_t step = d->dir;
                s_position[i] += step;
                evt_push(ENC_EVENT_ROTATE, d->id, step);
            }
            while (s->accum <= -d->substeps) {
                s->accum += d->substeps;
                int8_t step = (int8_t)-d->dir;
                s_position[i] += step;
                evt_push(ENC_EVENT_ROTATE, d->id, step);
            }
        }

        /* Push (active-low), 3-of-N debounce — Pico semantics. */
        if (!d->sw_port) continue;
        uint8_t lvl = (HAL_GPIO_ReadPin(d->sw_port, d->sw_pin) == GPIO_PIN_SET)
                    ? 1 : 0;
        if (lvl != s->sw_stable) {
            if (++s->sw_count >= 3) {
                s->sw_stable = lvl;
                s->sw_count  = 0;
                s_pushed[i]  = (uint8_t)(lvl == 0);
                evt_push(ENC_EVENT_PUSH, d->id, (int8_t)(lvl == 0));
            }
        } else {
            s->sw_count = 0;
        }
    }
}

/* --- EN4 push, debounced upstream (MCP INT + main-loop edge) --- */

void enc_set_ext_push(uint8_t id, bool pushed) {
    if (id < 1 || id > ENC_COUNT) return;
    uint8_t v = pushed ? 1 : 0;
    if (s_pushed[id - 1] != v) {
        s_pushed[id - 1] = v;
        evt_push(ENC_EVENT_PUSH, id, (int8_t)v);
    }
}

/* --- shared contract (include/encoders.h) --- */

bool enc_pop_event(enc_event_t *out) {
    if (evt_tail == evt_head) return false;
    *out = evt_ring[evt_tail];
    evt_tail = (uint8_t)((evt_tail + 1) & (EVT_RING - 1));
    return true;
}

int32_t enc_position(uint8_t id) {
    if (id < 1 || id > ENC_COUNT) return 0;
    return s_position[id - 1];
}

bool enc_pushed(uint8_t id) {
    if (id < 1 || id > ENC_COUNT) return false;
    return s_pushed[id - 1] != 0;
}
