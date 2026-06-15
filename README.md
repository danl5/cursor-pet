# Cursor Pet

A pixel pet companion for your Cursor AI coding sessions, running on M5Stack StickS3.

## Features

- **3 states with real-time animations**: Idle (blinking), Thinking (thought bubbles), Working (green glow)
- **Procedurally generated** — no image files needed, all animation is drawn in code
- **Double-buffered rendering** — zero flicker via TFT_eSprite
- **WiFi controlled**: Cursor hooks POST state changes over HTTP
- **Easy WiFi setup**: AP mode with web config page, no code changes needed

## Controls

| Button | Action |
|--------|--------|
| **B** (short press) | Toggle between Pet and Settings screen |
| **B** (long press, 1s) | Enter AP mode for WiFi configuration |
| **A** | Return to Pet from Settings screen |

## Hardware

- M5Stack StickS3 (ESP32-S3, 135x240 LCD)

## Prerequisites

1. Install [PlatformIO](https://docs.platformio.org/en/latest/core/installation.html):

```bash
pip install platformio
```

2. Connect StickS3 to your computer via USB cable.

## Build & Flash

```bash
cd arduino
pio run -t upload
```

Open Serial Monitor to verify:

```bash
pio device monitor
```

You should see the pet animation on the LCD and WiFi connection logs in the serial output.

## WiFi Configuration

The device stores WiFi credentials in NVS (flash memory). On first boot or when no config is saved, it enters **AP mode** automatically.

### First-time setup

1. Power on the device — it creates a WiFi hotspot `CursorPet-Setup` (password: `12345678`)
2. Connect your phone to this hotspot
3. Open `http://192.168.4.1` in a browser
4. Enter your WiFi SSID and password, tap **Save & Reboot**
5. Device reboots and connects to your WiFi

### Changing WiFi later

- **Long press B button** (1 second) → enters AP mode for reconfiguration
- **Short press B button** → toggles between Pet and Settings screen (shows WiFi status, IP, etc.)
- **Press A button** → returns to Pet from Settings screen

## Cursor Hooks Setup

### Step 1: Copy hooks to your project

```bash
# In your project directory
mkdir -p .cursor/hooks
cp /path/to/cursor-pet/hooks/hooks.json .cursor/
cp /path/to/cursor-pet/hooks/notify.sh .cursor/hooks/
cp /path/to/cursor-pet/hooks/on_error.sh .cursor/hooks/
chmod +x .cursor/hooks/*.sh
```

### Step 2: Set the device IP

Add to your shell profile (`~/.zshrc` or `~/.bashrc`):

```bash
export CURSOR_PET_IP="192.168.x.x"
```

Find your StickS3's IP from the Serial Monitor output.

### Step 3: Test the connection

```bash
curl http://192.168.x.x/health
```

Should return `{"status":"ok"}`.

## How It Works

```
Cursor Agent
    │
    ├── sessionStart     ──→ notify.sh idle
    ├── afterAgentThought ──→ notify.sh thinking
    ├── preToolUse        ──→ notify.sh working
    ├── postToolUse       ──→ notify.sh working
    ├── stop              ──→ notify.sh idle
    └── postToolUseFailure ──→ on_error.sh
                                    │
                                    ▼
                            StickS3 HTTP Server
                                    │
                                    ▼
                            Pixel Pet Animation
```

## API

| Endpoint | Method | Body | Description |
|----------|--------|------|-------------|
| `/api/state` | POST | `{"state":"idle\|thinking\|working"}` | Set pet state |
| `/api/stats` | POST | `{"tokens":N,"tasks":N,"errors":N}` | Update stats |
| `/api/config` | GET | — | Get WiFi status and config |
| `/api/config` | POST | `{"ssid":"...","password":"..."}` | Save WiFi config & reboot |
| `/api/reboot` | POST | — | Reboot device |
| `/config` | GET | — | Web config page (open in browser) |
| `/health` | GET | — | Health check |

### Example

```bash
# Set state to working
curl -X POST http://192.168.x.x/api/state \
  -H "Content-Type: application/json" \
  -d '{"state":"working"}'

# Update stats
curl -X POST http://192.168.x.x/api/stats \
  -H "Content-Type: application/json" \
  -d '{"tokens":1234,"tasks":5,"errors":0}'
```

## Pet States

| State | Animation | Trigger |
|-------|-----------|---------|
| Sleep | Eyes closed, floating zzz | No active session |
| Idle | Blinking, cheek blush, tail wag | Session connected, nothing running |
| Thinking | Looking up, blue thought bubbles | Agent is reasoning |
| Working | Green glow ring, bouncing paws | Tool execution in progress |
| Error | Red flash, X eyes | Tool failure |

## Project Structure

```
cursor-pet/
├── arduino/
│   ├── platformio.ini        # PlatformIO build config
│   └── src/
│       ├── config.h          # Display/pet constants
│       ├── settings.h        # NVS settings (WiFi config)
│       ├── main.cpp          # Entry point, WiFi, AP mode, buttons
│       ├── pet.h / pet.cpp   # Pixel cat renderer (TFT_eSprite)
│       └── server.h / server.cpp  # HTTP server, config page, JSON API
├── hooks/
│   ├── hooks.json            # Cursor hook configuration
│   ├── notify.sh             # State notification script
│   └── on_error.sh           # Error handler script
└── README.md
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| LCD not displaying | Check USB connection, re-run `pio run -t upload` |
| WiFi connection fails | Long press B to enter AP mode, reconfigure via `192.168.4.1` |
| Pet not responding to Cursor | Verify `CURSOR_PET_IP` env var, test with `curl /health` |
| Flickering | Should not happen with TFT_eSprite; check if using correct firmware |
| Want to reset WiFi | Long press B to enter AP mode, or flash with `pio run -t erase && pio run -t upload` |
