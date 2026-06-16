#include <M5GFX.h>
#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"
#include "settings.h"
#include "pet.h"

M5GFX* lcd = nullptr;
PixelPet pet;

#define BTN_A_PIN 11
#define BTN_B_PIN 12

enum DisplayMode { MODE_PET, MODE_STATUS };
static DisplayMode currentMode = MODE_PET;

static bool btnA_pressed = false;
static bool btnB_released = false;
static uint32_t lastBPress = 0;
static const uint32_t LONG_PRESS_MS = 1000;

static bool bleConnected = false;
static BLECharacteristic* pTxChar = nullptr;

static int batteryPct = -1;

static bool authPending = false;
static bool authAccepted = false;
static bool authRejected = false;

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
    if (pct >= 0) batteryPct = pct;
}

void drawAuthScreen(const String& name) {
    lcd->fillScreen(TFT_BLACK);

    lcd->setTextDatum(TC_DATUM);
    lcd->setTextColor(0xFEA0, TFT_BLACK);
    lcd->setTextSize(2);
    lcd->drawString("Allow Connection?", LCD_WIDTH / 2, 30);

    lcd->setTextColor(0xC618, TFT_BLACK);
    lcd->setTextSize(1);
    lcd->drawString(name.c_str(), LCD_WIDTH / 2, 65);

    lcd->setTextColor(0x07E0, TFT_BLACK);
    lcd->setTextSize(2);
    lcd->drawString("A = Yes", LCD_WIDTH / 2, 110);

    lcd->setTextColor(0xF800, TFT_BLACK);
    lcd->drawString("B = No", LCD_WIDTH / 2, 145);

    lcd->setTextColor(0x4208, TFT_BLACK);
    lcd->setTextSize(1);
    lcd->drawString("Press A to authorize", LCD_WIDTH / 2, 180);
    lcd->drawString("this device to connect", LCD_WIDTH / 2, 195);
}

void drawStatusScreen() {
    lcd->fillScreen(TFT_BLACK);

    lcd->setTextDatum(TC_DATUM);
    lcd->setTextColor(0xFEA0, TFT_BLACK);
    lcd->setTextSize(2);
    lcd->drawString("Cursor Pet", LCD_WIDTH / 2, 10);

    lcd->setTextDatum(TL_DATUM);
    lcd->setTextSize(1);

    int y = 45;
    int lineH = 18;

    lcd->setTextColor(0xC618, TFT_BLACK);
    lcd->drawString("BLE:", 4, y);
    y += lineH;
    if (bleConnected) {
        lcd->setTextColor(0x07E0, TFT_BLACK);
        lcd->drawString("Connected", 4, y);
    } else {
        lcd->setTextColor(0xFEA0, TFT_BLACK);
        lcd->drawString("Advertising...", 4, y);
    }
    y += lineH + 6;

    lcd->setTextColor(0xC618, TFT_BLACK);
    lcd->drawString("Name:", 4, y);
    y += lineH;
    lcd->setTextColor(0x001F, TFT_BLACK);
    lcd->drawString(BLE_DEVICE_NAME, 4, y);
    y += lineH + 6;

    if (batteryPct >= 0) {
        lcd->setTextColor(0xC618, TFT_BLACK);
        lcd->drawString("Battery:", 4, y);
        y += lineH;
        lcd->setTextColor(0x07E0, TFT_BLACK);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", batteryPct);
        lcd->drawString(buf, 4, y);
        y += lineH + 6;
    }

    DeviceSettings& s = settingsGet();
    lcd->setTextColor(0xC618, TFT_BLACK);
    char sbuf[32];
    snprintf(sbuf, sizeof(sbuf), "Streak: %d day(s)", s.streakCount);
    lcd->drawString(sbuf, 4, y);
    y += lineH + 6;

    snprintf(sbuf, sizeof(sbuf), "Tasks: %d", pet.getTotalTasks());
    lcd->drawString(sbuf, 4, y);
    y += lineH + 6;

    const char* stages[] = {"Baby", "Kitten", "Adult", "Wizard"};
    snprintf(sbuf, sizeof(sbuf), "Char: %s", pet.getCharName());
    lcd->setTextColor(0x07E0, TFT_BLACK);
    lcd->drawString(sbuf, 4, y);
    y += lineH + 6;

    snprintf(sbuf, sizeof(sbuf), "Stage: %s", stages[pet.getGrowthStage()]);
    lcd->setTextColor(0x001F, TFT_BLACK);
    lcd->drawString(sbuf, 4, y);
    y += lineH + 10;

    lcd->setTextColor(0x4208, TFT_BLACK);
    lcd->drawString("A: Next character", 4, y);
    y += lineH;
    lcd->drawString("B: Back to pet", 4, y);
    y += lineH;
    lcd->drawString("Long B: Reset + clear auth", 4, y);
}

