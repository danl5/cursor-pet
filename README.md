# Cursor Pet

A pixel pet companion for your Cursor AI coding sessions, running on M5Stack StickS3. Communicates wirelessly via BLE — no WiFi, no IP, no config. Just pair once and it works.

## Features

- **5 reactive states**: Sleep, Idle, Thinking, Working, Error — all with pixel-art animations
- **Activity heatmap**: bottom-screen bars track thoughts (blue), tool calls (green), and session activity (yellow)
- **Coding streak**: daily count with celebrations at 7, 30, 100, 365 days. Positive-only, no punishment for breaks.
- **Shake interaction**: shake the device → cat wobbles (@_@). Uses built-in BMI270 IMU.
- **BLE communication**: Nordic UART Service (NUS). Zero WiFi configuration, zero network dependency.
- **Per-device authorization**: first connection requires A-button confirmation. Authorized devices are remembered.
- **Growth system**: Baby → Kitten → Adult → Wizard as tasks accumulate
- **6 idle behaviors**: stretch, yawn, tail chase, face wipe, look around, ear flop — randomly triggered

## Controls

| Button | Action |
|--------|--------|
| **A** | Return to Pet from Settings screen |
| **B** (short press) | Toggle between Pet and Settings screen |
| **B** (long press, 1s) | Factory reset (clear growth data + authorization) |
| **Shake** | Startle the pet (dizzy animation) |

## Hardware

- M5Stack StickS3 (ESP32-S3, 135x240 LCD, BMI270 IMU, BM8563 RTC)

## Prerequisites

1. Install [PlatformIO](https://docs.platformio.org/en/latest/core/installation.html):

```bash
pip install platformio
```

2. Install Python BLE library:

```bash
pip install bleak
```

## Build & Flash

Connect StickS3 via USB, then:

```bash
cd arduino
pio run -t upload
```

View serial output:

```bash
pio device monitor
```

You should see `BMI270 ready`, `BLE advertising as CursorPet`, and the pet animation on the LCD.

## Setup

### Step 1: Pair the device (one-time)

```bash
# In your Cursor project directory
mkdir -p .cursor/hooks
cp /path/to/cursor-pet/hooks/pair.py .cursor/hooks/
chmod +x .cursor/hooks/pair.py
python3 .cursor/hooks/pair.py
```

1. The script scans for `CursorPet` over BLE
2. The StickS3 screen shows **"Allow Connection?"**
3. **Press A** on the StickS3 to authorize
4. The device is now paired — this PC is remembered permanently

To reset authorization, long-press B on the device.

### Step 2: Install Cursor hooks

```bash
cp /path/to/cursor-pet/hooks/hooks.json .cursor/
cp /path/to/cursor-pet/hooks/notify_ble.py .cursor/hooks/
cp /path/to/cursor-pet/hooks/notify.sh .cursor/hooks/
chmod +x .cursor/hooks/notify_ble.py .cursor/hooks/notify.sh
```

No environment variables needed. No IP to configure.

### Step 3: Test

Open Cursor, ask the AI anything. The pet should react in real-time. Or test manually:

```bash
python3 .cursor/hooks/notify_ble.py thinking
python3 .cursor/hooks/notify_ble.py idle
```

## How It Works

```
Cursor Agent
    │
    ├── sessionStart        ──→ notify.sh reset    (idle + zero counters)
    ├── afterAgentThought   ──→ notify.sh thinking (state + activity)
    ├── preToolUse          ──→ notify.sh working   (state + activity)
    ├── postToolUse         ──→ notify.sh working
    ├── stop                ──→ notify.sh idle
    └── postToolUseFailure  ──→ on_error.sh error
                                    │
                                    ▼
                          notify_ble.py (Python BLE client)
                                    │
                              BLE (Nordic UART)
                                    │
                                    ▼
                            StickS3 BLE Server
                                    │
                                    ▼
                            Pixel Pet Animation
```

## BLE Protocol

Device advertises as `CursorPet`. Uses Nordic UART Service (NUS):

| Characteristic | UUID | Direction |
|---------------|------|-----------|
| TX | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` | Device → Host (notify) |
| RX | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` | Host → Device (write) |

Commands are JSON written to the RX characteristic:

```json
{"state": "thinking", "thoughts": 1}
{"state": "working", "tools": 1}
{"state": "idle"}
{"state": "error"}
{"state": "idle", "tokens": 0, "tasks": 0, "errors": 0}
```

## Pet States

| State | Animation | Trigger |
|-------|-----------|---------|
| Sleep | Eyes closed, floating zzz, drooped ears | No active session, BLE disconnected |
| Idle | Blinking, cheek blush, tail wag, random behaviors | Session connected, nothing running |
| Thinking | Looking up, blue thought bubbles | Agent is reasoning |
| Working | Green glow ring, bouncing paws | Tool execution in progress |
| Error | Red flash, X eyes, frowning | Tool failure |
| Shake | Wobbling with @_@ face | Device shaken (BMI270 IMU) |
| Celebrate | Sparkle particles + message | Task milestone or streak achievement |

## HUD

The pet screen shows:

- **Top bar**: battery %, state label, growth stage (Baby/Kitten/Adult/Wizard)
- **Bottom bar**: `T:tokens F:tasks E:errors` + streak day counter (D7, D30, etc.)
- **Activity bars** (bottom edge): blue=thoughts, green=tool calls, yellow=combined activity
- **Settings screen** (press B): BLE status, device name, battery, streak, tasks, growth stage

## Project Structure

```
cursor-pet/
├── arduino/
│   ├── platformio.ini       # PlatformIO build config
│   └── src/
│       ├── config.h                   # Display/pet/ble constants
│       ├── settings.h / settings.cpp  # NVS settings (growth, streak)
│       ├── sprites.h                  # Pixel art sprite data (16x16 cat)
│       ├── main.cpp                   # Entry point, BLE server, buttons, IMU
│       └── pet.h / pet.cpp            # Pixel cat renderer + state machine
├── hooks/
│   ├── hooks.json           # Cursor hook configuration
│   ├── pair.py              # One-time BLE pairing script
│   ├── notify_ble.py        # BLE notification client (main hook)
│   ├── notify.sh            # Hook wrapper → notify_ble.py
│   └── on_error.sh          # Error handler → notify_ble.py error
└── README.md
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| LCD not displaying | Check USB connection, re-run `pio run -t upload` |
| BLE device not found | Make sure StickS3 shows pet on screen (not blank). Reboot if needed. |
| Pairing timeout | Press A on the device within 20 seconds of running `pair.py`. If missed, run it again. |
| Pet not reacting to Cursor | Run `python3 pair.py` to re-authorize. Check serial output for errors. |
| Want to reset everything | Long-press B (1s) — clears growth data and authorization. Re-pair after reset. |
| BLE disconnected randomly | Normal BLE behavior — the pet goes to Sleep state until next hook fires. Hooks auto-reconnect. |
| Shake not working | BMI270 IMU may not be initialized. Check serial output for "BMI270 ready". |
