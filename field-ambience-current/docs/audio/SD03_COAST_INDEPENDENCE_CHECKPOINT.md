# SD03 — unabhängige COAST-Stimmen, 7. Oktober 2026

**Softwarevorbereitung geprüft; Quellenwahl SD02 und Hörabnahme SD03 offen.**
Der Auftrag zum nächsten Durchlauf wird als Prüfung des bestehenden Kandidaten
ausgeführt, ohne die noch ausstehende Quellenentscheidung vorwegzunehmen.
Weiterhin 24 geschlossene / 30 offene Gesamtaufgaben.

Basis: PR139, `551b57bfbce61b7fa1a20e76b8941bcd7fece927`.
[CI37608566520](https://github.com/Sancte3D/AMBIENT/actions/runs/37608566520)
hat alle sechs Jobs bestanden, einschließlich Default/Product/Journal-H743,
der vollständigen Host-Suite und des SD02-Komponentenpakets.

## Frage und Umfang

Verändert der Einsatz oder Release einer Stimme den Zustand der anderen?
Ist Grain eine gemeinsame mechanische Welle oder bleiben seine Anteile
stimmenbezogen? Der Produkt-Body-LFO ist schon deaktiviert; dieser Checkpoint
führt keine zusätzliche Bewegung, Zufälligkeit oder Klangparameter ein.

`review_coast_independence.py` kompiliert echte Product-DSP-Quellen als
Hostbibliothek. Die volle Fassung verwendet die unveränderte `bowed.c`;
die stationäre Gegenprobe entfernt nur Grain aus einer temporären Kopie,
mit denselben eindeutigen Ankern wie SD02. Jede Quelle, Diagnosefassung und
das Prüfwerkzeug erhalten einen Fingerprint im Manifest.

## Tatsächliche Folge

| Besitzer | Ton | Einsatz | Loslassen |
|---|---|---:|---:|
| 0 | D3, 146,832382 Hz | 0 s | 10 s |
| 1 | A3, 220 Hz | 3 s | 14 s |
| 2 | F#4, 369,994415 Hz | 6 s | 17 s |

Alle Töne werden durch die echte Product-Zulassung angenommen. Velocity 0,75,
Volume 0,6, Color/Attack/Release 0,5, COAST colour0, Seed 1234, Dry,
Nature aus. Jede Quelle endet natürlich vor 27 s; kein Clear-Abschluss.

## Prüfungen und Befund

- Echte Float-Quellen gemeinsam gegen die Summe dreier separat gespielter
  Quellen vergleichen. Die Zeitachse läuft auch durch alle Idle- und fremden
  Eventgrenzen weiter. Mono entfernt dabei nur die unterschiedlichen Slot-Pans.
  Die ganze Folge einschließlich sämtlicher Releases wird verglichen.
- Maximaler Summenrest **1,4901161e-8** bei vollem Tonkörper und ohne Grain:
  Float-Rundung, kein beobachteter Zustandswechsel einer Nachbarstimme.
- Grundtonentwicklung der Einzelstimmen: 200-ms-Hann-Demodulation an der
  tatsächlichen Eingangsfrequenz, von zwei Sekunden nach Onset bis vor Release.
  Volle Fassung: D3 **0,009491 dB**, A3 **0,008465 dB**, F#4 **0,011440 dB**
  maximale Sustain-Schwankung. Ohne Grain höchstens **0,000113 dB**.
- Der 0,13-Hz-Fit steht im Manifest, beschreibt aber ein begrenztes Fenster;
  er ist keine allgemeine Wahrnehmungs- oder Langzeitfreigabe. Im Product-Code
  bleibt `bodyInc=0`; gemeinsame User-Color-Werte sind weiterhin beabsichtigt.
- Grain wird pro Einzelstimme durch Full-minus-no-Grain isoliert, Fenster
  7,5–9,5 s: größte absolute Kreuzkorrelation **0,005526**. Unterschiedliche
  zugelassene Frequenzen in dieser Folge, kein Beweis für sämtliche Verläufe.
- Die echte Product-Kette liefert bei **64 und 512 Frames bytegleiches PCM**
  für beide Fassungen. Je drei bestätigte Starts/Releases, null Nonfinite-/
  Limiter-Ereignisse, leere tatsächliche Quellen am Ende.
- Raw True Peak jeweils **−16,7 dBFS**, Monoenergieverhältnis **0,960744**
  beziehungsweise **0,960747**; mittlerer DC unter 3,3e-8.

Mehrstimmige unabhängige Zustände sind hier softwareseitig bestätigt. Die
Startphase bleibt absichtlich dieselbe; der Grain-RNG wird weiterhin aus der
Frequenz initialisiert. Ein erneuter gleicher Ton erhält dieselben Initialwerte.
Das wird nicht mit zusätzlicher Zufälligkeit kaschiert und bleibt Teil der
Hörbewertung von Artikulation und Wiederholung. Gleichzeitig identische
Quellfrequenzen lässt der aktuelle Produktpfad nicht zu.

Kein Produkt-DSP oder Header geändert; kein zusätzliches RAM/Flash im
Produktlink, kein neues Gerätetiming behauptet. Die bekannte Host-/ARM-Basis
aus PR139 bleibt der Nachweis für den unveränderten Kern. Das neue Werkzeug
besteht lokal seine tatsächlichen DSP-/PCM-, Format-, Headroom-, Mono- und
Inputprüfungen; Python, Workflowstruktur und Diff sind geprüft. CI führt
diesen zusätzlichen Vergleich künftig ebenfalls aus.

## Kurzer Hörvergleich

Zuerst `SD03_COAST_Grain_AB_27s.wav`:

| Zeit | Derselbe mehrstimmige Verlauf |
|---|---|
| 0–12,75 s | vollständiger aktueller Kandidat |
| 12,75–14,25 s | Pause |
| 14,25–27 s | ohne Grain |

Die Montage enthält alle Einsätze und den ersten Release. Je ein
dokumentierter konstanter Vergleichsgain von +0,9 dB und 100-ms-Schlussfade
als Ausschnittbearbeitung; kein Firmware-Retirementnachweis durch diesen Fade.
Messung der Montage: **−26 LUFS**, True Peak **−15,8 dBFS**.

Die Einzeldateien enthalten den vollständigen 27-s-Verlauf mit allen
natürlichen Releases: RAW-Firmwarepegel und separate LISTEN-Kopien mit jeweils
konstant +1,4 dB, ohne AGC. Raum und Natur aus, Stereo PCM16/44,1 kHz,
Hash-/Ereignisnachweise in `SD03_INDEPENDENCE_METRICS.json` und `.events.csv`.

Hörfrage: Tragen die Einsätze und Ausklänge selbstständig, ohne gemeinsame
Welle, störende Reibung oder Rauschdecke? Dateiname und Zeitstelle benennen.
Die finale Grain-/Tonkörperwahl hängt weiter an SD02. SD03 bleibt deshalb
offen; der nächste einzelne Vorbereitungsdurchlauf ist SD04, HIGHLANDS trocken
gegen COAST, mit ausdrücklichem Keep/Remove-Gate.
