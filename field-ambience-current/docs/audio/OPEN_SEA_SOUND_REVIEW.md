# Open Sea — Sound zuerst, Paket 2026-09-22

## KERNURTEIL

Open Sea besitzt einen kohärenten harmonischen Ausgangspunkt und einen
gemeinsamen Raum. Eine konkrete Schwäche lag bereits vor jedem Effekt in
seiner Bowed-Stimme: starke periodische Grundtoneinbrüche. Diese Korrektur
stabilisiert das Material ohne Pegelanhebung. Ob die komplette World ruhig,
unverwechselbar und angenehm ist, bleibt eine Hörentscheidung.

## FUNDAMENTAL FALSCH

Zwei um ca. sieben Cent verstimmte Sägezähne wurden 60/40 gemischt. Im Laufe
der Schwebung wechselte der Grundton zwischen fast voller Addition und
weitgehender Auslöschung. Die zweite Harmonische (Oktave) war dabei zeitweise stärker als
der Grundton. Das ist eine plausible Mitursache des gemeldeten hohlen,
röhrenartigen Eindrucks; es beweist nicht, dass jeder störende Klang von
dieser einen Ursache kommt.

Korrigiert auf Hauptsaite / Nebenstimme ca. 85/15. Konstanten 0,71 / 0,125
erhalten die langfristige Oszillatorleistung:
`0.71² + 0.125² = 0.519725`, vorher `0.6² + 0.4² = 0.52`.
Verstimmung, Vibrato, Body, Resonatoren, Bow-Noise und Hüllkurven bleiben.
Keine neue Master-Normalisierung. Der gemeinsame Quellcode wirkt auch auf
Fjords; beide Farben wurden geprüft.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Es bleiben mehrere Bewegungsquellen: Oszillatorverstimmung, Vibrato,
  Body-LFO, Pad-Bewegung und Effekt-Chorus. Stabiler Grundton allein macht
  daraus noch keine zusammenhängende Bewegung.
- Dream bearbeitet das direkte Signal mit Tape → Chorus → Blur, ergänzt
  Delay und Reverb. Im kontrollierten Vergleich senkt die ganze Kette den
  RMS-Pegel gegenüber dem trockenen Mix um ca. 3,9 dB. Das ist weder allein
  ein Fehler noch ein Qualitätsbeleg: Klang und Lautheit müssen getrennt
  beurteilt werden.
- Shimmer bei Open Seas Default 20 % verändert im zwölfsekündigen Diagnosefall
  höchstens einen 16-Bit-PCM-Schritt pro Sample. Das beweist die Verdrahtung,
  aber keinen musikalisch relevanten Nutzen. Andere Pegel, Einstellungen und
  spätere Hallphasen sind damit nicht abgedeckt. Nicht pauschal lauter drehen.
- Wind/Naturtextur wurde für die musikalische Zerlegung bewusst ausgeblendet;
  deren Natürlichkeit ist in diesem Paket nicht geprüft oder freigegeben.

## LOCKED

Als Prinzip: ein hörbar verlässlicher Grundton, mit feiner Bewegung darüber.
Pad- und FX-Presets werden nicht gleichzeitig verändert, solange wir die
Ursache in der Quelle isolieren. Ein gemeinsamer Raum und begrenzte Stimmenzahl
bleiben. Einzelne Klangfarben und konkrete Effektmengen sind nicht locked.

## REMOVE / MERGE / REDESIGN

In diesem Paket nur die destruktiv tiefe Oszillatorschwebung reduzieren.
Keine neuen Filter, Stimmen oder Regler. Zwei bestehende Multiplikations-
konstanten ändern sich; keine neue DSP-Operation, kein neuer Zustand/Buffer.
Host-Binärgrößen vor/nach identisch; aktuelle H743-Map/DWT bleiben ungeprüft.

Die folgende Klangeinheit soll Chorus und Blur jeweils gegen einen einfacheren
Raum vergleichen. Jede Stufe muss eine eigenständige hörbare Rolle haben.
Für Shimmer zuerst Eingangspegel, Quantisierung, Feedback-/Return-Gain und
brauchbaren Reglerbereich prüfen; danach behalten, begrenzen oder entfernen.

## Messung und Regression

`test/test_bowed_balance.c` untersucht die echte trockene Stimme mit ihren
Body-/Sympathieresonanzen: MIDI 50/62/71/81, beide Farben, je 12 s gehalten.
Fundamental und Oktave werden mit gleichen Q=20-Bandpässen analysiert;
0,1-s-Energiefenster von Sekunde 2 bis 11. Es wird keine lange Hördatei erzeugt.

