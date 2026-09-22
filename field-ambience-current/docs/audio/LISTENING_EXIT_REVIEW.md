# Generate — natürlicher Ausstieg, Paket 2026-09-22

## KERNURTEIL

Der gemeinsame Effektraum war bereits durchgängig. Trotzdem verschwanden die
eigentlichen World-Stimmen beim Ausstieg zu einem Character nach etwa 15 ms.
Das hörbare Problem war die Quellenüberblendung, nicht ein zu kurzer Hall.
Die Quellen klingen jetzt unabhängig aus; H743-Last und Hörfreigabe bleiben offen.

## FUNDAMENTAL FALSCH

Zwei bestätigte und korrigierte Fehler:

1. Der Mixer multiplizierte den gesamten Ambient-Pfad mit dem Gegenstück zum
   Native-Crossfade. Nach dessen 662 Samples wurden freigegebene World-Stimmen
   gar nicht mehr gerendert. Ihre Hüllkurven und Audiozustände froren ein;
   übrig blieben nur Shared-FX-Tails. Ein trockener Regressionstest ergab im
   alten Build nach etwa 0,4 s exakt 0 PCM RMS statt eines Quellen-Releases.
2. Harmony deaktiviert manuelles Bass-Follow. Dieser Zustand galt auch in
   Generate: derselben World fehlte nach Harmony das automatische Fundament.
   Generate darf nun unabhängig von der gespeicherten manuellen Zuordnung den
   Bass führen; Stop gibt ihn ausdrücklich frei. Nach Rückkehr bleibt Harmony
   weiterhin alleiniger Besitzer seines manuell gesteuerten Basses.

## NOCH NICHT SELBSTVERSTÄNDLICH

- Während des Ausklangs können Ambient-Stimmen und ein manuell gespielter
  Native-Core gleichzeitig rechnen. Bisher betraf das wenige Fade-Blöcke;
  jetzt je nach Hüllkurve mehrere Sekunden. Kein H743-Leistungsversprechen.
- Der 15-Minuten-Displaytimer, Wake ohne versehentliche Note/Wertänderung und
  die Kennzeichnung manueller Voice/Character-Menüwerte fehlen weiter.
- Überlagerung kann mehr Mixenergie erzeugen. Bestehender Master/Schutz bleibt;
  die A/B-Probe ist kein Nachweis aller Extremkombinationen oder der Klangbalance.

## LOCKED

Generate erneut gibt die Bedienung sofort zurück. Der gemerkte Character ist
direkt spielbar; kein Warten auf die letzte Hallfahne und kein zusätzlicher
Exit-Modus. Während des Ausklangs werden keine weiteren automatischen Noten
erzeugt. Der gemeinsame Effektraum wird nicht neu gestartet.

## REMOVE / MERGE / REDESIGN

- Getrennte Mixer-Gains für den neuen Core und freigegebene Ambient-Quellen.
  Die Quellen behalten ihre Hüllkurven statt des bisherigen 15-ms-Wegblendens.
- Wind und endlose Texturen sind keine Noten-Releases: auf Dry UND Send etwa
  zwei Sekunden ausblenden. Originale Atmos-/Texture-Einstellungen unverändert;
  schnelles Zurückkehren blendet vom aktuellen Gain wieder ein.
- Reuse der bestehenden vier Bass-Scratchbuffer für den Hintergrundübergang.
  Keine zusätzlichen Audiopuffer, kein Heap, keine zweite FX-Instanz.
- Neue Zustandsfelder: zwei float-Gains, zwei uint32_t-Zähler = 16 Byte
  Nutzdaten. Host-BSS wächst einschließlich Alignment um 32 Byte; kein ARM-
  Map-Nachweis. Zusätzliche Kontroll-/Mixoperationen sind fest begrenzt.
- Ende erst nach Stimmenruhe PLUS mindestens 50 ms leisem Dry/Send-Signal:
  unter 0,000001 FS für die Summe der vier Kanalbeträge. Damit zählt auch die
  Resonanzkörper-Fahne nach dem letzten Pluck. Der neue Core beeinflusst diese
  Messung nicht. Danach wird Ambient wieder aus dem aktiven Renderpfad genommen.
- 64 s harte Fehlergrenze mit Ausblendung der letzten Sekunde schützt vor
  dauerhaft belegten/falsch geführten Quellen. Reguläre getestete Releases
  erreichen diese Grenze nicht. Sie ist kein musikalischer Default.

Manueller Character-Wechsel außerhalb dieses Generate-Ausstiegs verwendet
weiterhin den bisherigen kurzen Crossfade. Bei Rückkehr zu manuellem Ambient
bleibt dessen eigener Hintergrund wie bisher aktiv; das ist kein Master-Mute.

