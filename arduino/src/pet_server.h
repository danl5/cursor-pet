#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include "pet.h"

class PetServer {
public:
    PetServer(PixelPet& pet, uint16_t port = 80);
    void begin();
    void handleClient();

private:
    PixelPet& _pet;
    WebServer _server;

    void _handleState();
    void _handleStats();
    void _handleHealth();
    void _handleRoot();
    void _handleConfigPage();
    void _handleConfigGet();
    void _handleConfigSave();
    void _handleReboot();
    void _handleGrowth();
    void _handleGrowthReset();
    void _handleActivity();
    void _handleStreak();
};
