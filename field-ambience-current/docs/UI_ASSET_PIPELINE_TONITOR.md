# AMBIENT — was Tonitor für unsere Display-Dateien bringt

Stand 26.10.2026. Geprüft: das hochgeladene Tonitor 0.56, Archiv-SHA256
`9c52dcfebb94058bb0476763aaf4649603cd384015b1d8bf0aeeb1b51e275e1a`,
und AMBIENT auf PR185 plus eigenständigem Resonatorkandidaten.

## Ergebnis für den Entwurf

Ja: Die Firmware hilft beim Aufbau unserer UI-Dateien. Der nützliche Teil
ist ihr Herstellungsprinzip: Grafik, Schrift und veränderliche Werte werden
getrennt vorbereitet und im Gerät zusammengesetzt. Wir müssen keine fertigen
Screens als Bildfolge für jede mögliche Reglerstellung exportieren.

Tonitor liefert allerdings keinen Figma-/Illustrator-Export und keine
LVGL-Screens. `ui.c` beschreibt Ansichten und Zustände als C-Code. `gfx.c`
zeichnet Text, Rechtecke und Reglergrafik. `tools/gen_font.py` wandelt echte
Schriften vorab in kompakte C-Daten. Die sichtbaren SVGs in `docs/assets/`
sind Website-/Markenmaterial, kein ausführbares Display-Layout.

## Was im Quelltext tatsächlich steht

| Fundstelle im Tonitor-ZIP | Verfahren | Übertragung auf AMBIENT |
|---|---|---|
| `firmware/src/gfx.c`, `font_t`, `cv_text` | Vorbereitete Glyphen mit vier Bit Deckung, Breiten und Offsets; Text wird mit dem Hintergrund verrechnet. | Schrift bei ihren tatsächlichen Pixelgrößen erzeugen; dieselben Glyphen für Vorschau und Gerät verwenden. |
| `tools/gen_font.py`, `render_ttf`, `main` | TTF → Rasterglyphen → 4-Bit-Lauflängencodierung → C-Header. | Pipelineprinzip übernehmen; das Tonitor-Format ist nicht das bestehende AMBIENT- oder LVGL-Format. |
| `gfx.c`, `CV_MAX`, `CV_AREA`, `lcd_sync` | Ein RGB565-Band mit 240 × 56 Pixeln; auf dessen DMA-Ende warten, bevor der Puffer neu benutzt wird. | Begrenzte Zeichenpuffer, expliziter Pufferbesitz während DMA. Breite/Geometrie nicht kopieren. |
| `ui.c`, `ui_draw`, `shown_*` | Zuletzt dargestellten Zustand merken; unveränderte Bereiche auslassen. | Daten ändern das betroffene UI-Element; keine ständige komplette Neuzeichnung. |
| `ui.c`, `draw_dial`, `dial_until`, `dial_drawn_at` | Zeitweilige Regleransicht; Neuzeichnen dort höchstens alle 40 ms, wenn der Wert wechselte. | Präzisen Wert beim Drehen zeigen, danach zur musikalischen Ansicht zurückkehren. Dauer und Anordnung bleiben Entwurfsfragen. |
| `ui.c`, `draw_dial` | Bogenpunkte aus Wert und Winkel berechnen, Schrift separat darüber zeichnen. | Ein Kreis/Bogen wird durch den Parameter gesteuert; keine 101 einzelnen PNGs für 0–100 %. |

Das sind Verfahren, keine Übernahme fremder Screenlayouts. Die entsprechenden
Tonitor-Code-/Generatorsourcen sind GPL-3.0-only gekennzeichnet. Hier wurden
eigene Werkzeuge geschrieben; kein Tonitor-Code oder Fontdatensatz kopiert.

## Der aktuelle AMBIENT-Pfad und der geplante LVGL-Pfad

Das aktuelle Ziel bleibt **320 × 170 Pixel, ST7789V2, STM32H743**.
Die alternative 320 × 240-Konfiguration ist weiterhin nur vorbereitet.
LVGL bleibt die gewünschte UI-Basis; es ist im geprüften Quellstand noch
nicht integriert. Ein neuer Export ist deshalb nicht automatisch bereits
eine auf dem Gerät nutzbare UI.

| Bereich | Heute im Repository | Für die geplante LVGL-UI |
|---|---|---|
| Pixel | 4-Bit-Grau, globale Farbtönung über LUT | RGB565-Zeichenpfad für voneinander unabhängige Farben |
| Displaypuffer | 27.200 B Grau; H743-DMA mit zwei Zeilen à 640 B | Vorschlag: zwei Teilpuffer à 16 Zeilen / 10.240 B; gesamt 20.480 B, zusätzlich LVGL-Objekte/Fonts/Heap |
| Schrift | `baked_font.h`, `baked_font.c`, `baked_font_data.c`, `tools/generate_fonts.py` | Passende, versionierte LVGL-Font-C-Dateien; Glyphenbedarf inkl. Umlauten/♯ separat festlegen |
| Bildschirm | `menu.c` + `oled_draw.c` | Screen-/Widget-C-Dateien und Binding an bestehende Firmwarezustände |
| Transfer | `hal_h743/lcd_st7789_h743.c`, `oled_show_async`, Busy-Guard | Rechteck-Flush, sichere DMA-Puffer, passender Farb-/Bytepfad und erst nach fertigem Transfer Freigabe an LVGL |

Eine komplette RGB565-Fläche benötigt 108.800 B. Zwei 16-Zeilen-Puffer
brauchen jeweils 10.240 B — **nicht** beide zusammen. Das ist nur das
Pixelbudget, keine Zusage für Gesamt-RAM oder Bildrate. Die aktuellen
Audiotiming-/Stack-Gates gelten auch mit dem späteren Renderer.

