#include <M5GFX.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <M5PM1.h>
#include "config.h"
#include "settings.h"
#include "pet.h"
#include "pet_server.h"

// Global objects
M5GFX* lcd = nullptr;
M5PM1 pm1;
PixelPet pet;
PetServer server(pet, HTTP_PORT);

// Battery state
static int batteryPct = 0;
static bool batteryOk = false;

// Button pins (StickS3: KEY1=G11, KEY2=G12)
#define BTN_A_PIN 11
#define BTN_B_PIN 12

// Display modes
enum DisplayMode {
    MODE_PET,
    MODE_STATUS
};

static DisplayMode currentMode = MODE_PET;
bool inAPMode = false;
static uint32_t lastBPress = 0;
static const uint32_t LONG_PRESS_MS = 1000;

static bool btnA_pressed = false;
static bool btnB_released = false;

void readButtons() {
    static bool lastA = false, lastB = false;
    bool a = (digitalRead(BTN_A_PIN) == LOW);
    bool b = (digitalRead(BTN_B_PIN) == LOW);
    if (a && !lastA) btnA_pressed = true;
    if (!b && lastB) btnB_released = true;
    lastA = a;
    lastB = b;
}

void readBattery() {
    uint16_t vbat = 0;
    if (pm1.readVbat(&vbat) == M5PM1_OK) {
        int pct = (int)((vbat - 3000.0f) / (4200.0f - 3000.0f) * 100.0f);
        if (pct < 0) pct = 0;
        if (pct > 100) pct = 100;
        batteryPct = pct;
        batteryOk = true;
    }
}

// Status display
void drawStatusScreen() {
    lcd->fillScreen(TFT_BLACK);

    lcd->setTextDatum(TC_DATUM);
    lcd->setTextColor(0xFEA0, TFT_BLACK);
    lcd->setTextSize(2);
    lcd->drawString("Settings", LCD_WIDTH / 2, 10);

    lcd->setTextDatum(TL_DATUM);
    lcd->setTextSize(1);
    lcd->setTextColor(0xC618, TFT_BLACK);

    int y = 45;
    int lineH = 18;

    lcd->drawString("WiFi Status:", 4, y);
    y += lineH;
    if (WiFi.status() == WL_CONNECTED) {
        lcd->setTextColor(0x07E0, TFT_BLACK);
        lcd->drawString("Connected", 4, y);
    } else if (inAPMode) {
        lcd->setTextColor(0xFEA0, TFT_BLACK);
        lcd->drawString("AP Mode", 4, y);
    } else {
        lcd->setTextColor(0xF800, TFT_BLACK);
        lcd->drawString("Disconnected", 4, y);
    }
    y += lineH + 6;

    lcd->setTextColor(0xC618, TFT_BLACK);

    lcd->drawString("SSID:", 4, y);
    y += lineH;
    lcd->setTextColor(0x001F, TFT_BLACK);
    DeviceSettings& s = settingsGet();
    lcd->drawString(s.hasConfig ? s.wifiSSID : "(not set)", 4, y);
    y += lineH + 6;

    lcd->setTextColor(0xC618, TFT_BLACK);

    lcd->drawString("IP:", 4, y);
    y += lineH;
    lcd->setTextColor(0x001F, TFT_BLACK);
    if (WiFi.status() == WL_CONNECTED) {
        lcd->drawString(WiFi.localIP().toString(), 4, y);
    } else if (inAPMode) {
        lcd->drawString(WiFi.softAPIP().toString(), 4, y);
    } else {
        lcd->drawString("-", 4, y);
    }
    y += lineH + 6;

    lcd->setTextColor(0xC618, TFT_BLACK);

    lcd->drawString("Config:", 4, y);
    y += lineH;
    lcd->setTextColor(0x001F, TFT_BLACK);
    lcd->drawString(s.hasConfig ? "Saved" : "None", 4, y);
    y += lineH + 6;

    if (batteryOk) {
        lcd->setTextColor(0xC618, TFT_BLACK);
        lcd->drawString("Battery:", 4, y);
        y += lineH;
        lcd->setTextColor(0x07E0, TFT_BLACK);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", batteryPct);
        lcd->drawString(buf, 4, y);
        y += lineH + 6;
    }

    lcd->setTextColor(0x4208, TFT_BLACK);
    lcd->drawString("A: Back to pet", 4, y);
    y += lineH;
    lcd->drawString("Long B: Setup mode", 4, y);
    y += lineH;
    if (inAPMode) {
        lcd->drawString("Connect to:", 4, y);
        y += lineH;
        lcd->setTextColor(0xFEA0, TFT_BLACK);
        lcd->drawString("CursorPet-Setup", 4, y);
        y += lineH;
        lcd->setTextColor(0x001F, TFT_BLACK);
        lcd->drawString("Open 192.168.4.1", 4, y);
    }
}

