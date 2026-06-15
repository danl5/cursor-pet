#pragma once
#include <Arduino.h>
#include <M5GFX.h>

// Pet states
enum PetState {
    PET_SLEEP,
    PET_IDLE,
    PET_THINKING,
    PET_WORKING,
    PET_ERROR
};

class PixelPet {
public:
    PixelPet();
    void begin(LovyanGFX* display);
    void setState(PetState s);
    void setStats(int tokens, int tasks, int errors);
    void setBattery(int pct);
    void tick();

private:
    M5Canvas* _sprite = nullptr;
    PetState _state;
    PetState _lastState;
    uint32_t _frame;
    uint32_t _lastTick;
    int _tokens, _tasks, _errors;
    int _battery;

    void _drawHud();
    void _drawSleep();
    void _drawIdle();
    void _drawThink();
    void _drawWork();
    void _drawError();
    void _drawCat(int cx, int cy, uint16_t bodyColor);
    void _drawEars(int cx, int cy, uint16_t color);
    void _drawEyesOpen(int cx, int cy);
    void _drawEyesClosed(int cx, int cy);
    void _drawCheeks(int cx, int cy);
    void _drawZzz(int cx, int cy, int frame);
    void _drawThoughtBubbles(int cx, int cy, int frame);
    void _drawGlowRing(int cx, int cy);
    void _drawPaws(int cx, int cy, int frame);
};
