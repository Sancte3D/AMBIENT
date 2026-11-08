# AMBIENT — musikalisches System, 9. September 2026

Das Grundkonzept trägt: wenige Rollen, ein gemeinsamer Raum, langsame
Entwicklung und Vorrang für den Spieler. Die erste Verbesserung der
Spielbarkeit ließ jedoch echte Lücken zwischen Klangkern, Hüllkurven und
Komposition offen. Dieser zweite Pass verbindet diese Ebenen.

## Was geändert wurde und warum

| Befund | Änderung | Musikalische Folge |
|---|---|---|
| V2 rundete Frequenzen auf ganze MIDI-Töne | Hz-Callback und gebrochene MIDI-Tonhöhen in allen sechs Kernen | reine Stimmung bleibt bis zum Oszillator erhalten |
| Globale Klang-/Shape-Regler erreichten V2 nicht | vorhandene Filter folgen Brightness, Resonance, Sweep und EnvMod; natürliche Hüllkurven folgen Shape | Regler funktionieren über alle Klangkerne hinweg |
| Ersetzte Charakterstimmen wurden hart zurückgesetzt | vorbereitete nächste Stimme, 8-ms-Ausblendung der alten Stimme | sanfter Anschluss bei dichtem Spiel; weiter drei Stimmen pro Instrument |
| Generator sah hauptsächlich gehaltene Auto-Töne | feste Tonhöhen-Memory für gehaltene Töne, Ausklänge, Bass und Pedal | automatische Töne berücksichtigen den verbleibenden Klang |
| Schichten konnten gleichzeitig neu einsetzen | gemeinsamer Mindestabstand 1,4 s | weniger Ereignisstau, klarere Rollen |
| Komposition durchlief immer denselben Kreis | gewichteter Graph mit Belegung, Zustandsalter und Atem | mehr Variation bei begrenzter musikalischer Richtung |
| Bettlautstärke änderte nur neue Noten | geglätteter Gain pro Quelle | EMPTY kann auch eine gehaltene Fläche zurücknehmen |
| Eintönige Zufallsphrase wurde als Motiv gespeichert | mindestens zwei verschiedene Töne vor Archivierung; maximal zwei identische Einsätze | kein dauerhaft festhängender Melodieton |
| Modale Unterschiede kollabierten zu Dur/Moll | gemeinsame Skalentabelle, geschützter Pentatonikkern und seltene Modalfarbe | modale Unterschiede werden möglich, ohne Kollisionen zu erzwingen |
| Desert verwendete den Raum einer anderen Welt | eigene engere Raumparameter | fünf tatsächlich getrennte Raumcharaktere |

Orbit-Morphing und Storm-PWM wurden auf langsame Bewegung unter 0,15 Hz
gelegt. Kleine Chorus-/Vibratobewegungen bleiben absichtliche Charaktermerkmale.
Bamboo bekommt eine kurze, Shape-gesteuerte Anschlagrampe und behält seinen
LPG-Ausklang. Die sechs individuellen Parameter pro Synth bleiben gespeichert.

## Harmonie und Zeit

Alle automatischen Einstiege prüfen denselben Kontext. Verboten bleiben
Halbton- und Tritonusklassen sowie enge Sekunden im tiefen Register; eine
weite None wird nicht pauschal als tiefer Cluster behandelt. Ohne passenden
Ton folgt Pause. Motiv-Replay muss erneut Kollisions- und Oktavsprungregeln erfüllen.

Die Pitch-Memory hat 128 feste Tonhöhenplätze. Ihre Zeitabschätzung verbindet
die beim Anschlag gewählte Hüllkurve mit einem konservativen FX-Horizont.
Ein kürzer gedrehter Release löscht alte geschützte Töne nicht. Bei Tonart-
oder Moduswechsel werden generative Stimmen losgelassen und neu geplant,
während ihre alten Tonhöhen weiter berücksichtigt werden.

CALM/OPEN/DEEP/EMPTY/RETURN wählen aus zulässigen Nachbarn, berücksichtigen
Klangbelegung und Zustandsalter und vermeiden häufiges Hin-und-her. EMPTY
kommt spätestens beim sechsten Wechsel seit dem letzten Atem und führt über
RETURN zurück. Ziele gleiten rund vier Sekunden. EMPTY nimmt Eno-Einsätze
heraus; der Raum klingt weiter. Player-Presence pausiert die Entwicklung,
geplante Note-offs bleiben aktiv, Rückkehr erfolgt nach ungefähr acht Sekunden.

## Verifikation am aktuellen Code

- Vollständige `bash test/run_tests.sh`: bestanden.
- 16.335 Device-Prüfungen einschließlich spektralem Just-Nachweis,
  realer Ausgangsreaktion auf vier Makros und zwei Shape-Regler in sechs
  Synths sowie kontinuierlichem ersten Sample beim Voice-Handover.
- 3.575 automatische Einsätze in 18 simulierten 20-Minuten-Sitzungen
  (sechs Modi × drei Seeds): Latch, Spieler-Priorität, Ausklänge,
  Tonartwechsel, Einsatzabstand und Wiederholungsgrenze geprüft.
- Bestehende Effekt-, Szenen-, Bedien- und Blockgrößen-Tests bleiben grün;
  Hot-Path-Prüfung bestanden. 478.858 Effektprüfungen, keine Fehler.
- Render-Werkzeuge prüfen FLAC nach vollständigem Dekodieren gegen die
  ursprünglichen PCM-Bytes. Verpackung prüft exakte Länge und True-Peak-Reserve.

Hörmaterial: fünf Welten, ein voller 20-Minuten-Verlauf, Stimmwechsel bei
Horn/Bowed/Choir, alle sechs Synth-Regler und Instrumentwechsel. Vorher/Nachher
vergleicht gegen den ersten Sound-Pass (Tree `082a88f0`). Gemeinsamer Seed
bedeutet bei geändertem Generator bewusst keine identische Notenfolge.
Lautheitsabgleich ausschließlich mit konstantem Gain; keine zusätzliche Kompression.

## Grenzen und nächste reale Abnahme

V2 bleibt monophon; Autoplay bleibt im Ambient-Modus. Pitch-Memory ist ein
konservatives Modell, kein spektraler Hall-Tracker. Modalfarbe darf bei dichtem
Klang ausbleiben. Just betrifft die Grundstimmung; bewusstes Ensemble-Detune
und Vibrato erzeugen weiterhin Schwebungen. Der Mensch bestimmt seine Noten.

Die Tests belegen Funktionsverhalten und technische Grenzen, keine abschließende
musikalische Qualität. Die Render wurden technisch geprüft; eine subjektive
Hörabnahme wird nicht behauptet. Auf dem realen H743 fehlen weiterhin
DWT-Spitzenlast <60 %, Deadline-Soak sowie DAC/Verstärker- und Speaker/Kopfhörer-
Abnahme. Gerade Low-Mids, Anschlagbalance und lange FX-Fahnen dort beurteilen.
