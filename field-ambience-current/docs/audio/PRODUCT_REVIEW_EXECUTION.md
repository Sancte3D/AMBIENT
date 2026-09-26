# AMBIENT — Produktreview und nächste abgeschlossene Pakete

Stand: 2026-09-22, PR #129. Die ausdrückliche Nutzerentscheidung für einen
**autonomen Hörmodus mit gesperrten Spielflächen** ersetzt die vorherige
Planung „Generate bleibt gleichzeitig spielbar“. Der Router-Fix vom
2026-09-17 bleibt gültig; paralleles Character-Spiel mit automatischem Bett
ist kein notwendiger nächster Ausbau mehr.

**Aktuelle Arbeitspriorität: Sound vor Display; beruhigend statt aufmerksamkeitsfordernd.**
Automatisches Register und Einstiegslogik sind in `CALM_REGISTER_REVIEW.md`
reduziert; Alarm-/Sirenen-/Pieptoncharakter bleibt ein Ausschlusskriterium. Open Sea wird als erste
vollständige Klangwelt auseinandergenommen; Quellenkorrektur und Abtrag von
Effekten stehen in `OPEN_SEA_SOUND_REVIEW.md`. Die Ruhe-UI bleibt erforderlich,
wartet aber auf die nächsten abgeschlossenen Klangbausteine.

**Naturraum-Vertrag:** `NATURE_WORLD_CONTRACT.md`. Worlds müssen einen ruhigen
Naturort tragen; elektrische Störassoziationen ausschließen. Keine universelle
Beruhigungs-/Heilwirkung behaupten. Age erzeugt kein eigenes Brummen/Rauschen
mehr; Horn/Bowed und die geografischen Voice-Klischees bleiben zu prüfen.

## KERNURTEIL

Eine bewusst gewählte Landschaft, die selbstständig musikalisch weiterlebt,
ist ein klares Produktversprechen. Eigene Harmonik, Timbres und gemeinsame
Raumbearbeitung sind vorhanden. Noch fehlt die geschlossene Erfahrung vom
Start bis zum lautlosen Display und zum natürlichen Ausstieg. Mehr Synths
würden diese Lücke derzeit vergrößern.

## FUNDAMENTAL FALSCH

- Eine World darf im Hörmodus nicht von der zuletzt gewählten manuellen
  Synth-Engine stummgeschaltet oder von einer fremden Voice überlagert werden.
  Dieses Paket korrigiert die Auswahl und bindet gehaltene World-Stimmen
  an das tatsächliche Melodie-Release.
- Klangqualität darf nicht aus Featureanzahl, Namen oder Testzählern abgeleitet
  werden. „AAA“ bleibt ohne Vergleichshören und echte Ausgänge unbewiesen.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Rückkehr aus Generate erhält jetzt die Quell-Releases und lässt den
  Character sofort spielen. Host-Nachweis: `LISTENING_EXIT_REVIEW.md`.
  Die temporär längere Parallelverarbeitung braucht noch den H743-Lastnachweis.
- Character/Voice-Einstellungen wirken während Generate auf die manuelle
  Auswahl bzw. erst nach Rückkehr. Die Oberfläche kennzeichnet das noch nicht.
- Display-Ruhe nach 15 Minuten menschlicher Inaktivität und klangneutrales
  Aufwecken fehlen. Der jetzt implementierte Generate-Puls ersetzt das nicht.
- Note/Harmony/Land bleiben drei manuelle Spielweisen; ihre Reduktion ist
  offen. Gesture und physische Zellen teilen weiterhin Quellen; der Scene-
  Browser während gehaltener Noten bleibt eine gesonderte Routing-Prüfung.
- H743-Zellen sind im aktuellen HAL digitale Schalter mit fester Tap-Amplitude.
  Keine gemessene Anschlagsdynamik oder Drucktiefe behaupten.

## LOCKED

Als Prinzipien: gewählte World als harmonischer Kontext; ein gemeinsamer
Effektraum; musikalische Pausen; eindeutiger Empfänger für Anschlag/Loslassen;
Generate als bewusst gestarteter Hörmodus; Volume immer erreichbar.
Konkrete Klangfarben, Parametergrenzen, Haptik und Lichthelligkeit sind noch
nicht durch diese Prinzipien freigegeben.

## REMOVE / MERGE / REDESIGN

