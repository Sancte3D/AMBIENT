#ifndef FAM_MCP23017_H
#define FAM_MCP23017_H

/*
 * MCP23017 16-bit I²C I/O expander driver — C port of firmware/mcp23017.py.
 *
 * Field Ambience usage per SPEC v0.6 §7:
 *   GPA0-4 : CELL1..5  (input, pull-up, IRQ on change; pressed = logic 0)
 *   GPA5   : PCM_XSMT  (output; HIGH = PCM5102A un-muted)
 *   GPA6   : JACK_DETECT (input; with-plug pulls HIGH, no-plug pulls LOW)
 *   GPA7   : reserve
 *   GPB0-4 : MOD_SHIFT, MOD_HOLD, MOD_DRONE, MOD_GENERATE, MOD_CLEAR
 *   INTA   : mirrored (covers both ports), active-low, to Pico GP22
 *
 * I²C address: 0x20 (A0=A1=A2=GND). Bus: I2C1 on GP2(SDA)/GP3(SCL) at 400 kHz,
 * per firmware/config.py + SPEC.
 *
 * Register map assumes IOCON.BANK = 0 (power-on default).
 *
 * Threading note: the IRQ handler must NOT do I²C work. It only sets a flag
 * via mcp_irq_pending(); the main loop calls mcp_service() which drains the
 * latched IRQ by reading both GPIO ports (that auto-clears the interrupt).
 */

#include <stdbool.h>
#include <stdint.h>

#define MCP_I2C_ADDR      0x20
#define MCP_INT_PIN       22       /* Pico GP22 — SPEC v0.6 §5 */

/* Bit indices in the combined 16-bit (GPB<<8 | GPA) value. */
#define MCP_BIT_CELL1     0
#define MCP_BIT_CELL2     1
#define MCP_BIT_CELL3     2
#define MCP_BIT_CELL4     3
#define MCP_BIT_CELL5     4
#define MCP_BIT_XSMT      5
#define MCP_BIT_JACK      6
#define MCP_BIT_MOD_SHIFT     8
#define MCP_BIT_MOD_HOLD      9
#define MCP_BIT_MOD_DRONE    10
#define MCP_BIT_MOD_GENERATE 11
#define MCP_BIT_MOD_CLEAR    12
/* H7 board only (r18.83): EN4 (volume) push sits on GPB5 because all four
 * TIM-encoder GPIO pairs used up the free port pins. The main loop feeds
 * the level into enc_set_ext_push(4, …). Pico bench: unused. */
#define MCP_BIT_ENC4_SW      13

/* Initialise I²C1 on GP2/GP3 at 400 kHz, configure the MCP23017 for the
 * Field Ambience pin map (cells + modifier inputs with pull-ups, XSMT
 * output, INTA mirror active-low), and register the GP22 falling-edge IRQ.
 * Returns true if the device ACKed (sanity check); false if no MCP found. */
bool mcp_init(void);

/* True if a button-change IRQ has fired since the last mcp_service() call.
 * Set inside the GPIO IRQ handler; cleared by mcp_service(). */
bool mcp_irq_pending(void);

/* Service the IRQ: read GPIOA + GPIOB (auto-clears the interrupt), update
 * the cached state. Returns the new 16-bit combined value with each bit
 * raw from the chip (pressed/closed = 0, released/open = 1). Call this
 * from the main loop whenever mcp_irq_pending() is true. */
uint16_t mcp_service(void);

/* Return the most recent cached value without re-reading I²C. */
uint16_t mcp_state(void);

/* Drive GPA5 (PCM_XSMT). Will be wired into the audio path in Step 5; for
 * Step 3 it's available so we can demonstrate the output side works too. */
void mcp_set_xsmt(bool on);

/* --- h743 HAL: raw MCP GPIO read + PCA9685 LED-driver PWM (mcp23017_h743.c) --- */

/* Read GPIOA+GPIOB into *out as a 16-bit value packed (GPB<<8)|GPA — so GPB
 * bits land at 8-15 (see MCP_BIT_MOD_* above). Returns true on I²C ACK. */
bool mcp_read_gpio(uint16_t *out);

/* PCA9685 init: 1 kHz PWM, totem-pole OUTDRV. (/OE is hardwired LOW since
 * r18.84 — nothing to release; channels boot dark per DS LEDn_FULL_OFF.) */
void pca_init(void);

/* PCA9685 per-channel 12-bit PWM (on/off counts). */
void pca_set_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count);

/* r18.85 — second PCA9685 (U10 @ 0x41, A0=+3V3): 8-channel VU meter row
 * (hardware r18.66). Same register protocol as U6, different address. */
#define PCA2_I2C_ADDR     0x41
void pca2_init(void);
void pca2_set_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count);

#endif
