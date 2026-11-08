# SD33 — lange WOODLAND-Saite: Color kalibrieren

Stand2026-10-12, Product Sound Candidate Spec0.10. Eine einzelne Softwareeinheit:
die in PR157 belegte sehr schwache WOODLAND-Colorwirkung. SD33 bleibt offen;
musikalischer Nutzen, Familienauswahl und Hardware werden dadurch nicht abgenommen.

## Verhalten und begrenzter Umfang

Vorher verändert Color0/0,5/1 den Loop-Damp0,025/0,015/0,005. Der tatsächlich
gehaltene Klang unterscheidet sich kaum: FFT-Leistungszentrum181,08/181,27/181,46Hz.
Jetzt bleibt der schwingende Loop bei exakt der bisherigen Default-Auswertung.
Nur der Ausgang der langen Product-Saite bekommt eine nichtresonante Farbe:
L[n]=L[n−1]+a·(x[n]−L[n−1]); Ausgang=x+(2Color−1)·(x−L).
a=1−exp(−2π·min(2f,0,45SR)/SR) wird einmal bei Note-on berechnet.
Dark0 nimmt das tiefe Band, Bright1 ergänzt das obere Band. Color0,5 ist exakt
der bisherige Ausgang. Keine zusätzlichen Oszillatoren, Noise-, Sample- oder
Delaypools; zwei vorhandene Saiten. Dry und Room-Send erhalten dieselbe Farbe.
80-ms-Glättung ist pro Sample; der Filter verfolgt auch bei Neutral den Klang.
Kein kalter Filterzustand bei Livebedienung. Keine Transzendentalfunktion im
Sampleschritt; vorhandene Loop-Hz, rho, Dämpfung, Attack/Release und RNG bleiben.
Die tatsächliche Besitzer-/Retirementprüfung verfolgt die unveränderte Saite.
Clear stellt den aktuellen Colorwert wieder her. Invalidfloats bleiben ignoriert,
endliche Ausreißer werden geklemmt. Historische kurze Quellen bleiben unverändert.

Spec0.10 ändert bewusst ausschließlich die Bedeutung der WOODLAND-Colorendwerte.
Default0,5, Brightnessadapter und SCN7-Schema bleiben; gespeicherte Endwerte
bekommen die neue Farbe. Reference bleibt Default, Product Kandidat, Journal OFF.

## Vollständiger Vergleich zur echten Baseline

Exakter PR157 `ab673f49de056154d985290d5bc1f8a7f5bf43a5`, Tree
`f76ba92fa14a90846b3804b21704cf26f2b98888`; CI38147911200/run689 frisch SUCCESS.
251 getrackte Core-/Header-/Startup-/Linker-/CMakeinputs: nur drei verändert
(pluck.c, engine_product.c, pluck.h),248 exakt gleich. Inputs vor/nach Render versiegelt.
Je18 feste112-s-Fälle (drei Worlds × zwei Seeds1234/42 × Color0/0,5/1) plus drei
Livefälle;21 Kandidaten jeweils64/512,21 Baselines bei512:63 komplette Render,
117,6min PCM. Kandidaten vollständig blockbytegleich in PCM/Events/Status.
Alle ganzen Ereignisse und Statusdaten sind gegen Baseline und über Colorwerte
identisch: Frequenzen, Velocity, Holds, Besitzer, Note-on/off, Phrasen/Memory,
Zulassung und tatsächlich freigewordene Slots. WOODLAND Default ist vollständiges
PCM bytegleich; COAST/HIGHLANDS sind bei jedem Wert und Livewechsel PCM bytegleich.
Livewerte initial0,5; bei10/20/32/48/64/72s auf0/1/0,5/0/1/0,5, keine Sofortnote.
Quellen3/2/3 inklusive Releases, NaN/Limiter0, Raw True Peak≤−18,9dBFS,
Mono≥0,93090, DC≤1,27e−8. Stop80s, spätestens88,84s alle Besitzer frei;
letzte Sekunde aller112-s-Aufnahmen exakt still, kein künstliches Endfade.
Maximaler benachbarter PCM-Sprung um Liveänderungen0,02390; keine Klick-Hörabnahme.

