#ifndef FAM_BATTERY_H
#define FAM_BATTERY_H

/*
 * Battery state-of-charge — Step 12b #7.
 *
 * The PCB samples VBAT via a 100k/100k divider into Pico GP26/ADC0 (SPEC §2.2
 * r12). On-device firmware reads the raw 12-bit value, converts to volts,
 * passes that to battery_pct_from_voltage() and shows the result in the OLED
 * top-right indicator.
 *
 * This file is pure C / fully host-testable. The ADC-read shim that turns the
 * 12-bit count into a voltage lives in src/battery.c so the same single source
 * works on the device — there it samples ADC0; in tests it's stubbed.
 */

#include <stdint.h>
#include <stdbool.h>

/* Convert a measured battery voltage (V) into a state-of-charge percentage
 * (0..100). Uses a piecewise-linear LiPo discharge curve fit:
 *   ≥4.20 V → 100
 *    4.00 V →  85
 *    3.80 V →  60
 *    3.70 V →  40
 *    3.60 V →  20
 *    3.40 V →   5
 *   ≤3.00 V →   0
 * Above/below the band clamps to 100/0. */
int battery_pct_from_voltage(float volts);

/* USB present: battery state is hidden / replaced by a charging glyph in the
 * UI. The PCB exposes this on MCP-GPA7 (SPEC §7 r12); on host we just set it. */
void battery_set_usb_present(bool present);
bool battery_usb_present(void);

/* Set the cached % directly (host tests + the device's slow ADC polling
 * update path both write through this). Clamped to 0..100. */
void battery_set_pct(int pct);
int  battery_pct(void);

#endif
