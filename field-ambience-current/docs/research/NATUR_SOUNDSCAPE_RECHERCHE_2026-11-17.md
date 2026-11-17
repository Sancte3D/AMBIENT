> **Herkunft:** Recherche des Besitzers (mit ChatGPT Deep Research erstellt), am 17.11.2026 ins Repo gelegt.
> Unverändert übernommen. Die Marker `citeturn…` sind kaputte Quellverweise aus ChatGPT; die Quellen
> dahinter sind **nicht geprüft**. Die Literaturliste am Ende nennt DOIs; auch die sind nicht einzeln geprüft.
> Was AMBIENT daraus übernimmt, steht als To-do in `docs/worlds/WORLDS_ENTSCHEIDUNGSPAKET_2026-11-13.md`,
> Abschnitt „To-do aus der Natur-Recherche“.

# AMBIENT: Natur, Landschaft und generative Klangräume als belastbares Produkt- und Soundsystem

## Abstract, Forschungsrahmen und Evidenzlogik

### Ausgangsproblem

Das zentrale Problem von **AMBIENT** ist nicht die Frage, ob Natur und Ambient-Musik „zusammenpassen“. Historisch und kulturell tun sie das häufig. Das schwierigere Problem lautet: **Wie lässt sich eine Landschaft so in Klang, Interaktion und visuelle Referenz übersetzen, dass die Beziehung nachvollziehbar ist, ohne zu behaupten, ein bestimmter Synthesizerklang sei objektiv „Wald“, „Gletscher“ oder „Ozean“?**

Die Recherche führt zu einer klaren Antwort: Ein wissenschaftlich belastbares System sollte Landschaft **nicht primär über musikalische Metaphern**, sondern über beobachtbare Eigenschaften realer Soundscapes abbilden. ISO 12913 definiert Soundscape ausdrücklich nicht als bloßes Audiosignal, sondern als eine akustische Umgebung, wie sie von Menschen **in einem Kontext wahrgenommen, erlebt oder verstanden** wird. Soundquelle, physikalisches Signal, Kontext, Interpretation und Reaktion sind damit getrennte Ebenen. citeturn708888search7turn896348search0

Für AMBIENT ergibt sich daraus ein methodischer Grundsatz:

> **Natur liefert keine Tonart. Natur liefert zeitliche, spektrale, räumliche und ereignisbezogene Strukturen. Musik interpretiert diese Strukturen.**

Damit kann AMBIENT glaubwürdig naturbezogen sein, ohne pseudowissenschaftlich zu behaupten, dass ein Mollakkord „Tundra“, ein Lowpass-Filter „Nebel“ oder ein Drone „Berg“ sei.

### Zentrale Forschungsfragen

Die Recherche wurde auf fünf miteinander verknüpfte Fragen verdichtet:

1. Welche Eigenschaften realer Landschafts-Soundscapes lassen sich objektiv erfassen und für eine generative Engine nutzen?
2. Welche musikalischen Eigenschaften von Ambient, Minimalismus, Drone und elektroakustischer Musik eignen sich, um diese Umweltstrukturen zu übersetzen?
3. Welche Naturklänge besitzen empirisch relativ stabile Wahrnehmungseffekte – und wo beginnen kulturelle oder individuelle Unterschiede?
4. Wie stark dürfen Bild, Name und Produktnarrativ die Wahrnehmung führen, bevor die Naturreferenz nur noch durch Priming entsteht?
5. Welchen funktionalen Mehrwert kann ein physisches Instrument gegenüber App, Streaming-Playlist oder Noise-Machine tatsächlich bieten?

### Arbeitshypothesen für AMBIENT

**H1:** Klangwelten werden glaubwürdiger und unterscheidbarer, wenn sie unterschiedliche **generative Verhaltensmodelle** besitzen und nicht nur unterschiedliche Samples, Akkorde oder EQ-Kurven.

**H2:** Eine hybride Architektur aus realen Field Recordings und abstrakter musikalischer Synthese wird für AMBIENT wahrscheinlich belastbarer sein als entweder reine Naturwiedergabe oder komplett synthetische Naturmetaphern.

**H3:** Landschaftsnamen und Bilder erhöhen Wiedererkennbarkeit und wahrgenommene Natürlichkeit, können aber zugleich die Bewertung verzerren; deshalb müssen Audio-only-Tests von Audio-Visual-Tests getrennt werden. Framing-Effekte sind experimentell dokumentiert. citeturn292500search4turn292500search1

**H4:** „Entspannung“, „Regeneration“ oder „Stressreduktion“ dürfen höchstens als mögliche Nutzungserfahrung kommuniziert werden. Die Forschung zeigt durchschnittliche Vorteile natürlicher Klänge, aber hohe Heterogenität, unterschiedliche Outcomes und keinen universellen Effekt. citeturn897705search0turn897705search5

### Evidenzhierarchie dieser Recherche

Die folgenden Empfehlungen sind bewusst unterschiedlich stark gewichtet:

| Evidenzklasse | Bedeutung | Verwendung in AMBIENT |
|---|---|---|
| **A – empirisch belastbar** | Meta-Analysen, Standards, replizierte Experimente | Wahrnehmung, Soundscape-Evaluation, Naturklangwirkung |
| **B – theoriegestützt** | etablierte psychoakustische/ökologische Modelle | Parameterauswahl und Soundarchitektur |
| **C – Praxiswissen** | Komponisten, Field-Recordists, Soundscape-Praxis | kompositorische Methoden |
| **D – Designhypothese** | noch zu testende AMBIENT-Entscheidung | konkrete Mappings, Namen, UI |

Diese Trennung ist entscheidend. Beispielsweise ist die Beobachtung, dass reale Umweltklänge charakteristische zeitlich-spektrale Statistiken besitzen, empirisch gut gestützt. Die Entscheidung, daraus einen 16-Takt-Harmoniewechsel oder eine Filterbewegung abzuleiten, bleibt dagegen eine **kompositorische Designentscheidung**. citeturn796267search4turn796267search5

## Musikalische Grundlage: Was „Ambient“ für AMBIENT tatsächlich bedeutet

