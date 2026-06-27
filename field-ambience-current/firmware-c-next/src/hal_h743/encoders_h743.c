/*
 * encoders_h743.c — STM32H743 hardware-encoder skeleton.
 *
 * Implements include/encoders.h on the STM32H743. Replaces the RP2040 1 kHz
 * polled quadrature decoder (src/hal_pico/encoders_pico.c) with **native
 * TIM1/TIM2/TIM3/TIM4 hardware-encoder mode** — the H7 timers decode
 * quadrature in hardware, no firmware polling needed.
 *
 * Encoder map (all ALPS EC11E18244AU, all 4 identical) — MUST match the
 * generator NETS / docs/hardware/PINMAP.md (the schematic is the truth):
 *   EN1 "drive"   A=PA0  B=PA1   (TIM2)  SW=PE0   (STM32 GPIO)
 *   EN2 "bright"  A=PC6  B=PC7   (TIM3)  SW=PE1   (STM32 GPIO)
 *   EN3 "display" A=PD12 B=PD13  (TIM4)  SW=PE3   (STM32 GPIO)
 *   EN4 "volume"  A=PA8  B=PA9   (TIM1)  SW=MCP23017 GPB5
 *
 *   (r18.67 reconcile: the earlier comment had the A/B pairs rotated across
 *    EN2/3/4 and the switches on the wrong bus. Pins per generator NETS.)
 *
 * Timer assignments (one per encoder, all 16-bit counters):
 *   TIM2  CH1/CH2 = PA0/PA1   → EN1 drive   (AF1)
 *   TIM3  CH1/CH2 = PC6/PC7   → EN2 bright  (AF2)
 *   TIM4  CH1/CH2 = PD12/PD13 → EN3 display (AF2)
 *   TIM1  CH1/CH2 = PA8/PA9   → EN4 volume  (AF1, advanced timer, 16-bit OK)
 *
 * Each timer is configured in Encoder Mode 3 (counts both edges) so 18 PPR
 * × 4 = 72 counts/rev. The per-Encoder `substeps` setting in src/encoders.c
 * (active config: 2 for smooth, 4 for detent) divides this down to the
 * "user click" rate before the velocity-acceleration tier maths runs.
 *
 * Push switches: EN1/EN2/EN3 (drive/bright/display) are on dedicated STM32
 * GPIOs PE0/PE1/PE3 (GPIO/EXTI, no I²C round-trip). EN4 (volume) push is on
 * MCP23017 GPB5 — handled in mcp23017_h743.c (the MCU encoder-switch pins
 * were full, so volume's switch moved to the expander).
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
     *   5. Configure PE0/PE1/PE3 (EN1/2/3 push) as inputs with pull-up; sample at 1 kHz
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
