import time


def _oval(lcd, cx, cy, rx, ry, color):
    for y in range(cy - ry, cy + ry + 1):
        for x in range(cx - rx, cx + rx + 1):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 < 1:
                if 0 <= x < 135 and 0 <= y < 240:
                    lcd.fill_rect(x, y, 1, 1, color)


def _clear(lcd, cx, cy, rx, ry):
    _oval(lcd, cx, cy, rx, ry, 0x0000)


class PixelPet:
    def __init__(self, lcd):
        self.lcd = lcd
        self.state = "sleep"
        self.frame = 0
        self.last_tick = 0
        self.state_text = "Sleep"
        self.token_count = 0
        self.task_count = 0
        self.error_count = 0
        self.cx = 67
        self.cy = 130

    def set_state(self, state):
        labels = {
            "idle": "Idle", "thinking": "Thinking...",
            "working": "Working...", "sleep": "Sleep",
        }
        new_text = labels.get(state, state)
        if state != self.state:
            self.state = state
            self.state_text = new_text
            self.frame = 0
            self.lcd.fill(0x0000)
            self._draw_hud()

    def _draw_hud(self):
        self.lcd.fill_rect(0, 0, 135, 28, 0x10A2)
        self.lcd.text(self.state_text, 8, 8, 0xFFFF)
        stats = "T:%d F:%d E:%d" % (self.token_count, self.task_count, self.error_count)
        self.lcd.fill_rect(0, 212, 135, 28, 0x0000)
        self.lcd.text(stats, 4, 218, 0xC618)

    def _draw_sleep(self, f):
        cx, cy = self.cx, self.cy
        _oval(self.lcd, cx, cy, 22, 16, 0xFEA0)  # body
        _oval(self.lcd, cx, cy - 22, 15, 14, 0xFEA0)  # head
        # ears
        for dy in range(-40, -25):
            for dx in range(-15, -2):
                if (x := cx + dx) < 135 and (y := cy + dy) < 240 and x >= 0 and y >= 0:
                    if (dx + 9) * 0.4 + (dy + 40) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
            for dx in range(3, 15):
                if (x := cx + dx) < 135 and (y := cy + dy) < 240 and x >= 0 and y >= 0:
                    if -(dx - 9) * 0.4 + (dy + 40) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
        # closed eyes
        for x in range(cx - 10, cx - 3):
            self.lcd.fill_rect(x, cy - 20, 1, 1, 0x0000)
        for x in range(cx + 3, cx + 10):
            self.lcd.fill_rect(x, cy - 20, 1, 1, 0x0000)
        # zzz
        zx = cx + 25 + (f % 4) * 3
        zy = cy - 35 - (f % 4) * 5
        if 0 <= zx < 130 and 0 <= zy < 235:
            self.lcd.fill_rect(zx, zy, 4, 4, 0x4208)
            self.lcd.fill_rect(zx + 6, zy - 8, 3, 3, 0x4208)
            self.lcd.fill_rect(zx + 10, zy - 14, 2, 2, 0x4208)

    def _draw_idle(self, f):
        cx, cy = self.cx, self.cy
        _oval(self.lcd, cx, cy, 22, 16, 0xFEA0)
        _oval(self.lcd, cx, cy - 22, 15, 14, 0xFEA0)
        # ears
        for dy in range(-40, -25):
            for dx in range(-15, -2):
                if 0 <= (x := cx + dx) < 135 and 0 <= (y := cy + dy) < 240:
                    if (dx + 9) * 0.4 + (dy + 40) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
            for dx in range(3, 15):
                if 0 <= (x := cx + dx) < 135 and 0 <= (y := cy + dy) < 240:
                    if -(dx - 9) * 0.4 + (dy + 40) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
        # blink
        blink = f % 6 in [0, 3]
        if blink:
            for x in range(cx - 10, cx - 3):
                self.lcd.fill_rect(x, cy - 18, 1, 1, 0x0000)
            for x in range(cx + 3, cx + 10):
                self.lcd.fill_rect(x, cy - 18, 1, 1, 0x0000)
        else:
            for dy in range(-3, 0):
                for dx in range(-3, 0):
                    self.lcd.fill_rect(cx - 7 + dx, cy - 18 + dy, 1, 1, 0x0000)
                    self.lcd.fill_rect(cx + 7 + dx, cy - 18 + dy, 1, 1, 0x0000)
            self.lcd.fill_rect(cx - 6, cy - 20, 2, 2, 0xFFFF)
            self.lcd.fill_rect(cx + 6, cy - 20, 2, 2, 0xFFFF)
        # cheeks
        for dy in range(-2, 2):
            for dx in range(-3, 0):
                self.lcd.fill_rect(cx - 16 + dx, cy - 14 + dy, 1, 1, 0xF800)
                self.lcd.fill_rect(cx + 16 + dx, cy - 14 + dy, 1, 1, 0xF800)
        # tail
        tx = cx + 22 + (5 if f % 4 < 2 else -5)
        for i in range(8):
            self.lcd.fill_rect(tx + i, cy - 3 + i // 2, 2, 2, 0xFEA0)

    def _draw_think(self, f):
        cx, cy = self.cx, self.cy + 8
        _oval(self.lcd, cx, cy, 22, 16, 0xFEA0)
        _oval(self.lcd, cx, cy - 24, 15, 14, 0xFEA0)
        # ears
        for dy in range(-42, -27):
            for dx in range(-15, -2):
                if 0 <= (x := cx + dx) < 135 and 0 <= (y := cy + dy) < 240:
                    if (dx + 9) * 0.4 + (dy + 42) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
            for dx in range(3, 15):
                if 0 <= (x := cx + dx) < 135 and 0 <= (y := cy + dy) < 240:
                    if -(dx - 9) * 0.4 + (dy + 42) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
        # eyes up
        for dy in range(-4, -1):
            for dx in range(-3, 0):
                self.lcd.fill_rect(cx - 7 + dx, cy - 26 + dy, 1, 1, 0x0000)
                self.lcd.fill_rect(cx + 7 + dx, cy - 26 + dy, 1, 1, 0x0000)
        self.lcd.fill_rect(cx - 6, cy - 30, 2, 2, 0xFFFF)
        self.lcd.fill_rect(cx + 6, cy - 30, 2, 2, 0xFFFF)
        # thought bubbles
        by = cy - 48 - (f % 4) * 4
        _oval(self.lcd, cx, by, 3, 3, 0x001F)
        _oval(self.lcd, cx + 14, by - 10, 5, 5, 0x001F)
        # cheeks
        for dy in range(-2, 2):
            for dx in range(-3, 0):
                self.lcd.fill_rect(cx - 16 + dx, cy - 18 + dy, 1, 1, 0xF800)
                self.lcd.fill_rect(cx + 16 + dx, cy - 18 + dy, 1, 1, 0xF800)

    def _draw_work(self, f):
        cx, cy = self.cx, self.cy
        bounce = 2 if f % 2 == 0 else 0
        # green glow ring
        _oval(self.lcd, cx, cy, 28, 22, 0x0000)
        _oval(self.lcd, cx, cy, 28, 22, 0x07E0)
        _oval(self.lcd, cx, cy, 25, 19, 0x0000)
        # body
        _oval(self.lcd, cx, cy + bounce, 22, 16, 0xFEA0)
        _oval(self.lcd, cx, cy - 22 + bounce, 15, 14, 0xFEA0)
        # ears
        for dy in range(-40, -25):
            for dx in range(-15, -2):
                if 0 <= (x := cx + dx) < 135 and 0 <= (y := cy - 22 + bounce + dy + 22) < 240:
                    if (dx + 9) * 0.4 + (dy + 40) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
            for dx in range(3, 15):
                if 0 <= (x := cx + dx) < 135 and 0 <= (y := cy - 22 + bounce + dy + 22) < 240:
                    if -(dx - 9) * 0.4 + (dy + 40) > 0:
                        self.lcd.fill_rect(x, y, 1, 1, 0xFEA0)
        # focused eyes
        for dy in range(-3, 0):
            for dx in range(-3, 0):
                self.lcd.fill_rect(cx - 7 + dx, cy - 20 + bounce + dy, 1, 1, 0x0000)
                self.lcd.fill_rect(cx + 7 + dx, cy - 20 + bounce + dy, 1, 1, 0x0000)
        # typing paws
        py = cy + 10 + (3 if f % 2 == 0 else 0)
        _oval(self.lcd, cx - 14, py, 4, 3, 0xFEA0)
        _oval(self.lcd, cx + 14, py, 4, 3, 0xFEA0)
        # cheeks
        for dy in range(-2, 2):
            for dx in range(-3, 0):
                self.lcd.fill_rect(cx - 16 + dx, cy - 14 + bounce + dy, 1, 1, 0xF800)
                self.lcd.fill_rect(cx + 16 + dx, cy - 14 + bounce + dy, 1, 1, 0xF800)

    def init_screen(self):
        self.lcd.fill(0x0000)
        self._draw_hud()
        self._redraw()

    def _redraw(self):
        f = self.frame
        if self.state == "sleep":
            self._draw_sleep(f)
        elif self.state == "idle":
            self._draw_idle(f)
        elif self.state == "thinking":
            self._draw_think(f)
        elif self.state == "working":
            self._draw_work(f)

    def tick(self):
        now = time.ticks_ms()
        if time.ticks_diff(now, self.last_tick) < 200:
            return
        self.last_tick = now
        self._redraw()
        self.frame += 1
