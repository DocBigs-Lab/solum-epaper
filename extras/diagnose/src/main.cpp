/* =========================================================================
 * Diagnosefirmware fuer E-Paper-Panels aus Solum-Preisschildern
 *
 *   2,0" BWRY  EL020L8C4C, FPC-8161  Chip-ID 48 01 01  200 x 152, 2 Bit/Pixel
 *
 * Boards: Waveshare e-Paper ESP32 Driver Board (Umgebung esp32dev) und
 * Seengreat ESP32-C6 E-Paper Driver Board (Umgebung esp32c6).
 *
 * Eigenstaendig, nutzt die Bibliothek nicht. Bedienung ueber den seriellen
 * Monitor (115200 Baud), h zeigt das Menue. Unbekanntes Panel: zuerst nur
 * i, b und x senden, die lesen nur.
 * Anleitung: doc/diagnose.md, Panel: doc/panel-2.0-zoll.md
 * ========================================================================= */
#include <Arduino.h>

#if defined(BOARD_SEENGREAT_C6)
// Seengreat ESP32-C6 E-Paper Driver Board, S1 = ON (4-Draht), S2 = ON (0,47 Ohm)
#define PIN_SCK   5
#define PIN_DIN   4
#define PIN_CS    7
#define PIN_DC    3
#define PIN_RST   6
#define PIN_BUSY  2
#else
// Waveshare e-Paper ESP32 Driver Board, Schalter 1 = B, 2 = ON
#define PIN_SCK   13
#define PIN_DIN   14
#define PIN_CS    15
#define PIN_DC    27
#define PIN_RST   26
#define PIN_BUSY  25
#endif

// ---------- Bit-Bang SPI ----------------------------------------------------
static void bbByte(uint8_t v) {
    for (int i = 7; i >= 0; i--) {
        digitalWrite(PIN_SCK, LOW);
        digitalWrite(PIN_DIN, (v >> i) & 1);
        delayMicroseconds(1);
        digitalWrite(PIN_SCK, HIGH);
        delayMicroseconds(1);
    }
    digitalWrite(PIN_SCK, LOW);
}

static void sendIndex(uint8_t idx) {
    digitalWrite(PIN_DC, LOW);
    digitalWrite(PIN_CS, LOW);
    bbByte(idx);
    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_DC, HIGH);
}
static void sendIndexData(uint8_t idx, const uint8_t *d, int n) {
    sendIndex(idx);
    digitalWrite(PIN_CS, LOW);
    for (int i = 0; i < n; i++) bbByte(d[i]);
    digitalWrite(PIN_CS, HIGH);
}
static void sendIndex1(uint8_t idx, uint8_t v) { sendIndexData(idx, &v, 1); }

// BUSY low-aktiv: warten bis HIGH. Gibt Dauer aus, false bei Timeout.
static bool waitBusy(const char *what, uint32_t timeoutMs) {
    uint32_t t0 = millis();
    delay(1);
    while (digitalRead(PIN_BUSY) == LOW) {
        if (millis() - t0 > timeoutMs) {
            Serial.printf("  %s: TIMEOUT nach %lu ms (BUSY bleibt LOW)\n", what, (unsigned long)timeoutMs);
            return false;
        }
        delay(5);
    }
    Serial.printf("  %s: %lu ms\n", what, (unsigned long)(millis() - t0));
    return true;
}

static void hwReset(void) {
    digitalWrite(PIN_RST, HIGH); delay(5);
    digitalWrite(PIN_RST, LOW);  delay(10);
    digitalWrite(PIN_RST, HIGH); delay(10);
}

// ---------- Lesen ---------------------------------------------------------------
// Liest n Bytes nach cmd mit gegebenem Pull; zaehlt BUSY-LOW waehrend 50 ms danach
static void rd(uint8_t cmd, uint8_t pull, uint8_t *buf, int n, int *busyLow) {
    hwReset();
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_DC, LOW); bbByte(cmd);
    digitalWrite(PIN_DC, HIGH);
    for (int i = 0; i < n; i++) {
        pinMode(PIN_DIN, pull); delayMicroseconds(2);
        uint8_t v = 0;
        for (int b = 7; b >= 0; b--) {
            digitalWrite(PIN_SCK, LOW);  delayMicroseconds(1);
            digitalWrite(PIN_SCK, HIGH); delayMicroseconds(1);
            v = (uint8_t)((v << 1) | digitalRead(PIN_DIN));
        }
        digitalWrite(PIN_SCK, LOW);
        buf[i] = v;
    }
    pinMode(PIN_DIN, OUTPUT);
    digitalWrite(PIN_CS, HIGH);
    int low = 0;
    for (int i = 0; i < 500; i++) { if (!digitalRead(PIN_BUSY)) low++; delayMicroseconds(100); }
    if (busyLow) *busyLow = low;
}

