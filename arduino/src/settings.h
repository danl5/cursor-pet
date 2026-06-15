#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct DeviceSettings {
    char wifiSSID[33];
    char wifiPassword[65];
    char wifiIdentity[65];
    bool hasConfig;
    int totalTasks;
    int growthStage;

    int streakCount;
    int streakLastDay;
    int activityThoughts;
    int activityTools;
};

void settingsLoad();
void settingsSave();
void settingsSetWiFi(const char* ssid, const char* password, const char* identity = "");
void settingsClear();
void settingsResetGrowth();
void settingsUpdateStreak(int count, int lastDay);
void settingsAddActivity(int thoughts, int tools);
void settingsResetDailyStats();
DeviceSettings& settingsGet();
