/*
 * shape.c — global envelope shape. See shape.h.
 *
 * Nur Zustand + Mapping; kein DSP. Die Faktoren werden bei note-on gelesen.
 * Die Exponentialkurve wird beim Setzen EINMAL berechnet (nicht bei jedem
 * Abruf), damit note-on billig bleibt und kein powf in Stimmen-Nähe landet.
 */
#include "shape.h"
#include <math.h>

/* Endpunkte so gewählt, dass MIN*MAX == 1 → 0.5 ergibt exakt Faktor 1.0. */
#define ATK_MIN 0.125f
#define ATK_MAX 8.0f
#define REL_MIN 0.25f
#define REL_MAX 4.0f

/* WICHTIG: neutral vorinitialisiert, NICHT 0. Eine Stimme darf ihre Zeiten
 * auch dann skalieren, wenn shape_init() (noch) nicht lief — bei statischer
 * Null-Initialisierung waere der Faktor 0 und die Stimme wuerde durch Null
 * teilen. Der Test test_ember.c faehrt genau diesen Fall (Voice ohne Engine). */
static float s_atk01 = 0.5f, s_rel01 = 0.5f;
static float s_atk_scale = 1.0f, s_rel_scale = 1.0f;

static float clamp01(float v){ return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/* scale = MIN · (MAX/MIN)^x  — geometrisch, also musikalisch gleichmässig. */
static float geo(float x, float lo, float hi){ return lo * powf(hi / lo, x); }

void shape_init(void) {
    s_atk01 = s_rel01 = 0.5f;
    s_atk_scale = s_rel_scale = 1.0f;
}

void shape_set_attack(float v01) {
    s_atk01 = clamp01(v01);
    s_atk_scale = geo(s_atk01, ATK_MIN, ATK_MAX);
}
void shape_set_release(float v01) {
    s_rel01 = clamp01(v01);
    s_rel_scale = geo(s_rel01, REL_MIN, REL_MAX);
}

float shape_attack_01(void)     { return s_atk01; }
float shape_release_01(void)    { return s_rel01; }
float shape_attack_scale(void)  { return s_atk_scale; }
float shape_release_scale(void) { return s_rel_scale; }
