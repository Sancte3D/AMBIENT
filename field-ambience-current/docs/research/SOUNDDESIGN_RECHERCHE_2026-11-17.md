> **Herkunft:** zweite Recherche des Besitzers (ChatGPT Deep Research), am 17.11.2026 ins Repo gelegt. Unverändert.
> Die Marker `citeturn…`/`iturn…` sind kaputte Quellverweise; Quellen, Preise und Produktdaten sind **nicht geprüft**.
> Achtung: Die Recherche beschreibt eine ältere Engine (Glass/Tape/Particle, Chord Cloud – Stand des Ordners
> `OpenAI Snyth Models/`) und eigene acht Landschaften, nicht die V3-Firmware und nicht die 16 Kandidaten.
> Was übernommen wird: `docs/worlds/WORLDS_ENTSCHEIDUNGSPAKET_2026-11-13.md`, Abschnitt „To-do aus der Natur-Recherche“, R10–R17.

# AMBIENT — Forschungsbericht zu Sounddesign, Synthese, Naturklang und generativen Klangsystemen

## Forschungsrahmen und zentrale These

AMBIENT sollte nicht als „Synthesizer mit Naturthema“ entwickelt werden. Die belastbarere Produktthese lautet: **AMBIENT ist ein generatives Musikinstrument, dessen musikalische Zeit, Klangmaterialien und Interaktionslogik teilweise aus Eigenschaften natürlicher Klanglandschaften abgeleitet werden.** Diese Formulierung ist entscheidend, weil die Forschung zwar relativ konsistent zeigt, dass natürliche Klangumgebungen bestimmte restaurative oder stressbezogene Effekte haben können, daraus aber weder folgt, dass jedes Naturgeräusch entspannend ist, noch dass synthetische Ambient-Musik dieselbe Wirkung hat. Meta-Analysen finden positive Effekte natürlicher Klänge auf verschiedene physiologische und subjektive Maße, berichten jedoch zugleich deutliche Heterogenität zwischen Studien, Stimuli, Populationen und Versuchsanordnungen. citeturn790288search0turn790288search1turn790288search7

Für den Bericht werden vier Befundgrade verwendet. **A** bedeutet: durch peer-reviewte Forschung oder technische Primärdokumentation gut belegt. **B** bedeutet: durch mehrere fachlich belastbare Quellen oder dokumentierte Künstler-/Produktionsaussagen gestützt. **C** bezeichnet eine plausible Designableitung aus A/B, die für AMBIENT experimentell überprüft werden muss. **D** bezeichnet subjektive Nutzererfahrungen oder Community-Aussagen; sie sind wertvoll zur Hypothesenbildung, aber keine Evidenz für allgemeine Wirkung.

Ambient selbst ist historisch wesentlich breiter als „lange Synth-Pads“. Brian Enos programmatische Beschreibung von 1978 formuliert Musik, die verschiedene Aufmerksamkeitsgrade zulassen und zugleich Umgebung gestalten soll. Wissenschaftliche Arbeiten zu *Music for Airports* betonen gerade die Spannung zwischen Hintergrundfunktion und aufmerksamem Hören. Reichs Konzept des „gradual process“ liefert ein zweites relevantes Prinzip: Ein musikalischer Prozess kann sehr langsam verlaufen und trotzdem wahrnehmbar und strukturell nachvollziehbar sein. Diese beiden Ideen sind für AMBIENT wichtiger als ein bestimmter Vintage-Synthesizerklang. citeturn789593search12turn789593search1turn980187search4

Daraus ergibt sich eine erste Kernanforderung: Eine World muss **nach etwa 30 Sekunden identifizierbar**, nach zehn Minuten noch kohärent und nach 30–60 Minuten nicht offensichtlich zyklisch sein. Das lässt sich nicht allein durch zufällige LFOs erreichen. Es erfordert mindestens drei unterschiedliche Zeitskalen: Ereignisse im Sekundenbereich, strukturelle Veränderungen über Minuten und sehr langsame Formverläufe über zehn oder mehr Minuten. Algorithmische Kompositionsforschung unterscheidet entsprechend deterministische, regelbasierte, stochastische und lernbasierte Systeme; für ein eingebettetes Instrument wie AMBIENT spricht viel für eine regelbasierte stochastische Architektur, weil sie berechenbar, musikalisch begrenzbar und wesentlich leichter zu validieren ist als ein datengetriebenes generatives Modell. citeturn980187search1turn980187search9turn980187search13

### Was die Forschung bereits klar macht

Die aktuelle Soundengine mit Glass/Tape/Particle, Harmony Field, Chord Cloud, Motion Engine, Diffuser, Mod Delay und Reverb enthält grundsätzlich fast alle notwendigen Bausteine. Das Problem ist weniger „zu wenig DSP“ als **fehlende Hierarchie zwischen Quelle, musikalischem Prozess und Effekt**. Geräte wie Torso S-4, MOOD MKII und Microcosm demonstrieren, wie schnell ein System zwar spektakuläre Texturen produziert, aber auch zu einem Effektgenerator werden kann, wenn die Verarbeitung selbst die musikalische Identität ersetzt. Die AMBIENT-World darf deshalb nicht primär durch „Reverb-Preset + Filter + Sample“ definiert sein. citeturn350337view2turn350337view3turn238790search1

Für AMBIENT sollte die Reihenfolge konzeptionell sein:

**World → musikalische Regeln → Quellen → Modulation → Raum → Naturmaterial.**

Nicht:

**Natur-Sample → großer Reverb → Ambient.**

Diese Unterscheidung zieht sich durch den gesamten Bericht.

## Ambient als musikalisches System

Eine vollständige computergestützte Spektralanalyse der folgenden Aufnahmen wurde im Rahmen dieser Recherche **nicht durchgeführt**. Aussagen über Frequenzspektren, Dynamik oder exakte Effektparameter werden deshalb nicht als Messdaten dargestellt. Wo Produktionsmethoden dokumentiert sind, werden sie als Fakten behandelt; Harmonik oder klangliche Funktion werden nur dort konkret benannt, wo wissenschaftliche Analyse oder Künstlerquellen dies stützen. Alles Weitere ist eine Designinterpretation für AMBIENT.

### Referenzkorpus

