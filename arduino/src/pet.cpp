#include "pet.h"
#include "config.h"
#include "sprites.h"

#define C_BG       0x0000
#define C_HUD_BG   0x10A2
#define C_WHITE    0xFFFF
#define C_GRAY     0xC618

#define BLINK_INTERVAL  12
#define BLINK_DURATION  2

PixelPet::PixelPet()
    : _state(PET_SLEEP), _lastState(PET_SLEEP),
      _frame(0), _lastTick(0), _tokens(0), _tasks(0), _errors(0), _battery(-1),
      _totalTasks(0), _growthStage(0), _streakCount(0), _streakDay(0),
      _activityThoughts(0), _activityTools(0),
      _shakeTimer(0), _confettiOffset(0),
      _celebrateTimer(0), _celebrateFrame(0),
      _idleBehavior(IDLE_NONE), _idleBehaviorTimer(0), _idleCooldown(0) {
    _celebrateReason[0] = '\0';
    _modelName[0] = '\0';
    _toolLabel[0] = '\0';
    _charIndex = 0;
}

void PixelPet::begin(LovyanGFX* display) {
    _sprite = new M5Canvas(display);
    _sprite->createSprite(LCD_WIDTH, LCD_HEIGHT);
    _sprite->fillScreen(C_BG);
    _drawHud();
    _sprite->pushSprite(0, 0);
}

void PixelPet::setState(PetState s) {
    if (s != _state) {
        if (s == PET_WORKING)      _tasks++;
        else if (s == PET_ERROR)   _errors++;
        if (s != PET_IDLE && _state == PET_IDLE) {
            _idleBehavior = IDLE_NONE;
            _idleBehaviorTimer = 0;
        }
        if (s == PET_IDLE) {
            _idleCooldown = random(60, 120);
        }
        _state = s;
        _frame = 0;
    }
}

void PixelPet::setStats(int tokens, int tasks, int errors) {
    _tokens = tokens;
    _tasks = tasks;
    _errors = errors;
}

void PixelPet::setBattery(int pct) {
    _battery = pct;
}

void PixelPet::setGrowthData(int totalTasks, int stage) {
    _totalTasks = totalTasks;
    _growthStage = stage;
}

void PixelPet::addActivity(int thoughts, int tools) {
    _activityThoughts += thoughts;
    _activityTools += tools;
}

void PixelPet::triggerShake() {
    if (_state == PET_SHAKE || _celebrateTimer > 0) return;
    _lastState = _state;
    _state = PET_SHAKE;
    _shakeTimer = SHAKE_ANIM_FRAMES;
    _frame = 0;
}

void PixelPet::triggerCelebrate(const char* reason) {
    if (_celebrateTimer > 0) return;
    _lastState = _state;
    _celebrateTimer = 10;
    _celebrateFrame = 0;
    _confettiOffset = 0;
    strncpy(_celebrateReason, reason, sizeof(_celebrateReason) - 1);
    _celebrateReason[sizeof(_celebrateReason) - 1] = '\0';
}

void PixelPet::setStreak(int count, int today) {
    _streakCount = count;
    _streakDay = today;
}

void PixelPet::setChar(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= CHAR_COUNT) idx = CHAR_COUNT - 1;
    _charIndex = idx;
}

void PixelPet::setModel(const char* name) {
    strncpy(_modelName, name, sizeof(_modelName) - 1);
    _modelName[sizeof(_modelName) - 1] = '\0';
}

void PixelPet::setTool(const char* name) {
    strncpy(_toolLabel, name, sizeof(_toolLabel) - 1);
    _toolLabel[sizeof(_toolLabel) - 1] = '\0';
}

void PixelPet::nextChar() {
    _charIndex = (_charIndex + 1) % CHAR_COUNT;
}

void PixelPet::_updateGrowth() {
    if (_growthStage < GROWTH_KITTEN && _totalTasks >= 10) {
        _growthStage = GROWTH_KITTEN;
    } else if (_growthStage < GROWTH_ADULT && _totalTasks >= 50) {
        _growthStage = GROWTH_ADULT;
    } else if (_growthStage < GROWTH_WIZARD && _totalTasks >= 200) {
        _growthStage = GROWTH_WIZARD;
    }
}

