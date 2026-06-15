#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct DeviceSettings {
    char wifiSSID[33];
    char wifiPassword[65];
    bool hasConfig;
};

void settingsLoad();
void settingsSave();
void settingsSetWiFi(const char* ssid, const char* password);
void settingsClear();
DeviceSettings& settingsGet();
