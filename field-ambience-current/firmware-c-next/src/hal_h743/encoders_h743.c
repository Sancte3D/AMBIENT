/*
 * encoders_h743.c — STM32H743 hardware-encoder skeleton.
 *
 * Implements include/encoders.h on the STM32H743. Replaces the RP2040 1 kHz
 * polled quadrature decoder (src/hal_pico/encoders_pico.c) with **native
 * TIM2/TIM3/TIM4/TIM5 hardware-encoder mode** — the H7 timers decode
 * quadrature in hardware, no firmware polling needed.
 *
 * Encoder map (all ALPS EC11E18244AU after r18.22 NRND pivot, all 4 identical):
 *   EN1 "drive"   A=PA0  B=PA1   SW=via MCP23017 GPB0 (Modifier handler)
 *   EN2 "bright"  A=PA8  B=PA9   SW=via MCP23017 GPB1
 *   EN3 "display" A=PC6  B=PC7   SW=PA10  (push)
 *   EN4 "volume"  A=PD12 B=PD13  SW=via MCP23017 GPB2
 *
 *   (Exact pins per SPEC v0.7 §5; final pin map verified r18.24 vs DS12110.)
 *
 * Timer assignments (one per encoder, all 16-bit counters):
 *   TIM2  CH1/CH2 = PA0/PA1   → EN1
 *   TIM1  CH1/CH2 = PA8/PA9   → EN2  (advanced timer, 16-bit OK)
 *   TIM3  CH1/CH2 = PC6/PC7   → EN3
 *   TIM4  CH1/CH2 = PD12/PD13 → EN4
 *
 * Each timer is configured in Encoder Mode 3 (counts both edges) so 18 PPR
 * × 4 = 72 counts/rev. The per-Encoder `substeps` setting in src/encoders.c
 * (active config: 2 for smooth, 4 for detent) divides this down to the
 * "user click" rate before the velocity-acceleration tier maths runs.
 *
 * Push switches: EN3 (display) is on a dedicated STM32 GPIO (PA10) so the
 * menu wake-on-press fires without I²C round-trip. EN1/2/4 push-switches
 * are wired through the MCP23017 GPB bank — handled in mcp23017_h743.c.
 *
 * Sampling (Step 13.3 TODO): a 1 kHz SysTick (or low-prio timer ISR) reads
 * the 4 timer counters, diffs against last sample, divides by `substeps`,
 * pushes ENC_EVENT_ROTATE events into the same lock-free ring buffer used
 * today. Push-switch debouncing is also 1 kHz polled (3-of-N samples).
 */

#include "encoders.h"

/* Ring buffer (shared shape with the Pico impl — see src/hal_pico/encoders_pico.c) */
#define EVT_RING 32
static volatile enc_event_t evt_ring[EVT_RING];
static volatile uint8_t     evt_head = 0;
static volatile uint8_t     evt_tail = 0;

static volatile int32_t s_position[ENC_COUNT];
static volatile uint8_t s_pushed[ENC_COUNT];

void enc_init(void) {
    /* TODO(Step 13.3):
     *   1. Enable GPIOA/C/D clocks. Configure each encoder pin as AF input
     *      with internal pull-up (no external pull-up on the new EC11E THT
     *      package — the 10 kΩ on the schematic is *also* a pull-up that
     *      stacks in parallel; fine).
     *   2. Set AF1 (TIM1/TIM2) / AF2 (TIM3/TIM4) on each AB pin pair.
     *   3. For each timer:
     *      - Counter direction = up/down by encoder
     *      - Encoder Mode 3 (count both edges)
     *      - Auto-reload = 0xFFFF (free-running 16-bit wrap)
     *      - Input filter = ICF[3:0]=0x4 (~clk/8 → ~15 Mhz at 120 MHz core)
     *   4. Start the timers; the counter then runs autonomously and we just
     *      sample it at 1 kHz.
     *   5. Configure PA10 (EN3 push) as input with pull-up; sample at 1 kHz
     *      with 3-of-N debounce.
     *   6. SysTick already runs at 1 kHz for menu animations — hook in the
     *      encoder-sample tick there, not a separate timer.
     */
    for (int i = 0; i < ENC_COUNT; ++i) { s_position[i] = 0; s_pushed[i] = 0; }
    evt_head = evt_tail = 0;
}

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
