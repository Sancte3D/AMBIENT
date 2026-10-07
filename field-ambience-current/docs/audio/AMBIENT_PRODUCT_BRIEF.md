# AMBIENT — vollständiger Produktentwurf v1.0

> **Aktualisierung 2026-10-06:** Nutzer hat Sound-/Codeentwicklung,
> Archivierung und gelegentliche Entwicklungsmerges autorisiert.
> [AMBIENT_SOUND_DESIGN_TODO.md](AMBIENT_SOUND_DESIGN_TODO.md) definiert den
> aktuellen Soundauftrag und seine Abnahme. Die folgenden Freigabe-/Firmware-
> Angaben beschreiben den Entwurf vom 2026-10-04, nicht den heutigen Code.
> UI/Display und physische Tastenbelegung bleiben nachgeordnet und dürfen sich
> ändern; musikalische Audioverträge werden vorher festgelegt. Kein fertiger
> World-Klang und kein realer Hardwaretest ist dadurch bereits freigegeben.

2026-10-04. Entscheidungsreifer Konzeptvorschlag, keine Nutzerfreigabe der neuen
Einzelentscheidungen und keine Klang-/Hardwarezertifizierung. Setzt
[AMBIENT_WORLD_SYSTEMS.md](AMBIENT_WORLD_SYSTEMS.md) und
[AMBIENT_SOURCE_ARCHITECTURE.md](AMBIENT_SOURCE_ARCHITECTURE.md) zusammen.
Bei Widersprüchen gelten ausdrückliche Nutzerentscheidungen vor diesem Brief.

## KERNURTEIL

AMBIENT ist ein eigenständiges Instrument für ruhige musikalische Umgebungen,
die man selbst anstoßen, beeinflussen und ohne weitere Arbeit hören kann.
Seine größte Stärke ist die Verbindung eines vorstellbaren Ortes mit einem
musikalisch lebenden System. Seine größte Schwäche wäre ein geteiltes Produkt:
ruhige Worlds zum Zuhören und ein davon unabhängiger Synth-Katalog zum Spielen.
Ich empfehle eine gemeinsame Klangidentität für beide Nutzungsarten.

**Produktversprechen:** AMBIENT lässt ruhige musikalische Landschaften
entstehen, die sich zusammenhängend entwickeln und mit wenigen Eingriffen
gestalten lassen.

Der Unterschied zur Wiedergabe fertiger Musik soll in tatsächlich veränderbarer
Komposition, nachvollziehbarem Einfluss und der eigenständigen physischen
Nutzung liegen. Das beschreibt den Entwurf; es beweist keine Marktneuheit.
Ein schöner historischer Name allein trägt dieses Versprechen nicht.

## FUNDAMENTAL FALSCH

- Eine World als Presetwechsel unter demselben generischen Notenstrom verkaufen.
- Mit einer für alle Worlds ständig laufenden Pad-/Bass-/Noise-Schicht
  Eigenständigkeit und hörbare Pausen wieder zunichtemachen.
- Jede Reglerstellung zulassen und den Nutzer schlechte Kombinationen reparieren
  lassen. Der bewusst begrenzte musikalische Spielraum ist Teil der Gestaltung.
- Historische Authentizität oder universelle neurodivergente Beruhigung
  behaupten. Klang wird zeitgenössisch gestaltet; individuelle Wahrnehmung bleibt.
- Zusätzliche Features aufnehmen, weil Taster, Code oder Displayplatz vorhanden sind.

## NOCH NICHT SELBSTVERSTÄNDLICH

### Natur, Klang und Geschichte haben unterschiedliche Aufgaben

Naturorte sind die erste verständliche Auswahl: COAST, WOODLAND, HIGHLANDS als
Arbeitsnamen. Klangfarbe und Ereignisgrammatik vermitteln diese Welt. Geschichte,
Artefakte und Materialien können ihre besondere Herkunft/Assoziation erklären
und Bildmotive liefern. Diese Identitätsebene muss nicht für jede Funktion
eine technische Begründung liefern; sie muss zum hörbaren Erlebnis passen.

Keine Zuordnung von Landschaften zu einer angeblich einheitlichen antiken
Kultur. Für spätere Referenzen jeweils festhalten: reales Objekt, belegte
Eigenschaften, eigene gestalterische Interpretation und nicht belegte Annahmen.
Zunächst Quellen auswählen, dann passende Referenzgeschichte kuratieren.