| Referenz | Dokumentierter bzw. belastbarer Kern | Relevanz für AMBIENT | Befund |
|---|---|---|---|
| Brian Eno — *Ambient 1: Music for Airports* | Unterschiedlich lange Bandschleifen überlagern sich; bei „2/1“ sind mehrere Looplängen dokumentiert. Szabo beschreibt u. a. F-Aeolisch geprägtes Material und lange, unregelmäßige Leerstellen. citeturn789593search6turn789593search7 | Asynchrone Wiederholung statt Zufall. Wenige Elemente können lange Rekombination erzeugen. | A/B |
| Brian Eno — *Discreet Music* | Frühe prozessbasierte Tape-Arbeit und wichtiger Vorläufer seiner späteren generativen Systeme. Eno beschreibt generative Musik als ein System, dessen Verhalten nach dem Einrichten teilweise autonom entsteht. citeturn980187search6turn980187search7 | „System statt Komposition“ als zentrale Architektur. | B |
| Biosphere — *Substrata* | Beatarmes Ambient mit starkem Landschaftsbezug; Jenssen beschreibt Field Recordings als Mittel, eine ansonsten im Studio erzeugte Atmosphäre deutlich zu verändern. citeturn789593search8turn789593search9 | Naturmaterial muss nicht dominant sein; leise reale Textur kann Kontext erzeugen. | B |
| Loscil — *Submers* | Scott Morgan beschreibt Looping als zentralen Arbeitsprozess und das langfristige Hören eines Loops ohne Ermüdung als eigenes Qualitätskriterium; er verwendet häufig Samples, um die von ihm empfundene Kälte synthetischer Klänge zu brechen. citeturn789593search10turn789593search11 | Sehr relevant für „10-Minuten-Test“ und hybride akustisch-elektronische Quellen. | B |
| Stars of the Lid — *And Their Refinement of the Decline* | Interviews dokumentieren u. a. Piano-Loops, Gitarrendrones, Streicher, Sampler und extrem langsame Entwicklung. citeturn976531search0turn976531search1 | Klangfarbe und harmonisches Voice-Leading können wichtiger sein als Ereignisdichte. | B |
| William Basinski — *The Disintegration Loops* | Physischer Zerfall alter Bandschleifen veränderte das wiederholte Material schrittweise. citeturn976531search4turn976531search5 | Extrem starke Referenz für **gerichtete Veränderung**: Wiederholung + irreversible Drift statt Random-LFO. | B |
| Tim Hecker — *Ravedeath, 1972* | Ausgangsmaterial wurde auf Kirchenorgel aufgenommen, anschließend elektronisch geschichtet, montiert und transformiert. Hecker beschreibt das Album ausdrücklich als Hybrid aus Studio- und Live-Prozess. citeturn976531search2turn976531search3 | Zeigt, dass aggressive oder dichte Klangmassen trotzdem Ambient funktionieren können; AMBIENT muss nicht permanent „weich“ sein. | B |
| GAS — *Pop* / GAS-Projekt | Wolfgang Voigt beschreibt ausdrücklich die Überlagerung von Club und Wald sowie das Verlorengehen in einem imaginären Wald. citeturn976531search6turn976531search7 | Naturbezug kann konzeptionell sein, ohne realistische Waldaufnahme zu imitieren. | B |
| Hiroshi Yoshimura — *Green* | Yoshimura verstand „GREEN“ als natürliche zyklische Szenerie; das Album entstand mit digitalen Yamaha-FM-Synthesizern und zeigt damit, dass Naturassoziation nicht von organischer Klangquelle abhängt. citeturn976531search10 | Wichtig gegen die Annahme „Natur = Field Recording“. | B |
| Fennesz — *Venice* | Fennesz dokumentiert Gitarrenaufnahmen, Softsynths, Sampler und Field Recordings; digitale Bearbeitung transformiert die Gitarre teils stark. citeturn163075search2turn163075search3 | Gute Referenz für das Verwischen der Grenze zwischen erkennbarer Quelle und Textur. | B |
| Harold Budd / Brian Eno — *The Plateaux of Mirror* | Teil von Enos ursprünglicher Ambient-Reihe und historisch wichtig für die Verbindung von akustischem Instrument und Umgebungsklang. citeturn789593search12 | Pianistische Ereignisse in einem größeren Raum statt permanenter Drone. | B |
| Laraaji — *Ambient 3: Day of Radiance* | Ebenfalls Teil der frühen Ambient-Reihe; zeigt, dass Ambient nicht zwingend homogen, langsam oder dunkel sein muss. citeturn789593search12 | Warnung vor zu enger Definition von „ambient = washed pad“. | B |
| Steve Reich — *It’s Gonna Rain / Come Out* | Prozessmusik mit graduell hörbarer Phasenverschiebung; Reich formuliert ausdrücklich, dass der Prozess sowohl Detail als auch Gesamtform bestimmen kann. citeturn980187search15 | Ideales Modell für langsame, nachvollziehbare Parameterentwicklung. | A/B |
| Alvin Lucier — *I Am Sitting in a Room* | Rekursive Aufnahme/Abspielprozesse lassen die Raumresonanzen zunehmend das Sprachmaterial dominieren; in der Forschung häufig als autonom verlaufender Prozess eingeordnet. citeturn789593search5 | Resonanz als Strukturgeber, nicht nur Effekt. | B |
| Fennesz — *Endless Summer* | Fennesz’ Biografie beschreibt die Verschmelzung von Gitarrenmaterial und Computerprozessierung sowie eine stärkere melodische Ebene innerhalb komplexer Texturen. citeturn163075search5 | Textur darf Harmonie nicht vernichten. | B |
| Tim Hecker — *Virgins* / Hecker-Werk allgemein | Hecker zeigt, dass Geräusch, Übersteuerung und instabile Texturen musikalisch produktiv sein können; für AMBIENT ist das eher Grenzreferenz als Ziel. Seine dokumentierte Organ-/Elektronikpraxis zeigt bewusst hybride Klangquellen. citeturn976531search2 | „Schmutz“ dosieren, nicht vollständig beseitigen. | C |
| Pauline Oliveros — Deep-Listening-Tradition | Die relevante Idee ist weniger eine bestimmte Synthese als das aktive Wahrnehmen von Raum, Dauer und Umgebung. | Für AMBIENT eher HCI- und Listening-Referenz als Preset-Vorbild. | C |
| Moderne modulare Ambient-Praxis | Künstler wie Kaitlyn Aurelia Smith betonen haptisches Patchen und das Instrument als performative Erweiterung, nicht nur als Presetmaschine. citeturn976531search9 | Rechtfertigt physische Hardware, wenn Eingriffe musikalisch unmittelbar und körperlich verständlich bleiben. | B |

### Was sich musikalisch daraus ableiten lässt

Der stärkste gemeinsame Nenner ist **nicht Reverb**, sondern kontrollierte Zeit. Eno nutzt nicht deckungsgleiche Wiederholungsperioden, Basinski eine irreversible Materialveränderung, Reich graduelle Prozesse, Loscil Looping mit kontinuierlicher Bearbeitung und Stars of the Lid extrem langsame harmonische Entwicklung. Daraus folgt für AMBIENT: Die Motion Engine sollte nicht primär mehrere unabhängige LFOs erzeugen, sondern **Prozesse mit charakteristischer zeitlicher Richtung und Gedächtnis**. citeturn789593search6turn976531search4turn980187search15turn789593search10

Eine zweite Gemeinsamkeit ist **begrenztes Material**. Viele der überzeugendsten Referenzen gewinnen ihre Langzeitwirkung nicht aus ständig neuen Noten, sondern daraus, dass dieselben wenigen Quellen in wechselnden Beziehungen erscheinen. Für ein eingebettetes Instrument ist das ideal: AMBIENT braucht keinen komplexen Kompositionsalgorithmus, sondern wenige sehr gute Quellen, gutes Voice-Leading und mehrere voneinander unabhängige Zeitkonstanten. Das ist eine Designableitung, die empirisch getestet werden muss. **Befundgrad C.**

Eine dritte Konsequenz betrifft Harmonie. Vollkommen zufällige Akkordwahl wäre für dein Ziel kontraproduktiv. Ein besseres System hält ein kleines Pitch-Class-Set stabil und verändert hauptsächlich **Voicing, Register, Inversion, Auslassungen und einzelne Töne**. Ein Beispiel wäre ein Harmony Field aus fünf bis sieben zugelassenen Tonhöhenklassen, von denen pro Event nur zwei bis vier aktiv sind. Ein Akkordwechsel sollte vorzugsweise den gemeinsamen Ton behalten und nur eine oder zwei Stimmen bewegen. Das ist keine universelle Ambient-Regel, sondern ein robustes Designprinzip zur Reduktion harmonischer Sprünge. **Befundgrad C.**

### Konsequenz für die vorhandenen Klangprobleme

Der „hornartige“ Charakter ist sehr wahrscheinlich nicht primär ein Reverb-Problem. Typische technische Ursachen wären eine zu statische periodische Quelle, stark ausgeprägte mittlere Harmonische, resonante Filterspitzen, mehrere nahezu identisch modulierte Stimmen oder ein zu enger Formantbereich. Die Lösung sollte deshalb vor dem Reverb erfolgen: spektral weniger starre Oszillatoren, leichte unabhängige Drift, subtil unterschiedlich gefilterte Layer, sanftere Attack-Phasen sowie probabilistische oder langsame Variation von Timbre und Voicing. Das muss an der konkreten Engine gemessen werden; ohne Spektralanalyse des aktuellen Outputs bleibt dies eine technische Hypothese. **Befundgrad C.**

