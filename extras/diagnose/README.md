# Diagnosefirmware

Erkennt Solum-Panels, liest Chip-ID und Temperatur, zeichnet Testbilder und reinigt das Panel. Bedienung über den seriellen Monitor mit 115200 Baud.

Anleitung: [doc/diagnose.md](../../doc/diagnose.md)

| Board | fertige Firmware (Adresse `0x0`) | PlatformIO |
|---|---|---|
| Waveshare e-Paper ESP32 Driver Board | [bin/diagnose-waveshare-esp32.bin](bin/diagnose-waveshare-esp32.bin) | `pio run -e esp32dev -t upload` |
| Seengreat ESP32-C6 E-Paper Driver Board | [bin/diagnose-seengreat-esp32c6.bin](bin/diagnose-seengreat-esp32c6.bin) | `pio run -e esp32c6 -t upload` |

Die `.bin` lassen sich ohne Entwicklungsumgebung im Browser flashen, z. B. mit [ESPConnect](https://thelastoutpostworkshop.github.io/ESPConnect/). Nach Änderungen an `src/main.cpp` erzeugt `./make-bin.sh` beide Dateien neu.