static void readId(void) {
    uint8_t b[3];
    rd(0x70, INPUT_PULLUP, b, 3, nullptr);
    const char *panel = (b[0] == 0x48 && b[1] == 0x01 && b[2] == 0x01) ? "2,0\" BWRY EL020L8C4C (200 x 152)"
                                                                      : "unbekannt";
    Serial.printf("ID 0x70: %02X %02X %02X -> %s\n", b[0], b[1], b[2], panel);
    uint8_t t;
    rd(0x40, INPUT_PULLUP, &t, 1, nullptr);
    Serial.printf("Temperatur 0x40: %02X = %d Grad C\n", t, (int8_t)t);
}

static void busyDiag(void) {
    Serial.println("\n--- BUSY-Diagnose ---");
    hwReset();
    delay(200);   // Reset-Phase abwarten, sonst faellt die Pruefung in den BUSY-Wechsel
    pinMode(PIN_BUSY, INPUT_PULLUP);   delay(5); int up = digitalRead(PIN_BUSY);
    pinMode(PIN_BUSY, INPUT_PULLDOWN); delay(5); int dn = digitalRead(PIN_BUSY);
    Serial.printf("  Ruhe: Pull-up=%d Pull-down=%d -> %s\n", up, dn,
                  up == dn ? "getrieben" : "OFFEN (Pin nicht verbunden?)");
    digitalWrite(PIN_RST, LOW);  delay(10);
    digitalWrite(PIN_RST, HIGH);
    String s;
    for (int i = 0; i < 50; i++) { s += digitalRead(PIN_BUSY) ? '1' : '0'; delayMicroseconds(200); }
    s += '|';
    for (int i = 0; i < 200; i++) { s += digitalRead(PIN_BUSY) ? '1' : '0'; delayMicroseconds(500); }
    Serial.printf("  Verlauf (Pull-down, Reset 10 ms | danach 100 ms):\n  %s\n", s.c_str());
    pinMode(PIN_BUSY, INPUT);
}

// Nur Lese-/Abfragebefehle. Ausgelassen: Power ON, Refresh, Deep Sleep, OTP-Schreiben.
static void scanReads(void) {
    const uint8_t cmds[] = {0x40, 0x43, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
                            0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x81, 0x82,
                            0x92, 0xB9, 0x1B, 0x2D, 0x2E, 0x2F, 0xF0, 0xF1};
    Serial.println("\n--- Lese-Scan (nur getriebene Antworten, 8 Byte) ---");
    uint8_t up[8], dn[8];
    for (uint8_t c : cmds) {
        int bl1, bl2;
        rd(c, INPUT_PULLUP, up, 8, &bl1);
        rd(c, INPUT_PULLDOWN, dn, 8, &bl2);
        int drv = 0;
        for (int i = 0; i < 8; i++) if (up[i] == dn[i]) drv++;
        if (drv == 0 && bl1 == 0) continue;
        Serial.printf("  %02X: ", c);
        for (int i = 0; i < 8; i++) {
            if (up[i] == dn[i]) Serial.printf("%02X ", up[i]); else Serial.print("-- ");
        }
        Serial.printf(" BUSY-low %d/500\n", bl1);
    }
    Serial.println("--- Scan fertig ---");
}

// ---------- Bilder ------------------------------------------------------------
// Panelformat: 152 Zeilen zu 200 Pixeln, 2 Bit pro Pixel, Zeile 0 unten am Flexkabel.
// Farbcodes: 00 schwarz, 01 weiss, 10 gelb, 11 rot.
static const int W = 200, H = 152;
enum { C_BLACK = 0, C_WHITE = 1, C_YELLOW = 2, C_RED = 3 };
enum Pattern { P_BARS, P_RULER, P_FILL };
static Pattern g_pattern = P_BARS;
static uint8_t g_fill = C_WHITE;

// x, y im Panelformat (y = 0 unten)
static uint8_t pixel(int x, int y) {
    switch (g_pattern) {
        case P_BARS:   // vier Balken, weisse Linie alle 50 Zeilen
            if (y % 50 == 0) return C_WHITE;
            return (uint8_t)(x * 4 / W == 0 ? C_BLACK : x * 4 / W == 1 ? C_WHITE : x * 4 / W == 2 ? C_YELLOW : C_RED);
        case P_RULER:  // schwarz alle 10 px, rot alle 50 px, gelbes Quadrat am Datenanfang (unten links)
            if (x < 24 && y < 24) return C_YELLOW;
            if (x % 50 == 0 || y % 50 == 0) return C_RED;
            if (x % 10 == 0 || y % 10 == 0) return C_BLACK;
            return C_WHITE;
        default:
            return g_fill;
    }
}

