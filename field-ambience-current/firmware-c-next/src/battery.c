/*
 * Battery — host-portable. Voltage→% curve + cached state.
 * On the device, main.c samples ADC0 every ~500 ms, converts to volts via the
 * 100k/100k divider (V_battery = 2 · V_adc) and feeds battery_set_pct().
 */

#include "battery.h"

static int   s_pct;
static bool  s_usb;

/* Knee points in (volts, %). The curve has tighter knees at the low end
 * because LiPo voltage drops fast under 20%. */
typedef struct { float v; int p; } pt_t;
static const pt_t CURVE[] = {
    { 4.20f, 100 },
    { 4.00f,  85 },
    { 3.80f,  60 },
    { 3.70f,  40 },
    { 3.60f,  20 },
    { 3.40f,   5 },
    { 3.00f,   0 },
};
#define N_PTS (int)(sizeof CURVE / sizeof CURVE[0])

int battery_pct_from_voltage(float v) {
    if (v >= CURVE[0].v)         return 100;
    if (v <= CURVE[N_PTS-1].v)   return 0;
    for (int i = 0; i < N_PTS - 1; ++i) {
        float v1 = CURVE[i].v, v2 = CURVE[i+1].v;
        if (v <= v1 && v >= v2) {
            int   p1 = CURVE[i].p, p2 = CURVE[i+1].p;
            float t  = (v - v2) / (v1 - v2);
            int   p  = p2 + (int)((p1 - p2) * t + 0.5f);
            if (p < 0)   p = 0;
            if (p > 100) p = 100;
            return p;
        }
    }
    return 0;
}

void battery_set_pct(int p) {
    if (p < 0)   p = 0;
    if (p > 100) p = 100;
    s_pct = p;
}
int  battery_pct(void) { return s_pct; }

void battery_set_usb_present(bool on) { s_usb = on; }
bool battery_usb_present(void)        { return s_usb; }
