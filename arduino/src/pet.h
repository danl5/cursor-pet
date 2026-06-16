#pragma once
#include <Arduino.h>
#include <M5GFX.h>

// Pet states
enum PetState {
    PET_SLEEP,
    PET_IDLE,
    PET_THINKING,
    PET_WORKING,
    PET_ERROR,
    PET_SHAKE,
    PET_CELEBRATE
};

// Growth stages
enum GrowthStage {
    GROWTH_BABY,     // 0-9 tasks
    GROWTH_KITTEN,   // 10-49 tasks
    GROWTH_ADULT,    // 50-199 tasks
    GROWTH_WIZARD    // 200+ tasks
};

// Character types
enum PetChar : int {
    CHAR_CAT = 0,
    CHAR_ROBOT,
    CHAR_MELON,
    CHAR_SUN,
    CHAR_COUNT
};

const char* const charNames[] = {"Cat", "Robot", "Melon", "Sun"};

// Idle behavior types (randomly triggered during PET_IDLE)
enum IdleBehavior {
    IDLE_NONE = 0,
    IDLE_STRETCH,
    IDLE_YAWN,
    IDLE_TAIL_CHASE,
    IDLE_FACE_WIPE,
    IDLE_LOOK_AROUND,
    IDLE_EAR_FLOP,
    IDLE_COUNT  // sentinel, must be last
};

class PixelPet {
public:
    PixelPet();
    void begin(LovyanGFX* display);
    void setState(PetState s);
    PetState getState() const { return _state; }
    void setStats(int tokens, int tasks, int errors);
    void setBattery(int pct);
    void setGrowthData(int totalTasks, int stage);
    void addActivity(int thoughts, int tools);
    void triggerShake();
    void triggerCelebrate(const char* reason);
    void setStreak(int count, int today);
    void tick();

    int getTotalTasks() const { return _totalTasks; }
    int getGrowthStage() const { return _growthStage; }
    int getActivityThoughts() const { return _activityThoughts; }
    int getActivityTools() const { return _activityTools; }
    int getStreakCount() const { return _streakCount; }
    int getCharIndex() const { return _charIndex; }
    void nextChar();
    void setChar(int idx);

private:
    M5Canvas* _sprite = nullptr;
    PetState _state;
    PetState _lastState;
    uint32_t _frame;
    uint32_t _lastTick;
    int _tokens, _tasks, _errors;
    int _battery;
    int _totalTasks;
    int _growthStage;
    int _streakCount;
    int _streakDay;
    int _charIndex;

    int _activityThoughts;
    int _activityTools;

    int _shakeTimer;
    int _confettiOffset;
    int _celebrateTimer;
    int _celebrateFrame;
    char _celebrateReason[32];

    IdleBehavior _idleBehavior;
    int _idleBehaviorTimer;
    int _idleCooldown;
    void _tickIdleBehavior();

    void _updateGrowth();
    void _drawHud();
    void _drawSleep();
    void _drawIdle();
    void _drawThink();
    void _drawWork();
    void _drawError();
    void _drawCelebrate();
    void _drawShake();
    void _drawCat(int cx, int cy, uint16_t bodyColor);
    void _drawEars(int cx, int cy, uint16_t color);
    void _drawEyesOpen(int cx, int cy);
    void _drawEyesClosed(int cx, int cy);
    void _drawCheeks(int cx, int cy);
    void _drawZzz(int cx, int cy, int frame);
    void _drawThoughtBubbles(int cx, int cy, int frame);
    void _drawGlowRing(int cx, int cy);
    void _drawPaws(int cx, int cy, int frame);
public:
    static void drawBootScreen(LovyanGFX* display, const char* message, int progress = -1);
};
