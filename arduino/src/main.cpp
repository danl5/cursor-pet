#include <M5Unified.h>
#include "config.h"
#include "pet.h"
#include "server.h"

// Global objects
TFT_eSPI lcd = TFT_eSPI();
PixelPet pet(lcd);
PetServer server(pet, HTTP_PORT);

// BSSID scanning for same-name APs
bool connectWiFi() {
    Serial.printf("Scanning for %s...\n", WIFI_SSID);
    int n = WiFi.scanNetworks();
    int bestIdx = -1;
    int bestRSSI = -999;

    for (int i = 0; i < n; i++) {
        if (WiFi.SSID(i) == WIFI_SSID && WiFi.RSSI(i) > bestRSSI) {
            bestIdx = i;
            bestRSSI = WiFi.RSSI(i);
        }
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    if (bestIdx >= 0) {
        Serial.printf("Found %s (RSSI:%d), connecting...\n", WIFI_SSID, bestRSSI);
        uint8_t* bssid = WiFi.BSSID(bestIdx);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD, 0, bssid);
    } else {
        Serial.printf("%s not found, trying without BSSID...\n", WIFI_SSID);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
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

void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    cfg.internal_rtc = false;
    cfg.internal_spk = false;
    cfg.internal_mic = false;
    M5.begin(cfg);

    lcd.init();
    lcd.setRotation(1);
    lcd.fillScreen(TFT_BLACK);

    pet.begin();
    connectWiFi();
    server.begin();
}

void loop() {
    M5.update();
    server.handleClient();
    pet.tick();
}