Brian Enos Definition von Ambient ist für das Produkt relevanter als das heutige Genre-Klischee „Pad + Reverb“. In den Liner Notes zu *Ambient 1: Music for Airports* beschreibt Eno Ambient als Musik, die mehrere Grade von Aufmerksamkeit zulassen soll, statt einen bestimmten Hörmodus zu erzwingen; sein Systemgedanke zielte zudem darauf, Umgebungen nicht durch standardisierte Hintergrundmusik zu glätten, sondern deren Charakter zu ergänzen. citeturn708888search2turn708888search9

Später beschrieb Eno generative Musik als das Entwerfen eines Systems, dessen Regeln Klang hervorbringen, anstatt jeden einzelnen musikalischen Verlauf zu komponieren. In Interviews verweist er ausdrücklich darauf, dass ähnliche Resultate entstehen sollen, ohne identisch wiederholt zu werden. Seine frühen Tape-Systeme beruhten bereits auf unterschiedlich langen, gegeneinander laufenden Prozessen. citeturn708888search3turn708888search4

Für AMBIENT ist das wichtig: **Generativität ist nicht dasselbe wie Zufall.** Ein gutes System besitzt einen stabilen Identitätsraum, innerhalb dessen kontinuierliche Variation stattfindet. Enos Bild des Flusses – derselbe Fluss, aber nie exakt derselbe Zustand – ist als Designprinzip sehr nah an realen Landschaften. citeturn708888search5

Minimalismus liefert eine zweite wichtige Grundlage. Steve Reichs *Music as a Gradual Process* argumentiert für musikalische Systeme, deren Veränderungsprozess selbst hörbar wird und sich langsam genug entwickelt, dass der Hörer ihn wahrnehmen kann. Das unterscheidet sich fundamental von zufällig auftauchenden „schönen Ambient-Ereignissen“. citeturn680219search8turn680219search5

Drone-Musik wiederum zeigt, dass musikalische Entwicklung nicht zwingend über Melodie oder Akkordprogression stattfinden muss. Die frühe Minimalmusik um La Monte Young entwickelte lange gehaltene Klangzustände und mikroskopische Veränderungen innerhalb eines scheinbar statischen Materials. citeturn680219search6turn680219search7

Für AMBIENT ergibt sich daraus folgende **musikalische Grammatik**:

| Parameter | Ambient-taugliche Funktion | Risiko |
|---|---|---|
| Harmonik | langsame harmonische Felder statt funktionaler Progression | zu statisch oder kitschig |
| Dissonanz | geringe bis moderate kontrollierte Schwebungen | „meditativ“ ≠ automatisch konsonant |
| Rhythmus | Prozesse, Pulsationen, Ereignisabstände statt Beat | zu regelmäßig → Loop-Gefühl |
| Dichte | langsam modulierte Anzahl gleichzeitig aktiver Quellen | zu viele Events → Aufmerksamkeit steigt |
| Timbre | langfristige spektrale Evolution | stereotype „Natur-Pads“ |
| Dynamik | geringe Durchschnittsdynamik, aber Mikrovariation | Totkompression wirkt künstlich |
| Raum | Schichtung von Nah/Fern, Höhe/Breite, Bewegung | übertriebener Reverb = Fantasy |
| Zeitstruktur | mehrskalige Veränderungen | kurze LFOs werden schnell erkennbar |

Musique concrète und Soundscape Composition bieten die dritte Grundlage. Pierre Schaeffer verwendete ab 1948 aufgezeichnete reale Geräusche als kompositorisches Material; spätere elektroakustische Traditionen entwickelten daraus Verfahren, Klang nach Textur, Morphologie und Wahrnehmung statt nur nach Tonhöhe zu organisieren. citeturn680219search0turn680219search2

Barry Truax und Hildegard Westerkamp gehen für AMBIENT einen Schritt weiter: Bei Soundscape Composition bleibt der Bezug zwischen Klang und seinem Herkunftskontext relevant, selbst wenn das Material transformiert wird. Westerkamp beschreibt Field Recordings gerade nicht als perfekt kontrollierbare „Instrumente“: Eine Umweltaufnahme enthält das unkontrollierte Leben eines Ortes. citeturn796267search7turn754233search8

Das führt zu einer wichtigen Designentscheidung: **AMBIENT sollte reale Naturaufnahmen nicht lediglich als dekoratives Layer unter Pads legen.** Dadurch würde Natur zur Textur und die eigentliche Musik bliebe unabhängig. Stärker wäre, wenn Umweltaufnahmen sowohl **Material als auch Steuerdaten** für das generative System liefern.

## Naturklänge, Soundscape-Forschung und was wissenschaftlich tatsächlich belegbar ist

Die moderne Soundscape-Forschung unterscheidet häufig **Biophonie** – von Lebewesen erzeugte Klänge –, **Geophonie** – Wind, Regen, Wasser, geophysikalische Prozesse – und **Anthrophonie** – menschlich erzeugten Klang. Pijanowski et al. etablierten diese Unterscheidung als zentralen Rahmen der Soundscape Ecology. citeturn708888search1turn377325search2

Entscheidend ist, dass reale Landschaften **keine festen Sound-Presets** besitzen. Soundscapes verändern sich mit Tageszeit, Jahreszeit, Wetter, Tieraktivität, Vegetationsstruktur, Topografie und menschlicher Aktivität. Ecoacoustic-Forschung zeigt deutliche Tages- und Saisonmuster und zugleich, dass einzelne akustische Indizes stark von Kontext und Wetter beeinflusst werden können. citeturn377325search0turn377325search3turn377325search11

Das widerspricht unmittelbar einer naiven Zuordnung wie:

„Forest = Vögel + Wind + warmes Pad.“

Ein Wald am Morgen, bei Starkregen, im Winter und nachts sind vier deutlich unterschiedliche Soundscapes. Tropische und temperierte Wälder zeigen ebenfalls unterschiedliche Tagesmuster und Komplexitäten. citeturn377325search18

### Naturklang und Wohlbefinden

Die bislang wichtigste Meta-Analyse für diese Fragestellung ist Buxton et al. 2021. Sie identifizierte 36 Publikationen zu Naturklängen; 18 gingen in Meta-Analysen ein. Im Mittel zeigten sich Verbesserungen bei Stress/Belästigung sowie bei positiven beziehungsweise gesundheitsnahen Outcomes. Gleichzeitig war die Heterogenität zwischen Studien hoch, und nur ein kleiner Teil untersuchte klassische physiologische Gesundheitsmaße; viele Studien verwendeten Stimmung, Präferenz oder Kognition als Zwischenvariablen. citeturn897705search0turn897705search1