### Der manuelle Teil muss denselben Charakter behalten

Empfohlener Produktkern: Manuell spielt man die Klangfamilie der gewählten World
innerhalb ihres harmonischen Zusammenhangs. Automatisch verwendet dieselbe
Familie ihre eigene Kompositionsgrammatik. Die Wirkung unterscheidet sich durch
Spielhandlung bzw. autonome Entwicklung, nicht durch einen fremden Klangkatalog.

Das ist eine neue Empfehlung. Die vorhandenen sechs V2-Engines bleiben bis
zur Produktentscheidung als Bestand/vergleichbare Kandidaten erhalten. Ihre
Implementierung, IDs und Scenes werden in der Konzeptphase nicht gelöscht.
Die vorherige Quellenarchitektur ließ den manuellen Bestand offen; sie ist
damit nicht als Freigabe sämtlicher sechs Engines im fertigen Produkt zu lesen.

Fünf digitale Spielflächen bedeuten weder fünf unabhängige Synths noch
Anschlagsdynamik. Das spätere Mapping muss für die begrenzte Stimmenzahl
verständlich sein. Keine Druck-/Geschwindigkeitssteuerung ohne passende,
nachgewiesene Sensorik versprechen. Note/Harmony/Land, Gesture, Hold und Drone
werden an diesem Kern geprüft; noch keine neue physische Belegung festschreiben.

## LOCKED

Ausdrücklich vorgegebene Richtung und bestehende Nutzerpräferenzen:

- Ruhige, harmonische Töne mit unterschiedlichen, vorstellbaren Naturwelten.
- Generate als bewusst gestarteter autonomer Hörmodus; Spielflächen, Hold und
  Drone sind darin gesperrt. Keine gleichzeitige manuelle Performance erzwingen.
- Lautstärke und Ausstieg bleiben unmittelbar erreichbar.
- Displayruhe nach 15 Minuten menschlicher Inaktivität; Musik läuft weiter.
- Generate-Licht zeigt Aktivität; keine Alarm-/Taktgeberwirkung beabsichtigt.
- Konzept → Sounddesign → UX/UI/Display → echtes Gerät.
- Nutzerseitige Hördateien höchstens 30 Sekunden, außer ausdrücklich gewünscht.

Nicht locked: Namen, genaue Klangparameter, finale Anzahl der Worlds,
Bedienbelegung, LED-Helligkeit, Hüllkurven und manuelle Engine-Auswahl.

## REMOVE / MERGE / REDESIGN

| Entscheidung | Empfohlener Umfang | Begründung |
|---|---|---|
| Worlds | Drei unterscheidbare Systeme zuerst | Identität beweisen, bevor Varianten ergänzt werden |
| Manuelles Spielen | Klangfamilie und Harmonie der World nutzen | Dasselbe Instrument bleibt bei beiden Nutzungen erkennbar |
| Pad/Bass/Drone | Nur ausdrücklich geplante Rollen | Keine versteckten Pflichtschichten oder doppelte Grundtöne |
| Effekte | Ein gemeinsamer Raumpfad, dosierte Quellformung | Raum verbindet, Quellen und Grammatik unterscheiden |
| Makros | Wenige musikalische Wirkungen | Parameter dürfen nicht gegeneinander repariert werden müssen |
| Neue Variante | Interne Fähigkeit, noch keine zusätzliche Taste | Generate-Semantik nicht unbemerkt überladen |
| Szene/Recording/Gesture | Nachrangige Kandidaten | Kernnutzung muss ohne Browser, Speicherung oder Aufnahme vollständig sein |
| Kultur/Artefakt | Kuratierte Identität und gegebenenfalls Klangquelle | Entdecken ermöglichen, ohne das Produkt zu einer Kulturen-Datenbank zu machen |
| Bestehende Taster | Nutzen prüfen, Platz nicht automatisch füllen | Eine schwache Funktion gewinnt nicht durch eigene Hardware |

Eine endgültige Entfernung physischer Controls ist eine spätere gemeinsame
Hardware-/UX-Entscheidung. Dieser Brief beauftragt keine CAD-/PCB-Änderung.

## BESTE VERSION

