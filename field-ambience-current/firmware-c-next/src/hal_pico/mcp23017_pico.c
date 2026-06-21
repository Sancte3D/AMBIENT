/*
 * MCP23017 driver — C port of firmware/mcp23017.py.
 *
 * The init sequence is a byte-for-byte mirror of the MicroPython version so
 * the chip comes up with identical configuration on both firmware paths
 * during the v0.9 transition.
 */

#include "mcp23017.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

/* --- Register map, IOCON.BANK = 0 (power-on default) --- */
#define REG_IODIRA   0x00
#define REG_IODIRB   0x01
#define REG_IPOLA    0x02
#define REG_GPINTENA 0x04
#define REG_GPINTENB 0x05
#define REG_INTCONA  0x08
#define REG_INTCONB  0x09
#define REG_IOCON    0x0A
#define REG_GPPUA    0x0C
#define REG_GPPUB    0x0D
#define REG_GPIOA    0x12
#define REG_GPIOB    0x13
#define REG_OLATA    0x14

/* --- Bus + pin map per SPEC v0.6 §5 / firmware/config.py --- */
#define MCP_I2C        i2c1
#define MCP_PIN_SDA    2
#define MCP_PIN_SCL    3
#define MCP_I2C_HZ     (400 * 1000)

static volatile bool s_irq_pending = false;
static volatile uint16_t s_state = 0xFFFF;   /* idle = all pulled-up = 1s */
static uint8_t s_olata = 0x00;               /* shadow of OLATA (port A out latch) */

/* --- low-level I²C helpers --- */

static bool reg_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { reg, val };
    int r = i2c_write_blocking(MCP_I2C, MCP_I2C_ADDR, buf, 2, false);
    return r == 2;
}

static bool reg_read(uint8_t reg, uint8_t *out) {
    int r = i2c_write_blocking(MCP_I2C, MCP_I2C_ADDR, &reg, 1, true /* keep bus */);
    if (r != 1) return false;
    r = i2c_read_blocking(MCP_I2C, MCP_I2C_ADDR, out, 1, false);
    return r == 1;
}

/* --- IRQ handler. Keep it tiny — no I²C in here. --- */

static void gp22_irq(uint gpio, uint32_t events) {
    (void)gpio; (void)events;
    s_irq_pending = true;
}

/* --- public API --- */

bool mcp_init(void) {
    /* I²C1 + GPIO func. External pull-ups (R4=R5=4.7k) exist on the board;
     * we also enable internal pull-ups as a brown-out belt-and-braces. */
    i2c_init(MCP_I2C, MCP_I2C_HZ);
    gpio_set_function(MCP_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(MCP_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(MCP_PIN_SDA);
    gpio_pull_up(MCP_PIN_SCL);

    /* Probe: write any byte and check ACK. */
    uint8_t dummy;
    if (!reg_read(REG_IODIRA, &dummy)) {
        return false;
    }

    /* IODIR: 1 = input. Port A: GPA0-4 in, GPA5 out, GPA6 in (jack), GPA7 in
     *   → 0b1101_1111 = 0xDF. Port B: all inputs. */
    reg_write(REG_IODIRA, 0xDF);
    reg_write(REG_IODIRB, 0xFF);

    /* GPPU: enable pull-ups on switch inputs (GPA0-4) + jack-detect (GPA6)
     *   and modifier inputs (GPB0-4). */
    reg_write(REG_GPPUA, 0b01011111);
    reg_write(REG_GPPUB, 0b00011111);

    /* IPOL: input polarity normal (pressed handled in software). */
    reg_write(REG_IPOLA, 0x00);

    /* INT-on-change for switches + jack. INTCON=0 → compare against previous,
     * so any toggle triggers an interrupt. */
    reg_write(REG_GPINTENA, 0b01011111);
    reg_write(REG_GPINTENB, 0b00011111);
    reg_write(REG_INTCONA,  0x00);
    reg_write(REG_INTCONB,  0x00);

    /* IOCON: MIRROR=1 (bit6) → INTA reflects both ports;
     *        INTPOL=0 (bit1)  → active-low (matches Pico GP22 + R20 pull-up);
     *        ODR=0  (bit2)    → push-pull. */
    reg_write(REG_IOCON, 0b01000000);

    /* PCM_XSMT (GPA5) default LOW = muted at boot. R_XSMT_PD pulls it low
     * too but set the latch explicitly. */
    s_olata = 0x00;
    reg_write(REG_OLATA, s_olata);

    /* Configure GP22 as input with pull-up + falling-edge IRQ. R20 = 10k
     * external pull-up on INTA is already on the board (SPEC v0.6 H3 fix). */
    gpio_init(MCP_INT_PIN);
    gpio_set_dir(MCP_INT_PIN, GPIO_IN);
    gpio_pull_up(MCP_INT_PIN);
    gpio_set_irq_enabled_with_callback(MCP_INT_PIN,
                                       GPIO_IRQ_EDGE_FALL,
                                       true,
                                       &gp22_irq);

    /* Prime the cached state with one read so the OLED can render initial
     * positions correctly + any pending IRQ is drained. */
    (void)mcp_service();
    return true;
}

bool mcp_irq_pending(void) {
    return s_irq_pending;
}

uint16_t mcp_service(void) {
    s_irq_pending = false;
    uint8_t a = 0xFF, b = 0xFF;
    reg_read(REG_GPIOA, &a);
    reg_read(REG_GPIOB, &b);
    s_state = (uint16_t)((b << 8) | a);
    return s_state;
}

uint16_t mcp_state(void) {
    return s_state;
}

void mcp_set_xsmt(bool on) {
    if (on) s_olata |= (1u << 5);
    else    s_olata &= (uint8_t)~(1u << 5);
    reg_write(REG_OLATA, s_olata);
}