Abruptes Ausklingen und Klicks haben dagegen meist klar technische Ursachen: diskontinuierliche Amplitudenänderungen, harte Buffer-Wechsel, ungesmoothete Parameter, abrupt terminierende Granularfenster oder Delay-/Freeze-Zustände ohne Crossfade. Hier sollte es eine systemweite Regel geben: **kein Audiozustand darf hart umgeschaltet werden**. Parameter, Voice-Stealing, Loop-Grenzen, Sample-Wechsel, Reverb-Freeze und World-Wechsel benötigen Rampen oder Crossfades. **Befundgrad C**, technisch unmittelbar prüfbar.

Aliasing ist vor allem bei nicht bandbegrenzten Oszillatoren, Waveshaping, harter FM/PM und Pitch-Shifting kritisch. Daraus folgt nicht, dass alles oversampled werden muss. Auf STM32H743 sollte Oversampling gezielt auf nichtlineare oder aliasingkritische Blöcke begrenzt werden; einfache Drones und gefiltertes Rauschen benötigen es nicht zwingend.

## Wettbewerbsprodukte und die eigentliche Marktlücke

iturn989032image0turn989032image2turn989032image1turn989032image3

Die wichtigste Beobachtung aus dem Benchmark ist, dass praktisch alle relevanten Geräte **einen Ausschnitt** von AMBIENT bereits sehr gut lösen: LIVEN Ambient Ø bietet eine sofortige Ambient-Architektur, COSMOS langfristige rekombinierende Erinnerung, S-4 tiefes Audio-Sculpting, MOOD MKII musikalische Mikro-Loops und Zufallsfunde, NightSky Reverb als spielbaren Syntheseprozess, Lemondrop Granularsynthese, Lyra-8 organisch-instabiles Verhalten und norns programmierbare Generativität. Die Differenzierung kann deshalb nicht „Ambient-Hardware“ oder „generative Sounds“ lauten. citeturn350337view0turn350337view1turn350337view2turn350337view3turn350337view4turn804588search5

### Vergleichsmatrix

| Produkt | Verifizierte Architektur | Stärke | Schwäche/Lücke aus AMBIENT-Sicht |
|---|---|---|---|
| **SONICWARE LIVEN Ambient Ø** | Vier Layer: Drone, Pad, Atmos, Noise; Blendwave-Synthese mit sechs Strukturen/32 Waves; Noise-Layer mit bis zu 8 s Stereo-Samples; eigene Hüllkurve, Filter und zwei LFOs je Layer; neun Reverbs und sechs Master-FX; 64-Step-Sequencer. Aktuell $299. citeturn350337view0 | Sehr schnelle Ambient-Ergebnisse; klare Schichtung; Natur-/Noise-Material bereits integriert. | Starke Konkurrenz zur Idee „einfaches Ambient-Gerät“. Differenzierung muss tiefer als vier Layers + Reverb gehen. Nutzerberichte loben schnelle Resultate, kritisieren teils Kunststoffanmutung bzw. begrenzte Vielseitigkeit; das ist anekdotisch. citeturn458623reddit11turn458623reddit13 |
| **SOMA COSMOS** | „Drifting Memory Station“ mit 44,7 s Sound Memory; basiert konzeptionell auf rekombinierenden Delay-/Memory-Strukturen statt gewöhnlichem Phrase-Looper. Netto 650 € laut Hersteller. citeturn804588search1turn848990search1 | Extrem relevante Referenz für nicht offensichtliche Wiederholung und „musical memory“. | Kein vollständiger Klanggenerator; bewusst abstrakte Bedienung. Ein Erfahrungsbericht beschreibt genau diese Symbol-/Modusabstraktion als mental belastend. citeturn458623search8 |
| **Torso S-4** | Vier Stereo-Tracks; pro Track Material → Granular → 48-Band resonanter Filter → Color → Space; vier Modulatoren/Track; 24-bit/48 kHz; 4 GB Speicher; 9 Encoder, 3,5″ Display. citeturn350337view2 | Sehr starke Transformation vorhandenen Materials; Granular + Filterbank + Modulation hervorragend für evolving textures. | Hohe Funktionsdichte. Sound On Sound lobte Interface, Tape-/Granular-/Resonator-Seite, dokumentierte aber zum damaligen Review-Zeitpunkt unfertige Funktionen und Bugs; Firmware hat sich danach weiterentwickelt. citeturn458623search7turn238790search15 |
| **Chase Bliss MOOD MKII** | Zwei interagierende Kanäle: immer mithörender Micro-Looper plus Echtzeit-Raum-/Effektkanal; Stereo, Freeze, Overdub, Clock, internes Modulationssystem. $399. citeturn350337view3 | Aus extrem wenig Eingangsmaterial entstehen musikalische „happy accidents“. Clock verändert gleichzeitig Sample-Rate, Länge und Tonhöhe in musikalisch gekoppelter Weise. | Erzeugt spektakuläre Ergebnisse, verlangt aber Lernen. Ein aktueller Nutzer beschreibt es eher als zusätzliches Instrument als als normalen Effekt; das ist für AMBIENT eine wichtige, aber anekdotische HCI-Beobachtung. citeturn187289reddit70 |
| **Strymon NightSky** | Stereo-Reverb als Sounddesign-Instrument; variable Processing Rate/Core Size, Pitch/Harmonics, resonantes Filter, Modulation, 8-Step-Reverb-Pitch-Sequencer, Infinite/Freeze; 24-bit/96 kHz; Herstellerpreis $429. citeturn804588search4turn848990search3 | Beweist, dass Reverb selbst eine spielbare Stimme werden kann. | Gefahr für AMBIENT: Shimmer, Freeze und riesige Tails werden schnell zum stilistischen Klischee. |
| **monome norns** | Linux-basierter programmierbarer Soundcomputer; Lua-Steuerung, SuperCollider-Engines, sample-cutter, granulator, drone, asynchronous looper usw.; 3 Encoder und 3 Keys; 1,2-GHz-Quadcore, 1 GB RAM bei aktuellen klassischen norns-Generationen. citeturn804588search5 | Extrem starke Referenz für „wenige Controls, wechselnde Instrumentidentität“. | Offene Programmierplattform statt fokussiertem Consumer-Instrument; höhere kognitive Einstiegshürde. |
| **SOMA LYRA-8** | Acht nichttraditionelle Voice-Generatoren, FM-artige Kreuzmodulation, Hyper-LFO, zwei kreuzgekoppelte modulierte Delaylines und Distortion. citeturn458623search9turn350337view6 | Organische Instabilität und emergentes Verhalten ohne Sequencer. | Schwer reproduzierbar und teils bewusst rau/aggressiv; nur begrenzt passend für „musikalisch sichere“ Bedienung. |
| **1010music Lemondrop** | Vierstimmig; zwei Granulatoren + ein Oszillator; 16 Grains/Oszillator, 128 gesamt; Filter, Envelopes, LFOs, Mod-Sequencer; 48 kHz, 32-bit intern, microSD; $399. citeturn350337view7 | Sehr kompakte Granularengine mit echtem Samplematerial. | Touchscreen und tiefe Patchparameter widersprechen teilweise AMBIENTs Ziel geringer musikalischer Vorbildung. |
| **Hologram Microcosm** | Micro-Loops, Granules, Glitch, Multi-Delay, Pitch, Hold/Freeze, 60-s-Phrase-Looper und Stereo-Reverb; 24-bit/48 kHz; aktuelle Herstellerseite listet ungefähr $459, wobei Preise zeitlich variieren. citeturn238790search0turn238790search1 | Sehr schnelle Transformation einfacher Eingaben zu komplexen Texturen. | Deutlich „effect-first“. Gerade das sollte AMBIENT vermeiden: die World sollte trotz Effektarchitektur eine eigene musikalische Grammatik besitzen. |

### Was diese Produkte für AMBIENT tatsächlich lehren