### A. Die ersten Sekunden

Einschalten führt in einen stillen, bedienbaren Zustand. Die zuletzt gewählte
World und eine begrenzte Lautstärkeeinstellung dürfen wiederhergestellt werden;
autonome Wiedergabe startet erst nach bewusstem Generate. Eine unerwartete
Fortsetzung bei Power-on gehört nicht zum Entwurf.

Nach Generate entsteht eine weich einsetzende, tonal erkennbare Quelle der
gewählten World. Kein gemeinsamer Boot-Pad-Sound vor der eigentlichen World.
Der erste Ton muss die Identität tragen, ohne Klangflächen künstlich sofort zu
füllen. Ziel für den späteren Entwurf: hörbarer Beginn innerhalb etwa einer
Sekunde; das ist noch keine gemessene Firmware-/Display-Startzeit.

Die Landschaft entwickelt sich anschließend, ohne Eingaben oder Displayblick
zu benötigen. Die jeweils eigene Grammatik entscheidet, wann weitere Töne
sinnvoll sind. Lauter und dichter sind keine obligatorischen Entwicklungsziele.

### B. Einfluss auf die laufende Welt

Lautstärke beeinflusst den Ausgang unmittelbar und getrennt von musikalischer
Dichte. World-Auswahl ist eine bewusste Veränderung des Ortes. Weitere logische
Einflussgrößen werden vor dem UX-Mapping beschrieben:

| Einfluss | Gemeinsame Bedeutung | COAST | WOODLAND | HIGHLANDS |
|---|---|---|---|---|
| Aktivität | Wie häufig musikalische Gelegenheiten genutzt werden | Überlappung und Eintritte | Figuren und Antworten | Fragmenthäufigkeit |
| Farbe | Tonale Fülle/Helligkeit innerhalb des jeweiligen Charakters | Obertongemisch und zurückhaltende Farbnoten | Anregungsfarbe und Dämpfung | Oberton-/Artikulationsverlauf |
| Raum | Nähe und Verbindung, bei erhaltenem Tonkörper | Diffuse Breite | Nähe und sparsame Reflexion | Distanz mit zurücktretendem Ausklang |

Diese drei Größen sind Kandidaten, keine zusätzliche Featureliste und keine
endgültige Belegung der vier Encoder. Lautstärke und World-Auswahl benötigen
ebenfalls Bedienraum. Die UX-Phase entscheidet, welche Größe direkt, verbunden
oder voreingestellt ist. Keine versteckte fünfte Hauptfunktion voraussetzen.

Alle nutzerseitig erreichbaren Bereiche müssen den World-Charakter erhalten.
Aktivität=hoch darf aus WOODLAND keinen Sequencer und aus HIGHLANDS keinen
Dauerruf machen. Raum=hoch darf Töne nicht vollständig verdecken. Farbe=hell
darf keine exponierten Pfeif-/Alarmtöne erzeugen. Mindestpausen, Stimmengrenzen
und Quellpegel werden deshalb intern begrenzt, nicht durch Warnmeldungen erklärt.

Änderungen nehmen musikalisch sinnvoll Einfluss: Helligkeit kann geglättet auf
laufende Töne wirken, Kontur/Motivwahl bevorzugt auf die nächsten Ereignisse.
Lautstärke/Stop müssen nicht auf eine Phrase warten. Exakte Glättungszeiten und
Parameterkurven sind Sounddesign, keine hier erfundene Implementierung.

### C. Wechseln, loslassen und zurückkehren

Bei Worldwechsel werden alte Ereignisse nicht weiter ergänzt. Deren Quellen
klingen innerhalb des gemeinsamen Budgets aus; neue passende Töne übernehmen
freien Platz. Harmonischer Zusammenhang wird zunächst gehalten. Mehrere schnelle
Auswahlen führen zum zuletzt gewählten Ziel, nicht zu einer Queue ganzer Welten.

Generate erneut beendet autonomes Planen sofort und führt zum manuellen Spielen
zurück. Ausklänge bleiben musikalisch erhalten, während neue manuelle Eingaben
nach begrenzter, weicher Quellenübergabe reagieren dürfen. Eine vollständig
doppelte Renderarchitektur ist dafür keine automatisch akzeptierte Lösung.
Der letzte manuelle Zustand wird zuverlässig wiederhergestellt.