24 direkte Float-Solo-Fälle,60–1760Hz, drei Colorwerte: größter Pitchfehler
0,03369Cent, alle acht Neutralaufnahmen gegen Baseline floatbytegleich.
Fundamental-Decayvergleich aus0,90–1,15/1,40–1,65s: höchstens0,00151% relative
Abweichung. Dies ist ein gemessener Zweifenster-Decay, keine volle T60-Messung.
Der ungesteuerte110-Hz-Ton endet bei allen drei Werten im selben512-Block,
bei37,849s erstmals inaktiv geprüft; keine vorgezogene Dämpfung.
Gerenderter440-/220-Hz-Leistungsquotient0,06189→0,15325 belegt geänderte Obertöne,
statt allein Phase oder Gesamtpegel. Float64/512, NaN/Inf, Clamp, Live und
historische kurze Quelle bestehen im neuen normalen Hosttest.

## Konkrete Wirkung und WAVs

Seed1234, echte gehaltene Passage6–12,5s: FFT-Center175,67/181,27/188,86Hz;
Min→Max relativ zum Default7,278% statt0,208%. Seed42 bestätigt7,263%.
FFT misst keine Klangqualität oder garantierte Hörbarkeit. Die Filterfarbe hat
auch echte Pegelwirkung: im38,5-s-Hörfile−26,4/−25,3/−23,0LUFS je Abschnitt.
Diese Unterschiede bleiben erhalten; kein getrenntes Normalisieren und keine AGC.

AMBIENT_WOODLAND_Color_calibrated_0_50_100_38p5s.wav: gleiche echte Passage3–15,5s,
Color0 bei0–12,5s,0,5 bei13–25,5s,1 bei26–38,5s. Festes Gain+8,2dB,
gesamt−24,5LUFS/−11,3dBTP.40-ms-Schnittkanten und zwei0,5-s-Vergleichslücken.
AMBIENT_WOODLAND_Color_Before_After_27p5s.wav: jeweils gleiche Passage3–9,5s,
alt0→neu0→alt1→neu1. Je6,5s, drei0,5-s-Lücken, ein Gain+7,7dB,
gesamt−24,8LUFS/−11,8dBTP. Kein künstlicher Quellenstopp/Time-stretch.
Vollständige RAWs nur in OS/tmp, nach versiegelter Prüfung entfernt; exportiert
sind nur zwei WAVs, alle vollständigen Ereignis-/Status-CSVs und JSON.

## Host und H743

Vollständiger unveränderter Ablauf des firmware-c-next Hostrunners: Exit0,
einschließlich Product-/Scene-/World-/Stress-/Room-/Clear-/Ownership-/Hotpathchecks.
Baseline/Kandidat H743 Product Release mit denselben SHA-/größengeprüften Ubuntu-
Noble GCC13.2.1/Newlib4.4-Paketen, CMake3.31.6, Journal OFF: beide Linkaudits bestehen.
Baseline-ELF d99fb5c0b0536c2e376fe2447a25542287356f93f41cd143f5a1f675f4d18105
exakt reproduziert. Flashdelta+236B, D1+32B,
D2+0B, DTCM+0B. Raum/Diffusion/DMA weiter intern.
Einzelne Compilerframes sind keine verschachtelte ISR-/Runtime-Stackmessung.
CPU/DWT/Underruns, physische Bedienung und Hören bleiben offen.
Historische COAST-/Colorjobs bleiben auf PR156/PR157; neue aktuelle WOODLAND-
Color-CI reproduziert diese Quellen, einschließlich Solo-/Ensemble-Gegenproben.
Details/Hashes: [SD33_WOODLAND_COLOR_METRICS.json](SD33_WOODLAND_COLOR_METRICS.json)
und [SD33_WOODLAND_COLOR_ARM_METRICS.json](SD33_WOODLAND_COLOR_ARM_METRICS.json).

Nächste einzelne Vorbereitung: Attack0/0,5/1 im tatsächlichen Ensemble jeder World
prüfen. Release folgt getrennt danach. Keine UI vor Soundentscheidung.
