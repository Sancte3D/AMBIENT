# Video- und Webindex

Die Videos selbst und vollständige Transkripte sind nicht enthalten. Diese
Datei enthält eigene Kurzbeschreibungen und verweist auf passende Offline-
Quellen.

## Phil's Lab

### Audio DSP with STM32

Playlist:
https://www.youtube.com/playlist?list=PLXSyc11qLa1ZCn0JCnaaXOWN6Z46rK9jd

Themen:

- Echtzeit-Audiopfad;
- IIR- und Notch-Filter;
- Tremolo;
- I²S + DMA;
- ADC + DMA + Timer;
- Codec-Treiber;
- Double Buffering.

Offline dazu:

- `upstream_sources/STM32CubeH7/.../SAI/SAI_AudioPlay`
- `upstream_sources/CMSIS-DSP`
- `upstream_sources/libDaisy`

### Allgemeine STM32-Reihe

Playlist:
https://www.youtube.com/playlist?list=PLvEoilT4QIN1v-lBNQT_fRxc8dX3O58i7

Besonders relevant:

- Custom-Hardware-Bring-up;
- SWD, SPI, PWM und USB;
- DMA und FreeRTOS;
- Treiberentwicklung aus Datenblättern.

## YetAnotherElectronicsChannel

Kanal:
https://www.youtube.com/c/YetAnotherElectronicsChannel

### IIR Filters with I²S

https://www.youtube.com/watch?v=lNBrGOk0XzE

Lernwert: Echtzeit-I²S-Pfad und einfache Filter. Der verlinkte Repositorycode
besitzt keine gefundene Weiterverteilungslizenz und ist daher nicht enthalten.
Als legale Offline-Grundlage dienen STM32CubeH7 und CMSIS-DSP.

### Reverb

https://www.youtube.com/watch?v=nRLXNmLmHqM

Lernwert: Reverb aus parallelen Feedback-Comb-Filtern und seriellen
Allpass-Stufen. Das Paket enthält in `clean_room` eine unabhängig neu
geschriebene MIT-0-Implementierung.

### CMSIS-DSP FIR/IIR

https://www.youtube.com/watch?v=vCcALaGNlyw

Lernwert: ARM-optimierte Filter im Audiostream. Der vollständige offizielle
CMSIS-DSP-Quelltext und dessen Beispiele sind offline enthalten.

## Daisy

Kanal:
https://www.youtube.com/@daisy_sound/videos

Audioeinstieg:
https://electro-smith.github.io/libDaisy/md_doc_2md_2__a3___getting-_started-_audio.html

Offline dazu:

- `upstream_sources/libDaisy`
- `upstream_sources/DaisyExamples`
- `upstream_sources/DaisySP`

## Offizielle STM32-Schulung

STM32CubeIDE-Playlist:
https://www.youtube.com/playlist?list=PLnMKNibPkDnFlFe2NTzTLsh4Acoh-cvYR

Hinweis: Neuere STM32CubeIDE-2.x-Workflows trennen CubeMX und IDE stärker als
ältere Videos. Die `.ioc`-Dateien und generierten Projekte im Paket bleiben
trotzdem nützlich.
