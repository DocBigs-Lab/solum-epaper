// SPDX-License-Identifier: MIT
// Copyright (c) 2026 DocBigs-Lab
// Gemeinsame Typen fuer die Solum-Panels.
#pragma once

#include <stdint.h>

// Farben fuer Adafruit GFX. Weiss, Schwarz und Rot haben dieselben Werte wie in SesEpaper.
enum : uint16_t {
    EPD_WHITE  = 0,
    EPD_BLACK  = 1,
    EPD_RED    = 2,
    EPD_YELLOW = 3,
};

// Pinbelegung. Standard: Waveshare e-Paper ESP32 Driver Board (SKU 15823).
struct EpdPins {
    int8_t sck  = 13;
    int8_t din  = 14;
    int8_t cs   = 15;
    int8_t dc   = 27;
    int8_t rst  = 26;
    int8_t busy = 25;
};

// Seengreat ESP32-C6 E-Paper Driver Board (S1 = ON, S2 = ON).
inline EpdPins epdPinsSeengreatC6() {
    EpdPins p;
    p.sck = 5; p.din = 4; p.cs = 7; p.dc = 3; p.rst = 6; p.busy = 2;
    return p;
}
