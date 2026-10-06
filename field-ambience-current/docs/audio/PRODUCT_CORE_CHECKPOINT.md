# Sound implementation checkpoint — 2026-10-06

## KERNURTEIL

COAST hat eine nachweisbar stabilere trockene Grundtonbasis. Der neue
Grammatikbaustein enthält drei tatsächlich verschiedene Abläufe und führt
gehörte Motive erst nach erfolgreicher Zulassung fort. Die grundlegende
Schwäche bleibt die Integration: Das derzeitige Geräteprogramm verwendet noch
den alten Fünf-Welten-Katalog und den gemeinsamen Scheduler. Dieser Commit
liefert geprüfte Kontrolllogik, keine drei fertigen oder gehörten Welten.

## FUNDAMENTAL FALSCH

Der alte manuelle Pfad kann zusätzliche Pad-/Bassschichten starten und die
angestrebte gemeinsame Drei-Stimmen-Grenze umgehen. Unterschiedliche Namen
und Effektpresets erfüllen die geforderte musikalische Eigenständigkeit nicht.
Die Quellen-/Besitzerverwaltung und die produktseitigen Eintrittspunkte müssen
vor einer Katalogumstellung gemeinsam geschlossen werden.

## NOCH NICHT SELBSTVERSTÄNDLICH

- COAST: 6–14 s Halten, 4–10 s zwischen Vorschlägen, höchstens zwei gehaltene
  Rollen, lokale Schritte/gemeinsamer Ton und episodische Ruhe.
- WOODLAND: 2–3 gehörte Töne als Figur, daraus abgeleitete Antwort, Variation
  nur am letzten Grad, dann 6–16 s Ruhe. Natürlicher Pluck-Decay bleibt Aufgabe
  der Quelle; die Grammatik fügt keinen Sustain hinzu.
- HIGHLANDS: 2–4 zusammenhängende Töne, 1,5–5 s Halten und 8–20 s Ruhe.
  Ausschließlich Kandidat; vor Produktaufnahme sind trockener Tonkörper und
  Artikulation aus SD04–SD05 zu hören. Die Abbruchregel bleibt bestehen.
- Activity verändert ausschließlich den Ereignisabstand; jede Welt bleibt
  innerhalb desselben MIDI-50–69-Registers und derselben gewählten
  Major-/Minor-Pentatonik. Alle zwölf Keys sind geprüft.
- Die Score-Zeit ist bewusst von Audiozeit, Quellenbelegung und Tail-Gedächtnis
  getrennt. Diese Integration fehlt noch; eine reine Score-Simulation belegt
  weder hörbare Überlappung noch Musikalität langer PCM-Läufe.
- Episodengrenzen erlauben Ruhe/Neuanordnung, verändern bislang keine eigene
  harmonische Fokusfolge. SD17 ist deshalb noch nicht abgeschlossen.

## LOCKED

Technische Teilverträge: Vorschläge verändern keinen RNG, keine Zeit und kein
gehörtes Motiv. Nur erfolgreicher Commit schreibt diese Zustände. Antworten
beziehen sich auf tatsächlich eingetragene Noten. Timer funktionieren über
uint32-Überlauf. 11.520 deterministische Transaktionen wurden lokal bestanden
und sind jetzt Bestandteil der Host-CI. Dies ist kein perceptual Sound-Lock.

COAST/Input-Checkpoint wurde als PR130 gemerged:
89c352ca9faa361ed3589a1527740dc52504c52c, Baum
ffc302654441f06fc20af467ed6b40089915709b. Geprüfter PR-Head
65a137bede53ddeb312a6c2c9ac34db5bb6bb74f; alle fünf CI-Jobs grün,
Run 37489985682. H743 Release: FLASH 255964 B, DTCM 119440 B,
D1 417344 B, D2 258112 B. CPU/Stack am Silizium sind offen.
Der D1-Unterschied zur Roadmap-Baseline ist −64 B im tatsächlichen Link,
keine behauptete gesamte Einsparung aus Host-Structgrößen.

