# WOODLAND — trockener Saitenansatz, 2026-10-06

## KERNURTEIL

Eine runde, ausklingende Saite ist ein schlüssiger Kern für WOODLANDs nahe
Details und kleine Antworten. Die alte Anregung war dafür unzuverlässig:
gleiche Tonhöhen konnten sehr verschiedene Grundtonenergie, Pegel und
Gleichanteile erzeugen. Dieser Fehler ist korrigiert. Ob der neue Ansatz
beruhigend, materialhaft und als eigene Welt erkennbar wirkt, ist noch kein
belegtes Hörergebnis. Die Weltgrammatik und der neue Katalog sind nicht Teil
dieses Quellenpakets.

## FUNDAMENTAL FALSCH

Die alte Quelle behandelte den Zufall als Tonkörper. Ein kurzes Rauschstück
mit festem Verstärkungsfaktor garantierte weder den Grundton noch den
Anschlagspegel. Bei mehreren Tönen konnte dadurch ein Oberton die beabsichtigte
Tonlage überdecken. Diese Anregung ist ersetzt, nicht mit Hall kaschiert.

Auch „mehr Dämpfung = dunkler“ war im bisherigen Zweitap-Filter falsch:
sein Verlust hängt von `d * (1-d)` ab und geht oberhalb des Mittelpunkts
wieder zurück. Die bewegte Verzögerung und Interpolation machten den Verlauf
zusätzlich abhängig von der Tonhöhe. Der neue symmetrische Filter trennt
Tonhöhe und Verlust.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Der trockene Ton ist ein Quellenkandidat, noch keine Waldwelt. Motiv,
  Antwort und Ruhe müssen die räumliche Vorstellung später tragen.
- Ein tragfähiger Grundton kann bei langer, dichter Wiederholung trotzdem
  als Summen erlebt werden. Die Begrenzung von Dichte und Dauer bleibt nötig.
- SHAPE beeinflusst jetzt auch den Eintritt dieser Quelle: 4–32 ms statt
  eines ungeformten Impulses. Die endgültige Makroabstimmung erfolgt nach
  dem Vergleich der drei Quellen.
- Der bestehende Send von 0,50 und externe Body/FX werden hier nicht gehört
  oder freigegeben. Die trocken erkennbare Identität muss später erhalten bleiben.
- Hohe Regressionstöne bis 1760 Hz sind technische Testfälle. Sie geben kein
  musikalisch oder sensorisch akzeptiertes Produktregister vor.

## LOCKED

- Zwei feste Verzögerungspuffer, Quellenbesitz und Ablehnung belegter Slots:
  Ausklänge werden nicht für einen neuen Anschlag abgeschnitten.
- Der gemeinsame 20-ms-Stop für Dry und Send bleibt erhalten.
- Fraktionale Stimmung bleibt; die neue Dämpfung bewegt den Lesekopf nicht.
  Tatsächliches PCM, nicht nur eine Formel, prüft die Tonhöhe.
- Klangkern ohne SD, Samples, Zusatzpad oder zweite Rauminstanz.

## REMOVE / MERGE / REDESIGN

Umgesetzt in `pluck.c`:

1. Rauschimpuls durch höchstens acht harmonische Saitenmoden ersetzen.
   Kleine Variation der Anregungsposition verändert die Artikulation,
   ohne den Grundton zufällig verschwinden zu lassen.
2. Anregung zentrieren und ihren Peak auf die angefragte Amplitude normieren.
   Ein Ausgangsfilter um 10 Hz entfernt verbleibenden Startup-Gleichanteil
   außerhalb der Rückkopplung. Das ist keine Behandlung von Netzbrummen.
3. Weicher kubischer Eintritt, bei Note-on aus SHAPE abgeleitet und begrenzt.
4. Symmetrische Rückkopplung `[a, 1-2a, a]`, `a = damp * 0.25 / 0.9`:
   feste Verzögerung um einen Sample, konsistenter Obertonverlust und
   weiterhin 80-ms-Smoothing. Interpolation bleibt fraktional.
5. Natürliches Abklingen, Besitz und Stop-Verhalten behalten. Der nominelle
   T60-Verlust ist kein Versprechen identischer hörbarer Dauer über alle Register.

Es gibt keine laufende Lautheitsregelung und keinen zusätzlichen Klangkörper.
Der vorhandene Saitenpool wird wiederverwendet; vier kleine Zustandswerte pro
Stimme kommen hinzu. Onset-Arbeit ist begrenzt und nutzt die vorhandene Sinus-LUT.

## BESTE VERSION

