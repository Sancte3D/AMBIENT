# AMBIENT — Konzeptfreigabe vor Umsetzung

Verbindliche Arbeitsreihenfolge des Nutzers, 2026-09-26:
**Konzept → Sounddesign → UX/UI/Display → Test am echten Gerät.**

## Was jetzt getan wird

Das Produkt wird derzeit ohne verfügbaren physischen Testaufbau entwickelt.
Die nächste Entscheidung wird daher anhand von Produktlogik, Quellen,
Anwendungsfällen und technischer Plausibilität getroffen. Keine neue Hörprobe
und kein Hardwareversuch sind Voraussetzungen für die Konzeptentscheidung.
Vorhandene Host-Renderings und bestandene Tests ersetzen keinen Gerätetest.
Weitere Timbre-Korrekturen pausieren; bisheriger Code bleibt erhalten.

Konzeptfreigabe bedeutet einen konsistenten, umsetzbaren Entwurfsauftrag.
Sie ist keine externe Zertifizierung und beweist weder Beruhigung noch
Klangqualität, Bedienbarkeit oder Rechenreserve. Diese Annahmen werden für die
späteren Phasen ausdrücklich festgehalten. Ein später widerlegter Grundsatz
muss korrigierbar bleiben.

## Phasen und jeweiliges Ergebnis

| Phase | Arbeit | Abschluss |
|---|---|---|
| 1. Konzept | Zweck, Nutzerhandlung, Produktidentität, Rolle von Natur/Kultur, Klangrollen, Reduktion und technische Grenzen zusammenhängend entscheiden | Kurzer Produktbrief mit begründeter Richtung, Funktionsumfang, Ausschlüssen und offenen Annahmen; gemeinsam freigegeben |
| 2. Sounddesign | Quellen, Kompositionslogik, Harmonie, Bewegung, Raum und Parameterbereiche aus dem Brief ableiten | Zusammenhängende Klangarchitektur; kleine abgeschlossene Implementierungspakete mit passenden Softwareprüfungen; Hörbeispiele höchstens 30 s |
| 3. UX/UI/Display | Bedienung, Zustände, Reglerbelegung, Feedback, Display/Licht und physische Zuordnung aus dem Instrument ableiten | Durchgängiger Bedienablauf und umsetzbare Oberfläche |
| 4. Echtes Gerät | Klang über tatsächliche Ausgänge, Haptik, Verständlichkeit, Langzeitverhalten, DWT/CPU/RAM und Fehlerfälle überprüfen | Empirische Freigabe bzw. gezielte Revision anhand realer Befunde |

In Phase 1 muss die grundsätzliche Nutzerhandlung verständlich sein; konkrete
Screens, Animationen, Reglerkurven und UI-Implementierung gehören in Phase 3.
Bestehende Hardwaregrenzen werden schon im Konzept berücksichtigt, damit kein
Produkt versprochen wird, das nur durch ungeplante Hardwareänderungen entsteht.

## Ausgangspunkt, noch kein endgültiger Produktbrief

Ein eigenständiges physisches Ambient-Instrument ermöglicht musikalische
Gestaltung ohne Leistungsdruck und einen bewusst gestarteten autonomen
Hörmodus. Ruhige Entwicklung, harmonischer Zusammenhang und wenige direkte
Eingriffe sind Zielgrößen. Generate mit gesperrten Spielflächen ist eine bereits
geäußerte Nutzerpräferenz; das Verhalten bleibt Ausgangspunkt, keine offene
Rückfrage ohne neuen sachlichen Grund.

Naturorte und eine mögliche Verbindung mit Kultur/Geschichte werden auf ihre
tragende Rolle geprüft. Kein Bestandsschutz für die aktuellen fünf Worlds,
sechs Synths, Instrumentenzuordnungen oder Effektmenüs. Der kulturelle Ansatz
ist noch nicht beschlossen; keine Umbenennung oder neue Synthese daraus ableiten.

## Nächste Konzeptentscheidungen, in dieser Reihenfolge

1. **Produktzweck:** Welches konkrete Erlebnis trägt ein eigenes physisches
   Instrument? Verhältnis zwischen selbst gestalten und selbstständig zuhören
   beschreiben; aus vorhandenen Nutzerpräferenzen ableiten.
2. **Identität:** Naturwelten als Hauptprinzip, historisch informierte Klangkörper
   als Hauptprinzip oder als interne Inspirationsquelle bewerten. Eine Richtung
   begründet empfehlen; konkurrierende Leitideen nicht einfach addieren.
3. **Klanglogik:** Wenige unterschiedliche Rollen definieren und erklären,
   wodurch sie zusammengehören. Ruhig darf nicht zur Gleichförmigkeit sämtlicher
   Stimmen führen. Historische Belege, gestalterische Ableitungen und eigene
   Kompositionsentscheidungen klar trennen.
4. **Umfang:** Nur Funktionen behalten, die aus dem Zweck folgen. Jede zusätzliche
   World, Engine, Bedienebene und Effektrolle muss einen eigenen Nutzen erklären.
5. **Machbarkeit:** Bestehende MCU-/Speicher-/Control-Grenzen aus tatsächlichen
   Unterlagen berücksichtigen; ungemessene Performance als Annahme führen.

## Freigabekriterien für Phase 1

- Das Produkt lässt sich ohne Featureliste in einem präzisen Satz erklären.
- Die typische Nutzung und das Verhältnis von Spiel- zu Hörmodus sind eindeutig.
- Natur/Kultur/Geschichte haben eine begründete gemeinsame Hierarchie.
- Herkunft beeinflusst im Kulturansatz mindestens eine konkrete Klang- oder
  Interaktionsregel; reine Beschriftung rechtfertigt keinen Konzeptwechsel.
- Jede vorgesehene Klangrolle und Funktion ist notwendig oder klar nützlich;
  vorhandene Implementierung allein ist keine Begründung.
- Unterschiedlichkeit der Stimmen und gemeinsame ruhige Identität sind vereinbar.
- Der Umfang ist für die vorhandene Hardware plausibel begrenzt; fehlende
  Messungen werden nicht als erwiesene Eigenschaften dargestellt.
- Es gibt eine ausdrücklich empfohlene Richtung und nachvollziehbare Gründe,
  warum die übrigen Richtungen nicht verfolgt werden.

Aktueller Stand: **Konzeptfreigabe offen.** Erst diese Arbeit abschließen,
bevor die frühere Choir/Forest- und Guembri/Desert-Queue wieder aufgenommen wird.
