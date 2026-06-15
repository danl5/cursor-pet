#include "settings.h"

static DeviceSettings _deviceSettings;
static Preferences _settingsPrefs;

void settingsLoad() {
    _settingsPrefs.begin("cursorpet", true);
    _deviceSettings.hasConfig = _settingsPrefs.getBool("has_cfg", false);
    _settingsPrefs.getString("ssid", _deviceSettings.wifiSSID, sizeof(_deviceSettings.wifiSSID));
    _settingsPrefs.getString("pass", _deviceSettings.wifiPassword, sizeof(_deviceSettings.wifiPassword));
    _settingsPrefs.getString("identity", _deviceSettings.wifiIdentity, sizeof(_deviceSettings.wifiIdentity));
    _deviceSettings.totalTasks = _settingsPrefs.getInt("total_tasks", 0);
    _deviceSettings.growthStage = _settingsPrefs.getInt("growth_stage", 0);
    _deviceSettings.streakCount = _settingsPrefs.getInt("streak_cnt", 0);
    _deviceSettings.streakLastDay = _settingsPrefs.getInt("streak_day", 0);
    _deviceSettings.activityThoughts = _settingsPrefs.getInt("act_thought", 0);
    _deviceSettings.activityTools = _settingsPrefs.getInt("act_tools", 0);
    _settingsPrefs.end();

    if (!_deviceSettings.hasConfig) {
        _deviceSettings.wifiSSID[0] = '\0';
        _deviceSettings.wifiPassword[0] = '\0';
        _deviceSettings.wifiIdentity[0] = '\0';
    }
}

void settingsSave() {
    _settingsPrefs.begin("cursorpet", false);
    _settingsPrefs.putBool("has_cfg", _deviceSettings.hasConfig);
    _settingsPrefs.putString("ssid", _deviceSettings.wifiSSID);
    _settingsPrefs.putString("pass", _deviceSettings.wifiPassword);
    _settingsPrefs.putString("identity", _deviceSettings.wifiIdentity);
    _settingsPrefs.putInt("total_tasks", _deviceSettings.totalTasks);
    _settingsPrefs.putInt("growth_stage", _deviceSettings.growthStage);
    _settingsPrefs.putInt("streak_cnt", _deviceSettings.streakCount);
    _settingsPrefs.putInt("streak_day", _deviceSettings.streakLastDay);
    _settingsPrefs.putInt("act_thought", _deviceSettings.activityThoughts);
    _settingsPrefs.putInt("act_tools", _deviceSettings.activityTools);
    _settingsPrefs.end();
}

void settingsSetWiFi(const char* ssid, const char* password, const char* identity) {
    strncpy(_deviceSettings.wifiSSID, ssid, sizeof(_deviceSettings.wifiSSID) - 1);
    _deviceSettings.wifiSSID[sizeof(_deviceSettings.wifiSSID) - 1] = '\0';
    strncpy(_deviceSettings.wifiPassword, password, sizeof(_deviceSettings.wifiPassword) - 1);
    _deviceSettings.wifiPassword[sizeof(_deviceSettings.wifiPassword) - 1] = '\0';
    strncpy(_deviceSettings.wifiIdentity, identity, sizeof(_deviceSettings.wifiIdentity) - 1);
    _deviceSettings.wifiIdentity[sizeof(_deviceSettings.wifiIdentity) - 1] = '\0';
    _deviceSettings.hasConfig = true;
    settingsSave();
}

void settingsClear() {
    _deviceSettings.hasConfig = false;
    _deviceSettings.wifiSSID[0] = '\0';
    _deviceSettings.wifiPassword[0] = '\0';
    _deviceSettings.wifiIdentity[0] = '\0';
    settingsSave();
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