**LIVEN Ambient Ø ist der direkteste Wettbewerber.** Wenn AMBIENT lediglich Drone, Pad, Noise, Nature Sample, Reverb und ein paar Makros bietet, ist die Produktidee zu wenig differenziert. Sonicware deckt dieses Feld bereits sehr konkret und für $299 ab. citeturn350337view0

**COSMOS liefert die stärkste funktionale Inspiration.** Nicht das konkrete Delay-System sollte kopiert werden, sondern die Idee, dass vergangenes Material wieder auftaucht, ohne als konventioneller Loop identifizierbar zu sein. Das eröffnet eine echte AMBIENT-Signatur: Jede Cell könnte ein kleines Langzeitgedächtnis besitzen und frühere Ereignisse probabilistisch, leiser, anders gefiltert oder transponiert zurückbringen. citeturn804588search1

**MOOD zeigt, wie stark gekoppelte Parameter funktionieren können.** Ein einzelner Regler kann sinnvoll gleichzeitig Zeit, Pitch, Samplequalität und Effektcharakter verändern, solange diese Kopplung musikalisch nachvollziehbar bleibt. Für AMBIENT bedeutet das: Encoder sollten nicht einfach technische Einzelparameter darstellen. Ein „Distance“-Encoder könnte beispielsweise simultan Direct-Level senken, High-Frequency-Damping erhöhen, Early/Late-Ratio verschieben und Reverb-Level leicht erhöhen. Das ist eine Designableitung, keine Eigenschaft des MOOD. citeturn350337view3

**S-4 zeigt das Gegenrisiko:** enorme Klangtiefe, aber sehr viele Möglichkeiten. Für ein spezialisiertes selbstfinanziertes Bachelorprodukt ist ein tiefes Modulationssystem intern sinnvoll, aber nicht als sichtbare Oberfläche. AMBIENT sollte intern S-4-artige Komplexität besitzen, äußerlich aber wesentlich stärker kuratiert sein. citeturn350337view2turn458623search7

Damit entsteht eine plausible Marktlücke: **ein Instrument mit der sofortigen Zugänglichkeit des LIVEN, dem musikalischen Gedächtnis von COSMOS/MOOD, der klanglichen Tiefe von Granular-/Resonator-Systemen, aber einer stark kuratierten, naturbezogenen generativen Grammatik statt klassischem Sequencing oder Effekt-Preset-Design.** Diese Positionierung ist eine strategische Interpretation. **Befundgrad C.**

## DSP-Architektur für STM32H743

Der STM32H743 besitzt einen Cortex-M7 mit bis zu 480 MHz, Double-Precision-FPU, DSP-Instruktionen, 16 KB I- und 16 KB D-Cache, bis zu 2 MB internes Flash und bis zu 1 MB SRAM, einschließlich 128 KB DTCM und 64 KB ITCM; außerdem stehen externe Speicherinterfaces einschließlich Quad-SPI und FMC zur Verfügung. Das ist für einen anspruchsvollen Echtzeit-Synthesizer leistungsfähig, aber nicht annähernd vergleichbar mit der Speicher- und CPU-Situation eines S-4 oder norns. citeturn210518search7turn210518search6

### Speicher ist das eigentliche Limit

Bei 48 kHz benötigen 60 Sekunden unkomprimiertes Mono-Audio ungefähr:

`48.000 × 60 × 2 Byte = 5,76 MB` bei 16 Bit.

Stereo sind bereits **11,52 MB pro Minute**. Bei 32-bit-Float wären es 23,04 MB/min Stereo. Diese Werte ergeben sich direkt aus Samplingrate × Wortbreite × Kanälen; verglichen mit maximal 2 MB internem Flash und 1 MB SRAM ist klar, dass umfangreiche Field-Recording-Banken **externen Speicher benötigen**. citeturn210518search7

Daraus folgt eine klare Architekturentscheidung: **interner Flash für Code, Presets und kurze Impuls-/Wavetable-Daten; externer QSPI-Flash oder microSD für Field Recordings und längere Texturen.** Streaming von microSD erfordert Buffering und Fehlerbehandlung; QSPI ist für kurze, häufig gelesene Sampledaten deterministischer. Welche Variante bereits auf der AMBIENT-Hardware vorhanden ist, muss hardwareseitig geprüft werden.

### Empfohlene Signalarchitektur

```text
                 HARMONY FIELD
                      │
        ┌─────────────┼─────────────┐
        │             │             │
      GLASS          TAPE        PARTICLE
        │             │             │
        └──────┬──────┴──────┬──────┘
               │             │
          VOICE MIX      NATURE BED
               │             │
               └──────┬──────┘
                      │
              SPECTRAL SAFETY
        HPF → soft EQ → resonance guard
                      │
                 DIFFUSER
                      │
                 MOD DELAY
                      │
              MEMORY / GHOST
                      │
                   REVERB
                      │
           stereo width / limiter
                      │
                    OUT
```

Die zentrale Änderung gegenüber einer üblichen Ambient-Kette ist **Spectral Safety vor dem großen Raum**. Reverb vergrößert bereits vorhandene Probleme. Ein hornartiger Peak, Low-Mid-Matsch oder scharfe 4–8-kHz-Struktur wird im langen Reverb nicht korrigiert, sondern über Sekunden akkumuliert.

### Reverb

Schroeders klassische Arbeit etabliert kombinierte Delay-/Allpass-Strukturen als Grundlage synthetischer Reverberation. Dattorros späterer Ansatz ist für musikalische algorithmische Reverbs besonders relevant, weil Diffusion, Tankstruktur, Damping und Modulation getrennt kontrollierbar werden. citeturn456314search1turn456314search0

Für AMBIENT ist **ein gut abgestimmter algorithmischer Stereo-Reverb** sinnvoller als mehrere mittelmäßige Algorithmen. Priorität:

Input Diffusion → modulierter Reverb-Tank → frequenzabhängige Dämpfung → Stereo-Decorrelation.

Das Reverb sollte intern mindestens diese Designparameter besitzen: size/decay, damping, diffusion, modulation depth/rate, pre-delay und wet. Der Nutzer benötigt davon nicht alle.

**Signature-Mapping:** Encoder „Space“ verändert simultan Decay, Diffusion und Wet, aber mit unterschiedlichen Kurven. Ein eigener „Distance“-Parameter beeinflusst eher Pre-delay, Early/Late-Balance und Damping.

### Delay und musikalisches Gedächtnis

Normale Stereo-Delays sind billig. Lange Float-Delaylines sind speicherintensiv. Vier Sekunden Stereo-Float benötigen bereits rund 1,536 MB, also mehr als der praktisch frei verfügbare interne SRAM. Deshalb sollten längere Memory-Funktionen entweder 16-bit, mono/dual-mono, downsampled oder extern gespeichert werden.

Für AMBIENT ist eine effiziente Lösung ein **Multi-Tap-Memory-Pool** statt fünf unabhängiger langer Delays. Eine einzige ringförmige Bufferstruktur kann verschiedene Cells mit unterschiedlich alten Reads versorgen. Taps erhalten eigene Gain-, Filter-, Pan- und leichte Pitch-Variation.

### Granular / Particle

Roads beschreibt Microsound als Organisation sehr kurzer Klangpartikel, aus deren Dichte sich Pulse, Linien und Texturen bilden können. Für AMBIENT ist Granularsynthese sinnvoll, aber nicht als 128-Grain-S-4-/Lemondrop-Ansatz. citeturn456314search2turn350337view7

Praktisch sinnvoll wäre zunächst **8–16 gleichzeitige Grains insgesamt**, abhängig von Interpolation und Windowing. Kritische Parameter sind:

grain size: ungefähr 20–400 ms  
density: stark worldabhängig  
position jitter  
pitch: meist sehr klein, selten große Transpositionen  
pan spread  
window shape  
reverse probability.

Wichtiger als Grainzahl ist **gute Fensterung und Interpolation**. Ungeschnittene Rechteckfenster erzeugen Klicks; aggressive Pitchtransposition und schlechte Resampling-Interpolation erzeugen schnell den „Toy“-Charakter.

### Resonator / Modal