Der vorhandene Graupfad kann ein pink-oranges Hauptbild mit gleichzeitig
andersfarbigen UI-Elementen nicht originalgetreu wiedergeben. Dafür brauchen
wir den RGB565-/LVGL-Pfad. Das Hauptbild wird als eigenes Asset erhalten;
Kreise, Bögen, Text und Status sind separate Ebenen darüber.

## So sollten unsere UI-Dateien aufgebaut sein

| Datei / Teil | Inhalt |
|---|---|
| `screen_master.svg` oder editierbarer Figma-/Illustrator-Entwurf | Exakt 320 × 170, getrennte Ebenen mit stabilen Namen; Gestaltungsquelle, keine direkt ausführbare Firmware. |
| `world_background.png` | Originales, natives Farbbild; Werte und Status nicht in dieses Bild einbrennen. |
| `icons/…png` | Kleine Icons/Masken einzeln, mit nativer Pixelgröße und transparentem Hintergrund. |
| `layout.json` | Positionen, Größen, Ebenenreihenfolge, Farben, Schriftgrößen und Bindings; als Entwurfsvertrag versionieren. |
| `ui_assets.c/.h`, `ui_fonts.c` | Für den ausgewählten Renderer erzeugte Bild-/Schriftdaten. |
| `ui_screen_*.c` und `ui_bindings.c` | Laufende Screens, Werte, Auswahl-/Bearbeitungszustände und Meldungen. |

Das sind Dateikonventionen für den nächsten UI-Schritt, keine schon
vorhandenen Screen-Dateien oder beschlossene Encoder-Neubelegung.
Stabile Ebenennamen zum Beispiel: `world_background`, `parameter_label`,
`parameter_value`, `value_arc`, `value_marker`, `hold_status`, `scene_status`.
Eine SVG-ID allein erzeugt noch keine Firmware-Verbindung; das Binding muss
sie ausdrücklich einem realen Zustand zuordnen.

Für jeden Zustand eine native Vorschau erstellen: Minimum/Neutral/Maximum,
Navigieren/Bearbeiten, Hold/Drone/Generate, Mute und Scene-Speicherfehler.
Szenen umfassen fünf Slots. Encoder 1 erhält durch diese Untersuchung keine
neue Funktion. Inhalte folgen weiter `AMBIENT_UI_Inhalte.pdf`, 20.10.2026;
die dortige genaue Anordnung ist noch nicht freigegeben.

## Bereits ergänztes Exportwerkzeug

`firmware-c-next/tools/prepare_ui_asset.py` prüft die exakte PNG-Größe und
erzeugt C-Header, rohe Pixeldaten, Manifest mit Hashes und eine aus diesen
Bytes zurückgerechnete Vorschau. Es skaliert oder beschneidet nichts.

```sh
# Opaques Farbbild für den späteren RGB565-Pfad, CPU-Speicherreihenfolge:
python3 tools/prepare_ui_asset.py world_background.png build-ui-assets --name world_background --format rgb565le --size 320 170

# Transparente 24x24-Iconmaske, pro Zeile links das hohe Nibble:
python3 tools/prepare_ui_asset.py icon_hold.png build-ui-assets --name icon_hold --format alpha4 --size 24 24
```

`rgb565be` erzeugt stattdessen die explizite MSB-zuerst-Reihenfolge für einen
passenden SPI-Pfad. Rot ist dann `F8 00`, Grün `07 E0`, Blau `00 1F`.
CPU-Asset und Paneltransfer nicht beide vertauschen. `gray4` ist für opake
Graubilder; Farbbilder werden dort absichtlich abgewiesen. `alpha4` enthält
nur Deckung und braucht beim Zeichnen eine Farbe sowie echtes Compositing.

Die `.raw`-Datei besitzt **keinen** LVGL-Binärheader. Der C-Header ist
rendererneutral und benötigt einen passenden Blitter beziehungsweise
LVGL-Descriptor. Dafür bei der LVGL-Integration den offiziellen Converter
der dann festgelegten Version verwenden. Aktueller Export und geplantes
LVGL-Asset werden nicht als formatgleich ausgegeben. Ebenso rendert der
heutige AMBIENT-Code diese neuen Assets noch nicht automatisch.

Geprüft: Rot/Grün/Blau und Bytefolge, komplette 320×170-Puffergrößen, ungerade
Maskenbreiten mit Zeilenpadding, Alpha sowie falsche Farbe/Transparenz.
Die generierten Testheader werden außerdem als C kompiliert. Das ist ein
Offline-Dateibeleg, keine Display-/Framerate-/Geräteabnahme.

## Dokumentation und nächster UI-Schritt

Die offiziellen LVGL-9.5-Unterlagen bestätigen Bild-C-Dateien mit Descriptor,
den Offline-Bildconverter sowie den separaten Fontconverter. Die Version
für AMBIENT muss vor einer echten Integration festgeschrieben werden.

- [Bilddaten und Konvertierung](https://lvgl.io/docs/open/9.5/main-modules/images/adding_images)
- [Schriften und eigene Font-Dateien](https://lvgl.io/docs/open/9.5/main-modules/fonts/built_in_fonts)
- [RGB565 und Byte-Reihenfolge](https://lvgl.io/docs/open/9.5/main-modules/display/color_format)
- [Display-Anbindung](https://lvgl.io/docs/open/9.5/main-modules/display/setup.html)

Nächster sinnvoller UI-Schritt: **einen** Parameter-Screen mit Originalbild,
separatem Bogen und live gebundenem Wert in einer festgelegten LVGL-Version
am Host prüfen; danach den H743-Rechtecktransfer anschließen. Die aktuelle
Arbeit liefert dafür die geprüfte Dateipipeline und die Quellenanalyse.