Clear ist der erreichbare Weg zum Stillwerden: kein neues automatisches Ereignis,
Quellen kontrolliert freigeben und Raum zügig weich zurücknehmen. Im Normalbetrieb
kein sampleharter Cut. Die World-Auswahl bleibt erhalten. Clear darf nicht heimlich
auch eine andere World, Tonart oder gespeicherte Einstellung laden. Technische
Mute-/Fehlerbehandlung ist getrennt vom musikalischen Ausklingen zu entwerfen.

Die konkreten Ausstiegzeiten werden im Sounddesign gesetzt. Unbegrenztes Warten
auf selbstabklingende Quellen ist nicht zulässig. Pluck benötigt dafür den in der
Quellenarchitektur beschriebenen sanften Stop-/Dämpfpfad.

### D. Display und Licht

Nach 15 Minuten ohne menschliche Bedienung wird die Anzeige dunkel. Automatische
Noten, Releases, LED-Puls und Composer-Aktivität setzen den Timer nicht zurück.
Displayruhe ändert weder Seed noch Musikzustand, Raum oder Lautstärke.

Die erste allgemeine Navigation weckt zunächst klangneutral auf. Volume,
Generate und Clear müssen auch bei dunklem Display unmittelbar ihre eigentliche
Aufgabe ausführen. Ein bewusstes Stop darf nicht nur als Wake verbraucht werden.
Die genaue Zuordnung von Encoder-Drehung/Push ist eine spätere UX-Entscheidung.

Generate-Licht bleibt langsam, zurückhaltend und unabhängig vom musikalischen
Takt. Bestehender Viersekundenpuls ist ein Kandidat, keine optische Freigabe.
Schlafende Anzeige und ruhiges Licht sollen keine Aufmerksamkeit einfordern.
Keine abstrakten Audioanimationen als dauerhafte Beschäftigung auf dem Display.

### E. Zustandsvertrag

| Zustand | Audio | Spielflächen | Generate | Clear |
|---|---|---|---|---|
| PLAY, still | Bis zur Eingabe still | Manuelle World-Klänge | Hörmodus beginnt | Bleibt still, Kontext erhalten |
| PLAY, klingend | Manuelle Quellen/Ausklänge | Manuell | Alte Eingaben sauber lösen, World übernimmt | Stillwerden |
| LISTEN | World komponiert selbstständig | Gesperrt | Planung stoppt, Rückkehr PLAY | Planung stoppt, Stillwerden und PLAY |
| LISTEN, Display dunkel | Unveränderte Musik | Gesperrt | Sofortiger Ausstieg und Wake | Sofortiges Stillwerden und Wake |
| Übergang | Begrenzte alte/neue Quellen | Zielmodus maßgeblich, alte Releases behalten Besitzer | Letztes ausdrückliches Ziel gilt | Hat Vorrang |

Display dunkel ist eine unabhängige Anzeigeeigenschaft, kein neuer musikalischer
Modus. Releases werden immer an den Besitzer des ursprünglichen Press-Ereignisses
gegeben. Ein Moduswechsel darf einen späteren Release nicht zur neuen Quelle
umdeuten. Wiederholter Clear bleibt unschädlich; Stop hat Vorrang vor Navigation.

### F. Erinnerung und Speicherung

World, musikalische Makros und eine Version/Seed-Kombination beschreiben eine
Ausprägung. Note-/Releasezustände sind Sitzungsgeschehen; beim Einschalten dürfen
sie nicht als klingende Latches rekonstruiert werden. Keine Autosave-Schreiboperation
pro generierter Note. Dauerhafte Schreibstrategie muss Scene-Flash und Lebensdauer
der tatsächlich eingesetzten Speicherung berücksichtigen.

Scene-Laden darf keine unerwartete Erhöhung des Masterpegels oder sofortige
autonome Wiedergabe auslösen. Das ist eine Produktanforderung, keine Behauptung
über die heutige Scene-Implementierung. Migration alter fünf World-IDs und sechs
Engine-IDs wird vor einem Firmwareumbau separat geplant; keine stille Neuzuordnung.

### G. Hardware und Form