void PixelPet::tick() {
    uint32_t now = millis();
    if (now - _lastTick < 500) return;
    _lastTick = now;

    if (_celebrateTimer > 0) {
        _celebrateTimer--;
        _celebrateFrame++;
        _confettiOffset = (_confettiOffset + 1) % 8;
        _sprite->fillScreen(C_BG);
        _drawHud();
        _sprite->fillRect(0, 28, LCD_WIDTH, 184, C_BG);
        _drawCelebrate();
        _sprite->pushSprite(0, 0);
        if (_celebrateTimer == 0) {
            _state = _lastState;
            _frame = 0;
        }
        return;
    }

    if (_state == PET_SHAKE) {
        if (_shakeTimer > 0) {
            _shakeTimer--;
            _sprite->fillRect(0, 28, LCD_WIDTH, 184, C_BG);
            _drawShake();
        } else {
            _state = _lastState;
            _frame = 0;
        }
        _drawHud();
        _sprite->pushSprite(0, 0);
        _frame++;
        return;
    }

    if (_lastState == PET_WORKING && _state != PET_WORKING && _state != PET_SHAKE && _state != PET_SLEEP) {
        _totalTasks++;
        _updateGrowth();
        _lastState = _state;
        _celebrateTimer = 6;
        _celebrateFrame = 0;
        _confettiOffset = 0;
        _celebrateReason[0] = '\0';
        _frame = 0;
        return;
    }

    if (_state != _lastState && _lastState != PET_SHAKE) {
        _sprite->fillScreen(C_BG);
        _lastState = _state;
    }
    _drawHud();
    _sprite->fillRect(0, 28, LCD_WIDTH, 184, C_BG);

    switch (_state) {
        case PET_SLEEP:   _drawSleep(); break;
        case PET_IDLE:    _drawIdle(); break;
        case PET_THINKING: _drawThink(); break;
        case PET_WORKING: _drawWork(); break;
        case PET_ERROR:   _drawError(); break;
        default: break;
    }

    _sprite->pushSprite(0, 0);
    _frame++;
}

void PixelPet::_tickIdleBehavior() {
    if (_idleCooldown > 0) {
        _idleCooldown--;
        return;
    }

    if (_idleBehavior == IDLE_NONE && random(100) < 3) {
        _idleBehavior = (IdleBehavior)(random(IDLE_COUNT - 1) + 1);

        switch (_idleBehavior) {
            case IDLE_STRETCH:    _idleBehaviorTimer = 30; break;
            case IDLE_YAWN:       _idleBehaviorTimer = 20; break;
            case IDLE_TAIL_CHASE: _idleBehaviorTimer = 25; break;
            case IDLE_FACE_WIPE:  _idleBehaviorTimer = 16; break;
            case IDLE_LOOK_AROUND:_idleBehaviorTimer = 20; break;
            case IDLE_EAR_FLOP:   _idleBehaviorTimer = 12; break;
            default: break;
        }
    }

    if (_idleBehavior != IDLE_NONE) {
        _idleBehaviorTimer--;
        if (_idleBehaviorTimer <= 0) {
            _idleBehavior = IDLE_NONE;
            _idleCooldown = random(30, 80);
        }
    }
}

