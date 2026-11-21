#!/bin/sh
# Baut die Diagnosefirmware fuer beide Boards und fasst Bootloader,
# Partitionstabelle und App zu je einer .bin fuer Adresse 0x0 zusammen.
# Ergebnis: bin/diagnose-waveshare-esp32.bin, bin/diagnose-seengreat-esp32c6.bin
set -e
cd "$(dirname "$0")"

pio run -e esp32dev -e esp32c6

B=.pio/build
A0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
mkdir -p bin

esptool --chip esp32 merge-bin -o bin/diagnose-waveshare-esp32.bin \
    0x1000 $B/esp32dev/bootloader.bin 0x8000 $B/esp32dev/partitions.bin \
    0xe000 "$A0" 0x10000 $B/esp32dev/firmware.bin

esptool --chip esp32c6 merge-bin -o bin/diagnose-seengreat-esp32c6.bin \
    0x0 $B/esp32c6/bootloader.bin 0x8000 $B/esp32c6/partitions.bin \
    0xe000 "$A0" 0x10000 $B/esp32c6/firmware.bin