Ein naher, runder Ton besitzt sofort eine eindeutige Höhe, einen sanften
Ansatz und einen natürlichen Abschied. Eine Antwort übernimmt später einen
Gedanken des ersten Tons. Die geringe Artikulationsvariation verhindert
sterile Gleichheit; weder Rauschen noch Detune müssen Lebendigkeit vortäuschen.
Ruhe bleibt eine echte Möglichkeit. Raum verbindet diese Ereignisse und darf
ihre Unterschiede nicht verdecken. Ob acht Moden schon den besten Materialcharakter
liefern, entscheidet der Hörvergleich, nicht ihre technische Eleganz.

### Messbelege dieses Pakets

Referenz: Quellstand vor diesem Paket, Remote-Checkpoint
`d1a420b84b16d9f1c201c4ed66f9defc3a083241`.
Deterministischer Vergleich: 12 Anschläge pro Ton, Dämpfung 0,42, SHAPE neutral,
60–440 Hz. Stereo wird über beide Kanäle ausgewertet, damit wechselnde Position
keine Pegelschwankung vortäuscht. Fenster: 0,10–0,60 s nach dem Eintritt.

| PCM-Merkmal | Referenz | Neuer Kandidat |
|---|---:|---:|
| Größte RMS-Streuung gleicher Töne | 10,01 dB | 0,09 dB |
| Kleinstes Verhältnis Grundton / stärkster Oberton 2–5 | 0,053 | 2,62 |
| Größter Betrag Fenstermittel / RMS | 0,715 | 0,009 |
| Größter Sample-Sprung in den ersten 50 ms | 0,483 | 0,041 |
| Größter Peak bei Anregungsamplitude 0,4 | 0,777 | 0,402 |

Das Fenstermittel enthält auch den Rand eines abklingenden Sinus und ist keine
isolierte DC-Messung. Sample-Sprünge enthalten normale Tonbewegung; sie sind
kein alleiniger Klicknachweis. Zahlen zeigen reproduzierbar die Änderung,
nicht einen bewiesenen Entspannungseffekt. Rohdaten und Quellfingerprints:
[WOODLAND_DRY_METRICS.json](WOODLAND_DRY_METRICS.json).

Neue Hostregression: 1.347 Checks ohne Fehler; 15 Register-/Dämpfungskombinationen
mit jeweils zwölf Anschlägen, SHAPE-Eintritt, endliche Randfälle und monotoner
Obertonverlust. Über diesen erweiterten Bereich beträgt die RMS-Streuung
maximal 0,103 dB. Separate Tonhöhenprüfung: 35 statische und sechs laufende
Dämpfungsänderungen, größte statische Abweichung 0,034 Cent. Laufende Änderungen
werden in 0,25–0,50 s geprüft, während noch ein messbarer Ton besteht, statt
die bereits verklungene obere Note im späten Fenster zu interpretieren.

Der Hörvergleich enthält dieselben drei Töne und Zeitpunkte, ohne Body,
Hall, Hintergrund oder zusätzlichen EQ: 0–12 s Referenz, 12–14 s Stille,
14–26 s Kandidat. Beide Ausschnitte erhalten jeweils einen festen Pegel;
keine einzelne Note wird nachträglich nivelliert. Ausschnittziel: -26,9 LUFS,
gesamte Datei -26,7 LUFS, True Peak -10,0 dBFS. Die letzte Fahne wird nur am
Dateiende ausgeblendet. Reproduktion: `tools/review_woodland_dry.py` mit dem
alten `pluck.c` als Referenz. Lange interne Tests exportieren keine langen WAVs.

## TEST AM GERÄT

1. Trockener Vergleich bei gleicher Lautheit über Speaker und Kopfhörer;
   60–440 Hz, mehrere Anschläge, Minimum/Mitte/Maximum von BRIGHTNESS und SHAPE.
   Grundton muss eindeutig bleiben; kein hörbarer Klick, Alarmcharakter oder
   störend gleichförmiger Dauerton. Ungeeignete Register werden eingeschränkt.
2. Mit nur einer Quelle beginnen, dann zwei harmonisch kompatible Töne und
   echte Pausen. Mono darf weder Tonkörper verlieren noch Pegelsprünge zeigen.
3. Stop in den ersten Millisekunden und mitten im Ausklang; schnelle erneute
   Starts und volle Slots. Kein harter Abriss, kein wiederkehrender alter Ton.
4. Erst danach Body und den gemeinsamen Raum zuschalten. Wenn sie Tonhöhe oder
   Artikulation verwischen, ihren Anteil reduzieren oder die Funktion verwerfen.
5. DWT, Stack und Ausgangsreserve bei Doppelanschlag, Live-Makros und
   Generate-/native-Übergängen messen: `peak_load < 0.60`, keine Deadline-Misses.
   Hosttests und Linkerbelegung ersetzen diese Prüfung nicht.

Nächster Quellenblock: COAST/Bowed trocken. WOODLAND-Grammatik, Body/Send,
Pegelabgleich zwischen Quellen und das physische Hörurteil bleiben offen.