void PixelPet::_drawHud() {
    _sprite->fillRect(0, 0, LCD_WIDTH, 28, C_HUD_BG);

    _sprite->setTextSize(1);
    _sprite->setTextDatum(TL_DATUM);
    if (_battery >= 0) {
        char bbuf[8];
        snprintf(bbuf, sizeof(bbuf), "%d%%", _battery);
        _sprite->setTextColor(C_WHITE, C_HUD_BG);
        _sprite->drawString(bbuf, 4, 4);
    }

    _sprite->setTextDatum(TC_DATUM);
    _sprite->setTextColor(C_WHITE, C_HUD_BG);
    _sprite->setTextSize(1);
    const char* labels[] = {"Sleep", "Idle", "Think", "Work", "ERROR", "?!", "*"};
    int labelIdx = (_state <= PET_ERROR) ? _state : PET_IDLE;
    if (_toolLabel[0] && _state == PET_WORKING && _shakeTimer == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s", _toolLabel);
        _sprite->drawString(buf, LCD_WIDTH / 2, 4);
    } else {
        _sprite->drawString(labels[labelIdx], LCD_WIDTH / 2, 4);
    }

    const char* stages[] = {"Baby", "Kitten", "Adult", "Wizard"};
    _sprite->setTextDatum(TR_DATUM);
    _sprite->setTextColor(0xFEA0, C_HUD_BG);
    if (_totalTasks >= 200) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%s ~", charNames[_charIndex]);
        _sprite->drawString(buf, LCD_WIDTH - 4, 4);
    } else if (_totalTasks >= 50) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%s *", charNames[_charIndex]);
        _sprite->drawString(buf, LCD_WIDTH - 4, 4);
    } else {
        _sprite->drawString(charNames[_charIndex], LCD_WIDTH - 4, 4);
    }

    _sprite->fillRect(0, 212, LCD_WIDTH, 28, C_BG);
    _sprite->setTextSize(1);
    _sprite->setTextColor(C_GRAY, C_BG);
    _sprite->setTextDatum(TL_DATUM);
    char buf[48];
    snprintf(buf, sizeof(buf), "T:%d F:%d E:%d", _tokens, _tasks, _errors);
    _sprite->drawString(buf, 4, 214);

    if (_streakCount > 0) {
        char sbuf[16];
        snprintf(sbuf, sizeof(sbuf), "D%d", _streakCount);
        _sprite->setTextDatum(TR_DATUM);
        _sprite->setTextColor(0xFEA0, C_BG);
        _sprite->drawString(sbuf, LCD_WIDTH - 4, 214);
    }

    if (_modelName[0] && _state != PET_SLEEP && _state != PET_SHAKE && _celebrateTimer == 0) {
        _sprite->setTextDatum(BC_DATUM);
        _sprite->setTextColor(0x4208, C_BG);
        _sprite->drawString(_modelName, LCD_WIDTH / 2, 206);
    }

    int barY = 226;
    int barH = 4;
    int barPad = 2;
    int barW = (LCD_WIDTH - barPad * 4) / 3;

    int tBar = min(_activityThoughts, ACTIVITY_BAR_MAX);
    int tlBar = min(_activityTools, ACTIVITY_BAR_MAX);

    _sprite->fillRect(barPad, barY, barW, barH, 0x2104);
    if (tBar > 0) _sprite->fillRect(barPad, barY, (barW * tBar) / ACTIVITY_BAR_MAX, barH, THOUGHT_COLOR);

    int x2 = barPad * 2 + barW;
    _sprite->fillRect(x2, barY, barW, barH, 0x2104);
    if (tlBar > 0) _sprite->fillRect(x2, barY, (barW * tlBar) / ACTIVITY_BAR_MAX, barH, TOOL_COLOR);

    int x3 = barPad * 3 + barW * 2;
    int maxDur = 60;
    int dur = min(_activityThoughts + _activityTools, maxDur);
    _sprite->fillRect(x3, barY, barW, barH, 0x2104);
    if (dur > 0) _sprite->fillRect(x3, barY, (barW * dur) / maxDur, barH, DURATION_COLOR);
}

static const uint16_t* getSprite(PetState state, int charIndex, int& scale, int& sw, int& sh) {
    scale = 4; sw = SPRITE_W; sh = SPRITE_H;
    if (charIndex == CHAR_ROBOT) {
        scale = 4; sw = SPRITE_W; sh = SPRITE_H;
        switch (state) {
            case PET_SLEEP:   return robot_sleep;
            case PET_THINKING: return robot_think;
            case PET_ERROR:   return robot_error;
            default:          return robot_idle_open;
        }
    }
    if (charIndex == CHAR_MELON) {
        scale = 4; sw = SPRITE_W; sh = SPRITE_H;
        switch (state) {
            case PET_SLEEP:   return melon_sleep;
            case PET_THINKING: return melon_think;
            case PET_ERROR:   return melon_error;
            default:          return melon_idle_open;
        }
    }
    if (charIndex == CHAR_SUN) {
        scale = 4; sw = SPRITE_W; sh = SPRITE_H;
        switch (state) {
            case PET_SLEEP:   return sun_sleep;
            case PET_THINKING: return sun_think;
            case PET_ERROR:   return sun_error;
            default:          return sun_idle_open;
        }
    }

    return sun_idle_open;  // fallback
}

