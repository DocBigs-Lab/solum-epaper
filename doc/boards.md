# Boards

Das Solum-Panel hat ein 24-poliges Flexkabel mit derselben Belegung wie die SES-imagotag-Panels aus [SesEpaper](https://github.com/DocBigs-Lab/ses-imagotag-bwr-epaper) und braucht eine Hochspannungsschaltung auf dem Board (MOSFET an `NGDRV`, Strommessung an `RESE`). Die beiden Boards unten bringen das mit. Das Flexkabel steckt direkt in der Buchse, ohne Verlängerung.

## Flexkabel (Panelseite)

| FPC-Pin | Signal | Funktion |
|---|---|---|
| 2 | NGDRV | Gate des MOSFET im Hochspannungswandler |
| 3 | RESE | Strommessung des Wandlers |
| 9 | BUSY | LOW = beschäftigt, HIGH = bereit |
| 10 | RST | Reset, aktiv LOW |
| 11 | DC | LOW = Befehl, HIGH = Daten |
| 12 | CS | Chip Select |
| 13 | SCL | SPI-Takt |
| 14 | SDA | SPI-Daten, bidirektional (Lesen nur per Bit-Bang) |
| 15, 16 | VDDIO, VDD | 3,3 V |
| 17 | VSS | GND |
| 20–24 | VDH, VGH, VDL, VGL, VCOM | Kondensatoren für die erzeugten Spannungen |

Quelle: Datenblatt Pervasive E2741JS0B2, Abschnitt 5. Für das Solum-Panel gibt es kein Datenblatt; es läuft an beiden Boards mit dieser Belegung.

## Waveshare e-Paper ESP32 Driver Board (SKU 15823)

| Schalter | Stellung | Bedeutung |
|---|---|---|
| 1 (A/B) | **B** | Messwiderstand des Wandlers: B = 0,47 Ω (große Panels), A = 3 Ω |
| 2 (ON/OFF) | **ON** | Versorgung des USB-Seriell-Chips. Bei OFF erkennt der Rechner das Board nicht |

| Signal | GPIO |
|---|---|
| SCK | 13 |
| DIN | 14 |
| CS | 15 |
| DC | 27 |
| RST | 26 |
| BUSY | 25 |

Im Code: `EpdPins` mit Standardwerten, also z. B. `Epd20 epd;`. PlatformIO-Umgebung `esp32dev` (offizielle Plattform `espressif32`).

## Seengreat ESP32-C6 E-Paper Driver Board

| Schalter | Stellung | Bedeutung |
|---|---|---|
| S1 | **ON** | 4-Draht-SPI (OFF = 3-Draht) |
| S2 | **ON** | Messwiderstand des Wandlers 0,47 Ω (OFF = 3 Ω) |

| Signal | GPIO |
|---|---|
| SCK | 5 |
| DIN | 4 |
| CS | 7 |
| DC | 3 |
| RST | 6 |
| BUSY | 2 |

Weitere Anschlüsse: Akku-Spannung an GPIO 0, LED an GPIO 8.

Im Code: `epdPinsSeengreatC6()`, also z. B. `Epd20 epd(epdPinsSeengreatC6());`. PlatformIO-Umgebung `esp32c6`. Der C6 braucht Arduino-Core 3.x, deshalb die Plattform von [pioarduino](https://github.com/pioarduino/platform-espressif32/releases) und die Flags `-DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1` für die serielle Ausgabe über USB.

Beim Neustart trennt sich die USB-Verbindung kurz. Die ersten Zeilen im Monitor können dabei fehlen.

### Schlafstrom (Akkubetrieb)

Im Auslieferungszustand braucht das Board im Deep Sleep etwa 3,2 mA, ein 3000-mAh-Akku hält damit nur 5–6 Wochen. Drei Bauteile verursachen fast alles:

| Bauteil | Strom | Abhilfe |
|---|---|---|
| PWR-LED (Vorwiderstand R8) | ~1,1 mA | auslöten |
| R12, Pull-up an BS (S1 = ON legt BS auf Masse) | ~0,8–1,2 mA | auslöten, S1 bleibt ON |
| R2, Pull-up an BUSY (das Panel hält BUSY im Schlaf LOW) | ~1,2 mA | auslöten, die Firmware schaltet dann den internen Pull-up an BUSY ein. **Erst flashen, dann auslöten** |

Mit allen drei Änderungen und festen Pegeln an CS, RST, DC und LED im Schlaf: etwa 70 µA (gemessen mit dem 4,2"-Panel aus SesEpaper). Messungen, Fotos der Bauteile und Laufzeiten: [Hardware-Doku des Wetter-Kalenders](https://github.com/DocBigs-Lab/esp32c6-epaper-weather-calendar/blob/main/docs/hardware.md).

Die Klasse `Epd20` setzt BUSY als normalen Eingang. Nach dem Auslöten von R2 muss der Pull-up in der eigenen Firmware gesetzt werden (`pinMode(2, INPUT_PULLUP)` nach `begin()`).

## Schalter umstellen

Nur bei abgezogenem USB.

## Andere Boards

Jedes Board mit 24-poliger Buchse und Hochspannungsschaltung für Pervasive-Panels sollte gehen. Die Pins lassen sich in `EpdPins` frei setzen:

```cpp
EpdPins pins;
pins.sck = 18; pins.din = 23; pins.cs = 5; pins.dc = 17; pins.rst = 16; pins.busy = 4;
Epd20 epd(pins);
```