| Messung über acht Probes | Vorher | Nachher |
|---|---:|---:|
| Grundtonschwankung max/min | 8,24–13,45 dB | 2,44–3,11 dB |
| Schlechtestes Grundton/Oktav-Verhältnis | −9,00 dB | +1,27 dB |
| Neue Regression | 8/8 Fälle rot | 8/8 Fälle grün |

Ein ergänzender Offline-Band/Hilbert-Probe mit 5./95. Perzentilen bestätigt
die Richtung (andere Fenster-/Bandbreitendefinition, daher andere Zahlen).
Der breite RMS-Pegel bleibt über die geprüften trockenen Töne innerhalb
ca. 0,1 dB des alten Werts. Im eigentlichen B4-Produktpfadvergleich: +0,012 dB.

Volle `bash test/run_tests.sh` bestanden: inklusive neuer Quellregression,
Hotpath-Lint, Produkt-Engine (15.159 Checks) und FX (478.860 Checks).
Die neue Regression wurde gegen das alte Shared-Library-Build tatsächlich
ausgeführt und schlug in allen acht Fällen fehl; keine bloßen Konstantentests.

### Quelle → Pad → Raum

`tools/review_open_sea.py --before baseline.so --after candidate.so --output /tmp/open-sea-review`

Gleiche Diagnose-Noten, frischer Prozess je Render: World-Bett D3, melodisches
Pad B4 und Bowed B4 mit typischer generativer Amplitude. Kein automatischer
Composer und keine erfundene Behauptung einer live gespielten Performance.
Die Naturtextur bleibt aus; der tatsächliche Open-Sea-FX-Send bleibt erhalten.
Messfenster 2–8 s, danach identische Releases bei 9 s.

| Produktpfad | RMS vorher | RMS nachher |
|---|---:|---:|
| Nur Bowed | −32,971 dBFS | −32,959 dBFS |
| Nur Pads + Bass | −32,147 dBFS | −32,147 dBFS |
| Bowed + Pads/Bass, trocken | −29,947 dBFS | −29,950 dBFS |
| Derselbe Mix mit Dream | −33,842 dBFS | −33,854 dBFS |

Das zeigt eine Änderung im zeitlichen Klangverhalten bei praktisch gleicher
mittlerer Lautstärke. Es ist keine automatische Behauptung „klingt besser“.
Beim isolierten Einzelton sind L/R proportional; Stereo entsteht erst durch
mehrere Quellen und den gemeinsamen Raum. Mono ist kein Fehler an sich.

FX-Abtrag am neuen Build: ohne Blur RMS +1,153 dB, ohne FX-Motion +1,219 dB.
Nur der jeweilige FX-Parameter wird auf null gesetzt; die Pad-Bewegung bleibt.
Ohne Shimmer: Differenz-RMS ca. −104 dBFS, maximal 1 PCM-Schritt über 12 s.
Weitere Werte und Provenienz: `OPEN_SEA_STEMS_METRICS.json`.

## BESTE VERSION

Ein tragfähiger, sanfter Ton darf leben, ohne regelmäßig seinen Körper zu
verlieren. Das Pad gibt Tiefe; der Raum verbindet, statt Fehler zu verdecken.
Erst wenn diese drei Rollen überzeugen, kommt die Naturtextur wieder dazu.
Die Stückzahl der Worlds/Synths ist kein Qualitätsziel.

### Hörprobe: 28,25 Sekunden

Jeweils ALT, dann NEU; jeder Abschnitt 4,5 s, dazwischen 0,25 s Pause:

| Startzeit | Abschnitt |
|---|---|
| 0 / 4,75 s | Stimme trocken: alt / neu |
| 9,5 / 14,25 s | Stimme + Pad/Bass: alt / neu |
| 19 / 23,75 s | Derselbe Mix + Dream: alt / neu |

Ein gemeinsamer Export-Gain +4,8 dB für alle sechs Abschnitte, keine
abschnittsweise Lautheitsanpassung. Nur 80-ms-Randfades gegen Schnittklicks.
Export −25,0 LUFS, −13,4 dBTP. Technisch ausgewertet, noch nicht subjektiv
freigegeben; die Hörprobe ist gezielte Klangdiagnostik, kein fertiges Musikstück.

## TEST AM GERÄT

Gleicher Pegel, Kopfhörer und Gerätespeaker, Stereo und Mono. Auf zyklisch
hohl werdenden Tonkörper achten, anschließend Bewegungsmenge und Raum prüfen.
Fjords mitprüfen, weil es dieselbe Quelle verwendet. Kurze unabhängige
Generate-Ausschnitte müssen danach bestätigen, dass die Korrektur auch in
echten Phrasen nützt. Kein „AAA“-Urteil allein aus den Messwerten.