void PixelPet::_drawSleep() {
    int scale, sw, sh;
    const uint16_t* sprite = getSprite(PET_SLEEP, _charIndex, scale, sw, sh);
    int x = (LCD_WIDTH - sw * scale) / 2;
    int y = 80;

    drawSpriteToCanvas(_sprite, x, y, sprite, scale);

    int zOffset = (_frame % 8) * 4;
    _sprite->setTextSize(2);
    _sprite->setTextColor(0x4208, C_BG);
    _sprite->setTextDatum(TL_DATUM);
    _sprite->drawString("z", x + sw * scale + 5, y + 10 - zOffset);
    _sprite->setTextSize(1);
    _sprite->drawString("z", x + sw * scale + 15, y + 5 - zOffset / 2);
}

void PixelPet::_drawIdle() {
    _tickIdleBehavior();

    int scale, sw, sh;
    const uint16_t* sprite;

    scale = 4; sw = SPRITE_W; sh = SPRITE_H;
    bool eyesOpen;
    if (_idleBehavior == IDLE_YAWN) {
        eyesOpen = false;
    } else {
        eyesOpen = (_frame % BLINK_INTERVAL) >= BLINK_DURATION;
    }
    if (_charIndex == CHAR_ROBOT)
        sprite = eyesOpen ? robot_idle_open : robot_idle_closed;
    else if (_charIndex == CHAR_MELON)
        sprite = eyesOpen ? melon_idle_open : melon_idle_closed;
    else
        sprite = eyesOpen ? sun_idle_open : sun_idle_closed;

    int offsetX = 0, offsetY = 0;
    switch (_idleBehavior) {
        case IDLE_STRETCH: {
            int t = 30 - _idleBehaviorTimer;
            if (t < 6)       offsetY = -t * 2;
            else if (t < 12) offsetY = -10;
            else if (t < 22) offsetY = -(10 - (t - 12) * 2);
            break;
        }
        case IDLE_TAIL_CHASE:
            offsetX = (_idleBehaviorTimer % 3 == 0) ? 2 : -2;
            break;
        case IDLE_FACE_WIPE:
            offsetX = 3;
            break;
        case IDLE_LOOK_AROUND: {
            int t = 20 - _idleBehaviorTimer;
            if (t < 5)       offsetX = -4;
            else if (t < 10) offsetX = 0;
            else if (t < 15) offsetX = 4;
            break;
        }
        default: break;
    }

    int x = (LCD_WIDTH - sw * scale) / 2 + offsetX;
    int y = 80 + offsetY;

    drawSpriteToCanvas(_sprite, x, y, sprite, scale);

    switch (_idleBehavior) {
        case IDLE_YAWN: {
            int t = 20 - _idleBehaviorTimer;
            int mouthY = y + 7 * scale;
            int mouthW = 6, mouthH;
            if (t < 5)       mouthH = t + 2;
            else if (t < 13) mouthH = 7;
            else             mouthH = 7 - (t - 12);
            if (mouthH < 0) mouthH = 0;
            _sprite->fillEllipse(x + sw * scale / 2, mouthY, mouthW, mouthH, 0x0000);
            if (t >= 6 && t <= 12) {
                _sprite->fillEllipse(x + sw * scale / 2, mouthY + 2, mouthW - 2, mouthH - 2, SPRITE_PINK);
            }
            break;
        }
        case IDLE_FACE_WIPE: {
            int t = 16 - _idleBehaviorTimer;
            int pawX = x + 12 * scale - (t % 3) * 2;
            int pawY = y + 5 * scale + (t % 2);
            _sprite->fillEllipse(pawX, pawY, 4, 3, SPRITE_YELLOW);
            break;
        }
        case IDLE_STRETCH: {
            int t = 30 - _idleBehaviorTimer;
            if (t >= 3 && t <= 10) {
                int pawOff = (t - 3);
                _sprite->fillEllipse(x + 4 * scale, y - 4 + pawOff, 3, 3, SPRITE_YELLOW);
                _sprite->fillEllipse(x + 12 * scale, y - 4 + pawOff, 3, 3, SPRITE_YELLOW);
            }
            break;
        }
        case IDLE_EAR_FLOP: {
            int t = 12 - _idleBehaviorTimer;
            if (t < 4) {
                int earY = y - 2 + (t % 2);
                _sprite->drawLine(x + 4 * scale, y - 6, x + 4 * scale, earY, SPRITE_YELLOW);
                _sprite->drawLine(x + 5 * scale, y - 6, x + 5 * scale, earY + 1, SPRITE_YELLOW);
            }
            break;
        }
        default: break;
    }
}