Resonatoren sind für AMBIENT besonders interessant, weil sie eine Brücke zwischen Naturgeräusch und Musik ermöglichen: Rauschen, Tropfen, Impulse oder kurze Field-Recording-Fragmente können modale Strukturen anregen. DaisySP enthält unter anderem Resonator- und Modal-Synthese-Komponenten und steht unter MIT-Lizenz. Mutable Instruments’ STM32-Projekte, darunter Rings/Elements-bezogene Codebasen, stehen laut Repository unter MIT-Lizenz; einzelne Projekte und Dateien müssen trotzdem jeweils geprüft werden. citeturn210518search0turn210518search1

Ein Resonator sollte nicht permanent auf voller Resonanz laufen. Genau dort entstehen harte, nasale Peaks. Empfehlenswert ist eine kleine modale Bank mit kontrollierter Q-Begrenzung, dynamischer Excitation und langsamer Variation der Modalfrequenzen.

### Chorus, Ensemble und Modulation

Für Pads ist subtil moduliertes Delay meist effizienter und musikalisch sinnvoller als zusätzliche Oszillatorstimmen. Zwei oder drei kurze, langsam modulierte Delay-Taps können Stereo- und Tonhöhenbewegung erzeugen. Entscheidend ist, dass die LFOs **nicht phasengleich** laufen.

Die Motion Engine sollte deshalb zwischen mindestens drei Arten unterscheiden:

**periodisch** — selten und sehr langsam;  
**bounded stochastic** — Wert bewegt sich zufällig innerhalb eines erlaubten Bereichs;  
**directed drift** — Parameter besitzt über Minuten eine Richtung und kehrt später langsam um.

Die dritte Form ist für lange Sessions besonders relevant. Ein einfacher Random-LFO klingt nach einiger Zeit oft nach Modulation; ein gerichteter Prozess klingt eher nach Entwicklung.

### Spectral Freeze und Shimmer

Freeze ist musikalisch attraktiv, aber teuer, wenn FFT-basiert. Ein Reverb-Tank-Freeze oder Loop-Freeze ist erheblich günstiger. Für die erste AMBIENT-Version würde ich **kein vollwertiges FFT-Spectral-Freeze priorisieren**.

Shimmer ist ebenfalls kein Kernbedarf. Pitch-shifted Reverb ist stark mit zeitgenössischem Ambient verbunden und wird von Geräten wie NightSky prominent eingesetzt. Genau deshalb besteht Differenzierungsrisiko. citeturn848990search3

Wenn Shimmer vorhanden ist, sollte es eher als seltene „Air“-Komponente mit niedrigem Feedback arbeiten als als permanent hörbare Oktave.

### Syntheseverfahren nach Priorität

| Verfahren | Nutzen für AMBIENT | Embedded-Kosten | Priorität |
|---|---|---:|---|
| bandbegrenzte Subtraktiv-/Wavetable-Voices | sehr gut kontrollierbar, günstig | niedrig–mittel | **sehr hoch** |
| Sample/Granular | Naturintegration, organische Textur | RAM/Flash hoch, CPU mittel | **sehr hoch** |
| Resonator/Modal | Natur → Tonalität, Signature-Potenzial | mittel | **hoch** |
| FM/PM | Glass, metallische und klare Timbres | niedrig–mittel, aber aliasingkritisch | hoch |
| Colored Noise | Wind/Wasser/Textur/Exciter | sehr niedrig | **sehr hoch** |
| Chorus/Mod Delay | Bewegung und Stereo | niedrig | **sehr hoch** |
| FDN/Dattorro-Reverb | räumliche Identität | mittel | **sehr hoch** |
| FFT-Spectral Processing | spektakulär, aber komplex | hoch | niedrig für MVP |
| Additive Synthese | kontrolliertes Spektrum | mittel–hoch bei vielen Partials | mittel |
| umfangreiches Physical Modeling | spannend, aber Entwicklungsrisiko | variabel | später |

### Bibliotheken und Lizenzlage

**CMSIS-DSP** ist für Cortex-M optimiert und enthält Filter-, Transformations-, Mathe-, Interpolations- und Statistikfunktionen. Das aktuelle Repository steht unter Apache-2.0. Es ist für FFT, Filter, Vektoroperationen und Grundlagen-DSP die naheliegendste Basisschicht. citeturn456314search3turn210518search2

**DaisySP** steht unter MIT und enthält bereits Oszillatoren, Envelopes, Noise, Filter, Granular Player, Resonatoren, Physical Modeling und Effekte. Die Bibliothek ist jedoch nicht automatisch „drop-in optimal“ für jede STM32H743-Konfiguration; CPU, Speicherzugriffe und Audio-Blockgröße müssen auf deiner Firmware gemessen werden. citeturn210518search0

**Mutable Instruments STM32-Code** ist laut offiziellem Repository MIT-lizenziert, während AVR-Projekte GPLv3 sind. Das muss file-by-file überprüft werden, bevor Code übernommen wird. citeturn210518search1

**Faust** kann DSP nach C/C++ übersetzen. Die Compiler-Lizenz überträgt sich laut Faust-Dokumentation nicht automatisch auf generierten Code; entscheidend sind die Lizenzen verwendeter Libraries und Architekturdateien. Die Standardlibraries enthalten eine LGPL-Ausnahme, die proprietäre resultierende Programme erlaubt. citeturn132662search4turn132662search3

Damit ist keine der genannten Bibliotheken pauschal „kompatibel mit deinem Produkt“. Die korrekte Aussage lautet: **Sie sind technisch und lizenzseitig prüfenswerte Kandidaten.**

## Natur als akustisches und generatives Material

Soundscape Ecology trennt Klangquellen einer Landschaft sinnvoll in **Geophonie** – Wind, Wasser, Eis, Niederschlag; **Biophonie** – Tiere und andere biologische Klangquellen; und **Anthrophonie** – menschlich erzeugte Klänge. Diese Taxonomie ist für AMBIENT wesentlich besser als eine rein visuelle Landschaftskategorisierung. citeturn790288search4

Das bedeutet: Ein „Forest“-Preset ist nicht deshalb Wald, weil auf dem Display Bäume erscheinen. Es müsste eine charakteristische Kombination aus geophonischer Grundschicht, biologischen Ereignissen, räumlicher Dichte und musikalischer Übersetzung besitzen.

### Vier Integrationsgrade

**Direkte Aufnahme.** Ein reales Field Recording bleibt erkennbar. Das schafft den klarsten Naturbezug, birgt aber die Gefahr, nach einer „Nature Sounds App“ zu klingen.

**Synthetische Approximation.** Wasser, Wind oder Rauschen werden durch Noise, Filter, Modulation und Transienten abstrahiert. Das spart Speicher und kann musikalischer sein, verliert aber dokumentarische Glaubwürdigkeit.

**Transformation.** Das reale Recording bleibt Quellmaterial, wird jedoch granular, resonant oder spektral so verändert, dass Natur und Instrument verschmelzen. Fennesz und Biosphere zeigen unterschiedliche künstlerische Varianten eines solchen hybriden Denkens. citeturn789593search9turn163075search2

**Natur als Steuerprozess.** Dies ist konzeptionell die stärkste Variante. Nicht der Klang des Meeres wird abgespielt; Eigenschaften wie zyklische Wellen, unregelmäßige Windböen oder episodische Vogelaktivität beeinflussen Ereignisdichte, Modulation oder Dynamik. Die konkrete mathematische Übersetzung wäre künstlerisch und darf nicht als objektive Simulation der Landschaft verkauft werden.

### Reduktion der Worlds

Acht Landschaften sind für die erste Validierung zu viel. OCEAN, FOREST, DESERT und HIGHLANDS erzeugen vier deutlich unterschiedliche akustische Grundlogiken. Lake überlappt mit Ocean hinsichtlich Wasser; Grassland mit Highlands/Forest hinsichtlich Wind und Vegetation; Tundra und Glacier sind visuell stark, akustisch aber schwieriger ohne stereotype „icy shimmer“-Kodierung klar zu differenzieren.