Die oft zitierten Ergebnisse „Wasser ist am besten“ oder „Vögel reduzieren Stress“ sollten deshalb nicht als Produktgesetz behandelt werden. In Buxtons Synthese hatten Wasserklänge den größten mittleren Effekt auf positive/gesundheitsbezogene Outcomes, während Vogelklänge besonders bei Stress/Annoyance auffielen; die Konfidenzintervalle waren jedoch breit und die zugrunde liegenden Studien sehr heterogen. citeturn897705search1

Ein neuerer Review von 2024 fand Naturklänge gegenüber Stille für einige physiologische Parameter günstiger, aber keine signifikante Differenz bei allen untersuchten Outcomes, darunter wahrgenommener Stress in der aggregierten Analyse. citeturn897705search5

Auch Einzelstudien sind differenzierter als das populäre Narrativ. Eine EEG-Studie von 2024 fand beispielsweise keine generelle höhere EEG-„Relaxation“ natürlicher Geräusche gegenüber Stille, obwohl fließendes Wasser und Vogelgesang bestimmte subjektive Stressmaße verbesserten. citeturn897705search2

Für AMBIENT folgt daraus:

**Zulässig:** „entwickelt für ruhige, immersive Hörsituationen“, „unterstützt fokussiertes Zuhören“, „kann als persönliche Ruheumgebung genutzt werden“.

**Nicht belastbar:** „regeneriert das Nervensystem“, „senkt Stress“, „verbessert Konzentration“ als allgemeines Produktversprechen.

### Warum Wasser und Vögel trotzdem interessant sind

Wasser ist akustisch besonders nützlich, weil verschiedene Wasserzustände sehr unterschiedliche Texturen erzeugen: Tropfen, Rinnsal, Strom, Brandung, Regen und diffuse Oberflächenbewegung liefern sowohl kontinuierliche als auch diskrete Ereignisse. In Experimenten werden Wassergeräusche regelmäßig als positiv oder restaurativ bewertet; Lautstärke und Kontext verändern die Wirkung jedoch deutlich. In einer VR-Studie wurde leiseres Wasser beispielsweise günstiger bewertet als lauteres. citeturn897705search4

Vogelklänge sind noch stärker kontextabhängig. Qualitative Interviews von Ratcliffe et al. zeigten, dass Vogelgesang häufig mit Erholung assoziiert wurde, aber **nicht alle Vogelarten und Rufe** als angenehm oder restaurativ wahrgenommen wurden; akustische Eigenschaften, Ästhetik, Erinnerungen und individuelle Naturbeziehungen beeinflussten die Bewertung. citeturn897705search6

Das ist für AMBIENT hochrelevant: Ein randomisiertes „Bird Layer“ ist keine sichere Naturalness-Maschine.

### Spektrum, Ereignisdichte und natürliche Dynamik

Analysen natürlicher Geräusche zeigen, dass deren zeitlich-spektrale Modulation nicht beliebig verteilt ist. Singh und Theunissen fanden, dass natürliche Sounds im Allgemeinen stärker in langsamen spektralen und zeitlichen Modulationsbereichen liegen als in simultan extrem schnellen zeitlichen und spektralen Veränderungen. citeturn796267search4

McDermott und Simoncelli konnten reale Soundtexturen wie Regen oder Tiergruppen durch statistische Modelle des auditorischen Systems teilweise rekonstruieren. Wichtig ist dabei, dass einfache Spektralverteilungen allein nicht ausreichten: Korrelationen zwischen Frequenzkanälen und Modulationsstatistiken waren wesentlich für erkennbare und natürlich wirkende Texturen. citeturn796267search5turn796267search6

Das ist eine direkte technische Inspiration für AMBIENT: **Natürlichkeit sollte eher als statistisches Verhalten als als Sample-Identität modelliert werden.**

### Vorsicht mit „1/f = Natur“

Voss und Clarke fanden in den 1970er-Jahren 1/f-artige Strukturen in zeitlichen Lautstärke- und Tonhöhenfluktuationen vieler Musikbeispiele. Spätere Studien fanden 1/f-ähnliche zeitliche Muster auch in bestimmten Umweltklängen. citeturn796267search0turn796267search3

Aber: Yang et al. zeigten explizit, dass **angenehme Umweltgeräusche nicht allgemein 1/f-Strukturen besitzen**. Manche Vogelklänge zeigten entsprechende Strukturen auf bestimmten Zeitskalen, andere Umweltkategorien nicht. citeturn796267search9

Daraus folgt: Pink Noise darf ein Werkzeug sein, aber nicht die Behauptung „Pink Noise klingt natürlich, weil Natur 1/f ist“.

## Von realer Landschaft zu Klangsystem: die empfohlene Taxonomie

Eine wichtige Konsequenz der Ecoacoustic-Forschung lautet: Landschaftstypen können akustisch unterschiedlich sein, aber der Unterschied entsteht nicht allein aus Geografie. Habitatstruktur, Tiergemeinschaft, Wasser, Wetter und Tageszeit interagieren. In Amazonien konnten akustische Kennwerte Habitattypen statistisch unterscheiden; ähnliche Arbeiten zeigen zugleich, dass Ergebnisse stark standortabhängig sein können. citeturn377325search7turn377325search17

Deshalb würde ich für AMBIENT nicht acht gleichberechtigte „Welten“ erzwingen. **Sechs Kernlandschaften sind konzeptionell stärker als acht schwach differenzierte.**

| Name | Reale Grundlage | Akustisch belastbare Quellen | Generatives Prinzip | Risiko |
|---|---|---|---|---|
| **COAST** | Küsten weltweit | Brandung, Wind, Küstenvögel | überlagerte quasizyklische Prozesse | kann schnell „Spa Ocean“ wirken |
| **RIVER** | fließende Gewässer | Strömung, Turbulenz, Uferbiophonie | kontinuierlicher Flow + lokale Ereignisse | Überschneidung mit Coast |
| **FOREST** | bewaldete Habitate | Bioakustische Vielfalt, Blätter/Wind, Regen | viele unabhängige Akteure | „Vogelwald“-Klischee |
| **WETLAND** | Marsch, Moor, Uferzonen | Amphibien, Insekten, Vögel, Wasser | hohe episodische/chorische Aktivität | je Region sehr verschieden |
| **GRASSLAND** | offene Graslandschaften | Wind, Insekten, verstreute Tierereignisse | weite Textur + geringe Schichtung | akustisch subtil |
| **DRYLAND** | aride/semiaride Räume | Wind, seltene Bioereignisse, Insekten | hohe Leerräume, seltene Ereignisse | Gefahr „Desert = mystisch“ |

