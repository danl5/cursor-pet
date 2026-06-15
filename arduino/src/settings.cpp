#include "settings.h"

static DeviceSettings _deviceSettings;
static Preferences _settingsPrefs;

void settingsLoad() {
    _settingsPrefs.begin("cursorpet", true);
    _deviceSettings.totalTasks = _settingsPrefs.getInt("total_tasks", 0);
    _deviceSettings.growthStage = _settingsPrefs.getInt("growth_stage", 0);
    _deviceSettings.streakCount = _settingsPrefs.getInt("streak_cnt", 0);
    _deviceSettings.streakLastDay = _settingsPrefs.getInt("streak_day", 0);
    _deviceSettings.activityThoughts = _settingsPrefs.getInt("act_thought", 0);
    _deviceSettings.activityTools = _settingsPrefs.getInt("act_tools", 0);
    _settingsPrefs.end();
}

void settingsSave() {
    _settingsPrefs.begin("cursorpet", false);
    _settingsPrefs.putInt("total_tasks", _deviceSettings.totalTasks);
    _settingsPrefs.putInt("growth_stage", _deviceSettings.growthStage);
    _settingsPrefs.putInt("streak_cnt", _deviceSettings.streakCount);
    _settingsPrefs.putInt("streak_day", _deviceSettings.streakLastDay);
    _settingsPrefs.putInt("act_thought", _deviceSettings.activityThoughts);
    _settingsPrefs.putInt("act_tools", _deviceSettings.activityTools);
    _settingsPrefs.end();
}

void settingsResetGrowth() {
    _deviceSettings.totalTasks = 0;
    _deviceSettings.growthStage = 0;
    settingsSave();
}

void settingsUpdateStreak(int count, int lastDay) {
    _deviceSettings.streakCount = count;
    _deviceSettings.streakLastDay = lastDay;
    settingsSave();
}

void settingsAddActivity(int thoughts, int tools) {
    _deviceSettings.activityThoughts += thoughts;
    _deviceSettings.activityTools += tools;
}

void settingsResetDailyStats() {
    _deviceSettings.activityThoughts = 0;
    _deviceSettings.activityTools = 0;
    settingsSave();
}

DeviceSettings& settingsGet() {
    return _deviceSettings;
}