void PixelPet::_drawThink() {
    int scale, sw, sh;
    const uint16_t* sprite = getSprite(PET_THINKING, _charIndex, scale, sw, sh);
    int x = (LCD_WIDTH - sw * scale) / 2;
    int y = 90;

    drawSpriteToCanvas(_sprite, x, y, sprite, scale);

    int bubbleY = y - 10 - (_frame % 6) * 3;
    _sprite->fillEllipse(x + sw * scale / 2, bubbleY, 4, 4, 0x001F);
    _sprite->fillEllipse(x + sw * scale / 2 + 15, bubbleY - 12, 6, 6, 0x001F);
    _sprite->fillEllipse(x + sw * scale / 2 + 30, bubbleY - 25, 8, 8, 0x001F);
}

void PixelPet::_drawWork() {
    int scale, sw, sh;
    const uint16_t* sprite = getSprite(PET_WORKING, _charIndex, scale, sw, sh);
    int bounce = (_frame % 2 == 0) ? 4 : 0;
    int x = (LCD_WIDTH - sw * scale) / 2;
    int y = 80 + bounce;

    uint16_t ringColor = 0x07E0;
    if (strcmp(_toolLabel, "Read") == 0)
        ringColor = 0x001F;
    else if (strcmp(_toolLabel, "Shell") == 0)
        ringColor = 0xFEA0;

    _sprite->drawEllipse(x + sw * scale / 2, y + sh * scale / 2,
                         sw * scale / 2 + 8, sh * scale / 2 + 8, ringColor);
    _sprite->drawEllipse(x + sw * scale / 2, y + sh * scale / 2,
                         sw * scale / 2 + 7, sh * scale / 2 + 7, ringColor);

    drawSpriteToCanvas(_sprite, x, y, sprite, scale);

    int pawY = y + sh * scale - 5;
    _sprite->fillEllipse(x + 10, pawY, 5, 4, SPRITE_YELLOW);
    _sprite->fillEllipse(x + sw * scale - 10, pawY, 5, 4, SPRITE_YELLOW);
}

void PixelPet::_drawError() {
    int scale, sw, sh;
    const uint16_t* sprite = getSprite(PET_ERROR, _charIndex, scale, sw, sh);
    int x = (LCD_WIDTH - sw * scale) / 2;
    int y = 80;

    if (_frame % 2 == 0) {
        drawSpriteToCanvas(_sprite, x, y, sprite, scale);
    } else {
        drawSpriteToCanvas(_sprite, x, y, robot_idle_open, scale);
    }
}