void handleBLECommand(const char* json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) return;

    const char* state = doc["state"] | "";
    if (strlen(state) > 0) {
        PetState s = PET_IDLE;
        if (strcmp(state, "idle") == 0)        s = PET_IDLE;
        else if (strcmp(state, "thinking") == 0) s = PET_THINKING;
        else if (strcmp(state, "working") == 0)  s = PET_WORKING;
        else if (strcmp(state, "sleep") == 0)    s = PET_SLEEP;
        else if (strcmp(state, "error") == 0)    s = PET_ERROR;
        pet.setState(s);
    }

    int thoughts = doc["thoughts"] | 0;
    int tools = doc["tools"] | 0;
    if (thoughts > 0 || tools > 0) {
        pet.addActivity(thoughts, tools);
    }

    int tokens = doc["tokens"] | -1;
    int tasks = doc["tasks"] | -1;
    int errors = doc["errors"] | -1;
    if (tokens >= 0 || tasks >= 0 || errors >= 0) {
        pet.setStats(tokens, tasks, errors);
    }
}

class PetServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* s) override {
        Serial.println("BLE connecting...");

        Preferences p;
        bool previouslyAuthorized = false;
        if (p.begin("cursorpet", true)) {
            previouslyAuthorized = p.getBool("authorized", false);
            p.end();
        }
        if (previouslyAuthorized) {
            bleConnected = true;
            Serial.println("Previously authorized - accepted");
            return;
        }

        drawAuthScreen("New Device");
        authPending = true;
        authAccepted = false;
        authRejected = false;

        uint32_t start = millis();
        while (millis() - start < 20000) {
            delay(50);
            if (authAccepted) {
                Preferences p2;
                p2.begin("cursorpet", false);
                p2.putBool("authorized", true);
                p2.end();
                bleConnected = true;
                authPending = false;
                Serial.println("Auth accepted");
                return;
            }
            if (authRejected) {
                authPending = false;
                Serial.println("Auth rejected - disconnecting");
                s->disconnect(0);
                return;
            }
        }

        authPending = false;
        Serial.println("Auth timeout - disconnecting");
        s->disconnect(0);
    }

    void onDisconnect(BLEServer* s) override {
        bleConnected = false;
        Serial.println("BLE disconnected");
        BLEDevice::startAdvertising();
    }
};

class PetCharCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) override {
        String val = c->getValue().c_str();
        Serial.printf("BLE rx: %s\n", val.c_str());
        bleConnected = true;
        handleBLECommand(val.c_str());
    }
};