// BSSID scanning for same-name APs
bool connectWiFi(const char* ssid, const char* password) {
    if (strlen(ssid) == 0) return false;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.printf("Scanning for %s...\n", ssid);
    int n = WiFi.scanNetworks();
    int bestIdx = -1;
    int bestRSSI = -999;

    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == ssid && WiFi.RSSI(i) > bestRSSI) {
            bestIdx = i;
            bestRSSI = WiFi.RSSI(i);
        }
    }

    if (bestIdx >= 0) {
        uint8_t bssid[6];
        memcpy(bssid, WiFi.BSSID(bestIdx), 6);
        WiFi.scanDelete();
        Serial.printf("Found %s (RSSI:%d), connecting...\n", ssid, bestRSSI);
        WiFi.begin(ssid, password, 0, bssid);
    } else {
        WiFi.scanDelete();
        Serial.printf("%s not found, trying without BSSID...\n", ssid);
        WiFi.begin(ssid, password);
    }

    for (int i = 0; i < 40; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
        delay(500);
    }
    Serial.println("WiFi connect failed");
    return false;
}

void startAPMode() {
    Serial.println("Starting AP mode...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("CursorPet-Setup", "12345678");
    delay(100);
    Serial.printf("AP started. IP: %s\n", WiFi.softAPIP().toString().c_str());
    inAPMode = true;
}

void setup() {
    Serial.begin(115200);

    // Init buttons
    pinMode(BTN_A_PIN, INPUT_PULLUP);
    pinMode(BTN_B_PIN, INPUT_PULLUP);

    // Enable EXT_5V_EN (GPIO38 = LCD BL + 5V power enable) BEFORE display init
    pinMode(38, OUTPUT);
    digitalWrite(38, HIGH);
    delay(100);

    // Init I2C and M5PM1 for battery reading
    Wire.begin(47, 48);
    if (pm1.begin(&Wire, M5PM1_DEFAULT_ADDR, 47, 48, M5PM1_I2C_FREQ_100K) == M5PM1_OK) {
        Serial.println("M5PM1 init OK");
        readBattery();
        pet.setBattery(batteryPct);
    } else {
        Serial.println("M5PM1 init failed");
    }

    // Init display
    lcd = new M5GFX();
    lcd->init();
    lcd->setRotation(0);
    lcd->fillScreen(TFT_BLACK);

    pet.begin(lcd);
    settingsLoad();

    DeviceSettings& s = settingsGet();
    bool connected = false;

    if (s.hasConfig) {
        connected = connectWiFi(s.wifiSSID, s.wifiPassword);
    }

    if (!connected) {
        startAPMode();
    }

    server.begin();
}

void loop() {
    readButtons();
    delay(10);

    // B button: short press = toggle display, long press = enter AP mode
    if (btnB_released) {
        uint32_t held = millis() - lastBPress;
        if (held >= LONG_PRESS_MS) {
            if (!inAPMode) {
                startAPMode();
                currentMode = MODE_STATUS;
                pet.setState(PET_SLEEP);
            }
        } else {
            if (currentMode == MODE_PET) {
                currentMode = MODE_STATUS;
                pet.setState(PET_SLEEP);
            } else {
                currentMode = MODE_PET;
            }
        }
        lastBPress = 0;
        btnB_released = false;
    }

    if (digitalRead(BTN_B_PIN) == LOW && lastBPress == 0) {
        lastBPress = millis();
    }

    // A button: back to pet mode from status
    if (btnA_pressed && currentMode == MODE_STATUS) {
        currentMode = MODE_PET;
    }
    btnA_pressed = false;

    // Update battery every 10 seconds
    static uint32_t lastBatteryRead = 0;
    if (millis() - lastBatteryRead >= 10000) {
        lastBatteryRead = millis();
        readBattery();
        pet.setBattery(batteryPct);
    }

    // Update display
    switch (currentMode) {
        case MODE_PET:
            server.handleClient();
            pet.tick();
            break;
        case MODE_STATUS:
            server.handleClient();
            drawStatusScreen();
            delay(200);
            break;
    }
}