| Paket | Entscheidung / Nachweis | Stand |
|---|---|---|
| Hörmodus und World-Phrasen | Zellen/Hold/Drone sperren; manuelle Engine merken; kuratierte World-Stimme, eigene Zeitprofile; ruhiger Generate-Puls. | Implementiert, Host-geprüft; `LISTENING_WORLD_REVIEW.md`. |
| Ausstieg | World-Quellen ausklingen lassen, Hintergrund über 2 s ausblenden, Character sofort verfügbar; keine zusätzlichen Audiopuffer. | Host-geprüft; H743-Last und Hörfreigabe offen. |
| Ruhe | Display nach 15 min menschlichem Idle dunkel; erste Wake-Bedienung klangneutral. Generate/Volume/Clear unmittelbar; keine Generator-Ereignisse als Aktivität. | Nachgeordnet: Sound hat Vorrang. |
| Klangwelt | Open-Sea-Grundton korrigiert und Stimme/Pad/Raum getrennt verglichen. Automatisches Register auf D3..A4 gesenkt. Chorus/Blur-Auslöschungen begrenzt (`CALM_MOTION_REVIEW.md`). Alps-Quelle umgebaut (`ALPS_BODY_REVIEW.md`); Bowed-Vibrato entfernt und Bogenrauschen reduziert (`BOWED_MOTION_REVIEW.md`); jetzt Choir/Forest, dann Guembri/Desert. | Aktive Priorität; keine zusätzlichen Synths. |
| Einzelstimmen | Dew-Attack, Glimmer-Ratio/oberes Register, Ambient-Retuning; nacheinander Hüllkurven und brauchbare Reglerbereiche. | Offen; `SOUND_REVIEW_QUEUE.md`. |
| Raum und Natur | Jeweils Effektstufen entfernen und vergleichen; gemeinsame Raumlogik. Wind/Noise über mehrere unabhängige 20–30-s-Ausschnitte auf periodische Bewegung und homogene Textur prüfen. | Offen; frühere Korrekturen sind keine Hörfreigabe. |
| Physisches Produkt | CAD/PCB/Bilder, Displayhierarchie, Sichtbarkeit, Haptik und echte Ausgänge. | Aktuelle Evidenz und Prototyp nötig. |

Nicht hinzufügen: eine zweite vollständige Renderarchitektur nur für die alte
Idee paralleler Bedienung. Ein Release-Übergang muss trotzdem CPU, RAM und
Pitch-Memory korrekt behandeln. Gemeinsame Bass-/Drone-Funktion prüfen;
kein Bestandsschutz für sechs Character oder neun FX-Menümodi.

## BESTE VERSION

World wählen. Generate drücken. Dieselbe Landschaft entwickelt sich hörbar
weiter, ohne Tastendruck zu verlangen. Die gesperrten Spielflächen laden nicht
zu folgenlosen Parameterexperimenten ein; eine verständliche Rückmeldung
macht den Hörmodus sichtbar. Ein langsames Licht bestätigt Aktivität, das
Display wird nach 15 Minuten menschlicher Ruhe dunkel. Volume bleibt direkt.
Generate erneut lässt die Welt ausklingen und gibt das Instrument zurück.
World-Wechsel sind bewusste Entscheidungen, kein willkürliches Landschafts-
Hopping. Verschiedene Worlds brauchen verschiedene musikalische Zeitverläufe,
aber dieselbe Qualität bei Pegel, Übergängen und Raum.

## TEST AM GERÄT

1. Aus jeder der sechs manuellen Engines Generate starten: selbstständiger
   Klang, keine hängenbleibenden alten Töne. Alle fünf Zellen, Hold, Drone,
   Shift+Hold und Scene-Zellen dürfen keine neuen Noten/Modi auslösen.
2. Generate erneut und Clear (auch mit Shift): kein neuer automatischer Ton;
   Übergang und Release getrennt trocken/mit Raum prüfen. Quell-Releases müssen
   trotz sofortiger manueller Noten weiterlaufen; maximale Shape-Dauer testen.
3. Generate-Licht im dunklen Raum beurteilen: Aktivität sichtbar, keine
   Alarmwirkung, keine Blendung, beim Ausstieg vollständig aus.
4. Nach Timer-Implementierung: 15 Minuten ohne menschliche Eingabe, während
   Musik weiterläuft. Wake-Interaktion darf weder Note noch Wertänderung
   auslösen. Volume, Clear und Generate müssen sofort wirken.
5. Gleiche Lautstärke, Kopfhörer und Gerätelautsprecher, Stereo und Mono:
   20–30 s je World. Vordergrund darf nicht wie ein Soloinstrument vor einem
   fremden Pad stehen; keine tubuläre Schärfe oder dominante Raumfahne.
6. Aktueller H743-Build/Map/Stack und DWT: `peak_load < 0.60`, keine Deadline-
   Misses, besonders an World- und Character-Übergängen. Hostlaufzeit ist kein
   Ersatz. CAD/Haptik ohne aktuellen Prototyp nicht freigeben.