## REMOVE / MERGE / REDESIGN

Nächste notwendige Einheit: gemeinsame Quellenverwaltung für Manual und
Generate, einschließlich Releases, Originalbesitzer und tatsächlichen
Frequenzen. Erst anschließend diese Score-Vorschläge an die Zulassung binden.

Entscheidung für den zu prüfenden Produktkern: ein gemeinsamer Raum und Dry,
separate optionale Natur, keine Pflicht-Pad/Bass/Drone-Schicht. Detune,
Shimmer, Reverse, Tape, Blur und die archivierten Synths erhalten keinen
Produktplatz ohne eigenständigen belegten Nutzen. Gegenwärtig sind diese
alten Pfade noch erreichbar/gelinkt; die Entscheidung ist noch keine Entfernung.

Katalogwechsel braucht eine neue Scene-Version mit expliziter SCN5/SCN6-
Migration. Keine Wiederverwendung alter World-/FX-IDs. Bedienbare Parameter
und gespeicherte Werte müssen denselben reduzierten Klangvertrag verwenden.
Die spätere physische Tasten-/Displaygestaltung bleibt offen.

## BESTE VERSION

COAST verbindet unabhängige ruhige Hüllkurven; WOODLAND erzeugt erkennbare
Figuren und Antworten; HIGHLANDS verdient seinen Platz durch einen eigenen
warmen Tonbeginn oder entfällt. Derselbe gehaltene Ton besitzt seine Quelle
auch nach World-/Tuning-Wechsel bis zum realen Ende. Eine volle Belegung
weist Vorschläge ehrlich ab. Raum und optionales Wetter ändern keine
Harmonik und erzeugen keine zusätzliche musikalische Rolle.

Lokaler Integrationsentwurf existiert in engine_product.c, ambient_room.c,
nature.c sowie Quellen-Handshake-/Tone-Änderungen. Einzelne Übersetzungseinheiten
kompilierten lokal mit -Werror. Vor der vollständigen PCM-/Regressionsprüfung
fiel die Ausführungsumgebung aus. Diese Dateien wurden ausdrücklich NICHT in
diesen geprüften Commit übernommen oder auf H743 aktiviert.

Vor Wiederaufnahme dieses Entwurfs konkret korrigieren/prüfen:
1. Pluck-Send .50 auf denselben realen .35-Sendvertrag der übrigen Quellen
   umstellen; ein Kommentar allein ist keine Implementierung.
2. Clear darf Source-Color/Damping nicht heimlich auf Initialwerte zurücksetzen.
3. Sehr kurzer Press/Release vor dem ersten Audioblock: tatsächlicher Start,
   Storno, Hook und Ledger müssen dieselbe Bedeutung haben.
4. Audio-/Control-Handshake, Freigabe nach realem Release und globales Budget
   mit deterministisch unterbrochenen Übergängen prüfen.
5. Festen Quellenpegel, Raum-Wet und Naturpegel mit echtem PCM kalibrieren.
6. Scene-Migration, Parametergrenzen und Menü-Erreichbarkeit vor Umschaltung.
7. Vollständige Host-Suite, H743-Link, Bankdeltas und Sound-Hörvergleich.

## TEST AM GERÄT

Erst nach Integration: drei Welten ohne Labels identifizieren, gemeinsame
Töne/Übergänge in Mono beurteilen und langsame ruhige Nutzung prüfen. DWT-Peak
unter 0,60, null Deadline-Misses und gemessene Stackreserve. Reale DAC-/Amp-/
Lautsprecher-/Kopfhörer-Ausgabe und Boot/Mute/Clear/Recall prüfen.
Weder Heilwirkung noch allgemeine Verträglichkeit für neurodivergente Menschen
werden aus diesen Code- oder PCM-Prüfungen abgeleitet.
