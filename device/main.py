import time
import network
from machine import Pin, SPI
from config import SSID, PASSWORD, HTTP_PORT
from pixel_pet import PixelPet
from http_server import HttpServer

COL_OFF = 53
ROW_OFF = 40
W = 135
H = 240


class Lcd:
    def __init__(self):
        backlight = Pin(38, Pin.OUT)
        backlight.value(1)

        self.spi = SPI(1, baudrate=40000000, polarity=0, phase=0,
                       sck=Pin(40), mosi=Pin(39))
        self.dc = Pin(45, Pin.OUT)
        self.cs = Pin(41, Pin.OUT)
        rst = Pin(21, Pin.OUT)

        rst.value(0)
        time.sleep_ms(50)
        rst.value(1)
        time.sleep_ms(120)

        self._cmd(0x01)
        time.sleep_ms(150)
        self._cmd(0x11)
        time.sleep_ms(50)
        self._cmd(0x36)
        self._data(0x00)
        self._cmd(0x3A)
        self._data(0x55)
        self._cmd(0xB2)
        self._data(bytes([0x0C, 0x0C, 0x00, 0x33, 0x33]))
        self._cmd(0xB7)
        self._data(0x35)
        self._cmd(0xBB)
        self._data(0x19)
        self._cmd(0xC0)
        self._data(0x2C)
        self._cmd(0xC2)
        self._data(0x01)
        self._cmd(0xC3)
        self._data(0x12)
        self._cmd(0xC4)
        self._data(0x20)
        self._cmd(0xC6)
        self._data(0x0F)
        self._cmd(0xD0)
        self._data(bytes([0xA4, 0xA1]))
        self._cmd(0x21)
        self._cmd(0x29)
        time.sleep_ms(50)

        self.fill(0x0000)

    def _cmd(self, c):
        self.dc.value(0)
        self.cs.value(0)
        self.spi.write(bytes([c]))
        self.cs.value(1)

    def _data(self, d):
        self.dc.value(1)
        self.cs.value(0)
        if isinstance(d, int):
            self.spi.write(bytes([d]))
        else:
            self.spi.write(d)
        self.cs.value(1)

    def _set_window(self, x0, y0, x1, y1):
        ax0 = x0 + COL_OFF
        ax1 = x1 + COL_OFF
        ay0 = y0 + ROW_OFF
        ay1 = y1 + ROW_OFF
        self._cmd(0x2A)
        self._data(bytes([(ax0 >> 8) & 0xFF, ax0 & 0xFF,
                           (ax1 >> 8) & 0xFF, ax1 & 0xFF]))
        self._cmd(0x2B)
        self._data(bytes([(ay0 >> 8) & 0xFF, ay0 & 0xFF,
                           (ay1 >> 8) & 0xFF, ay1 & 0xFF]))
        self._cmd(0x2C)

    def fill(self, color):
        self._set_window(0, 0, W - 1, H - 1)
        self.dc.value(1)
        self.cs.value(0)
        hi = (color >> 8) & 0xFF
        lo = color & 0xFF
        total = W * H
        for written in range(0, total, 500):
            n = min(500, total - written)
            self.spi.write(bytes([hi, lo]) * n)
        self.cs.value(1)

    def fill_rect(self, x, y, w, h, color):
        self._set_window(x, y, x + w - 1, y + h - 1)
        self.dc.value(1)
        self.cs.value(0)
        hi = (color >> 8) & 0xFF
        lo = color & 0xFF
        total = w * h
        for written in range(0, total, 500):
            n = min(500, total - written)
            self.spi.write(bytes([hi, lo]) * n)
        self.cs.value(1)

    def rect(self, x, y, w, h, color):
        self.fill_rect(x, y, w, h, color)

    def text(self, s, x, y, color=0xFFFF):
        FONT = {
            ' ': [0x00, 0x00, 0x00, 0x00, 0x00],
            'C': [0x3E, 0x41, 0x41, 0x41, 0x22],
            'I': [0x00, 0x41, 0x7F, 0x41, 0x00],
            'E': [0x7F, 0x49, 0x49, 0x49, 0x41],
            'd': [0x20, 0x54, 0x54, 0x54, 0x78],
            'l': [0x00, 0x41, 0x7F, 0x40, 0x00],
            'e': [0x38, 0x54, 0x54, 0x58, 0x00],
            'r': [0x7C, 0x08, 0x04, 0x04, 0x08],
            's': [0x48, 0x54, 0x54, 0x54, 0x20],
            'T': [0x00, 0x01, 0x7F, 0x01, 0x00],
            'h': [0x7C, 0x08, 0x04, 0x04, 0x78],
            'i': [0x00, 0x04, 0x7D, 0x40, 0x00],
            'n': [0x7C, 0x08, 0x04, 0x04, 0x78],
            'k': [0x00, 0x7F, 0x10, 0x28, 0x44],
            'W': [0x3C, 0x40, 0x30, 0x40, 0x3C],
            'o': [0x38, 0x44, 0x44, 0x44, 0x38],
            'a': [0x20, 0x54, 0x54, 0x54, 0x78],
            'b': [0x7F, 0x48, 0x44, 0x44, 0x38],
            '0': [0x3E, 0x51, 0x49, 0x45, 0x3E],
            '1': [0x00, 0x42, 0x7F, 0x40, 0x00],
            '2': [0x42, 0x61, 0x51, 0x49, 0x46],
            '3': [0x21, 0x41, 0x45, 0x4B, 0x31],
            '4': [0x18, 0x14, 0x12, 0x7F, 0x10],
            '5': [0x27, 0x45, 0x45, 0x45, 0x39],
            '6': [0x3C, 0x4A, 0x49, 0x49, 0x30],
            '7': [0x01, 0x71, 0x09, 0x05, 0x03],
            '8': [0x36, 0x49, 0x49, 0x49, 0x36],
            '9': [0x06, 0x49, 0x49, 0x29, 0x1E],
            '.': [0x00, 0x30, 0x30, 0x00, 0x00],
            ':': [0x00, 0x36, 0x36, 0x00, 0x00],
        }
        cx = x
        for ch in s:
            glyph = FONT.get(ch, [0x7F] * 5)
            for col in range(5):
                byte = glyph[col]
                for row in range(8):
                    if byte & (1 << row):
                        self.fill_rect(cx + col, y + row, 1, 1, color)
            cx += 6


