// SPDX-License-Identifier: MIT
// Copyright (c) 2026 DocBigs-Lab
// Treiber fuer das 2,0" Vierfarb-Panel (Schwarz/Weiss/Gelb/Rot) aus Solum-
// Preisschildern (EL020L8C4C, Flexkabel FPC-8161), 200 x 152 Pixel.
// Ablauf nach dem Solum-Treiber von OpenEPaperLink (Controller-Typ 0x0D).
// Details: doc/panel-2.0-zoll.md
#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <SPI.h>

#include "SolumCommon.h"

class Epd20 : public Adafruit_GFX {
public:
    static constexpr int16_t  WIDTH_PX  = 200;   // Querformat, Flexkabel unten
    static constexpr int16_t  HEIGHT_PX = 152;
    static constexpr uint32_t BUFFER_BYTES = (uint32_t)WIDTH_PX * HEIGHT_PX / 4;   // 2 Bit pro Pixel: 7600

    explicit Epd20(const EpdPins &pins = EpdPins());
    ~Epd20();

    // Speicher anlegen, Pins und SPI einrichten. false, wenn kein Speicher frei ist.
    bool begin();

    // Antwort auf Befehl 0x70 (24 Bit). Erwartet: 0x480101.
    uint32_t readChipId();

    // Puffer an das Panel senden und Refresh ausfuehren (ca. 22 s). false bei BUSY-Timeout.
    bool display();

    // Dauer des letzten Refresh in ms.
    uint32_t lastRefreshMs() const { return _lastRefreshMs; }

    // Adafruit_GFX
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillScreen(uint16_t color) override;

private:
    void sendCommand(uint8_t index, const uint8_t *data, uint32_t len);
    void sendCommand(uint8_t index, std::initializer_list<uint8_t> data);
    bool waitWhileBusy(uint32_t timeoutMs);
    void hardwareReset();

    EpdPins _pins;
    SPIClass *_spi = &SPI;
    SPISettings _spiSettings{4000000, MSBFIRST, SPI_MODE0};
    uint8_t *_buf = nullptr;   // 2 Bit pro Pixel im Panelformat
    uint32_t _lastRefreshMs = 0;
};
