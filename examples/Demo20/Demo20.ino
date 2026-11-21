// Demo: Epd20-Treiber mit Adafruit GFX auf dem Solum 2,0" BWRY (200 x 152).
// Das Bild wird einmal beim Start gezeichnet.
// Board: Waveshare e-Paper ESP32 Driver Board (Schalter 1 = B, 2 = ON) oder,
// mit -DBOARD_SEENGREAT_C6, Seengreat ESP32-C6 E-Paper Driver Board (S1 = ON, S2 = ON).
//
// Serieller Monitor (115200 Baud):
//   d = Demo neu zeichnen
//   c = Panel reinigen: 2 Runden schwarz, rot, gelb, weiss (ca. 3 Minuten)
#include <Arduino.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#include <Epd20.h>

#include "logo.h"

#if defined(BOARD_SEENGREAT_C6)
static Epd20 epd(epdPinsSeengreatC6());
#else
static Epd20 epd;
#endif

// DocBigsLab-Logo: Haus-Icon rot, Schriftzug schwarz
static void drawLogo(int16_t x0, int16_t y0) {
    epd.drawBitmap(x0 + LOGO_ICON_X, y0 + LOGO_ICON_Y, LOGO_ICON_BITS, LOGO_ICON_W, LOGO_ICON_H, EPD_RED);
    epd.drawBitmap(x0 + LOGO_TEXT_X, y0 + LOGO_TEXT_Y, LOGO_TEXT_BITS, LOGO_TEXT_W, LOGO_TEXT_H, EPD_BLACK);
}

static void drawDemo() {
    epd.fillScreen(EPD_WHITE);

    // Kopfzeile
    epd.fillRect(0, 0, 200, 26, EPD_RED);
    epd.setFont(&FreeSansBold9pt7b);
    epd.setTextColor(EPD_WHITE);
    epd.setCursor(8, 19);
    epd.print("Epd20");
    epd.setFont(&FreeSans9pt7b);
    epd.setCursor(78, 19);
    epd.print("Solum 2,0\"");

    // Logo
    drawLogo((200 - LOGO_W) / 2, 31);

    // Farbfelder
    const uint16_t colors[4] = {EPD_BLACK, EPD_WHITE, EPD_YELLOW, EPD_RED};
    for (int i = 0; i < 4; i++) {
        int16_t x = 8 + i * 47;
        epd.fillRect(x, 86, 41, 24, colors[i]);
        epd.drawRect(x, 86, 41, 24, EPD_BLACK);
    }

    // Formen
    epd.fillCircle(26, 131, 15, EPD_BLACK);
    epd.fillCircle(26, 131, 8, EPD_YELLOW);
    epd.fillTriangle(54, 146, 72, 116, 90, 146, EPD_RED);
    epd.fillRoundRect(100, 116, 92, 30, 7, EPD_YELLOW);
    epd.setTextColor(EPD_BLACK);
    epd.setCursor(126, 137);
    epd.print("Gelb!");

    // Rahmen
    epd.drawRect(0, 0, 200, 152, EPD_BLACK);
}

static void refresh(const char *what) {
    Serial.printf("%s ...\n", what);
    bool ok = epd.display();
    Serial.printf("%s %s, Refresh %lu ms\n", what, ok ? "fertig" : "BUSY-Timeout",
                  (unsigned long)epd.lastRefreshMs());
}

static void clean() {
    const uint16_t seq[4] = {EPD_BLACK, EPD_RED, EPD_YELLOW, EPD_WHITE};
    for (int round = 0; round < 2; round++) {
        for (uint16_t c : seq) {
            epd.fillScreen(c);
            refresh("Reinigen");
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Demo Epd20 (Solum 2,0\" BWRY) ===");
    if (!epd.begin()) {
        Serial.println("Kein Speicher frei.");
        return;
    }
    Serial.printf("Chip-ID 0x70: %06lX (erwartet 480101)\n", (unsigned long)epd.readChipId());
    drawDemo();
    refresh("Demo");
    Serial.println("d = Demo neu zeichnen, c = reinigen");
}

void loop() {
    if (!Serial.available()) return;
    switch (Serial.read()) {
        case 'd': drawDemo(); refresh("Demo"); break;
        case 'c': clean(); drawDemo(); refresh("Demo"); break;
        default: break;
    }
}
