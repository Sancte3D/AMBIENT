# Alps — Tonkörper statt Hornsignal, 2026-09-23

## KERNURTEIL

Horn/Bowed wurden trocken in D3..A4 und bei drei Pegeln untersucht. Horn
enthielt eine starke Suboktave, einen festen 950-Hz-Formanten und einen
resonanten Attack-Filter. Diese bewusste Brass-Imitation widerspricht der
ruhigen Naturwelt. Horn ist jetzt ein runder, langsam einsetzender Alps-Ton;
der interne Name/Voice-ID bleibt kompatibel. Bowed bleibt in diesem Paket
unverändert und ist noch nicht abschließend hörend freigegeben.

## FUNDAMENTAL FALSCH

Die World muss nicht das örtlich assoziierte Instrument imitieren. Ein
zusätzliches tieferes Summen oder eine cupped-horn-Färbung ist kein notwendiges
Merkmal von Alps. Solche Anteile unter Hall zu verstecken wäre keine Lösung.

## NOCH NICHT SELBSTVERSTÄNDLICH

Kein Spektralwert beweist angenehmen Klang. Die neue Quelle verliert bewusst
Brass-Charakter; Hörvergleich muss bestätigen, dass sie dabei musikalisch
interessant bleibt und nicht als nackter Sinuston wahrgenommen wird.
Die niedrigere Gesamtenergie ist überwiegend entfernte Sub-/Obertonenergie,
keine Master-Absenkung. Das A/B ist deshalb lautheitsangeglichen.

## LOCKED

Stabiler gespielter Grundton, weicher eigener Einsatz, eindeutige Note-off-
Zuordnung, drei begrenzte Stimmen und bestehende weiche Stimmenübergaben.

## REMOVE / MERGE / REDESIGN

- Suboszillator und festen 950-Hz-Formant entfernt.
- Vorhandenen Sinus am gespielten Grundton genutzt: `0.45*saw - 0.35*sin`.
  Die negative Sinuspolarität passt zum Rampen-Grundton; die Amplituden
  erhalten dessen ursprüngliche Anregung annähernd (`.45*2/pi+.35 ≈ 2/pi`).
- Pitch-folgenden Tiefpass von Q=1,6 auf 0,707 gesetzt, Öffnung eingegrenzt.
- Attack 130 → 450 ms bei neutralem Shape; kurzer Luftanteil 0,5 → 0,12.
  Shape wirkt weiter; Attack min/max 56,25 ms / 3,6 s. Release unverändert.
- Ein Filter und eine Phasenführung weniger, kein zusätzlicher Oszillator.
  Host `.bss` −192 Byte, Text −112 Byte, `.data` unverändert. Kein H743-Lastnachweis.

### Nachweis

`test/test_horn_body.c`: zwölf reale Source-Probes, MIDI 50/55/62/69 und
Amplitude 0,18/0,42/0,58. Gleiche Q=25-Analysebänder für Grundton, Sub und
Harmonische 3..8; Sustainfenster 2..4 s. Anfangsenergie separat 0..100 ms.
Produktgrenzen wurden am alten Code ausgeführt: 12/12 rot; neuer Code 12/12
grün. Der Grundton-Gain wird mitgeprüft: Stummschalten würde nicht bestehen.

| Messung über zwölf Probes | Vorher | Nachher |
|---|---:|---:|
| Harmonische 3..8 relativ zum Grundton | −3,45..−2,12 dB | −18,96..−17,91 dB |
| Erste 100 ms relativ zum Sustain | −7,32..−7,03 dB | −17,79..−17,63 dB |
| Grundton-RMS / Eingangsamp | 0,1153..0,1176 | 0,1114..0,1118 |

Der Grundton bleibt innerhalb ca. 0,5 dB; der frühere Sub lag nur ca. 6,7 dB
darunter. Im neuen C-Test liegt das Sub-Band bei ca. −31,5 dB, begrenzt durch
Filterübersprechen; das ist kein neu erzeugter Suboszillator.
Die ergänzende Offline-Auswertung von Bowed ergibt in beiden Farben bei
allen zwölf Register-/Pegel-Probes unveränderte Werte und keinen vergleichbaren
Subton. Das vorhandene Grundton-Stabilitätstest bleibt grün; Bogenrauschen,
Vibrato und Resonanzwirkung bleiben der nächste gezielte Vergleich.

Volle `bash test/run_tests.sh` grün, einschließlich Voice-Gates, Releases,
Hotpath-Prüfung und 489.609 FX-Checks. Keine neue Firmware-Abhängigkeit.

## BESTE VERSION

Ein weicher tragfähiger Ton, der in einem stillen Tal aufblüht. Grundton
präsent, obere Details leise; Identität entsteht durch Atemform und Raum.
Vorhandene Horn-Voice-Einstellungen profitieren ebenfalls von dieser Änderung.

## TEST AM GERÄT

`tools/review_horn_body.py --before baseline.so --after candidate.so
--output /tmp/horn-body` erzeugt 27,5 s: trocken alt/neu ab 0/7 s, mit Pad und
Dream alt/neu ab 14/21 s. Diagnosenoten D4, G3-Bett; keine Generate-Performance.
Naturtextur für die Isolation aus. Je 6,5 s mit kurzen Randfades/Pausen;
feste Export-Gains +2,9/+6,9/+2,4/+4,8 dB, je −25 LUFS. Gesamtexport −25 LUFS,
−13,2 dBTP, keine Rohdaten-Clips. Auf Körper ohne Brummen, weichen Eintritt,
Restcharakter und Integration achten. Kopfhörer, Speaker und Mono bleiben offen.