def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if wlan.isconnected():
        return wlan
    print(f"Scanning for {SSID}...")
    results = wlan.scan()
    best = None
    best_rssi = -999
    for r in results:
        name = r[0].decode()
        rssi = r[3]
        if name == SSID and rssi > best_rssi:
            best = r
            best_rssi = rssi

    if best:
        bssid = best[1]
        print(f"Found {SSID} (RSSI:{best_rssi}), connecting...")
        wlan.connect(SSID, PASSWORD, bssid=bssid)
    else:
        print(f"{SSID} not found, trying without BSSID...")
        wlan.connect(SSID, PASSWORD)

    for i in range(20):
        if wlan.isconnected():
            print(f"Connected! IP: {wlan.ifconfig()[0]}")
            return wlan
        time.sleep(0.5)
    print("WiFi connect failed")
    return wlan


def main():
    lcd = Lcd()
    pet = PixelPet(lcd)
    pet.init_screen()
    server = HttpServer(HTTP_PORT)

    def on_command(cmd_type, data):
        if cmd_type == "state":
            state = data.get("state", "idle")
            pet.set_state(state)
        elif cmd_type == "pet":
            if "state" in data:
                pet.set_state(data["state"])
        elif cmd_type == "stats":
            if "tokens" in data:
                pet.token_count = int(data["tokens"])
            if "tasks" in data:
                pet.task_count = int(data["tasks"])
            if "errors" in data:
                pet.error_count = int(data["errors"])

    server.set_handler(on_command)
    server.start()

    wlan = connect_wifi()
    ip = wlan.ifconfig()[0] if wlan.isconnected() else "no IP"
    print(f"Cursor Pet: http://{ip}:{HTTP_PORT}")

    while True:
        server.poll()
        pet.tick()
        time.sleep_ms(10)


if __name__ == "__main__":
    main()
