/*
 * shape.h — global ENVELOPE SHAPE (r19.60). Säule "SHAPE" aus
 * docs/SYNTH_IDENTITY.md: Attack und Release unter der Hand des Spielers.
 *
 * WARUM SKALEN STATT ABSOLUTER ZEITEN
 * Jede Stimme hat eine *natürliche* Hüllkurve, die ihren Charakter ausmacht:
 * das Horn bläst in 130 ms an, der Bogen schwillt in 300 ms, das Pad blüht in
 * 800 ms. Absolute Attack-Zeiten würden diese Unterschiede plattmachen — alle
 * Stimmen klängen gleich. Stattdessen liefert dieses Modul einen FAKTOR, mit
 * dem jede Stimme ihre eigene Zeit multipliziert. Die relativen Verhältnisse
 * (und damit der Charakter) bleiben erhalten, aber das ganze Instrument lässt
 * sich Richtung "perkussiv" oder "atmend" schieben.
 *
 * MAPPING
 * Exponentiell, nicht linear — ein linearer Zeitregler fühlt sich kaputt an,
 * weil das Gehör Zeitverhältnisse logarithmisch wahrnimmt (der Schritt von
 * 10 ms auf 20 ms ist so gross wie der von 1 s auf 2 s). 0.5 = exakt neutral
 * (Faktor 1.0), also identisch zum Klang vor r19.60.
 *
 *   attack:  0.0 → ×0.125 (8× schneller)  ·  0.5 → ×1.0  ·  1.0 → ×8.0
 *   release: 0.0 → ×0.25  (4× kürzer)     ·  0.5 → ×1.0  ·  1.0 → ×4.0
 *
 * Die Faktoren werden bei NOTE-ON gelesen (Kontrollrate), nie pro Sample —
 * hot-path-sicher. Laufende Töne behalten ihre Hüllkurve, neue folgen dem
 * neuen Wert; so gibt es beim Drehen keine Sprünge in klingenden Noten.
 */
#ifndef FAM_SHAPE_H
#define FAM_SHAPE_H

void  shape_init(void);

/* 0..1 vom Spieler; 0.5 = neutral. */
void  shape_set_attack(float v01);
void  shape_set_release(float v01);
float shape_attack_01(void);
float shape_release_01(void);

/* Faktoren, mit denen eine Stimme ihre eigene Zeit multipliziert. */
float shape_attack_scale(void);    /* 0.125 .. 8.0, 1.0 = unverändert */
float shape_release_scale(void);   /* 0.25  .. 4.0, 1.0 = unverändert */

#endif /* FAM_SHAPE_H */
