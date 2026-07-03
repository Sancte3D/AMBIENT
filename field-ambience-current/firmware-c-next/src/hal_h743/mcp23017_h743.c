/*
 * mcp23017_h743.c — STM32H743 I²C1 driver skeleton for MCP23017 + PCA9685.
 *
 * The H7 has TWO I²C devices on one bus (per SPEC §4 BOM):
 *   - MCP23017 @ 0x20 (A0=A1=A2=GND) — 16 GPIO expansion: 5 modifier buttons
 *     (Shift/Hold/Drone/Generate/Clear) + 3 encoder push switches + PCM XSMT
 *     + USB-VBUS sense, etc.
 *   - PCA9685 @ 0x40 (A0-A5=GND) — 16-channel PWM LED driver: 5 modifier
 *     LEDs + 10 cell LEDs (5×2 base/shift) + 1 backlight gate.
 *
 * Pin map (DS12110-verified r18.24):
 *   PB6 (pin 92) = I2C1_SCL (AF4) — 4.7 kΩ pull-up to +3V3
 *   PB7 (pin 93) = I2C1_SDA (AF4) — 4.7 kΩ pull-up to +3V3
 *   PC13 (pin 7) = MCP_INT (EXTI) — wake on button press
 *
 * The PCA9685 needs separate PWM-frequency-init (~1 kHz, well above audible).
 * r18.84: /OE is hardwired LOW (pull-down) on the PCB — the old pull-up
 * design had NO net for firmware to drive it (LEDs could never enable).
 * Boot-dark is guaranteed by the chip itself: NXP DS Table 7, LEDn_FULL_OFF
 * defaults to 1 for every channel — nothing for firmware to sequence.
 *
 * Step 13.3 (TODO): STM32CubeH7 I²C1 init at 400 kHz Fast Mode + EXTI13
 * interrupt for MCP_INT. The existing src/hal_pico/mcp23017_pico.c register
 * sequence is bit-for-bit portable — only the i2c_write_blocking() / i2c_read
 * primitives change.
 */

#include "mcp23017.h"
#include <stdint.h>
#include <stdbool.h>

/* TODO(Step 13.3): expose i2c_h743_write(addr, buf, len) / i2c_h743_read
 * helpers backed by HAL_I2C_Master_Transmit / HAL_I2C_Master_Receive. */

bool mcp_init(void) {
    /* TODO: enable I2C1 clock + GPIOB clock, configure PB6/PB7 as AF4
     * open-drain, configure PC13 as input with EXTI13 rising-edge.
     * Then run the MCP23017 register-init sequence (IODIR, GPPU, IPOL,
     * INTCON, GPINTEN, DEFVAL) — identical to the Pico impl. */
    return true;  /* stub: assume ACK until the real I2C init lands */
}

bool mcp_read_gpio(uint16_t *out) {
    /* TODO: I2C read of MCP23017 GPIOA/B (16-bit). */
    (void)out;
    return false;
}

void mcp_set_xsmt(bool enable) {
    /* TODO: I2C read-modify-write of OLATA, toggle GPA5. */
    (void)enable;
}

void pca_init(void) {
    /* TODO: I2C init sequence for PCA9685: MODE1 sleep → set PRESCALE for
     * ~1 kHz → MODE1 wake → MODE2 OUTDRV totem-pole. (/OE ist seit r18.84
     * hardware-seitig LOW gepullt — nichts zu treiben; Kanaele booten per
     * DS-Default LEDn_FULL_OFF dunkel.) */
}

void pca_set_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count) {
    /* TODO: write LEDn_ON_L/H, LEDn_OFF_L/H over I2C. */
    (void)channel; (void)on_count; (void)off_count;
}