**ALPINE** wäre meine siebte optionale Kategorie. Sie ist visuell stark und real, aber akustisch weniger eindeutig. Wind, Wasser, Vogelrufe und große Distanzräume können plausibel sein, sind jedoch nicht exklusiv alpin. Deshalb würde ich Alpine erst aufnehmen, wenn Blindhörtests eine eigenständige Identität bestätigen.

**GLACIER** und **TUNDRA** würde ich zunächst nicht als Kernkategorien verwenden. Sie sind visuell außergewöhnlich, aber ihre alltagspsychologische Akustik ist weniger stabil und die Gefahr ist groß, dass die Soundwelt hauptsächlich aus kulturellen Stereotypen wie „hell“, „kalt“, „gläsern“, „spärlich“ konstruiert wird. Das wäre eine künstlerische Interpretation, keine echte ökologische Ableitung.

### Der entscheidende methodische Wechsel

Anstatt zu sagen:

**COAST → langsame Sinuswelle → Filter → Hall**

sollte der Workflow lauten:

**Reale Coast-Aufnahmen → Analyse → statistische Parameter → generatives Verhalten → musikalische Interpretation.**

Das kann technisch so aussehen:

| Messgröße aus Field Recording | Engine-Parameter | Evidenzstatus |
|---|---|---|
| Ereignisse pro Minute | Trigger-Wahrscheinlichkeit | direkte Ableitung |
| Verteilung der Ereignisabstände | stochastisches Timing | direkte Ableitung |
| Modulationsspektrum | LFO-/Envelope-Zeitskalen | theoriegestützt |
| spektrale Energiedichte | Synth-/Noise-Spektralgewichtung | direkte Ableitung |
| spektrale Varianz | Timbre-Bewegung | Designmapping |
| Dynamikverteilung | Amplitudenhüllkurven | direkte Ableitung |
| Kanal-/Mikrofonkorrelation | Stereo-/Raumbreite | theoriegestützt |
| Bio-/Geo-/Anthrophonie-Anteil | Layer-Gewichtung | theoriegestützt |
| Tageszeitmuster | langfristige Zustandsänderung | ökologische Ableitung |

Acoustic-Index-Literatur warnt davor, einzelne Indizes als direkte Biodiversitäts- oder Habitatwahrheit zu interpretieren. Mehrere Kennwerte gemeinsam und lange Aufnahmezeiträume liefern robustere Beschreibungen als einzelne kurze Samples. citeturn377325search0turn377325search3

### Konkrete Engine-Architektur

Die stärkste Lösung wäre aus meiner Sicht eine **dreischichtige hybride Engine**:

**Layer A – Environmental Ground**

Leise reale Aufnahmen bilden eine materielle Verbindung zur Natur. Sie sollten nicht als immer hörbares „Nature FX Pack“ eingesetzt werden, sondern teilweise bis an die Wahrnehmungsgrenze gehen.

**Layer B – Ecological Behaviour**

Aus realen Aufnahmen abgeleitete Ereignisdichten, Modulationszeiträume und Wahrscheinlichkeitsverteilungen steuern Synthese, Granularprozesse oder Sample-Ereignisse.

**Layer C – Musical Interpretation**

Tonales Material, Drones, Resonanzen und harmonische Felder bilden die kompositorische Ebene. Diese Ebene muss ausdrücklich als Interpretation verstanden werden, nicht als naturwissenschaftliches Abbild.

Beispiel **COAST**:

- drei überlagerte, unterschiedlich lange Amplitudenprozesse statt eines periodischen „Wave-LFO“;
- tieffrequente breitbandige Energie als langsam variierende Grundtextur;
- unregelmäßige hochfrequente transiente Komponenten;
- reales Brandungsmaterial nur partiell hörbar;
- musikalisches Material mit langen Decays und geringer Ereignisdichte;
- keine fixe „Ocean-Tonart“.

Beispiel **FOREST**:

- mehrere unabhängige Event-Agenten statt eines gemeinsamen Rhythmus;
- Aktivität abhängig von virtueller Tageszeit;
- kontinuierliches Leaf/Wind-Material als diffuse Textur;
- diskrete Bio-Events mit unterschiedlich langen Refraktärzeiten;
- Pads oder Tonfelder reagieren auf die **Gesamtdichte**, nicht umgekehrt.

Beispiel **DRYLAND**:

- niedrige durchschnittliche Ereignisdichte;
- größere Varianz der Ruheintervalle;
- Wind oder körnige Geophonie als zeitweise dominierende Textur;
- musikalische Ereignisse dürfen deutlich seltener auftreten;
- bewusstes Zulassen von akustischem „Nichts“.

Das ist ein großer Unterschied: **Stille wird zu einem Parameter.**

### Vermeidung von Loop-Wahrnehmung

Statische Natur-Loops werden schnell als Aufnahme erkannt. Für AMBIENT sollten daher mehrere Zeitskalen gleichzeitig wirken:

- Sekunden: Mikrotextur.
- Zehnersekunden: Ereigniscluster.
- Minuten: Dichte und Raum.
- 10–30 Minuten: atmosphärische Zustände.
- Sitzungsdauer: langfristige Entwicklung.

Diese mehrskalige Organisation passt sowohl zu generativer Musik als auch zu der in Ecoacoustics dokumentierten zeitlichen Veränderlichkeit realer Soundscapes. citeturn708888search4turn377325search0

Granularsynthese ist dafür besonders geeignet, solange sie nicht lediglich als Effekt verwendet wird. Truax hat Granularsynthese ausdrücklich für Soundscape-Komposition und Umweltklang eingesetzt; sie erlaubt, Material stark zu transformieren und dennoch strukturelle Spuren des Originals beizubehalten. citeturn796267search7turn754233search1

## Bilder, Namen, physisches Interface und Nutzererfahrung

### Namen sind keine neutralen Labels

