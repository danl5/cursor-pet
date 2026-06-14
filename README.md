# Cursor Pet

A pixel pet companion for your Cursor AI coding sessions, running on M5Stack StickS3.

## Features

- **3 states with real-time animations**: Idle (blinking), Thinking (thought bubbles), Working (green glow)
- **Procedurally generated** — no image files needed, all animation is code
- **WiFi controlled**: Cursor hooks POST state changes over HTTP
- **Stats display**: Token count, file edits, error count

## Hardware

- M5Stack StickS3 (ESP32-S3, 135x240 LCD)

## Setup

### 1. Configure WiFi

Edit `device/config.py`:
```python
SSID = "your-wifi-name"
PASSWORD = "your-wifi-password"
```

### 2. Upload to StickS3

Upload these 4 files via UiFlow2 File Manager:
- `config.py`
- `main.py`
- `pixel_pet.py`
- `http_server.py`

### 3. Skip UiFlow2 Startup

In WebTerminal:
```python
import esp32
nvs = esp32.NVS("uiflow")
nvs.set_u8("boot_option", 0)
```

Reboot. Device will run your code directly.

### 4. Install Cursor Hooks

```bash
cp hooks/hooks.json /path/to/your/project/.cursor/
cp hooks/notify.sh /path/to/your/project/.cursor/hooks/
cp hooks/on_error.sh /path/to/your/project/.cursor/hooks/
chmod +x /path/to/your/project/.cursor/hooks/*.sh
export CURSOR_PET_IP="192.168.x.x"
```

## API

| Endpoint | Method | Body | Description |
|----------|--------|------|-------------|
| `/api/state` | POST | `{"state":"idle\|thinking\|working"}` | Set pet state |
| `/api/stats` | POST | `{"tokens":N,"tasks":N,"errors":N}` | Update stats |
| `/health` | GET | — | Health check |
