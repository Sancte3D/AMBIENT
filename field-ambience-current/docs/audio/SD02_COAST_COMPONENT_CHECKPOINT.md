# SD02 — COAST-Komponenten, 7. Oktober 2026

Erster offener Punkt der verbindlichen To-do. **Softwarevergleich vorbereitet;
Quellenwahl und Hörabnahme offen.** SD03 folgt nach der Quellenentscheidung.
Keine der 30 offenen Gesamtaufgaben wird allein durch diese Messung abgehakt.

Basis: `d97824ec0039499969ef91353633b2c934818690`, Entwicklungsbranch
`claude/hall-sensor-bom-pcb-update-a8xj82`. PR138 ist darin bereits gemerged.
Dieser Checkpoint arbeitet auf einem eigenen Branch und verändert den
Produkt-DSP, Default, Szenen, Grammatiken und Parameterumfang nicht.

## Konkrete Lücke und Umsetzung

Das bisherige Dry-/Colorpaket zeigt den vollständigen Tonkörper; für SD02
fehlen Gegenproben der einzelnen Zusätze. `review_coast_components.py`
kompiliert deshalb temporäre, ausdrücklich als Diagnose markierte Kopien der
echten `bowed.c` und nutzt den bestehenden `render_product_preview.c`.

| Probe | Grain | Resonator 1,5f | Resonator 2f |
|---|---:|---:|---:|
| full | an | an | an |
| without_fifth | an | aus | an |
| without_octave | an | an | aus |
| without_grain | aus | an | an |
| body_only | aus | aus | aus |

Die Gegenproben behalten Zeitverlauf, Ausgangskette, Hüllkurven und alle
übrigen Komponenten. Keine neue Produkt-API, kein Diagnoseparameter im
ARM-Build. Das volle colour0-Kontroll-WAV und sein Ereignistrace müssen
bytegleich zum separat kompilierten, unveränderten Product-Renderer sein.
Die Quelländerungen werden durch eindeutige Textanker abgesichert und die
vollständigen Diagnosequellen mit SHA256 im Manifest festgehalten.

colour0 ist der aktuelle Produktpfad. colour1 ist die erhaltene dunklere
Quellenkonstruktion mit stärkerer Sympathie: ein gesonderter Diagnosekandidat,
kein zweites Produktpreset. Der aktuelle Color-Makro bleibt jeweils auf 0,5.

## Hörverfahren

Jede WAV ist exakt **27 s**, Stereo PCM16, 44,1 kHz. Die zehn Einzelproben
enthalten D3 / D4 / A4 in neun Sekunden langen Abschnitten, Velocity 0,75,
Volume 0,6, Attack/Release 0,5, Seed 1234, Room und Nature aus. Je Abschnitt
Key-up nach drei Sekunden, Clear bei 8,75 Sekunden; nach Init/Targets wird
vor der Note wie im bestehenden Renderer eingeregelt.

`raw` erhält den Firmwarepegel. Die separate `listen`-Kopie verwendet einen
konstanten Dateigain: Ziel −26 LUFS, maximal +12 dB, True Peak höchstens
−6 dBFS. Keine AGC. Jede rohe Datei behält ihren Ereignistrace.

Zuerst `SD02_COAST_colour0_AB_27s.wav` hören:

| Zeit | Identischer D4-Ausschnitt |
|---|---|
| 0–6 s | vollständiger aktueller Tonkörper |
| 7–13 s | ohne 1,5f-Resonator |
| 14–20 s | ohne 2f-Resonator |
| 21–27 s | ohne Grain |

Eine Sekunde Pause trennt die Ausschnitte. Diese kurze Hörmontage verwendet
je Abschnitt einen separat dokumentierten konstanten Vergleichsgain sowie
einen deklarierten Schlussfade von 100 ms. Dieser Fade ist Ausschnittbearbeitung
und **kein** Nachweis der natürlichen Firmware-Retirementzeit. Die zweite
Montage zeigt dieselbe Reihenfolge für colour1.

## Messbefund bei der Basis-SHA

Einsekündiges Hann-Spektrum ab Sekunde 1 jeder Note; Frequenzbänder jeweils
±8 Hz. Die Werte beschreiben spektrale Energie, keine wahrgenommene Rauigkeit
oder Beruhigung. Messfenster, tatsächliche Frequenzen und Hashes stehen in
`SD02_COMPONENT_METRICS.json`.

| colour0, D3/D4/A4 | 1,5f-Band gegenüber Grundton | 2f-Band gegenüber Grundton |
|---|---|---|
| vollständiger Tonkörper | −59,25 / −61,66 / −60,74 dB | −4,95 / −4,98 / −4,99 dB |
| ohne 1,5f-Resonator | −59,93 / −62,43 / −61,58 dB | −4,96 / −4,99 / −5,00 dB |
| ohne 2f-Resonator | −59,27 / −61,69 / −60,79 dB | −5,77 / −5,80 / −5,81 dB |
| ohne Grain | −101,85 / −103,30 / −98,83 dB | −4,95 / −4,98 / −4,99 dB |

Der 1,5f-Resonator ist in diesen Fenstern keine starke unabhängige Quinte;
der verbleibende Anteil hängt vor allem am Grain. Das folgt aus der
Gegenprobe und erlaubt noch keinen subjektiven Keep/Remove-Entscheid.
Das Entfernen des 2f-Resonators verändert den Oktavanteil hier um rund
0,8 dB, bei der dunkleren Konstruktion rund 1,3 dB.

Alle zehn Rohproben: Monoenergieverhältnis 0,94118, True Peak unter −23 dBFS,
mittlerer DC unter 0,0002; keine Nonfinite-/Limiter-Ereignisse im Renderer.
Die zwei kurzen Montagen: −26,1 LUFS, True Peak −19,2 / −19,3 dBFS.
Der vollständige colour0-Rohkontrollhash lautet
`dfc5f933722de5a7316b42d2ec31642b5ccce40e270a590e056c8744bf9bec6c`.

Keine neue DSP-Einheit: Delta des produktiven Quellen-/RAM-/Flashpfads null,
weil ausschließlich Hostwerkzeug, Dokumentation und CI hinzugefügt werden.
Tatsächliche ARM-Maps werden weiterhin durch den vorhandenen CI-Job geprüft.
Messung am Gerät und tatsächliche Ausgangslautstärke bleiben offen.

Lokal bestanden: vollständige `firmware-c-next/test/run_tests.sh`-Suite,
einschließlich realer 48-min-PCM-Probes, Journal-Abbruchtests und
489.609 Effektprüfungen mit null Fehlern. Das neue Komponentenwerkzeug prüft
zusätzlich jeden WAV-Umfang, endliche Pegel, Headroom/Mono/DC und den
bytegleichen unveränderten Kontrollrender. YAML und Python sind geprüft.

## Entscheidung und nächster Punkt

Dateiname, Variante und Zeitstelle für Buzz, Pfeifen, hohlen Körper oder
störende Reibung festhalten. Erst die gewählte Konstruktion in D3/D4/A4 und
Mono bestätigen; dann gegebenenfalls genau diese Komponente produktiv
entfernen oder behalten und SD02 abschließen. SD03 prüft anschließend
Mehrstimmigkeit/Grainentwicklung auf der ausgewählten Quelle.

Die späteren World-, Raum-, Nature-, UI- und Storage-Aufgaben werden nicht
mit dieser Quellenentscheidung vermischt.