Ein Experiment mit identischen Soundscapes zeigte, dass allein die Kennzeichnung einer Umgebung als „natural“ oder „urban“ deren wahrgenommene Natürlichkeit und Präferenz verändern konnte. citeturn292500search4turn292500search5

Für AMBIENT bedeutet das: Wenn der Nutzer zuerst „FOREST“ liest und anschließend einen komplexen Klang hört, beweist seine Zuordnung „Forest“ nicht, dass die Soundengine den Wald überzeugend repräsentiert.

Deshalb sollte die Produktentwicklung zwei Ebenen auseinanderhalten:

**Soundidentität:** Funktioniert ohne Namen und Bild?

**semantische Verstärkung:** Verbessern Name und Bild die Orientierung zusätzlich?

### Bilder können helfen – und betrügen

Audio-visuelle Kongruenz beeinflusst die Bewertung von Soundscapes. Studien zeigen höhere Realismus- und Präferenzwerte, wenn Klang und Landschaft visuell plausibel zusammenpassen. Sichtbares Wasser erhöht beispielsweise die Angemessenheit von Wassergeräuschen; bei Vogelklängen kann Vegetation als indirekter Hinweis genügen. citeturn292500search0turn292500search1turn292500search2

Das spricht **für** visuelle Landschaftsreferenzen, aber nicht zwingend für permanente Fotos.

Meine Empfehlung wäre:

**Kein dauerhaftes Landschaftsfoto als Hauptinterface.**

Stattdessen:

- Name der Landschaft;
- reduzierte abstrakte visuelle Struktur;
- langsame Veränderung passend zum Systemzustand;
- optional kurzer fotografischer Einstieg beim Wechsel;
- danach visuelle Zurücknahme.

Damit wird das Display zum Orientierungssystem, nicht zur eigentlichen Immersionsquelle.

### Warum ein physisches Gerät Sinn ergeben kann

Die Hardware ist nicht automatisch gerechtfertigt. „Kein Smartphone“ ist als USP zu schwach.

Die wissenschaftlich plausiblere Argumentation lautet: Digitale Musikinstrumente verlieren häufig Teile der körperlichen Rückmeldung klassischer Instrumente; haptische und multimodale Rückmeldung kann die Instrumentenwahrnehmung und bestimmte musikalische Aufgaben verbessern. citeturn668503search1turn668503search15

Für AMBIENT sollte Hardware deshalb nicht wie eine DAW in einer Box funktionieren. Der Mehrwert sollte sein:

**direkte körperliche Kontrolle über wenige hochwirksame makroskopische Eigenschaften eines komplexen Systems.**

Ich würde maximal etwa fünf primäre kontinuierliche Interaktionen vorsehen:

| Control | Nutzerverständnis | Engine |
|---|---|---|
| **SPACE** | nah ↔ weit | Raum, Early/Late Energy, Stereo |
| **DENSITY** | leer ↔ belebt | Eventpopulation |
| **MOTION** | ruhig ↔ bewegt | Modulationsraten/-tiefe |
| **TONE** | dunkel ↔ hell | Spektralgewichtung |
| **NATURE / MUSIC** | dokumentarisch ↔ musikalisch | Field/Synthesis-Balance |

Wichtig: Das sind **Interfacebegriffe**, nicht Naturgesetze.

Ein sechster interessanter Parameter wäre **TIME** oder **DAY**, der nicht einfach ein Filter ist, sondern das gesamte Verhaltensmodell verändert: Morgendämmerung, Tag, Abend, Nacht. Tageszeit ist in realen Soundscapes ein stark belegter Faktor. citeturn377325search0turn377325search18

### Agency statt Parameterflut

Generative Systeme haben ein spezielles UX-Problem: Zu wenig Kontrolle macht den Nutzer zum Zuhörer; zu viel Kontrolle macht das System zur DAW. Forschung zu interaktiver generativer Musik zeigt, dass nichtdeterministische Systeme Nutzer überfordern oder frustrieren können, wenn der Zusammenhang zwischen Eingabe und Output unklar ist. citeturn668503search17

Das optimale Modell für AMBIENT wäre deshalb:

**User steers; system composes.**

Der Nutzer entscheidet Richtung, Intensität und Charakter. Die Engine entscheidet Mikroereignisse.

### Qualitative Erfahrungsberichte

Praktikerberichte stützen vor allem einen Punkt: Field Recording wird häufig nicht nur wegen „schöner Naturgeräusche“, sondern wegen **Ort, Erinnerung und Aufmerksamkeit** geschätzt. Westerkamp beschreibt Kontextinformationen – Ort, Datum, Wetter – als Mittel, das Hören zu fokussieren. citeturn754233search8

Aktuelle Field-Recording-Communities nennen wiederholt „sense of place“, Erinnerung, Präsenz, Erhaltung verschwindender ruhiger Orte und das Wiedererleben eines Aufnahmeortes als Motivation. Das ist anekdotische und selbstselektierte Evidenz und darf nicht verallgemeinert werden, zeigt aber ein konsistentes qualitatives Motiv. citeturn808202reddit19turn808202reddit20

Interessant ist auch die Gegenposition: Nutzer kritisieren stereotype Vogel-/Wasser-Layer und bevorzugen teilweise Field Recordings, die nicht sofort dem üblichen „nature ambient“-Klischee entsprechen. citeturn808202reddit25

Das spricht für ein wichtiges AMBIENT-Prinzip:

**Authentizität entsteht nicht durch mehr Vogelgesang, sondern durch spezifischere Umweltstruktur.**

## Bachelor-Testplan und konkrete Entwicklungsstrategie

Die Produktidee sollte nicht mit einem einzigen „gefällt dir das?“-Test validiert werden. Nötig ist eine Reihe kleiner Experimente, die unterschiedliche Hypothesen isolieren.

### Phase A – akustische Grundlagen

Pro Landschaft werden zunächst reale Aufnahmen aus mehreren Regionen, Tageszeiten und Wetterbedingungen gesammelt.

Ziel ist **nicht**, einen angeblich perfekten „Forest Sound“ zu finden, sondern gemeinsame und variable Merkmale zu dokumentieren.

Zu erfassen:

- spektrale Energieverteilung;
- Lautheitsverteilung;
- Ereignisrate;
- Ereignisabstände;
- temporal modulation spectrum;
- spektrale Modulation;
- Stereo-/Spatial-Correlation;
- Bio-/Geo-/Anthrophonie;
- Tageszeit;
- Wetter;
- Saison;
- Aufnahmeort.