#### OCEAN

**Objektiver Naturbezug:** Brandung, Wellen, Wind und ggf. biologische Signale. Wassergeräusche sind in Studien häufig Bestandteil natürlicher Klangstimuli; ihre restaurative Wirkung ist jedoch nicht universell und hängt von Kontext und Person ab. citeturn790288search5turn790288search6

**Musikalische Übersetzung:** langsame zyklische Amplitudenbewegung mit nicht exakt regelmäßiger Periode; breitbandiges Noise mit ständig leicht wechselndem Filter; sehr langsame registerbezogene Drift; wenige harmonische Ereignisse.

**DSP:** filtered noise → slow envelope population → low resonator mix → diffuser → long dark reverb.

**Nicht tun:** hörbares „whoosh—whoosh“ alle acht Sekunden. Sobald der Prozess offensichtlich periodisch wird, wirkt er künstlich.

#### FOREST

**Objektiver Naturbezug:** stärkere Biophonie, Wind in Vegetation, lokale Impulse und unterschiedliche Entfernungen. Soundscape Ecology behandelt gerade die Zusammensetzung biologischer und geophysischer Quellen als Landschaftseigenschaft. citeturn790288search4

**Musikalische Übersetzung:** hohe Ereignisvielfalt bei geringer Einzeleventdichte; viele kurze, leise Ereignisse; größere Stereo-/Distanzvariation; weniger kontinuierliche tiefe Drone als Ocean.

**DSP:** leise Field-Grains + resonator excitations + scattered delay taps + mittelgroßer diffuser Raum.

**Nicht tun:** permanentes Birdsong-Sample. Das wird schnell dekorativ und semantisch zu konkret.

#### DESERT

**Objektiver Naturbezug:** geringe Biophonie ist möglich, aber „Desert = Stille“ wäre zu pauschal; reale Wüsten besitzen Wind, Tiere und stark zeitabhängige Aktivität.

**Musikalische Übersetzung:** viel Negativraum, seltene Ereignisse, lange Decays, hohe Dynamik zwischen Stille und Einzelereignis.

**DSP:** very sparse particle events → short modal resonator → filtered delay → long but low-density reverb.

Hier ist **Silence Density** selbst der zentrale World-Parameter.

#### HIGHLANDS

**Objektiver Naturbezug:** Wind, offene Räume, mögliche Wasserläufe, entfernte Tiere; akustisch keine universelle einzelne Signatur.

**Musikalische Übersetzung:** größere wahrgenommene räumliche Tiefe, langsame horizontale Bewegungen, tiefere harmonische Fundamente und gelegentlich deutliche, aber entfernte Ereignisse.

**DSP:** broad tonal bed → decorrelated delay → distance filtering → long diffuse reverb.

Der Begriff „Highlands“ funktioniert nur, wenn die visuelle und akustische Darstellung nicht behauptet, eine spezifische Kultur oder Region abzubilden.

### Wann Natur glaubwürdig wirkt

Naturintegration ist glaubwürdig, wenn mindestens zwei der folgenden Ebenen übereinstimmen:

1. reales Naturmaterial,
2. zeitliches Verhalten,
3. räumliches Verhalten,
4. musikalische Übersetzung,
5. Name/Bild.

Wenn nur Punkt 5 existiert, ist Natur Dekoration.

Wenn alle fünf gleichzeitig extrem offensichtlich sind, besteht hingegen die Gefahr von Kitsch: „Ocean“ + Wellenfoto + Meeresrauschen + wellenförmiger LFO + blauer UI. AMBIENT sollte deshalb eher **zwei bis drei Ebenen subtil koppeln**.

## Psychoakustik, Langzeithören und Nutzerforschung

Die Forschung rechtfertigt keine Behauptung wie „AMBIENT regeneriert den Nutzer“. Eine Meta-Analyse von 15 Studien mit 1.285 Teilnehmenden fand positive Effekte natürlicher Klangexposition auf verschiedene Angst- und physiologische Maße, betonte jedoch erhebliche Heterogenität. Eine weitere Meta-Analyse fand Vorteile gegenüber Stille bei einigen physiologischen Stressmaßen, aber nicht durchgehend bei subjektivem Stress. Eine neuere umfassende Soundscape-Meta-Analyse berichtet ebenfalls positive durchschnittliche Effekte natürlicher Sounds, mit erheblichen Moderatoren durch Kontext und Expositionsdauer. citeturn790288search0turn790288search1turn790288search3

Daraus folgt für die Kommunikation: **„für ruhiges, fokussiertes und immersives Hören entwickelt“** ist vertretbar. **„reduziert Stress“, „fördert Regeneration“ oder „beruhigt nachweislich“** sollte ohne produktspezifische Studie nicht als Produkteffekt behauptet werden.

### Hörermüdung

Auditorische Rauigkeit hängt unter anderem mit schnellen Amplitudenmodulationen und dicht benachbarten spektralen Komponenten zusammen und wird häufig als aversiv wahrgenommen. Die Literatur verbindet auditory roughness klar mit Unangenehmheit und aufmerksamkeitsbindenden Reaktionen. citeturn790288search8

Für AMBIENT ist das unmittelbar relevant. Deine „harten Resonanzen“ und „ermüdenden Höhen“ sollten nicht ausschließlich subjektiv debuggt werden. Zu jeder World sollte ein technischer QA-Pass gehören:

Spektrale Peaks über lange Zeit beobachten; Resonator-Q begrenzen; High-Shelf/Tilt-Ausgleich prüfen; DC entfernen; True Peak überwachen; einzelne Voice und Gesamtmix vergleichen; mindestens 20 Minuten bei realistischer Lautstärke hören.

Low-Mid-Matsch ist wiederum teilweise eine Maskierungsfrage: Überlappende spektrale Energie erschwert die Trennung konkurrierender Klangobjekte. Forschung zur auditory scene analysis beschreibt sowohl energetische als auch informationsbezogene Maskierung. citeturn790288search9

Für AMBIENT bedeutet das praktisch: Nicht fünf Cells gleichzeitig im Bereich 150–600 Hz sättigen. Eine Cell kann dort „Körper“ besitzen, während andere stärker ober- oder unterhalb arbeiten. Dieser Ansatz ist eine Designübertragung aus allgemeiner Maskierungsforschung. **Befundgrad C.**

### Agency statt Parameterkontrolle

Ein Kernproblem vieler generativer Systeme ist, dass völlige Kontrolle das System zum Synthesizer macht, völlige Autonomie dagegen zum Radio. AMBIENT sollte dazwischen liegen.

Der Nutzer kontrolliert **Wahrscheinlichkeitsräume**, nicht Noten.

Beispiel:

`Density` verändert Eventwahrscheinlichkeit und Voice-Lifetime.  
`Depth` verändert spektrale Dunkelheit, Register und Reverb-Damping.  
`Motion` verändert Driftgeschwindigkeit und Modulationsweite.  
`Distance` verändert Direkt-/Raumverhältnis, HF-Damping und Pre-delay.  
`Variation` verändert Wahrscheinlichkeit struktureller Abweichungen.

Ein Encoder sollte niemals plötzlich aus „schön“ „kaputt“ machen, sofern nicht ausdrücklich ein Performance-Modus gewählt wird.

### Vorgeschlagene Bachelor-Nutzerstudie

Die Studie sollte nicht versuchen, medizinische Stressreduktion nachzuweisen. Für ein Bachelorprojekt ist eine **Sound- und Interaction-Validation** wesentlich realistischer.

**Teilnehmende:** Ziel N ≈ 24–32 für Within-Subjects-Vergleiche; diese Zahl ist eine pragmatische Designempfehlung und keine formal berechnete Poweranalyse. Für die Bachelorarbeit sollte vor Studienstart anhand der primären Zielvariable eine Poweranalyse durchgeführt werden.

**Stimuli:** vier Pilot-Worlds × drei Naturintegrationen:

