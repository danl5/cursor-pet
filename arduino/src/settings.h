#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct DeviceSettings {
    int totalTasks;
    int growthStage;
    int streakCount;
    int streakLastDay;
    int activityThoughts;
    int activityTools;
};

void settingsLoad();
void settingsSave();
void settingsResetGrowth();
void settingsUpdateStreak(int count, int lastDay);
void settingsAddActivity(int thoughts, int tools);
void settingsResetDailyStats();
DeviceSettings& settingsGet();