Für Umweltanalyse sollten mehrere Kennwerte kombiniert werden, da einzelne Acoustic Indices stark kontextabhängig sind. citeturn377325search0turn377325search3

### Phase B – Blindhörtest

**Fragestellung:** Sind die generativen Welten ohne Label unterscheidbar?

Empfehlung für eine Bachelorarbeit: etwa **48–72 Teilnehmende** in einem Within-Subject-Design; eine formale Power-Analyse sollte nach einem Pilotversuch anhand der beobachteten Varianz erfolgen, statt eine Effektgröße zu erfinden.

Stimuli:

- 6 Landschaften;
- je 2–3 unabhängige generative Durchläufe;
- jeweils etwa 45–90 Sekunden;
- randomisierte Reihenfolge;
- gleiche wahrgenommene Lautheit.

Nach jedem Stimulus:

- offene Frage: „Welche Umgebung oder Eigenschaften stellst du dir vor?“
- Natürlichkeit 1–7;
- Ruhe 1–7;
- Weite/Enge 1–7;
- Dichte 1–7;
- Ereignisreichtum 1–7;
- Pleasantness;
- Eventfulness;
- Familiarity.

Pleasantness und Eventfulness sind aus der Soundscape-Forschung gut etablierte Wahrnehmungsdimensionen und bilden den Kern des ISO-nahen Soundscape-Circumplex. citeturn476224search0turn896348search3

**Erfolgskriterium:** Nicht „80 % erkennen Forest“. Das wäre unrealistisch und zu semantisch.

Besser:

- Landschaften bilden unterscheidbare Wahrnehmungsprofile;
- Verwechslungscluster werden sichtbar;
- dieselbe Landschaft erzeugt über mehrere Runs ähnliche Profile.

Statistisch: lineare bzw. ordinale Mixed-Effects-Modelle mit Teilnehmer und Stimulus als Random Effects; bei Kategorien zusätzlich Confusion Matrix und Clusteranalyse.

### Phase C – Label-Kongruenz

Nun werden identische Audiostimuli unter verschiedenen Bedingungen präsentiert:

1. kein Label;
2. korrektes Label;
3. falsches Label;
4. korrektes Label + Bild;
5. falsches Label + Bild.

Gemessen werden:

- Naturalness;
- plausibility/congruence;
- immersion;
- preference;
- perceived place identity.

So lässt sich feststellen, **wie viel der Landschaftsidentität aus dem Klang und wie viel aus Priming entsteht**. Framing- und Kongruenzeffekte machen diesen Test methodisch notwendig. citeturn292500search1turn292500search4

### Phase D – Field Recording vs Synthese

Vier Versionen derselben Landschaft:

**A:** reine Field Recording  
**B:** reine synthetische Interpretation  
**C:** Field Recording + unabhängige Ambient-Musik  
**D:** Field Recording + daraus abgeleitetes generatives Verhalten

Meine Hypothese wäre, dass **D** langfristig die stärkste AMBIENT-Variante wird. Das ist jedoch explizit eine Designhypothese.

Bewertet werden:

- Naturalness;
- musikalisches Interesse;
- Langzeittauglichkeit;
- „sense of place“;
- wahrgenommene Wiederholung;
- Wunsch zur aktiven Manipulation.

### Phase E – Hardware gegen Software

Dasselbe generative System wird einmal über AMBIENT-Hardware und einmal über Touchscreen gesteuert.

Aufgaben:

- Landschaft auswählen;
- „ruhiger“ machen;
- „offener“ machen;
- eine interessante Variation erzeugen;
- anschließend fünf Minuten frei explorieren.

Messen:

- Task Completion;
- Interaktionsfehler;
- NASA-TLX oder reduzierte subjektive Workload;
- wahrgenommene Kontrolle;
- visuelle Aufmerksamkeit;
- Flow/Engagement;
- qualitative Interviews.

Hardware gewinnt nur dann konzeptionell, wenn Nutzer beispielsweise **weniger hinschauen müssen, Ursache-Wirkung besser verstehen oder stärkere Kontrolle erleben**. Haptik allein ist kein Beweis für bessere UX. Studien an digitalen Musikinstrumenten zeigen positive Effekte haptischer Rückmeldung, aber die Effekte hängen von Interface und Aufgabe ab. citeturn668503search15turn668503search28

### ISO als methodische Referenz

Die aktuelle Normfamilie besteht aus ISO 12913-1:2014 für Definition und Rahmen, ISO/TS 12913-2:2018 für Datenerhebung sowie der 2025 revidierten ISO/TS 12913-3 für Analyse; die 2019er Ausgabe von Teil 3 wurde zurückgezogen. citeturn708888search0turn896348search0turn896348search1

Für eine Bachelorarbeit muss AMBIENT nicht vollständig ISO-konform untersucht werden. Aber die Trennung von **akustischer Umgebung, Person und Kontext** ist methodisch extrem wertvoll.

## Priorisierte Produktentscheidung, Bruchstellen und Ressourcen

### Was ich beibehalten würde

**Generatives Ambient-Instrument:** stark.

**Physisches Objekt:** sinnvoll, sofern die Interaktion schneller und körperlicher als App-Steuerung wird.

**Natur als zentrales Thema:** stark, aber nur wenn sie die Engine beeinflusst.

**Landschaftsnamen:** behalten.

**Field Recording:** integrieren.

### Was ich verändern würde

Die heutige Idee „Landschaft → Stimmung → passende Musik“ ist zu weich.

Ersetzen durch:

**Landschaft → reale Soundscape-Daten → Verhaltensmodell → musikalische Interpretation.**

Damit bekommt das Konzept eine belastbare innere Logik.

Ich würde außerdem von acht auf zunächst **sechs Kernlandschaften** reduzieren:

**COAST  
RIVER  
FOREST  
WETLAND  
GRASSLAND  
DRYLAND**

Optional später:

**ALPINE**

Nicht initial:

**GLACIER  
TUNDRA**

Der Grund ist nicht, dass Glacier oder Tundra uninteressant wären. Sondern dass sie momentan vor allem über **visuelle und kulturelle Imagination** differenzierbar wären, während das Projekt eigentlich eine glaubwürdige akustische Logik sucht.

### Was ich streichen würde

Ich würde folgende Aussagen vollständig vermeiden:

„Jede Landschaft besitzt ihre eigene Frequenz.“

„Bestimmte Harmonien entsprechen bestimmten Landschaften.“

„1/f Noise bildet die Natur ab.“

„432 Hz verbindet den Menschen mit der Natur.“

„Naturfrequenzen regulieren das Nervensystem.“

„Ambient Sound regeneriert nachweislich.“

Für diese Aussagen gibt es in diesem Kontext keine ausreichende Evidenz. Die Forschung unterstützt differenziertere Aussagen zu natürlichen Soundscapes und bestimmten Stress-/Präferenz-Outcomes, nicht diese vereinfachten Kausalbehauptungen. citeturn897705search0turn796267search9

### Reale Audioquellen

**National Park Service Natural Sounds Gallery:** besonders interessant für Prototypen, weil die dort bereitgestellten Dateien laut NPS Public Domain sind. Damit eignen sie sich hervorragend als frühes Referenzmaterial. citeturn347547search8turn347547search9

**Freesound:** enorme Vielfalt; einzelne Dateien stehen unter CC0, CC BY oder CC BY-NC. Lizenz muss pro Sound geprüft und gespeichert werden. Für ein kommerzielles Produkt sollte BY-NC ausgeschlossen werden. citeturn347547search6turn347547search7

**Macaulay Library / Cornell:** hervorragende Quelle insbesondere für Vogel- und Tieraufnahmen, aber kein frei kommerziell nutzbares Sample-Archiv. Rechte liegen grundsätzlich bei den Recordists; kommerzielle Verwendung benötigt Lizenzierung. citeturn347547search0turn347547search1

**British Library Wildlife & Environmental Sounds:** wissenschaftlich und kuratorisch sehr stark, mit enormem weltweitem Bestand; kommerzielle Nutzbarkeit muss jedoch individuell geklärt werden. citeturn347547search4turn347547search5

Langfristig wäre für AMBIENT eine eigene Recording-Pipeline wahrscheinlich besser: kontrollierte Metadaten, konsistente Mikrofonierung, eigene Rechte und präzise Verbindung zwischen Aufnahme und Engine.

### Hörbare Referenzen

Für die Entwicklung würde ich nicht nur klassische Ambient-Alben hören, sondern verschiedene Positionen entlang eines Spektrums:

**Brian Eno – *Ambient 1: Music for Airports***  
Referenz für Aufmerksamkeit, Raum und geringe Ereignisdichte. citeturn708888search2

**Brian Eno – *On Land***  
relevant für die Verbindung von Landschaftsimagination, Material und Atmosphäre. citeturn708888search9

**Steve Reich – frühe Prozessarbeiten**  
relevant für wahrnehmbare, selbstlaufende Prozesse. citeturn680219search8

**Hildegard Westerkamp – *Kits Beach Soundwalk***  
wichtig für Ort, Aufmerksamkeit und Transformation von Field Recording. citeturn754233search0turn754233search7

**Barry Truax – *River Run***  
besonders relevant für AMBIENT, weil Truax einen Fluss musikalisch modelliert, ohne ihn einfach naturalistisch abzubilden; er beschreibt die Arbeit selbst als plausiblen abstrakten Klangraum. citeturn754233search1

**Francisco López – *Himavanta***  
interessant als Gegenposition: Umweltklang wird nicht primär dokumentarisch, sondern als eigenständiges Klangmaterial behandelt. citeturn754233search5

Diese Spannweite ist produktiver als eine Spotify-Recherche nach „Relaxing Nature Ambient“.

### Die konzeptionelle Kernempfehlung

AMBIENT sollte nicht behaupten:

**„Wir übersetzen Landschaften in Musik.“**

Das klingt stark, ist aber wissenschaftlich kaum haltbar.

Präziser und interessanter wäre:

**AMBIENT untersucht reale akustische Verhaltensweisen von Landschaften und verwendet diese als Regeln für generative musikalische Systeme.**

Dann existieren drei sauber getrennte Ebenen:

**Realität:** Field Recordings und Soundscape-Daten.

**System:** statistische und zeitliche Verhaltensregeln.

**Interpretation:** musikalische Gestaltung.

Genau diese Trennung verhindert, dass das Konzept esoterisch oder willkürlich wird.

Die Natur liefert AMBIENT dann nicht die **Ästhetik**, sondern einen Teil der **Logik**.

## Literatur, Standards und Quellenapparat

