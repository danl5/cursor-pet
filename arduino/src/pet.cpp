#include "pet.h"

// Colors
#define C_BG       0x0000
#define C_BODY     0xFEA0  // yellow
#define C_EYE      0x0000  // black
#define C_WHITE    0xFFFF
#define C_RED      0xF800  // cheeks
#define C_BLUE     0x001F  // thinking
#define C_GREEN    0x07E0  // working
#define C_GRAY     0xC618
#define C_DARK     0x4208
#define C_HUD_BG   0x10A2

PixelPet::PixelPet(TFT_eSPI& lcd)
    : _lcd(lcd), _sprite(&lcd), _state(PET_SLEEP), _lastState(PET_SLEEP),
      _frame(0), _lastTick(0), _tokens(0), _tasks(0), _errors(0) {}

void PixelPet::begin() {
    _sprite.createSprite(LCD_WIDTH, LCD_HEIGHT);
    _sprite.fillScreen(C_BG);
    _drawHud();
    _sprite.pushSprite(0, 0);
}

void PixelPet::setState(PetState s) {
    if (s != _state) {
        _state = s;
        _frame = 0;
    }
}

void PixelPet::setStats(int tokens, int tasks, int errors) {
    _tokens = tokens;
    _tasks = tasks;
    _errors = errors;
}

void PixelPet::tick() {
    uint32_t now = millis();
    if (now - _lastTick < 200) return;
    _lastTick = now;

    if (_state != _lastState) {
        _sprite.fillScreen(C_BG);
        _drawHud();
        _lastState = _state;
    }

    switch (_state) {
        case PET_SLEEP:   _drawSleep(); break;
        case PET_IDLE:    _drawIdle(); break;
        case PET_THINKING: _drawThink(); break;
        case PET_WORKING: _drawWork(); break;
        case PET_ERROR:   _drawError(); break;
    }

    _sprite.pushSprite(0, 0);
    _frame++;
}

// ─── HUD ───

void PixelPet::_drawHud() {
    _sprite.fillRect(0, 0, LCD_WIDTH, 28, C_HUD_BG);
    _sprite.setTextDatum(TC_DATUM);
    _sprite.setTextColor(C_WHITE, C_HUD_BG);
    _sprite.setTextSize(2);
    const char* labels[] = {"Sleep", "Idle", "Thinking...", "Working...", "ERROR"};
    _sprite.drawString(labels[_state], LCD_WIDTH / 2, 7);

    _sprite.fillRect(0, 212, LCD_WIDTH, 28, C_BG);
    _sprite.setTextSize(1);
    _sprite.setTextColor(C_GRAY, C_BG);
    _sprite.setTextDatum(TL_DATUM);
    char buf[32];
    snprintf(buf, sizeof(buf), "T:%d F:%d E:%d", _tokens, _tasks, _errors);
    _sprite.drawString(buf, 4, 218);
}

// ─── Shared cat parts ───

void PixelPet::_drawCat(int cx, int cy, uint16_t bodyColor) {
    _sprite.fillEllipse(cx, cy, 22, 16, bodyColor);       // body
    _sprite.fillEllipse(cx, cy - 22, 15, 14, bodyColor);  // head
}

void PixelPet::_drawEars(int cx, int cy, uint16_t color) {
    // Left ear
    _sprite.fillTriangle(cx - 15, cy - 28, cx - 10, cy - 42, cx - 3, cy - 28, color);
    // Right ear
    _sprite.fillTriangle(cx + 15, cy - 28, cx + 10, cy - 42, cx + 3, cy - 28, color);
}

void PixelPet::_drawEyesOpen(int cx, int cy) {
    _sprite.fillEllipse(cx - 7, cy - 19, 3, 3, C_EYE);
    _sprite.fillEllipse(cx + 7, cy - 19, 3, 3, C_EYE);
    _sprite.fillCircle(cx - 6, cy - 21, 1, C_WHITE);
    _sprite.fillCircle(cx + 6, cy - 21, 1, C_WHITE);
}

void PixelPet::_drawEyesClosed(int cx, int cy) {
    _sprite.drawLine(cx - 10, cy - 18, cx - 3, cy - 18, C_EYE);
    _sprite.drawLine(cx + 3, cy - 18, cx + 10, cy - 18, C_EYE);
}

void PixelPet::_drawCheeks(int cx, int cy) {
    _sprite.fillEllipse(cx - 16, cy - 14, 3, 2, C_RED);
    _sprite.fillEllipse(cx + 16, cy - 14, 3, 2, C_RED);
}