A — reine Synthese  
B — Synthese + hörbares Field Recording  
C — Synthese + Naturmaterial/Prozess subtil transformiert.

Die Gesamtzahl von zwölf Stimuli sollte auf mehrere Sessions oder Teilmengen verteilt werden, um Ermüdung zu reduzieren.

**Blindphase:** keine Namen, keine Bilder. Bewertet werden:

Angenehmheit  
Natürlichkeit  
räumliche Tiefe  
musikalische Kohärenz  
Interesse  
wahrgenommene Wiederholung  
Hörermüdung  
freie Landschaftsassoziation.

**Kongruenzphase:** dieselben oder vergleichbare Stimuli mit Landscape-Name/Bild. Dadurch lässt sich messen, wie stark die Naturassoziation tatsächlich durch Klang oder erst durch semantisches Priming entsteht.

**Hardware-vs.-Software:** identische Klangengine einmal über AMBIENT-Hardware, einmal über eine einfache Softwareoberfläche. Nicht Klangqualität, sondern Agency, Flow, wahrgenommene Komplexität und Bedienfreude vergleichen.

**Langzeittest:** mindestens ausgewählte 10–20-Minuten-Sequenzen. Eine 30-Sekunden-Demo testet Charaktererkennung, aber nicht Repetition Fatigue.

**Randomisierung:** Reihenfolge innerhalb der Bedingungen randomisieren beziehungsweise mit Latin-Square-artiger Gegenbalancierung verteilen.

**Primäre Kriterien:** World-Identifizierbarkeit ohne Label, Wiederholungserkennung, Kohärenz, angenehmes Langzeithören und Kontrollverständnis.

**Sekundär:** Naturassoziation, emotionaler Charakter, wahrgenommene Premiumqualität.

Generative-Music-Forschung betont generell, dass automatische Kennzahlen allein musikalische Qualität nur unvollständig erfassen und subjektive sowie HCI-basierte Evaluation notwendig bleiben. citeturn980187search13

## AMBIENT Sound Design Specification

Die Forschung spricht für eine deutliche Vereinfachung und zugleich Vertiefung der Engine.

### Zielarchitektur

**Fünf Cells**, aber nicht fünf gleichartige Synthvoices.

Ich würde sie funktional differenzieren:

**Cell A — Ground**  
langsame tonale oder rauschhafte Grundfläche.

**Cell B — Harmonic**  
Chord Cloud / Harmony Field mit geringer Polyphonie.

**Cell C — Texture**  
Particle-/Sample-/Field-Recording-Schicht.

**Cell D — Events**  
seltene punktuelle Klangereignisse, Resonator-Impulse, Glocken-, Holz-, Eis-, Wasser- oder abstrakte Exciter.

**Cell E — Memory**  
arbeitet primär mit Material der anderen Cells und bringt Vergangenheit verändert zurück.

Dadurch besitzt jede Cell eine **musikalische Rolle**. Das ist wesentlich stärker als fünf nahezu gleichwertige Voice-Slots.

### Generatives System

Empfohlene Architektur:

```text
WORLD STATE
│
├─ pitch-set / tonal centre
├─ density range
├─ activity profile
├─ timbral bounds
├─ spatial profile
└─ nature process
        │
        ▼
LONG FORM STATE
10–30 min trajectory
        │
        ▼
EVENT SCHEDULER
bounded probability
        │
        ├───────────────┐
        ▼               ▼
HARMONY FIELD       TEXTURE EVENTS
        │               │
voice-leading         grains/noise
        │               │
        └───────┬───────┘
                ▼
             CELLS
                │
                ▼
          MEMORY ENGINE
                │
                ▼
         SPACE / REVERB
```

### Harmoniegenerator

Kein freies Random-Chord-System.

Ein minimaler Ansatz:

```text
allowed_pitches = world.pitch_set

current_voicing = [p1, p2, p3]

when harmonic_event:
    candidate = copy(current_voicing)

    change 1 voice usually
    change 2 voices occasionally
    change all voices almost never

    select new pitch from allowed_pitches
    minimize pitch distance
    reject:
        register collision
        excessive low interval density
        repeated exact voicing too often

    current_voicing = candidate
```

Die wichtigste Regel lautet nicht „Dur klingt gut“, sondern **harmonische Kontinuität**.

### Event Scheduler

Statt:

```text
if random() < probability:
    play()
```

besser:

```text
activity = world_base_density
activity += slow_form_curve(time)
activity += bounded_drift()

next_event =
    base_interval
    * random_log_range(0.7, 1.6)

if recent_event_count > limit:
    increase silence probability

if silence_time > world_max_silence:
    gently increase event probability
```

Das System erhält damit **lokale Erinnerung** und produziert weder Maschinengewehr-Ereignisse noch unbeabsichtigt minutenlange Leere.

### Signature-Effekte

Aus der Recherche ergeben sich sechs Kandidaten; vier davon reichen für den Bachelor-Prototyp.

**Ghost Memory — höchste Priorität.** Früheres Audiomaterial taucht leiser, gefiltert und leicht verändert wieder auf. Das könnte die stärkste Differenzierung gegenüber LIVEN Ambient Ø werden.

**Drift Diffuser — hohe Priorität.** Kleine asynchrone Delays und Diffusion verändern Stereo- und Zeitstruktur, ohne als Chorus hörbar zu sein.

**Resonant Terrain — hohe Priorität.** Natur-/Noise-Impulse regen eine kontrollierte modale Bank an. Damit wird Naturmaterial musikalisch, ohne einfach abgespielt zu werden.

**Weathering — hoch.** Sehr langsame gerichtete Veränderung von Bandbreite, Saturation, Noise und Mikro-Detuning. Inspiriert vom Prinzip des Materialwandels, nicht von einem einzelnen Basinski-Effekt. citeturn976531search4

**Spectral Mist — später.** FFT- oder Filterbank-basierte weiche Freeze-/Blur-Struktur; erst implementieren, wenn CPU-Budget bekannt ist.

**Air Shimmer — optional.** Sehr subtiler pitch-shifted feedback branch. Kein Hauptmerkmal.

### Vier Pilot-Worlds

| World | Ground | Events | Naturintegration | Generative Signatur |
|---|---|---|---|---|
| **OCEAN** | dunkles Noise + tiefe langsame Tonfläche | seltene breite Swells | Wasser/Wind als leises Grain-Material | mehrere unterschiedlich lange zyklische Bewegungen |
| **FOREST** | leichte diffuse Textur | viele kleine räumliche Ereignisse | Vegetation/Biophonie fragmentiert | lokale Ereigniscluster + lange Pausen |
| **DESERT** | sehr sparsamer Drone | seltene resonante Einzelereignisse | Wind/Textur, nicht „Wüsten-SFX“ | maximale negative space |
| **HIGHLANDS** | tiefes harmonisches Feld | entfernte, langsam bewegte Ereignisse | Wind/ferne Geophonie | hohe räumliche Tiefe und langsames Voice-Leading |

**Lake, Grassland, Glacier und Tundra sollten zunächst nicht gebaut werden.** Erst wenn Blindtests zeigen, dass vier Worlds tatsächlich akustisch unterscheidbar sind, lohnt eine Erweiterung.

### Klangqualitäts-Gates

Eine World wird erst akzeptiert, wenn sie fünf Tests besteht.

**30-Sekunden-Test:** Charakter vorhanden, ohne Bild verständlich unterschiedlich.

**10-Minuten-Test:** keine offensichtlich nervige Wiederholung.

**Solo-Test:** jede Quelle klingt schon vor Reverb musikalisch brauchbar.

**Dry/Wet-Test:** Effektkette verbessert das Material; Wet 100 % darf Fehler nicht bloß verstecken.

**Low-volume-Test:** Klang bleibt bei geringer Lautstärke lesbar und benötigt keine aggressive Hochtonenergie.

Dazu technische Gates:

