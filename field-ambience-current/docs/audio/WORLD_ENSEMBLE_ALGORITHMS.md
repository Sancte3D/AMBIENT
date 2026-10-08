# AMBIENT — verschiedene Ensemble-Algorithmen, 8. Oktober 2026

## Bestätigte musikalische Richtung

Der Nutzer hat die 27-s-Ensemble-Probe aus PR144 mit „ja besser!!!! weiter so“
positiv bewertet. Bestätigt ist diese Richtung: gehaltene gemeinsame Akkordtöne,
verschiedene Dauern und Registerwechsel. Das ist keine gesamte Produkt-/Geräte-
oder Familienabnahme. Als Nächstes braucht jede World eine eigene musikalische
Regel. Die Regeln werden einzeln umgesetzt und gehört.

| World | Musikalischer Algorithmus | Konkreter Stand |
|---|---|---|
| COAST | Zwei äußere Register nähern sich in Gegenbewegung einer gemeinsamen Mitte; ein Akkordton verbindet die Schritte | Seed-gesteuerter Host-Algorithmus und tatsächliche C-DSP-Hörprobe vorbereitet |
| WOODLAND | Eine kleine Akkordfigur stellt eine Idee vor; eine andere Stimme antwortet mit verwandter Kontur und verändertem Register; Variation und Ruhe | Nächster eigener Entwurf, noch nicht durch diese Änderung implementiert |
| HIGHLANDS | Weit verteilte gehaltene Stimmen verändern einzelne Akkordtöne mit eigenen Dauern; längere offene Pausen und Rückkehr | Geplanter eigener Algorithmus; keine heutige Implementierung behauptet |

Gemeinsamer Vertrag: begrenzter harmonischer Plan, klare Stimmrollen, echte
Hz-/Besitzerhistorie, bewusste Spannungen/Auflösungen. Die Klangfamilie allein
ist keine World-Identität. Unterschiedliche Regeln für Zusammenhang und Zeit
werden nicht mit denselben Zufallsereignissen und anderem Filter vorgetäuscht.

## COAST — Zusammenlaufen

Der Nutzer beschreibt zwei gleichzeitig gespielte äußere Oktaven: unten steigt
es, oben fällt es, bis die Linien in einer mittleren Lage zusammenfinden.
Die Probe setzt dies über musikalische Tonstufen um, mit D4 als Ziel:

- Unterer Weg: **D3 → A3 → B3 → D4**.
- Oberer Weg: **D5 → B4 → F#4 → D4**.
- Gemeinsamer Akkordton: leises F#3, von 0,7 bis 22,6 s gehalten.
- Beide äußeren Wege beginnen gleichzeitig. Ihre weiteren Einsätze und Dauern
  sind unabhängig; Seed bestimmt die gemeinsame Phrasenweite, obere Verzögerung,
  kleine Akzente und unterschiedliche Haltezeiten.
- Beim Ziel **18,49 s** (Seed1234) werden die beiden Tonabsichten zu **einer**
  gemeinsamen D4-Stimme zusammengeführt. Der Zielton bleibt bis 23,5 s stehen.
  Es werden keine zwei unkontrolliert gegeneinander phasenden Unisono-Oszillatoren
  erzeugt. Die gehaltenen Absichten der Rollen bleiben eine spätere Besitzfrage.

`review_converging_world.py` erzeugt den Score aus diesen Regeln und Seed,
setzt ihn in eine temporäre Kopie des vorhandenen echten C-Renderers ein und
kompiliert unveränderte Product-Bowed/Room/Shape/DSP-Quellen. Derselbe Tonkörper
wie im positiv bewerteten Entwurf, Room0,24, Nature0, Volume0,6. Kein neuer Layer.

Die ursprüngliche komponierte Ensemble-Probe bleibt als positiv bewertete
Referenz erhalten. Dieser neue COAST-Verlauf ist ein eigenständiger Algorithmus,
kein Ersatz-WAV unter derselben Identität.

## Prüfbefund

- Ausgewählter Seed1234: acht angenommene tatsächliche Quellenstarts;
  maximal drei reale Stimmen einschließlich Releases; alle Quellen enden
  natürlich vor 27 s. Kein Clear oder Schlussfade.
- Dry/Room jeweils bytegleiches tatsächliches PCM und Traces bei 64/512 Frames.
- PCM16 Stereo/44,1 kHz, 27 s; finite/Clipping-, Peak-, Mono- und DC-Prüfung.
  LISTEN −23 LUFS / −12,1 dBFS True Peak, ein konstanter Gain, RAW getrennt.
- 300 Planfälle über drei Mittellagen und 100 Seeds prüfen Gegenrichtung,
  gemeinsames Ziel, eindeutige Zielquelle und Rückkehrabstand für reale Releases.
  Alternative Seeds ändern die Phrasierung; gleicher Seed wiederholt denselben
  Plan. Die zusätzlichen Seeds sind Planchecks, keine weitere Hör-/Geräteabnahme.
- C mit `-Wall -Wextra -Werror`, Python, Workflow und Diff geprüft;
  Source-/Header-/Tool-Hashes und der temporäre C-Score sind nachvollziehbar.

## Grenzen und nächste Arbeit

Das ist ein **Host-World-Algorithmus**, noch nicht in den autonomen Product-
Generator integriert. D5/B4 erweitern den bisherigen Produktregister-Kandidaten;
pro Rolle werden direkte Attackwerte und kurzer SHAPE-Release=0 verwendet.
Produktquellen, Header, Defaults und UI wurden nicht geändert. Der Raum darf
am Dateiende einen leisen Resttail enthalten; kein vollständiges Room-Ende
behauptet. Neue COAST-Hörwahl und Geräteabnahme sind offen.

Die Integration braucht Chord-/Voicing-Zustand, erweiterte geprüfte Register,
Rollenbesitz beim gemeinsamen Ziel und einen planbaren Umgang mit tatsächlichen
Releases und Raumgedächtnis. Die bisherige pauschale Intervall-/Tail-Sperre darf
beabsichtigte Stimmführung nicht unbemerkt in Stillstand verwandeln. WOODLAND
bekommt anschließend seinen Antwort-Algorithmus als eigenen kleinen Durchlauf.
Weiterhin 24 technisch geschlossene / 30 offene Gesamtgates.