void PixelPet::_drawZzz(int cx, int cy, int frame) {
    int zy = cy - 35 - (frame % 4) * 6;
    int zx = cx + 25 + (frame % 4) * 3;
    _sprite.setTextSize(1);
    _sprite.setTextColor(C_DARK, C_BG);
    _sprite.setTextDatum(TL_DATUM);
    _sprite.drawString("z", zx, zy);
    _sprite.drawString("z", zx + 6, zy - 8);
    _sprite.drawString("z", zx + 10, zy - 14);
}

void PixelPet::_drawThoughtBubbles(int cx, int cy, int frame) {
    int by = cy - 45 - (frame % 4) * 4;
    _sprite.fillEllipse(cx, by, 3, 3, C_BLUE);
    _sprite.fillEllipse(cx + 14, by - 10, 5, 5, C_BLUE);
}

void PixelPet::_drawGlowRing(int cx, int cy) {
    _sprite.drawEllipse(cx, cy, 28, 22, C_GREEN);
    _sprite.drawEllipse(cx, cy, 27, 21, C_GREEN);
}

void PixelPet::_drawPaws(int cx, int cy, int frame) {
    int py = cy + 10 + (frame % 2 == 0 ? 3 : 0);
    _sprite.fillEllipse(cx - 14, py, 4, 3, C_BODY);
    _sprite.fillEllipse(cx + 14, py, 4, 3, C_BODY);
}

// ─── State animations ───

void PixelPet::_drawSleep() {
    int cx = LCD_WIDTH / 2, cy = 130;
    _drawCat(cx, cy, C_BODY);
    _drawEars(cx, cy - 22, C_BODY);
    _drawEyesClosed(cx, cy - 22);
    _drawZzz(cx, cy, _frame);
}

void PixelPet::_drawIdle() {
    int cx = LCD_WIDTH / 2, cy = 130;
    _drawCat(cx, cy, C_BODY);
    _drawEars(cx, cy - 22, C_BODY);
    bool blink = (_frame % 6 == 0 || _frame % 6 == 3);
    if (blink) {
        _drawEyesClosed(cx, cy - 22);
    } else {
        _drawEyesOpen(cx, cy - 22);
    }
    _drawCheeks(cx, cy - 22);
    // Tail wag
    int tx = cx + 22 + ((_frame % 4 < 2) ? 5 : -5);
    _sprite.drawLine(tx, cy - 3, tx + 8, cy + 2, C_BODY);
    _sprite.drawLine(tx + 1, cy - 2, tx + 9, cy + 3, C_BODY);
}

void PixelPet::_drawThink() {
    int cx = LCD_WIDTH / 2, cy = 138;
    _drawCat(cx, cy, C_BODY);
    _drawEars(cx, cy - 22, C_BODY);
    // Eyes looking up
    _sprite.fillEllipse(cx - 7, cy - 23, 3, 3, C_EYE);
    _sprite.fillEllipse(cx + 7, cy - 23, 3, 3, C_EYE);
    _sprite.fillCircle(cx - 6, cy - 25, 1, C_WHITE);
    _sprite.fillCircle(cx + 6, cy - 25, 1, C_WHITE);
    _drawThoughtBubbles(cx, cy, _frame);
    _drawCheeks(cx, cy - 22);
}

void PixelPet::_drawWork() {
    int cx = LCD_WIDTH / 2, cy = 130;
    int bounce = (_frame % 2 == 0) ? 2 : 0;
    _drawGlowRing(cx, cy);
    _drawCat(cx, cy + bounce, C_BODY);
    _drawEars(cx, cy - 22 + bounce, C_BODY);
    _drawEyesOpen(cx, cy - 22 + bounce);
    _drawPaws(cx, cy, _frame);
    _drawCheeks(cx, cy - 22 + bounce);
}

void PixelPet::_drawError() {
    int cx = LCD_WIDTH / 2, cy = 130;
    uint16_t errColor = (_frame % 2 == 0) ? C_RED : C_BODY;
    _drawCat(cx, cy, errColor);
    _drawEars(cx, cy - 22, errColor);
    // X eyes
    _sprite.drawLine(cx - 10, cy - 22, cx - 4, cy - 16, C_EYE);
    _sprite.drawLine(cx - 4, cy - 22, cx - 10, cy - 16, C_EYE);
    _sprite.drawLine(cx + 4, cy - 22, cx + 10, cy - 16, C_EYE);
    _sprite.drawLine(cx + 10, cy - 22, cx + 4, cy - 16, C_EYE);
}