keine Klicks bei Voice-Start/Stop,  
keine harten Patch-/World-Wechsel,  
keine unbeabsichtigten Peaks durch Resonatoren,  
keine hörbare Aliasingdominanz im vorgesehenen Pitchbereich,  
kein systematisches Clipping,  
kein wachsendes Reverb-/Feedback-Energieproblem.

### Entwicklungspriorität

Für ein selbstfinanziertes Bachelorprojekt würde ich ausdrücklich **nicht** versuchen, alle beschriebenen Synthesearten umzusetzen.

**Phase A — Qualität der Quelle.** Glass/Tape/Particle ohne großen Reverb verbessern. Klicks, Aliasing, hornartige Peaks und Envelope-Probleme beheben.

**Phase B — Harmony + Motion.** Harmony Field auf stabiles Voice-Leading reduzieren. Motion Engine in periodisch, bounded drift und long-form trajectory trennen.

**Phase C — ein exzellenter Space-Block.** Diffuser + Mod Delay + ein Reverb statt vieler Varianten.

**Phase D — Memory Engine.** Das ist die wichtigste neue Funktion und wahrscheinlich der stärkste Produktunterschied.

**Phase E — vier Worlds.** Dieselbe Engine, aber unterschiedliche Regeln und Quellen. Keine World-spezifischen Sondercode-Berge.

**Phase F — Naturvarianten und Blindtests.** Erst hier entscheiden, wie viel Field Recording wirklich nötig ist.

**Phase G — CPU-/Memory-Profiling.** Jeder DSP-Block einzeln, dann Worst-Case mit fünf Cells. Keine Prozentangaben aus Desktop-Simulationen übernehmen.

Die zentrale technische Empfehlung lautet daher:

> **Nicht mehr Soundgeneratoren bauen. Mehr musikalische Zeit bauen.**

AMBIENT hat bereits genügend Synthese- und Effektbausteine. Was ihm nach der Recherche am stärksten fehlt, ist ein hörbar intelligentes Verhältnis zwischen **Vergangenheit, Gegenwart und langsamer Veränderung**. Genau dort unterscheiden sich die stärksten Ambient-Referenzen von generischen „Pad + Reverb“-Sounds, und genau dort ist im Wettbewerbsvergleich noch Raum für ein fokussiertes, naturbezogenes Instrument.

### Bibliografie und Primärquellen

**Ambient / Musiktheorie**

Brian Eno, *Ambient 1: Music for Airports*, Liner Notes, 1978. Reproduziert u. a. hier:  
https://soundenvironments.wordpress.com/wp-content/uploads/2011/10/eno_ambience.pdf citeturn789593search12

Victor Szabo, “Unsettling Brian Eno’s Music for Airports”, *Twentieth-Century Music*, 2017. DOI: **10.1017/S147857221700024X**. citeturn789593search1turn789593search6

Christopher Scoates et al., *Brian Eno's Ambient 1: Music for Airports*, Oxford University Press, 2018. DOI: **10.1093/oso/9780190497293.001.0001**. citeturn789593search2

Steve Reich, “Music as a Gradual Process”, 1968; in *Writings on Music 1965–2000*. DOI: **10.1093/acprof:oso/9780195151152.003.0004**. citeturn980187search4turn980187search15

Brian Eno, “Generative Music”, Imagination Conference, San Francisco, 1996.  
https://www.inmotionmagazine.com/eno1.html citeturn980187search7

Curtis Roads, *Microsound*, MIT Press, 2002/2004. ISBN 9780262681544.  
https://mitpress.mit.edu/9780262681544/microsound/ citeturn456314search2

**DSP**

Manfred R. Schroeder, “Natural Sounding Artificial Reverberation”, *Journal of the Audio Engineering Society*, 10(3), 1962, 219–223.  
https://aes.org/publications/elibrary/elibrary-page/?id=849 citeturn456314search1

Jon Dattorro, “Effect Design, Part 1: Reverberator and Other Filters”, *Journal of the Audio Engineering Society*, 45(9), 1997, 660–684.  
https://aes.org/publications/elibrary/elibrary-page/?id=10160 citeturn456314search0

ARM, CMSIS-DSP Documentation.  
https://developer.arm.com/Additional%20Resources/CMSIS%20DSP%20Software%20Library citeturn456314search3

ARM CMSIS-DSP License, Apache-2.0.  
https://github.com/ARM-software/CMSIS-DSP/blob/main/LICENSE citeturn210518search2

Electro-Smith, DaisySP Documentation, MIT License.  
https://electro-smith.github.io/DaisySP/ citeturn210518search0

Mutable Instruments Eurorack Source Repository.  
https://github.com/pichenettes/eurorack citeturn210518search1

Faust Documentation, Licensing FAQ.  
https://faustdoc.grame.fr/manual/faq/ citeturn132662search4

STMicroelectronics, STM32H742/H743 Datasheet DS12110 Rev. 11, 2026.  
https://www.st.com/resource/en/datasheet/stm32h743ig.pdf citeturn210518search7

**Natur, Soundscape und Psychoakustik**

Pijanowski et al., “Soundscape Ecology: The Science of Sound in the Landscape”, *BioScience* 61(3), 2011, 203–216. DOI: **10.1525/bio.2011.61.3.6**. citeturn790288search4

“Effects of natural sound exposure on health recovery: A systematic review and meta-analysis”, 2024. PubMed PMID **38373459**. citeturn790288search0

“The effect of exposure to natural sounds on stress reduction: a systematic review and meta-analysis”, 2024. PubMed PMID **39285764**. citeturn790288search1

“Beyond noise and silence: urban soundscapes and their contribution to human well-being—a systematic review and meta-analysis”, *npj Urban Sustainability*, 2026. citeturn790288search3

“Roughness perception: A multisensory/crossmodal perspective”, 2022.  
https://pmc.ncbi.nlm.nih.gov/articles/PMC9481510/ citeturn790288search8

“Listening in complex acoustic scenes”, 2020.  
https://pmc.ncbi.nlm.nih.gov/articles/PMC7116612/ citeturn790288search9

**Generative Systeme**

Carnovalini & Rodà, “Computational Creativity and Music Generation Systems: An Introduction to the State of the Art”, 2020/2021.  
https://pmc.ncbi.nlm.nih.gov/articles/PMC7861321/ citeturn980187search1

Lerch et al., “Survey on the Evaluation of Generative Models in Music”, *ACM Computing Surveys*, 2025. DOI: **10.1145/3769106**. citeturn980187search13

**Künstler- und Produktionsquellen**

Biosphere, *Substrata* Archivseite.  
https://biosphere.no/substrata.html citeturn789593search8

Geir Jenssen/Biosphere Interview, FACT, 2016. citeturn789593search9

Loscil Interview, Secret Thirteen. citeturn789593search10

Stars of the Lid Interview, The Quietus. citeturn976531search0

William Basinski Interview, The Quietus. citeturn976531search4

Wolfgang Voigt/GAS, Red Bull Music Academy Lecture. citeturn976531search6

Christian Fennesz, *Venice 20* Produktionsnotizen. citeturn163075search2

**Produktquellen**

SONICWARE LIVEN Ambient Ø — offizielle Spezifikationen und aktuelle Produktseite. citeturn350337view0

SOMA COSMOS — offizielle Spezifikationen und Preisseite. citeturn804588search1turn848990search1

Torso Electronics S-4 — offizielle technische Spezifikationen und Sound On Sound Review. citeturn350337view2turn458623search7

Chase Bliss MOOD MKII — offizielle Produkt-/Technikseite. citeturn350337view3

Strymon NightSky — offizielle Produkt-/Support-Dokumentation. citeturn848990search3turn804588search4

monome norns — offizielle Dokumentation. citeturn804588search5

SOMA LYRA-8 — offizielle Spezifikation und Manual. citeturn458623search9turn350337view6

1010music nanobox lemondrop — offizielle technische Spezifikation und User Guide. citeturn350337view7turn458623search10

Hologram Electronics Microcosm — offizielle Produkt- und Supportseiten. citeturn238790search0turn238790search1