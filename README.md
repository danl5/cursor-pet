# Cursor Pet

A pixel pet companion for your Cursor AI coding sessions, running on M5Stack StickS3. Communicates wirelessly via BLE — no WiFi, no IP, no config. Just pair once and it works.

## Features

- **5 reactive states**: Sleep, Idle, Thinking, Working, Error — all with pixel-art animations
- **Activity heatmap**: bottom-screen bars track thoughts (blue), tool calls (green), and session activity (yellow)
- **Coding streak**: daily count with celebrations at 7, 30, 100, 365 days. Positive-only, no punishment for breaks.
- **Shake interaction**: shake the device → character wobbles (@_@). Uses built-in BMI270 IMU.
- **BLE communication**: Nordic UART Service (NUS). Zero WiFi configuration, zero network dependency.
- **Per-device authorization**: first connection requires A-button confirmation. Authorized devices are remembered.
- **6 idle behaviors**: stretch, yawn, tail chase, face wipe, look around, ear flop — randomly triggered
- **3 switchable characters**: Melon, Robot, Sun — cycle with one button (default: Melon)

## Controls

| Button | Context | Action |
|--------|---------|--------|
| **A** | Settings screen | Cycle to next character (saved to flash) |
| **B** (short press) | Pet screen → Settings, Settings → Pet | Toggle between screens |
| **B** (long press, 1s) | Any | Factory reset (clear growth + authorization) |
| **Shake** | Pet screen | Startle the pet (dizzy animation) |

## Characters

Press **B** to enter the settings screen, then press **A** to cycle through characters. Selection persists across reboots. Default is Melon.

| Character | Appearance | Notes |
|-----------|-----------|-------|
| **Melon** | Green rind, red flesh, seed eyes with white highlight | Default. X eyes on error. |
| **Robot** | Silver body, cyan LED eyes, antenna, orange grille | Chest panel blink. |
| **Sun** | Yellow body, orange radiating rays, warm smile | Dimmed sleep mode, red flare on error. |

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

## Pet States & Animations

| State | Sprite | Animation | Trigger |
|-------|--------|-----------|---------|
| **Sleep** | Eyes closed, drooped ears | Floating zzz rising up, slow breathing | No active session or BLE disconnected (3s idle) |
| **Idle** | Eyes open, pink cheek blush | Blinking every 30 frames, tail wagging | Session started, no action |
| **Idle: Stretch** | Eyes occasionally closed | Cat bounces up/leans back, paws extend | Random (3% per tick, after cooldown) |
| **Idle: Yawn** | Eyes closed | Mouth opens/forms pink ellipse, then closes | Random |
| **Idle: Tail Chase** | Eyes open | Cat wiggles left/right, tail whips fast | Random |
| **Idle: Face Wipe** | Eyes open, paw on face | Paw slides across cheek area | Random |
| **Idle: Look Around** | Eyes open | Cat shifts gaze left → center → right | Random |
| **Idle: Ear Flop** | Eyes open | Ears twitch up/down | Random |
| **Thinking** | Eyes looking up | Blue thought bubbles (3 circles rising/floating) | `afterAgentThought` hook |
| **Working** | Eyes open | Green glow ring (2 ellipses pulsing), cat bouncing, paws appear | `preToolUse` / `postToolUse` hook |
| **Error** | X eyes on red-tinted body, frowning | Alternates between error face and normal face (blinking red) | `postToolUseFailure` hook |
| **Shake** | Eyes open, wobbling | Cat shakes with sin/cos oscillation. `!` text above → `@_@` dizzy face → returns. White squiggle lines on dizzy. | Shake device (BMI270 > 2g threshold, 2s cooldown) |
| **Celebrate** | Eyes open, bouncing | 8 colorful sparkle particles rotate outward, "Done!" or streak message below. For streak celebrations: shows reason text ("7 Day Streak!") | Task complete or streak milestone |

## Screen Layout

The display is 135×240 portrait. The cat is a 16×16 hand-drawn pixel sprite scaled to 4× or 6×.

```
╔══════════════════════════╗  0px   ─── top bar (135×28, dark teal)
║ 85%     Idle      Melon ║         battery · state · character
╠══════════════════════════╣  28px
║                          ║
║         [sprite]         ║  184px pet area — centered sprite + overlay
║      64×64 @ 4× scale    ║     · Sleep: zzz floating
║                          ║     · Idle: blinking + idle behaviors
║                          ║     · Thinking: blue thought bubbles
║                          ║     · Working: green glow ring + paws
║                          ║     · Error: X eyes, red accents
║                          ║
╠══════════════════════════╣  212px  ─── footer (135×28, black)
║ T:0 F:3 E:0         D3   ║         stats · streak
║ ████████ ████████ ██████ ║         blue=thoughts green=tools yellow=activity
╚══════════════════════════╝  240px

### Key areas

**Top bar** (0–27px, dark teal `0x10A2`):
- Left: battery % (white)
- Center: state — Sleep / Idle / Think / Work / ERROR
- Right: character name — Melon / Robot / Sun

**Pet area** (28–211px, black):
- Cat sprite centered, scaled 4× (adult) or 6× (baby/kitten)
- Animated effects overlay: zzz, thought bubbles, glow ring, sparkles
- State-specific eye/mouth/color variations (see below)

**Bottom footer** (212–240px):
- `T: tokens  F: tool calls  E: errors` (in-session counters)
- `D#` streak day counter (right-aligned, yellow). At 7/30/100/365 days, triggers celebration.
- Three horizontal activity bars (4px tall at y=226):
  - Blue = thought events
  - Green = tool call events  
  - Yellow = combined session activity

### Sprite details

Each character is a 16×16 pixel sprite drawn at 4× scale (64×64 screen pixels). Sprites are hand-designed RGB565 arrays stored in PROGMEM. Five states per character: eyes open (idle), eyes closed (blink), sleeping, thinking, and error (X eyes with red accents).

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