void setupBLE() {
    BLEDevice::init(BLE_DEVICE_NAME);

    BLEServer* bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new PetServerCallbacks());

    BLEService* svc = bleServer->createService(SERVICE_UUID);

    BLECharacteristic* rxChar = svc->createCharacteristic(
        RX_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    rxChar->setCallbacks(new PetCharCallbacks());

    pTxChar = svc->createCharacteristic(
        TX_CHAR_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    pTxChar->addDescriptor(new BLE2902());

    svc->start();

    BLEAdvertising* adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(SERVICE_UUID);
    adv->setMinInterval(1600);
    adv->setMaxInterval(3200);

    BLEDevice::startAdvertising();
    Serial.println("BLE advertising as " BLE_DEVICE_NAME);
}

void setupPet() {
    pet.begin(lcd);
    settingsLoad();
    DeviceSettings& s = settingsGet();
    pet.setGrowthData(s.totalTasks, s.growthStage);
    pet.setStreak(s.streakCount, s.streakLastDay);
    pet.setChar(s.charIndex);
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
    if (count == STREAK_MILESTONE_WEEK) { pet.triggerCelebrate("7 Day Streak!"); milestone = true; }
    else if (count == STREAK_MILESTONE_MONTH) { pet.triggerCelebrate("30 Day Streak!"); milestone = true; }
    else if (count == STREAK_MILESTONE_100) { pet.triggerCelebrate("100 Days!"); milestone = true; }
    else if (count == STREAK_MILESTONE_YEAR) { pet.triggerCelebrate("1 Year Streak!"); milestone = true; }

    if (!milestone && count > 1 && count % 10 == 0) {
        pet.triggerCelebrate("Keep Going!");
    }
    settingsSave();
}

void clearAllAuthorizations() {
    Preferences p;
    p.begin("cursorpet", false);
    p.remove("authorized");
    p.end();
    Serial.println("Authorization cleared");
}

static bool imuOk = false;
static float accelX = 0, accelY = 0, accelZ = 0;
static uint32_t lastShakeMs = 0;

void readIMU() {
    if (!imuOk) return;
    M5.Imu.getAccel(&accelX, &accelY, &accelZ);
}

void checkShake() {
    if (!imuOk) return;
    float mag = sqrtf(accelX * accelX + accelY * accelY + accelZ * accelZ);
    if (mag > SHAKE_THRESHOLD) {
        uint32_t now = millis();
        if (now - lastShakeMs > SHAKE_COOLDOWN_MS) {
            lastShakeMs = now;
            pet.triggerShake();
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
    Serial.println(imuOk ? "BMI270 ready" : "IMU unavailable");

    lcd = &M5.Lcd;
    lcd->setRotation(0);
    lcd->fillScreen(TFT_BLACK);

    PixelPet::drawBootScreen(lcd, "Starting...", 30);

    readBattery();
    pet.setBattery(batteryPct);

    setupPet();
    setupBLE();

    int today = todayYMD();
    if (today > 0) checkStreak(today);

    PixelPet::drawBootScreen(lcd, "BLE Ready!", 100);
    delay(500);
}

void loop() {
    readButtons();

    if (authPending) {
        if (digitalRead(BTN_A_PIN) == LOW) {
            delay(30);
            if (digitalRead(BTN_A_PIN) == LOW) {
                authAccepted = true;
                lcd->fillScreen(TFT_BLACK);
                lcd->setTextColor(0x07E0, TFT_BLACK);
                lcd->setTextDatum(MC_DATUM);
                lcd->setTextSize(2);
                lcd->drawString("Accepted!", LCD_WIDTH / 2, LCD_HEIGHT / 2);
                delay(800);
                currentMode = MODE_PET;
            }
        }
        if (digitalRead(BTN_B_PIN) == LOW) {
            delay(30);
            if (digitalRead(BTN_B_PIN) == LOW) {
                authRejected = true;
                lcd->fillScreen(TFT_BLACK);
                lcd->setTextColor(0xF800, TFT_BLACK);
                lcd->setTextDatum(MC_DATUM);
                lcd->setTextSize(2);
                lcd->drawString("Rejected", LCD_WIDTH / 2, LCD_HEIGHT / 2);
                delay(800);
                currentMode = MODE_PET;
            }
        }
        delay(30);
        return;
    }

    readIMU();
    checkShake();

    readButtons();

    if (btnB_released) {
        uint32_t held = millis() - lastBPress;
        if (held >= LONG_PRESS_MS) {
            settingsResetGrowth();
            pet.setGrowthData(0, 0);
            clearAllAuthorizations();
            Serial.println("Factory reset: growth + auth cleared");
            lcd->fillScreen(TFT_BLACK);
            lcd->setTextColor(0xF800, TFT_BLACK);
            lcd->setTextDatum(MC_DATUM);
            lcd->drawString("Factory Reset", LCD_WIDTH / 2, LCD_HEIGHT / 2);
            delay(1500);
            currentMode = MODE_PET;
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
        pet.nextChar();
        DeviceSettings& s2 = settingsGet();
        s2.charIndex = pet.getCharIndex();
        settingsSave();
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

    if (!bleConnected) {
        static uint32_t lastIdleCheck = 0;
        if (millis() - lastIdleCheck >= 10000) {
            lastIdleCheck = millis();
            if (pet.getState() != PET_SLEEP) {
                pet.setState(PET_SLEEP);
                M5.Lcd.setBrightness(64);
            }
        }
    } else {
        M5.Lcd.setBrightness(128);
    }

    switch (currentMode) {
        case MODE_PET:
            pet.tick();
            break;
        case MODE_STATUS: {
            static uint32_t lastStatus = 0;
            if (millis() - lastStatus >= 500) {
                lastStatus = millis();
                drawStatusScreen();
            }
            break;
        }
    }
}