**Axelsson, Ö., Nilsson, M. E., & Berglund, B. (2010).** *A principal components model of soundscape perception.* Journal of the Acoustical Society of America, 128, 2836–2846. DOI: [10.1121/1.3493436](https://doi.org/10.1121/1.3493436). Grundlage des Pleasantness-/Eventfulness-Modells. citeturn476224search0

**Buxton, R. T., Pearson, A. L., Allou, C., Fristrup, K., & Wittemyer, G. (2021).** *A synthesis of health benefits of natural sounds and their distribution in national parks.* PNAS, 118(14), e2013097118. DOI: [10.1073/pnas.2013097118](https://doi.org/10.1073/pnas.2013097118). citeturn897705search0

**Fan, L., & Baharum, M. R. (2024).** *The effect of exposure to natural sounds on stress reduction: a systematic review and meta-analysis.* Stress, 27. DOI: [10.1080/10253890.2024.2402519](https://doi.org/10.1080/10253890.2024.2402519). citeturn897705search5

**Liu, C. et al. (2024).** *Effects of natural sound on human stress recovery based on EEG techniques.* Journal of Environmental Psychology. DOI: [10.1016/j.jenvp.2024.102365](https://doi.org/10.1016/j.jenvp.2024.102365). citeturn897705search2

**Michels, N., & Hamers, P. (2023).** *Nature Sounds for Stress Recovery and Healthy Eating: A Lab Experiment Differentiating Water and Bird Sound.* Environment and Behavior. DOI: [10.1177/00139165231174622](https://doi.org/10.1177/00139165231174622). citeturn897705search3

**Ratcliffe, E., Gatersleben, B., & Sowden, P. T. (2013).** *Bird sounds and their contributions to perceived attention restoration and stress recovery.* Journal of Environmental Psychology. DOI: [10.1016/j.jenvp.2013.08.004](https://doi.org/10.1016/j.jenvp.2013.08.004). citeturn897705search6

**Pijanowski, B. C. et al. (2011).** *Soundscape Ecology: The Science of Sound in the Landscape.* BioScience, 61(3), 203–216. DOI: [10.1525/bio.2011.61.3.6](https://doi.org/10.1525/bio.2011.61.3.6). citeturn708888search1

**Pijanowski, B. C. et al. (2011).** *What is soundscape ecology? An introduction and overview of an emerging new science.* Landscape Ecology, 26, 1213–1232. DOI: [10.1007/s10980-011-9600-8](https://doi.org/10.1007/s10980-011-9600-8). citeturn708888search8

**Bradfer-Lawrence, T. et al. (2019).** *Guidelines for the use of acoustic indices in environmental research.* Methods in Ecology and Evolution. DOI: [10.1111/2041-210X.13254](https://doi.org/10.1111/2041-210X.13254). citeturn377325search0

**Alcocer, I. et al. (2022/2023).** *Using acoustic indices in ecology: Guidance on study design, analyses and interpretation.* Methods in Ecology and Evolution. DOI: [10.1111/2041-210X.14194](https://doi.org/10.1111/2041-210X.14194). citeturn377325search3

**Singh, N. C., & Theunissen, F. E. (2003).** *Modulation spectra of natural sounds and ethological theories of auditory processing.* Journal of the Acoustical Society of America, 114, 3394–3411. DOI: [10.1121/1.1624067](https://doi.org/10.1121/1.1624067). citeturn796267search4

**McDermott, J. H., & Simoncelli, E. P. (2011).** *Sound texture perception via statistics of the auditory periphery: evidence from sound synthesis.* Neuron, 71, 926–940. DOI: [10.1016/j.neuron.2011.06.032](https://doi.org/10.1016/j.neuron.2011.06.032). citeturn796267search5

**Voss, R. F., & Clarke, J. (1975).** *1/f noise in music and speech.* Nature, 258, 317–318. DOI: [10.1038/258317a0](https://doi.org/10.1038/258317a0). citeturn796267search0

**Voss, R. F., & Clarke, J. (1978).** *1/f noise in music: Music from 1/f noise.* Journal of the Acoustical Society of America, 63, 258–263. DOI: [10.1121/1.381721](https://doi.org/10.1121/1.381721). citeturn796267search1

**Yang, M., De Coensel, B., Kang, J., et al. (2015).** *Presence of 1/f Noise in the Temporal Structure of Psychoacoustic Parameters of Natural and Urban Sounds.* Journal of the Acoustical Society of America, 138, 916–927. DOI: [10.1121/1.4927033](https://doi.org/10.1121/1.4927033). citeturn796267search3

**Truax, B. (1996).** *Soundscape, Acoustic Communication and Environmental Sound Composition.* Contemporary Music Review. DOI: [10.1080/07494469608629688](https://doi.org/10.1080/07494469608629688). citeturn796267search7

**Reich, S. (1968).** *Music as a Gradual Process.* Wiederabgedruckt in *Writings on Music 1965–2000*. Oxford University Press. citeturn680219search8turn680219search5

**ISO 12913-1:2014.** *Acoustics — Soundscape — Part 1: Definition and conceptual framework.* 2014, 2025 erneut bestätigt. citeturn708888search0

**ISO/TS 12913-2:2018.** *Acoustics — Soundscape — Part 2: Data collection and reporting requirements.* 2018, 2025 erneut bestätigt. citeturn896348search0

**ISO/TS 12913-3:2025.** *Acoustics — Soundscape — Part 3: Data analysis.* Zweite Edition, Mai 2025. citeturn896348search1

**Wang, R., & Zhao, J. (2019).** *A good sound in the right place: Exploring the effects of auditory-visual combinations on aesthetic preference.* Urban Forestry & Urban Greening. DOI: [10.1016/j.ufug.2019.05.018](https://doi.org/10.1016/j.ufug.2019.05.018). citeturn292500search0

**Effects of contexts in urban residential areas on the pleasantness and appropriateness of natural sounds (2020).** Sustainable Cities and Society. DOI: [10.1016/j.scs.2020.102475](https://doi.org/10.1016/j.scs.2020.102475). citeturn292500search1

**The Effect of Animate-Inanimate Soundscapes and Framing on Environments’ Evaluation and Predicted Recreation Time (2020).** International Journal of Environmental Research and Public Health, 17, 9086. DOI: [10.3390/ijerph17239086](https://doi.org/10.3390/ijerph17239086). citeturn292500search4

**Liu et al. / landscape multisensory assessment (2014).** *Multi-sensory landscape assessment: the contribution of acoustic perception to landscape evaluation.* Journal of the Acoustical Society of America. DOI: [10.1121/1.4898424](https://doi.org/10.1121/1.4898424). citeturn292500search7

**Papetti et al. / Audio-Tactile Rendering review (2021).** *Audio-Tactile Rendering: A Review on Technology and Methods to Convey Musical Information through the Sense of Touch.* Sensors, 21, 6575. DOI: [10.3390/s21196575](https://doi.org/10.3390/s21196575). citeturn668503search1

**Papetti et al. (2023).** *Design and Assessment of Digital Musical Devices Yielding Vibrotactile Feedback.* Arts, 12, 143. DOI: [10.3390/arts12040143](https://doi.org/10.3390/arts12040143). citeturn668503search0

**Westerkamp, H. / Velasco-Pufleau, L. (2022–2023).** *Listening is Action: A Soundwalk with Hildegard Westerkamp.* Forschungs-/Sounddokumentation zur Soundscape-Praxis. citeturn754233search0turn754233search7

**Cornell Lab / Macaulay Library.** Medienarchiv und Lizenzinformationen; kommerzielle Verwendung lizenzpflichtig bzw. rechteabhängig. citeturn347547search0turn347547search1

**U.S. National Park Service.** *Natural Sounds Gallery.* Bereitgestellte Audiodateien sind laut NPS Public Domain. citeturn347547search8

**Freesound.** Soundarchiv mit CC0-, CC-BY- und CC-BY-NC-Lizenzen; Lizenzstatus muss auf Asset-Ebene geprüft werden. citeturn347547search6turn347547search7

**British Library Sound and Vision Collections.** Umfangreiche Wildlife- und Environmental-Sound-Sammlung; Nutzungsrechte abhängig von Werk und Rechteinhabern. citeturn347547search4turn347547search5