static bool drawPanel(const char *name) {
    Serial.printf("\n--- %s ---\n", name);
    hwReset();
    if (!waitBusy("nach HW-Reset", 2000)) return false;
    sendIndex1(0x4D, 0x55);                       // herstellerspezifisch (Solum)
    sendIndex1(0xF3, 0x0A);
    sendIndex1(0x31, 0x00);
    const uint8_t bst[3] = {0xE5, 0x35, 0x3C};
    sendIndexData(0x06, bst, 3);                  // Booster
    sendIndex1(0x50, 0x57);                       // Rand/VCOM-Intervall
    const uint8_t psr[3] = {0x07, 0x09, 0x00};    // PSR + Dummy-Byte
    sendIndexData(0x00, psr, 3);
    uint32_t t0 = millis();
    sendIndex(0x10);
    digitalWrite(PIN_CS, LOW);
    for (int y = 0; y < H; y++) {
        for (int xb = 0; xb < W / 4; xb++) {
            uint8_t v = 0;
            for (int k = 0; k < 4; k++) v = (uint8_t)((v << 2) | pixel(xb * 4 + k, y));
            bbByte(v);
        }
    }
    digitalWrite(PIN_CS, HIGH);
    Serial.printf("  Daten gesendet: %lu ms\n", (unsigned long)(millis() - t0));
    sendIndex(0x04);                              // Power ON
    bool pon = waitBusy("Power ON", 5000);
    sendIndex(0x12);                              // Refresh
    bool ok = waitBusy("Refresh", 60000);
    sendIndex(0x02);                              // Power OFF
    waitBusy("Power OFF", 5000);
    Serial.println(pon && ok ? "Fertig." : "Ablauf nicht sauber (siehe Zeiten).");
    return pon && ok;
}

static void fill(uint8_t c, const char *name) {
    g_pattern = P_FILL;
    g_fill = c;
    drawPanel(name);
}

static void clean(void) {
    Serial.println("\n=== Reinigen: 2 Runden schwarz, rot, gelb, weiss ===");
    const uint8_t seq[4] = {C_BLACK, C_RED, C_YELLOW, C_WHITE};
    g_pattern = P_FILL;
    for (int r = 0; r < 2; r++) {
        for (uint8_t c : seq) {
            g_fill = c;
            if (!drawPanel("Reinigen")) return;
        }
    }
}

static void menu(void) {
    Serial.println();
    Serial.println("--- Panel erkennen (nur lesend) ---");
    Serial.println("i = Chip-ID (0x70) und Temperatur (0x40), 48 01 01 = Solum 2,0\" BWRY");
    Serial.println("b = BUSY-Diagnose");
    Serial.println("x = Lesescan ausgewaehlter Befehle");
    Serial.println("--- 2,0\" BWRY (200 x 152) ---");
    Serial.println("t = Testbild: Balken schwarz, weiss, gelb, rot");
    Serial.println("l = Lineal: schwarz alle 10 px, rot alle 50 px, gelbes Quadrat unten links");
    Serial.println("w, s, r, y = ganze Flaeche weiss, schwarz, rot, gelb");
    Serial.println("c = Reinigen: 2 Runden schwarz, rot, gelb, weiss (ca. 3 Minuten)");
    Serial.println("h = dieses Menue");
}

void setup(void) {
    Serial.begin(115200);
    pinMode(PIN_CS, OUTPUT);   digitalWrite(PIN_CS, HIGH);
    pinMode(PIN_DC, OUTPUT);   digitalWrite(PIN_DC, HIGH);
    pinMode(PIN_RST, OUTPUT);  digitalWrite(PIN_RST, HIGH);
    pinMode(PIN_BUSY, INPUT);
    pinMode(PIN_DIN, OUTPUT);  digitalWrite(PIN_DIN, LOW);
    pinMode(PIN_SCK, OUTPUT);  digitalWrite(PIN_SCK, LOW);
    delay(1000);
    Serial.println("\n=== Diagnose Solum E-Paper ===");
    readId();
    menu();
}

void loop(void) {
    if (!Serial.available()) return;
    switch ((char)Serial.read()) {
        case 'i': readId(); break;
        case 'b': busyDiag(); break;
        case 'x': scanReads(); break;
        case 't': g_pattern = P_BARS;  drawPanel("Testbild: Balken"); menu(); break;
        case 'l': g_pattern = P_RULER; drawPanel("Lineal"); menu(); break;
        case 'w': fill(C_WHITE, "Flaeche weiss"); menu(); break;
        case 's': fill(C_BLACK, "Flaeche schwarz"); menu(); break;
        case 'r': fill(C_RED, "Flaeche rot"); menu(); break;
        case 'y': fill(C_YELLOW, "Flaeche gelb"); menu(); break;
        case 'c': clean(); menu(); break;
        case 'h': case '?': menu(); break;
        default: break;
    }
}