## BESTE VERSION

Die Maschine hört auf, neue Ereignisse zu spielen. Der Raum und die vorhandenen
Töne enden nachvollziehbar, während der Mensch sofort übernehmen kann. Mehr
Hall würde einen abgeschnittenen Ton nur verdecken; die richtige Lösung erhält
seine Quelle. Nächster Bedienbaustein: Displayruhe und Wake. Danach World-
Vordergrund, Pad und Raum gegeneinander hören und ggf. reduzieren.

## Nachweise auf dem Host

`bash test/run_tests.sh` bestanden, einschließlich Hotpath-Lint; Produkt-Engine
15.159 Checks, FX 478.860 Checks, jeweils ohne Fehler. Keine neue Warnung in
den geänderten Dateien; bestehende Menü-Testwarnungen bleiben unverändert.

| Prüfung | Ergebnis |
|---|---|
| Trockener A/B-Regressionsfall | Alter Build 0,0; neuer Build ca. 751,4 PCM RMS nach ca. 0,4 s: Quellen-Release bleibt wirklich hörbar. |
| Alle sechs Characters | Sofort spielbar; Quellen-Overlap im getesteten Default-Fall vor 9,35 s beendet, keine verbleibenden Pads. |
| Maximales Shape-Release + Atmos/Texture + gehaltene manuelle Stimme | Quellen-Overlap vor 37,04 s beendet; manuelle Stimme spielt weiter. |
| Schnelle Reversals | 100 Start/Stop-Zyklen in 64-Frame-Blöcken; begrenzte Sampleänderung, keine verlorene Rückkehr. |
| Harmony → Generate → Harmony | Bass in Generate aktiv; nach Stop frei; ursprüngliche manuelle Bass-Zuordnung bleibt. |
| Absichtlich gehaltene zusätzliche Ambient-Quelle | Separater Host-Probe: 64-s-Schutz beendet Overlap, bei 65 s PCM-Peak 0 (FX aus). |
| A/B vor dem Exit | Erste 5 s beider Ausschnitte sample-identisch; Unterschied beginnt mit der untersuchten Handlung. |

Scheduler-/Audio-Tests sind keine H743-Laufzeitmessungen. Die Fehlergrenze wurde
zusätzlich per öffentlicher Engine-API im Shared-Library-Probe geprüft; die
übrigen Lebenszyklusfälle stehen in `test/test_synth_device.c`.

### 26,5-s-Hörvergleich

`tools/review_listening_exit.py --before baseline.so --after candidate.so --output /tmp/exit-review`

Erster Abschnitt: vorher, 0–13 s. Zweiter: nachher, 13,5–26,5 s. Open Sea läuft
aus seinem echten Generator; der 20-s-Vorlauf wird nur im Speicher berechnet.
Generate endet bei Sekunde 5 bzw. 18,5 der gelieferten Datei. Eine identische
manuelle Dusk-Note D4 folgt bei 6,5 bzw. 20 s und wird zwei Sekunden später
losgelassen. Keine geskriptete Ersatzmelodie für die generative World.

Ein einziger konstanter Export-Gain +8,6 dB für beide Abschnitte; keine
abschnittsweise Lautheitskorrektur oder Kompression. Je 120 ms Randfade nur
an den Ausschnittsgrenzen. Export: −25,1 LUFS, −12,9 dBTP. Beide Rohdateien:
0 geclippte Samples. Der Ausschnitt begrenzt lange Endfahnen; Firmware-Releases
werden dadurch nicht verändert. Gemessen, nicht subjektiv als gut freigegeben.

## TEST AM GERÄT

1. Aus jeder World zurück zu jedem Character: trocken und mit Raum hören;
   keine versteckte Lautstärkestufe, keine abgeschnittene Vordergrundstimme.
2. Sofort neue Noten/Hold/Clear, mehrmals Generate umschalten; Quellen dürfen
   nicht hängenbleiben. Lange Shape-Hüllkurven gesondert testen.
3. H743-DWT gerade WÄHREND der Überlappung: Worst-Case-World plus teuerster
   Core und FX; `peak_load < 0.60`, keine Deadline-Misses. Aktuelle Map/Stack
   prüfen. Vorher keine Freigabe der längeren Parallelverarbeitung.
4. Kopfhörer, Gerätelautsprecher und Mono: Bassfundament nach Note/Harmony/Land
   vergleichen; gleiche World darf nicht wegen des vorherigen Modus ausdünnen.
