#include <M5GFX.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "settings.h"
#include "pet.h"
#include "pet_server.h"

M5GFX* lcd = nullptr;
PixelPet pet;
PetServer server(pet, HTTP_PORT);

static int batteryPct = 0;
static bool batteryOk = false;

#define BTN_A_PIN 11
#define BTN_B_PIN 12

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
    int pct = M5.Power.getBatteryLevel();
    if (pct >= 0) {
        batteryPct = pct;
        batteryOk = true;
    } else {
        batteryOk = false;
    }
}

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

    lcd->setTextColor(0xC618, TFT_BLACK);
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "Streak: %d day(s)", pet.getStreakCount());
    lcd->drawString(sbuf, 4, y);
    y += lineH + 6;

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

bool connectWiFi(const char* ssid, const char* password, const char* identity, LovyanGFX* display) {
    if (strlen(ssid) == 0) return false;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    PixelPet::drawBootScreen(display, "Scanning WiFi...", 20);

    if (strlen(identity) > 0) {
        Serial.printf("Enterprise WiFi: %s (identity: %s)\n", ssid, identity);
        WiFi.begin(ssid, WPA2_AUTH_PEAP, identity, identity, password);
    } else {
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
            PixelPet::drawBootScreen(display, "Connecting...", 50);
            WiFi.begin(ssid, password, 0, bssid);
        } else {
            WiFi.scanDelete();
            Serial.printf("%s not found, trying without BSSID...\n", ssid);
            PixelPet::drawBootScreen(display, "Connecting...", 50);
            WiFi.begin(ssid, password);
        }
    }

    for (int i = 0; i < 20; i++) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            configTime(0, 0, "pool.ntp.org", "time.nist.gov");
            PixelPet::drawBootScreen(display, "Connected!", 100);
            delay(300);
            return true;
        }
        int progress = 50 + (i * 25) / 20;
        PixelPet::drawBootScreen(display, "Connecting...", progress);
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

static int todayYMD() {
    auto dt = M5.Rtc.getDateTime();
    return dt.date.year * 10000 + dt.date.month * 100 + dt.date.date;
}

static void checkStreak(int today) {
    DeviceSettings& s = settingsGet();
    int last = s.streakLastDay;
    int count = s.streakCount;

    if (today == last) return;

    if (today == 0 || last == 0) {
        count = 1;
    } else if (today == last + 1) {
        count++;
    } else {
        count = 1;
    }

    s.streakCount = count;
    s.streakLastDay = today;
    pet.setStreak(count, today);

    bool milestone = false;
    if (count == STREAK_MILESTONE_WEEK) {
        pet.triggerCelebrate("7 Day Streak!");
        milestone = true;
    } else if (count == STREAK_MILESTONE_MONTH) {
        pet.triggerCelebrate("30 Day Streak!");
        milestone = true;
    } else if (count == STREAK_MILESTONE_100) {
        pet.triggerCelebrate("100 Days!");
        milestone = true;
    } else if (count == STREAK_MILESTONE_YEAR) {
        pet.triggerCelebrate("1 Year Streak!");
        milestone = true;
    }

    if (!milestone && count > 1 && count % 10 == 0) {
        pet.triggerCelebrate("Keep Going!");
    }

    settingsSave();
}

static bool imuOk = false;
static float accelX = 0, accelY = 0, accelZ = 0;
static uint32_t lastShakeMs = 0;

static void readIMU() {
    if (!imuOk) return;
    M5.Imu.getAccel(&accelX, &accelY, &accelZ);
}

static void checkShake() {
    if (!imuOk) return;
    float mag = sqrtf(accelX * accelX + accelY * accelY + accelZ * accelZ);
    if (mag > SHAKE_THRESHOLD) {
        uint32_t now = millis();
        if (now - lastShakeMs > SHAKE_COOLDOWN_MS) {
            lastShakeMs = now;
            pet.triggerShake();
            Serial.printf("Shake detected! mag=%.2f\n", mag);
        }
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(BTN_A_PIN, INPUT_PULLUP);
    pinMode(BTN_B_PIN, INPUT_PULLUP);

    auto cfg = M5.config();
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);

    imuOk = M5.Imu.isEnabled();
    if (imuOk) {
        Serial.println("BMI270 IMU ready");
    } else {
        Serial.println("IMU not available - shake disabled");
    }

    lcd = &M5.Lcd;
    lcd->setRotation(0);
    lcd->fillScreen(TFT_BLACK);

    PixelPet::drawBootScreen(lcd, "Starting...", 10);

    readBattery();
    pet.setBattery(batteryPct);

    pet.begin(lcd);
    settingsLoad();

    DeviceSettings& s = settingsGet();
    pet.setGrowthData(s.totalTasks, s.growthStage);
    pet.setStreak(s.streakCount, s.streakLastDay);

    bool connected = false;

    if (s.hasConfig) {
        PixelPet::drawBootScreen(lcd, "Connecting WiFi...", 30);
        connected = connectWiFi(s.wifiSSID, s.wifiPassword, s.wifiIdentity, lcd);
    }

    if (!connected) {
        PixelPet::drawBootScreen(lcd, "Starting AP mode...", 80);
        delay(300);
        startAPMode();
    }

    int today = todayYMD();
    if (today > 0) {
        checkStreak(today);
    }

    server.begin();
    PixelPet::drawBootScreen(lcd, "Ready!", 100);
    delay(500);
}

void loop() {
    readButtons();
    readIMU();
    checkShake();
    delay(10);

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

    if (btnA_pressed && currentMode == MODE_STATUS) {
        currentMode = MODE_PET;
    }
    btnA_pressed = false;

    static uint32_t lastBatteryRead = 0;
    static int lastDayCheck = 0;
    if (millis() - lastBatteryRead >= 10000) {
        lastBatteryRead = millis();
        readBattery();
        pet.setBattery(batteryPct);

        DeviceSettings& s = settingsGet();
        if (s.totalTasks != pet.getTotalTasks() || s.growthStage != pet.getGrowthStage()) {
            s.totalTasks = pet.getTotalTasks();
            s.growthStage = pet.getGrowthStage();
            settingsSave();
        }

        int today = todayYMD();
        if (today > 0 && today != lastDayCheck) {
            lastDayCheck = today;
            checkStreak(today);
        }
    }

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
