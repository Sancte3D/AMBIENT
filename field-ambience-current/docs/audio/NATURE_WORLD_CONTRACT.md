# AMBIENT — ruhige Naturorte, 2026-09-23

## KERNURTEIL

AMBIENT soll bildlich vorstellbare Naturorte durch weiche musikalische Töne
erfahrbar machen. Lange Hüllkurven und harmonische Kollisionsprüfung sind eine
gute Grundlage. Ein tiefer, dauerhaft summender Synth oder ein geografischer
Name allein erfüllt diese Idee nicht. Klang und Entwicklung müssen den Ort
tragen. Dieser Vertrag präzisiert die bisherigen Klangreviews.

## FUNDAMENTAL FALSCH

- Elektrisches Brummen, aufdringliches Summen/Schnarren, Piepen, Sirenen und
  alarmartige Wiederholung widersprechen dem ausdrücklich gewünschten Produkt.
- Geografie rechtfertigt kein Instrumentenklischee: Alps muss kein Hornruf,
  Desert keine schnarrende Guembri sein. Die bisherigen Voice-Zuordnungen
  haben keinen Bestandsschutz.
- „Beruhigend für alle neurodivergenten Menschen“ ist keine belastbare
  Wirkungsgarantie. Bereits innerhalb des Autismus unterscheiden sich
  sensorische Präferenzen und können sich je nach Situation ändern.
  „Fast heilend“ beschreibt hier die gewünschte Geborgenheit, keine Behandlung.
  Grundlage: [National Autistic Society: sensory processing](https://www.autism.org.uk/advice-and-guidance/about-autism/sensory-processing),
  gelesen 2026-09-23. Daraus folgt keine Wirksamkeitsstudie zu AMBIENT.

## NOCH NICHT SELBSTVERSTÄNDLICH

Ein tiefes Signal kann weiter wie ein Motor, Trafo oder Insekt wirken.
Musikalische Körper brauchen sanfte Einsätze, tragfähige Tonhöhe, zurückhaltende
Obertöne und nachvollziehbares Ausklingen. Ein warmer gehaltener Akkord bleibt
möglich; seine Dauer allein macht ihn nicht zum störenden Brummen.

Naturtexturen unterstützen den Ort, ohne eine permanente Rauschdecke zu
erzwingen. Meer verlangt weder kreischende Möwen noch hart wiederholte Brandung;
Wald braucht keine alarmartigen Vogelrufe oder summenden Insekten. Ebenso wenig
rechtfertigt Naturgetreue plötzlich laute Ereignisse.

## LOCKED

Englische, unmittelbar vorstellbare Naturorte als Worlds. Ruhige musikalische
Entwicklung, weiche Übergänge, hörbare Pausen und erreichbare Lautstärke/Stop.
Individuelle Hörentscheidung gehört zur Qualität; keine universelle
„beruhigende Frequenz“ oder zusätzliche Therapie-Einstellung erfinden.

## REMOVE / MERGE / REDESIGN

### Abgeschlossen: Age erzeugt kein eigenes Brummen/Rauschen mehr

Im aktiven `firmware-c-next/src/ambient_effects.c` erzeugte Tape absichtlich
einen 50-Hz-Sinus und weißes Bandrauschen, auch bei stillem Eingang. Beide
Generatoren entfernt, inklusive Hum-Phasenzustand. Age färbt weiter gespieltes
Material mit Bandbegrenzung, Sättigung, Wow/Flutter und Drift. Kein Noise-Gate
und kein Abschneiden von Ausklängen. Keine zusätzlichen Buffer/Operationen.

`engine_set_age()` steuert diesen gemeinsamen Mastereffekt; das alte `tape.c`
gehört nicht mehr zum aktiven Renderpfad. Änderungen betreffen Tape und Dream
für alle Worlds/Character, nicht die separaten Naturtexturen. Das Entfernen
des Hiss-Zufallsaufrufs verschiebt die gemeinsame RNG-Folge; identische Seeds
müssen deshalb nicht mehr dieselben späteren Grain-Positionen erzeugen.

Neue Regression: drei Age-Werte (0/0,2/1), Tape/Dream, beide öffentlichen
Float-APIs. Drei Sekunden stiller Eingang müssen exakte Null liefern; danach
muss ein gespielter Ton weiter passieren. Am alten Code 12/12 Stille-Fälle rot.
Vorher bei Age 0,2 im Dream-Bus etwa −104 dBFS RMS, bei Age 1 etwa −63 dBFS.
Das sehr leise Default-Geräusch erklärt nicht automatisch das berichtete
instrumentale Summen. Elektrisches Brummen realer Hardware ist separat offen.

Nachher 12/12 Stille-Fälle exakt Null; gespielter Ton passiert weiter. Volle
`bash test/run_tests.sh` grün, inklusive 489.609 FX-Checks. Interner FX-Zustand
680 → 672 Byte im Host, Arena unverändert 214.489 Byte. Aktuelle H743-Map/
DWT bleiben ungeprüft. Durch die verschobene RNG-Folge beträgt der maximale
Dream-Probe-Swing jetzt 6,77 dB und bleibt unter dem bestehenden 8-dB-Gate.

### World-Richtung, noch keine Hörfreigabe oder pauschale Umbenennung

| Bestehende World | Vorstellbarer Ort / gewünschtes Verhalten | Konkrete Ausschlusskriterien |
|---|---|---|
| Open Sea | Ruhiges offenes Wasser; breite, unregelmäßig anschwellende Töne | Gleichförmiges Pumpen, Pfeifen, dauerhaftes scharfes Rauschen |
| Moss Fields | Geschützter moosiger Wald; weiche, nahe, gedämpfte Töne. Als Forest-Kandidat prüfen. | Insektensummen, Pieprufe, hohle Vokalresonanzen |
| Alps | Stilles Tal mit fernem Raum und klaren weichen Tönen | Hornsignal, Fanfare, schneidende Höhen |
| Fjords | Geschütztes tiefes Wasser zwischen Felswänden; langsame dunklere Reflexion | Metallisches Dröhnen, Bassdruck; bloße EQ-Kopie von Open Sea |
| Desert | Weite warme Landschaft; wenige runde Töne mit großzügigen Pausen | Schnarrende Steg-/Metallgeräusche, trockene harte Attacken |

Forest soll zuerst die vorhandene Moss-Rolle schärfen, keine sechste World
hinzufügen. Wenn Sea und Fjords im Blindvergleich nur durch Filterhelligkeit
unterscheidbar sind, zusammenführen oder eine grundsätzlich neu entwerfen.
IDs, Presets und Namen bleiben in diesem kleinen Paket unverändert.

## BESTE VERSION

Der Ort wird aus Raum, Nähe, Bewegung und musikalischer Dichte vorstellbar.
Man muss keine Synthese kennen. Der Klang entwickelt sich behutsam, ohne
regelmäßig Aufmerksamkeit einzufordern. Individuelle Vorlieben werden über
wenige sinnvolle vorhandene Controls berücksichtigt; Messwerte zertifizieren
keine Entspannung. Nächste Einheit: Horn/Bowed auf Brumm-/Summcharakter und
Resonanzen prüfen; danach Choir/Forest und Guembri/Desert grundsätzlich bewerten.

## TEST AM GERÄT

1. Age min/default/max: bei stummer Quelle kein vom Effekt erzeugter Ton oder
   Rauschteppich; gespielte leise Ausklänge bleiben erhalten. Hardwareausgänge
   separat prüfen, da Softwaretests Netzteil-/Verstärkerbrummen nicht abdecken.
2. Jeweils <=30 s, gleiche angenehme Hörlautstärke: Quelle trocken, dann World.
   Wird ein Motor, Trafo, Insekt oder Signal gehört? Ursache isolieren und
   korrigieren oder Stimme verwerfen; nicht durch mehr Hall verdecken.
3. Freiwillige Hörtests auch mit neurodivergenten Menschen: „Angenehm oder
   anstrengend? Welcher Teil stört? Möchtest du weiterhören?“ Individuelle
   Antworten getrennt festhalten. Ablehnung nicht mit einem Mittelwert übergehen.
4. Worlds zunächst ohne Namen vergleichen: entsteht ein unterscheidbarer
   Naturraum? Die konkrete Geografie darf subjektiv bleiben; elektrische
   Störassoziationen sind ein Redesign-Signal. Kopfhörer, Speaker und Mono prüfen.
