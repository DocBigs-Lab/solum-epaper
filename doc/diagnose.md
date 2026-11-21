# Diagnose: Panel erkennen und prüfen

Die Diagnosefirmware in [extras/diagnose](../extras/diagnose/) zeigt, ob ein Solum-Panel antwortet, ob es das bekannte 2,0"-BWRY ist und ob es noch ein sauberes Bild zeigt. Sie läuft ohne die Bibliothek und wird über den seriellen Monitor mit 115200 Baud bedient, eine Taste pro Befehl.

## 1. Flashen

| Board | Schalter | fertige Firmware (Adresse `0x0`) | PlatformIO |
|---|---|---|---|
| Waveshare e-Paper ESP32 Driver Board | 1 = **B**, 2 = **ON** | `bin/diagnose-waveshare-esp32.bin` | `pio run -e esp32dev -t upload` |
| Seengreat ESP32-C6 E-Paper Driver Board | S1 = **ON**, S2 = **ON** | `bin/diagnose-seengreat-esp32c6.bin` | `pio run -e esp32c6 -t upload` |

Ohne Entwicklungsumgebung geht es im Browser (Chrome, Edge) mit [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/):

1. **Connect**, Port wählen.
2. **Flash** → **Flash Firmware**, passende `.bin` laden, Offset `0x0`, flashen.
3. **Console** öffnen, 115200 Baud.
4. Befehle als einzelne Zeichen senden, zuerst `h` und `i`.

Verliert der C6 beim Öffnen der Konsole die Verbindung: **Disconnect**, USB kurz abziehen und ein einfaches Terminal nehmen, z. B. [Serial Terminal](https://googlechromelabs.github.io/serial-terminal/).

## 2. Panel erkennen (nur lesend)

| Taste | Funktion |
|---|---|
| `i` | Chip-ID (`0x70`) und Temperatur (`0x40`) |
| `b` | BUSY-Leitung prüfen |
| `x` | Lesescan ausgewählter Befehle |

`48 01 01` ist das 2,0"-BWRY. Eine andere Antwort heißt: anderes Panel, Ausgabe von `i`, `b` und `x` sichern und erst dann schreibende Befehle probieren.

## 3. Testbilder

| Taste | Bild |
|---|---|
| `t` | vier Balken: schwarz, weiß, gelb, rot |
| `l` | Lineal: schwarz alle 10 px, rot alle 50 px, gelbes Quadrat unten links |
| `w`, `s`, `r`, `y` | ganze Fläche weiß, schwarz, rot, gelb |
| `c` | Reinigen: 2 Runden schwarz, rot, gelb, weiß (ca. 3 Minuten) |

Ein Bild dauert etwa 23 Sekunden. Bleibt nach dem Flackern das alte Bild stehen, hat der Controller die herstellerspezifischen Befehle nicht angenommen, siehe [panel-2.0-zoll.md](panel-2.0-zoll.md#befehlsfolge).

## Neue .bin bauen

Nach Änderungen an `extras/diagnose/src/main.cpp`: `extras/diagnose/make-bin.sh`, dann die beiden `.bin` mit committen.