void PixelPet::_drawCelebrate() {
    int scale, sw, sh;
    const uint16_t* sprite = getSprite(PET_IDLE, _charIndex, scale, sw, sh);
    int bounce = (_celebrateFrame % 2 == 0) ? -8 : 0;
    int x = (LCD_WIDTH - sw * scale) / 2;
    int y = 80 + bounce;

    drawSpriteToCanvas(_sprite, x, y, sprite, scale);

    int cx = x + sw * scale / 2;
    int cy = y + sh * scale / 2;
    for (int i = 0; i < 8; i++) {
        float angle = (i * 3.14159f * 2) / 8 + _celebrateFrame * 0.3f;
        int dist = 30 + _celebrateFrame * 3;
        int sx = cx + (int)(cos(angle) * dist);
        int sy = cy + (int)(sin(angle) * dist);
        uint16_t colors[] = {0xFEA0, 0x07E0, 0x001F, 0xF800};
        _sprite->fillEllipse(sx, sy, 2 + (_celebrateFrame % 3), 2 + (_celebrateFrame % 3), colors[i % 4]);
    }

    if (_celebrateReason[0] != '\0' && _celebrateTimer < 8) {
        _sprite->setTextSize(1);
        _sprite->setTextColor(0x07E0, C_BG);
        _sprite->setTextDatum(TC_DATUM);
        _sprite->drawString(_celebrateReason, LCD_WIDTH / 2, 30);
    }

    _sprite->setTextSize(2);
    _sprite->setTextColor(0x07E0, C_BG);
    _sprite->setTextDatum(TC_DATUM);
    const char* msg = (_celebrateReason[0] != '\0') ? "Streak!" : "Done!";
    _sprite->drawString(msg, LCD_WIDTH / 2, y + sh * scale + 15);
}

void PixelPet::_drawShake() {
    int scale, sw, sh;
    const uint16_t* sprite = getSprite(PET_IDLE, _charIndex, scale, sw, sh);
    int shakeX = (int)(sin(_shakeTimer * 1.2f) * 6);
    int shakeY = (int)(cos(_shakeTimer * 1.7f) * 4);
    int x = (LCD_WIDTH - sw * scale) / 2 + shakeX;
    int y = 80 + shakeY;

    drawSpriteToCanvas(_sprite, x, y, sprite, scale);

    bool dizzy = (_shakeTimer < 4);
    if (dizzy) {
        _sprite->drawLine(x + 3 * scale, y + 4 * scale, x + 7 * scale, y + 3 * scale, 0xFFFF);
        _sprite->drawLine(x + 9 * scale, y + 3 * scale, x + 13 * scale, y + 4 * scale, 0xFFFF);
    }

    if (_shakeTimer > 8) {
        _sprite->setTextSize(1);
        _sprite->setTextColor(0xC618, C_BG);
        _sprite->setTextDatum(TC_DATUM);
        _sprite->drawString("!", x + sw * scale / 2, y - 12);
    } else if (_shakeTimer > 4) {
        _sprite->setTextSize(1);
        _sprite->setTextColor(0xC618, C_BG);
        _sprite->setTextDatum(TC_DATUM);
        _sprite->drawString("@_@", x + sw * scale / 2, y - 12);
    }
}

void PixelPet::drawBootScreen(LovyanGFX* display, const char* message, int progress) {
    display->fillScreen(TFT_BLACK);

    int scale = 6;
    int catW = 8 * scale;
    int catH = 8 * scale;
    int catX = (LCD_WIDTH - catW) / 2;
    int catY = 60;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            uint16_t pixel = pgm_read_word(&cat_boot[row * 8 + col]);
            if (pixel != SPRITE_TRANSPARENT) {
                display->fillRect(catX + col * scale, catY + row * scale, scale, scale, pixel);
            }
        }
    }

    display->setTextDatum(TC_DATUM);
    display->setTextColor(0xFEA0, TFT_BLACK);
    display->setTextSize(1);
    display->drawString(message, LCD_WIDTH / 2, catY + catH + 20);

    if (progress >= 0) {
        int barWidth = 100;
        int barHeight = 8;
        int barX = (LCD_WIDTH - barWidth) / 2;
        int barY = catY + catH + 40;

        display->fillRect(barX, barY, barWidth, barHeight, 0x4208);
        int fillWidth = (barWidth * progress) / 100;
        display->fillRect(barX, barY, fillWidth, barHeight, 0x07E0);
    }
}