Referenz: [Hardwareübersicht](../onboarding/HARDWARE_ENGINEER_START.md) beschreibt
STM32H743, vier Push-Encoder, fünf digitale Spielflächen, Modifier, Display und
Audioausgänge. Das ist eine dokumentierte Ausgangslage, keine Prüfung eines
gefertigten Geräts, aktuellen CADs oder vollständiger Schaltungsunterlagen.

Der Entwurf benötigt weder zusätzliche Drucksensorik noch zusätzliche
Audioaufnahmen zur Kompositionsverwaltung. Drei aktive tonale Quellen und ein
Raumprozessor sind eine Begrenzung der Architektur, keine erwiesene CPU-Reserve.
Die konkrete Quellen-/FX-Auswahl bleibt an Speicherbänke und Spitzenlast gebunden.

Der tatsächliche Ausgangscharakter muss auf kleinen Lautsprechern bestehen:
warme mittlere Tonkörper statt notwendigem Tiefbass. Kopfhörer dürfen zusätzliche
Details zeigen, aber keine im Speaker versteckten Schärfen offenbaren. Anschluss-
wechsel, Amp-Mute und Ausgangspegel sind reale elektrische Prüfungen. Ein
Line-out darf ohne Impedanz-/Pegelprüfung nicht als beliebiger Kopfhörerausgang
beworben werden. Laufzeit, akustischer Pegel und Gehäusepassung bleiben ungemessen.

Form/Control-Größe sollen die Prioritäten ausdrücken: World, unmittelbarer Einfluss,
Lautstärke und Generate müssen auffindbar sein. Gesperrte Spielflächen brauchen
verständlichen Zustand, kein blinkendes Fehlersignal. Die Hierarchie wird in der
UX-/Industrial-Design-Phase am tatsächlichen Layout geprüft, nicht hier erfunden.

## TEST AM GERÄT

Zuerst die vorgesehenen Software-/Hörprüfungen, dann das echte Instrument:

- World-Identität ohne Namen, Naturgeräusch und Pegelvorteil; mehrere Seeds und
  gleiche musikalische Ausgangslage. Je Datei maximal 30 Sekunden.
- Alle zugänglichen Makrogrenzen bleiben musikalisch nutzbar. Ein einzelner
  nur bei Default schöner Klang genügt nicht.
- Worldwechsel bei voller Belegung; Generate/Clear während Übergang; Release
  nach Moduswechsel; wiederholte schnelle Eingaben; keine hängenbleibenden Quellen.
- 15-Minuten-Ruhe mit weiterlaufender Musik, Wake ohne unbeabsichtigte Änderung,
  unmittelbarer Volume/Stop. Kein Audio-/Timer-Konflikt mit Displayarbeit.
- Mono, Speaker und geeignete Kopfhörer-/Line-out-Konfiguration: Tonkörper,
  Schärfe, Klicks, Pegel, Pausen und Raum. Niedrige Lautstärke ebenfalls prüfen.
- Reale Speicherbelegung, Stack, maximale Renderzeit und Deadline-Misses,
  besonders im Übergang. Alte CI-/Hostwerte nicht als aktuelle Messung verwenden.
- Nutzung ohne Erklärung: gewählte World erkennen, Hören beginnen, beeinflussen,
  still werden, zum Spielen zurückkehren. Fehlbedienungen zuerst im Entwurf lösen.

### Freigabestand

Der Entwurf ist zur Entscheidung ausgearbeitet. Die bereits vorgegebene Richtung
ist beibehalten; neue Empfehlungen (insbesondere die gemeinsame manuelle Palette)
sind nicht als Nutzerentscheidung markiert. Das Konzept-Gate bleibt bis zur
gemeinsamen Freigabe offen. Sounddesign, danach UX/UI und zuletzt Hardwaretests
haben jeweils eigene nachprüfbare Ergebnisse.

Eine Umsetzung darf nicht alle alten Systeme gleichzeitig umbauen. Kleine
Pakete mit eindeutiger Wirkung und Rückkehrmöglichkeit stehen in
[AMBIENT_IMPLEMENTATION_SEQUENCE.md](AMBIENT_IMPLEMENTATION_SEQUENCE.md).
Die Prüfung zentraler Entscheidungen aus allen fünf Perspektiven und die
Grenzen vorhandener technischer Evidenz stehen in
[AMBIENT_DECISION_LEDGER.md](AMBIENT_DECISION_LEDGER.md).
