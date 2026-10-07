# Third-Party Notices — Field Ambience

Diese Firmware enthält Software Dritter. Alle hier gelisteten Bestände stehen
unter **permissiven** Lizenzen (MIT · BSD-3-Clause · Apache-2.0 · SIL OFL 1.1)
und dürfen in
einem kommerziellen, geschlossenen Produkt verwendet werden. Die einzige
Auflage ist die **Erhaltung und Mitlieferung der Copyright-Hinweise** — genau
dafür existiert dieses Dokument.

> **Auslieferungspflicht:** BSD-3-Clause Ziffer 2 und die MIT-Lizenz verlangen,
> dass diese Hinweise auch bei Verteilung in **Binärform** beiliegen. Da wir ein
> Gerät mit kompilierter Firmware ausliefern, muss dieses Dokument dem Produkt
> beiliegen (Handbuch, Support-Seite oder Datei auf dem Gerät).

**Kein GPL/LGPL-Code und kein unlizenzierter Code ist in dieser Firmware
enthalten.** Copyleft-Bestände sind bewusst ausgeschlossen (siehe §5).

---

## 1. Moog-Ladder-Filter — MIT

**Datei:** `firmware-c-next/src/dsp_ladder.c`
**Herkunft:** C-Portierung aus DaisySP (`Source/Filters/ladder.{h,cpp}`), das
seinerseits aus der Teensy Audio Library portiert wurde. Algorithmus und
Koeffizienten unverändert; übersetzt C++ → C.

```
Ported from Audio Library for Teensy, Ladder Filter
Copyright (c) 2021, Richard van Hoesel
Copyright (c) 2024, Infrasonic Audio LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice, development funding notice, and this permission
notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.

Huovilainen New Moog (HNM) model as per CMJ jun 2006,
Richard van Hoesel, v.1.03 / Infrasonic-Daisy v1.7.
```

Der vollständige Header ist in der Quelldatei erhalten (Lizenzauflage erfüllt).

---

## 2. STM32H7xx HAL Driver — BSD-3-Clause

**Ort:** `firmware-c-next/src/hal_h743/vendor/STM32H7xx_HAL_Driver/`
**Lizenztext:** ebenda in `LICENSE.md`

```
Copyright 2017 STMicroelectronics. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED.
```

**Wichtig zu Ziffer 3:** ST darf nicht zur Bewerbung unseres Produkts genannt
werden („powered by STMicroelectronics" o. Ä. ist untersagt). Die reine Nennung
des verbauten Bauteils (STM32H743VIT6 in der BOM/Doku) ist davon nicht betroffen.

---

## 3. CMSIS — Apache-2.0

**Ort:** `firmware-c-next/src/hal_h743/vendor/CMSIS/`
Umfasst CMSIS-Core (ARM Limited) und CMSIS Device STM32H7xx (STMicroelectronics).
**Lizenztext:** `vendor/CMSIS/Device/ST/STM32H7xx/LICENSE.md` (Apache License 2.0).

Auflagen: Lizenztext beilegen, Copyright-Hinweise erhalten, Änderungen
kennzeichnen. Wir haben CMSIS **unverändert** übernommen.

---

## 4. Bitcount Grid Single — SIL OFL 1.1

**Ort:** `design/assets/BitcountGridSingle-Regular.ttf`
**Lizenztext:** `design/assets/BitcountGridSingle-OFL.txt`

Die Display-Schrift der Referenz-Richtung (`design/ui_ref.py`). Anders als die
Helvetica-Platzhalter in `design/` ist diese Schrift **auslieferbar**: die OFL
erlaubt Einbetten und Weitergabe, auch in kommerziellen Produkten.

Auflagen: Lizenztext beilegen und Copyright-Hinweise erhalten. Die Schrift darf
**nicht** allein verkauft werden, und ein abgeleiteter Font darf den
reservierten Namen nicht weiterführen. Sollten wir daraus einen Bitmap-Font für
die Firmware backen, ist das ein abgeleitetes Werk: dann unter **anderem Namen**
und wieder unter OFL veröffentlichen. Wir haben die Schrift **unverändert**
übernommen.

---

## 5. Bewusst NICHT verwendet (Copyleft / ohne Lizenz)

Diese Bestände liegen ggf. als Referenz im Repository-Branch
`agent/stm32-offline-knowledge-pack`, sind aber **nicht Teil der Firmware** und
dürfen nicht in sie kopiert werden:

| Bestand | Lizenz | Grund |
|---|---|---|
| `ST7789-STM32-GPL` | GPL-3.0 | Würde unsere Firmware unter GPL zwingen. Wir haben einen **eigenen** ST7789-Treiber (`lcd_st7789_h743.c`, entstanden beim CubeH7-Bring-up r18.86 — lange vor diesem Branch). |
| `DaisySP-LGPL` | LGPL-2.1 | Statisches Linken im Embedded-Kontext ist rechtlich heikel. |
| Mutable Instruments AVR-Code | GPL-3.0 | Nicht übernommen. |
| Mutable Instruments Hardware | CC-BY-SA-3.0 | Unser PCB ist eigenständig aus Datenblättern entwickelt. |
| ST USB Middleware, Cube *Applications* | SLA0044 | Nicht eingebunden (nur BSD-Bestände genutzt). |
| Controllerstech, YetAnother DSP, stm32-monosynth | **keine Lizenz** | Ohne Lizenz = kein Nutzungs-/Verteilungsrecht. Nicht verwendet. |

---

## 6. Arbeitsregel für neuen Fremdcode

1. **Lizenz zuerst prüfen** — kein Header/keine Lizenz ⇒ nicht verwenden.
2. **MIT/BSD/Apache:** Verwendung erlaubt, **Copyright-Header vollständig
   erhalten** und hier eintragen.
3. **GPL/LGPL:** nicht in die Firmware. Bei Bedarf Funktion eigenständig aus
   Datenblatt/Fachliteratur neu implementieren.
4. **Lernen ≠ Kopieren:** Prinzip verstehen und eigenständig implementieren ist
   legitim und meist technisch besser (passt zu unserer Blockrate, Ramping- und
   Hot-Path-Architektur). Das Entfernen von Headern oder maschinelles Umschreiben
   ist **kein** legaler Weg.

*Der eigene Code dieses Projekts ist proprietär (siehe `LICENSE-PROPRIETARY.md`).*
