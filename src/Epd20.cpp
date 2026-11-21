// SPDX-License-Identifier: MIT
// Copyright (c) 2026 DocBigs-Lab
// Ablauf nach OpenEPaperLink Tag_FW_nRF52811, uc_variant_029.cpp (Solum-Controller 0x0D).
#include "Epd20.h"

// Panelformat: 152 Zeilen zu je 200 Pixeln, 2 Bit pro Pixel (50 Byte pro Zeile),
// MSB = erstes Pixel. Die erste Zeile liegt unten (am Flexkabel), das erste Pixel links.
// Farbcodes: 00 schwarz, 01 weiss, 10 gelb, 11 rot.
static constexpr uint32_t LINE_BYTES = Epd20::WIDTH_PX / 4;

static uint8_t colorCode(uint16_t color) {
    switch (color) {
        case EPD_BLACK:  return 0;
        case EPD_YELLOW: return 2;
        case EPD_RED:    return 3;
        default:         return 1;   // weiss
    }
}

Epd20::Epd20(const EpdPins &pins) : Adafruit_GFX(WIDTH_PX, HEIGHT_PX), _pins(pins) {}

Epd20::~Epd20() {
    free(_buf);
}

bool Epd20::begin() {
    if (!_buf) _buf = (uint8_t *)malloc(BUFFER_BYTES);
    if (!_buf) return false;
    fillScreen(EPD_WHITE);

    pinMode(_pins.busy, INPUT);
    pinMode(_pins.rst, OUTPUT);
    digitalWrite(_pins.rst, HIGH);
    pinMode(_pins.dc, OUTPUT);
    digitalWrite(_pins.dc, HIGH);
    pinMode(_pins.cs, OUTPUT);
    digitalWrite(_pins.cs, HIGH);
    _spi->begin(_pins.sck, -1, _pins.din, -1);
    return true;
}

// ---------- Zeichnen --------------------------------------------------------

void Epd20::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || y < 0 || x >= width() || y >= height()) return;
    int16_t t;
    switch (getRotation()) {
        case 1: t = x; x = WIDTH_PX - 1 - y; y = t; break;
        case 2: x = WIDTH_PX - 1 - x; y = HEIGHT_PX - 1 - y; break;
        case 3: t = x; x = y; y = HEIGHT_PX - 1 - t; break;
        default: break;
    }
    uint32_t line = (uint32_t)(HEIGHT_PX - 1 - y);    // Zeile 0 liegt unten
    uint32_t i = line * LINE_BYTES + (x >> 2);
    uint8_t shift = (uint8_t)(6 - 2 * (x & 3));
    _buf[i] = (uint8_t)((_buf[i] & ~(0x03 << shift)) | (colorCode(color) << shift));
}

void Epd20::fillScreen(uint16_t color) {
    uint8_t c = colorCode(color);
    memset(_buf, c << 6 | c << 4 | c << 2 | c, BUFFER_BYTES);
}

// ---------- SPI ---------------------------------------------------------------

void Epd20::sendCommand(uint8_t index, const uint8_t *data, uint32_t len) {
    _spi->beginTransaction(_spiSettings);
    digitalWrite(_pins.dc, LOW);
    digitalWrite(_pins.cs, LOW);
    _spi->transfer(index);
    digitalWrite(_pins.cs, HIGH);
    digitalWrite(_pins.dc, HIGH);
    if (len) {
        digitalWrite(_pins.cs, LOW);
        _spi->writeBytes(data, len);
        digitalWrite(_pins.cs, HIGH);
    }
    _spi->endTransaction();
}

void Epd20::sendCommand(uint8_t index, std::initializer_list<uint8_t> data) {
    sendCommand(index, data.begin(), data.size());
}

bool Epd20::waitWhileBusy(uint32_t timeoutMs) {
    uint32_t t0 = millis();
    while (digitalRead(_pins.busy) == LOW) {   // LOW = beschaeftigt
        if (millis() - t0 > timeoutMs) return false;
        delay(10);
    }
    return true;
}

void Epd20::hardwareReset() {
    digitalWrite(_pins.rst, HIGH); delay(5);
    digitalWrite(_pins.rst, LOW);  delay(10);
    digitalWrite(_pins.rst, HIGH); delay(10);
}

// Lesen ueber die bidirektionale Datenleitung geht nur per Bit-Bang.
uint32_t Epd20::readChipId() {
    _spi->end();
    pinMode(_pins.sck, OUTPUT);
    pinMode(_pins.din, OUTPUT);
    digitalWrite(_pins.sck, LOW);
    hardwareReset();

    digitalWrite(_pins.cs, LOW);
    digitalWrite(_pins.dc, LOW);
    for (int b = 7; b >= 0; b--) {
        digitalWrite(_pins.sck, LOW);
        digitalWrite(_pins.din, (0x70 >> b) & 1);
        delayMicroseconds(1);
        digitalWrite(_pins.sck, HIGH);
        delayMicroseconds(1);
    }
    digitalWrite(_pins.sck, LOW);
    digitalWrite(_pins.dc, HIGH);
    pinMode(_pins.din, INPUT_PULLUP);
    delayMicroseconds(2);
    uint32_t id = 0;
    for (int i = 0; i < 24; i++) {
        digitalWrite(_pins.sck, LOW);  delayMicroseconds(1);
        digitalWrite(_pins.sck, HIGH); delayMicroseconds(1);
        id = (id << 1) | (uint32_t)digitalRead(_pins.din);
    }
    digitalWrite(_pins.sck, LOW);
    digitalWrite(_pins.cs, HIGH);
    pinMode(_pins.din, OUTPUT);

    _spi->begin(_pins.sck, -1, _pins.din, -1);
    return id;
}

// ---------- Bildaufbau ----------------------------------------------------------

bool Epd20::display() {
    hardwareReset();
    bool ok = waitWhileBusy(2000);
    sendCommand(0x4D, {0x55});                     // herstellerspezifisch (Solum)
    sendCommand(0xF3, {0x0A});                     // herstellerspezifisch
    sendCommand(0x31, {0x00});                     // herstellerspezifisch
    sendCommand(0x06, {0xE5, 0x35, 0x3C});         // Booster
    sendCommand(0x50, {0x57});                     // Rand/VCOM-Intervall
    sendCommand(0x00, {0x07, 0x09, 0x00});         // Panel-Einstellung + Dummy-Byte wie bei Solum
    sendCommand(0x10, _buf, BUFFER_BYTES);         // Bild, 2 Bit pro Pixel

    sendCommand(0x04, nullptr, 0);                 // Power ON
    ok = waitWhileBusy(5000) && ok;
    sendCommand(0x12, nullptr, 0);                 // Refresh
    delay(5);
    uint32_t t0 = millis();
    ok = waitWhileBusy(60000) && ok;               // ca. 22 s
    _lastRefreshMs = millis() - t0;

    sendCommand(0x02, nullptr, 0);                 // Power OFF
    waitWhileBusy(5000);
    return ok;
}
