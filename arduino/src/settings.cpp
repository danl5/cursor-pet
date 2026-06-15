#include "settings.h"

static DeviceSettings _deviceSettings;
static Preferences _settingsPrefs;

void settingsLoad() {
    _settingsPrefs.begin("cursorpet", true);
    _deviceSettings.hasConfig = _settingsPrefs.getBool("has_cfg", false);
    _settingsPrefs.getString("ssid", _deviceSettings.wifiSSID, sizeof(_deviceSettings.wifiSSID));
    _settingsPrefs.getString("pass", _deviceSettings.wifiPassword, sizeof(_deviceSettings.wifiPassword));
    _settingsPrefs.end();

    if (!_deviceSettings.hasConfig) {
        _deviceSettings.wifiSSID[0] = '\0';
        _deviceSettings.wifiPassword[0] = '\0';
    }
}

void settingsSave() {
    _settingsPrefs.begin("cursorpet", false);
    _settingsPrefs.putBool("has_cfg", _deviceSettings.hasConfig);
    _settingsPrefs.putString("ssid", _deviceSettings.wifiSSID);
    _settingsPrefs.putString("pass", _deviceSettings.wifiPassword);
    _settingsPrefs.end();
}

void settingsSetWiFi(const char* ssid, const char* password) {
    strncpy(_deviceSettings.wifiSSID, ssid, sizeof(_deviceSettings.wifiSSID) - 1);
    _deviceSettings.wifiSSID[sizeof(_deviceSettings.wifiSSID) - 1] = '\0';
    strncpy(_deviceSettings.wifiPassword, password, sizeof(_deviceSettings.wifiPassword) - 1);
    _deviceSettings.wifiPassword[sizeof(_deviceSettings.wifiPassword) - 1] = '\0';
    _deviceSettings.hasConfig = true;
    settingsSave();
}

void settingsClear() {
    _deviceSettings.hasConfig = false;
    _deviceSettings.wifiSSID[0] = '\0';
    _deviceSettings.wifiPassword[0] = '\0';
    settingsSave();
}

DeviceSettings& settingsGet() {
    return _deviceSettings;
}
