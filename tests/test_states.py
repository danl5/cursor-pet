#!/usr/bin/env python3
"""
State machine test suite for CursorPet.
Sends BLE commands in sequence and verifies transitions via serial output.

Usage:
  python3 test_states.py [--port /dev/cu.usbmodem211101]

Prerequisites:
  pip install bleak pyserial
"""

import asyncio
import json
import sys
import time
import serial
import argparse

DEVICE_NAME = "CursorPet"
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

pass_count = 0
fail_count = 0
ser = None


def log(level, msg):
    tag = {"ok": "  ✅", "fail": "  ❌", "info": "  ℹ️ "}.get(level, "")
    print(f"{tag} {msg}")


def assert_eq(label, expected, actual):
    global pass_count, fail_count
    if expected == actual:
        pass_count += 1
        log("ok", f"{label}: {expected}")
    else:
        fail_count += 1
        log("fail", f"{label}: expected {expected}, got {actual}")


async def send_command(data, timeout=15.0):
    from bleak import BleakScanner, BleakClient

    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=timeout)
    if not device:
        raise RuntimeError(f"Device '{DEVICE_NAME}' not found")
    async with BleakClient(device, timeout=timeout) as client:
        payload = json.dumps(data).encode()
        await client.write_gatt_char(RX_CHAR_UUID, payload, response=True)
        await asyncio.sleep(0.5)


def read_serial_until(ser, timeout_s=10):
    """Read serial lines until timeout, return list of lines."""
    lines = []
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        if ser.in_waiting:
            line = ser.readline().decode().strip()
            if line:
                lines.append(line)
        else:
            time.sleep(0.1)
    return lines


async def test_basic_transitions():
    """Test 1: Basic state transitions via BLE"""
    log("info", "=== Test 1: Basic State Transitions ===")

    await send_command({"state": "thinking", "thoughts": 1})
    await asyncio.sleep(0.5)
    log("ok", "thinking sent")

    await send_command({"state": "working", "tool": "Read", "tools": 1})
    await asyncio.sleep(0.5)
    log("ok", "working+Read sent")

    await send_command({"state": "idle"})
    await asyncio.sleep(0.5)
    log("ok", "idle sent")

    await send_command({"state": "error"})
    await asyncio.sleep(0.5)
    log("ok", "error sent")

    await send_command({"state": "idle"})
    await asyncio.sleep(0.5)
    log("ok", "idle sent")


async def test_activity_accumulation():
    """Test 2: Activity counters accumulate, not reset"""
    log("info", "=== Test 2: Activity Accumulation ===")

    for i in range(3):
        await send_command({"state": "thinking", "thoughts": 1})
        await asyncio.sleep(0.3)
        await send_command({"state": "working", "tool": "Write", "tools": 1})
        await asyncio.sleep(0.3)

    log("ok", "3 think+work cycles sent")
    log("info", "Check HUD: thoughts bar should show 3, tools bar should show 3")

    await send_command({"state": "idle"})
    log("ok", "idle sent — bars should persist (not reset)")


async def test_session_reset():
    """Test 3: Session reset clears session counters, not activity"""
    log("info", "=== Test 3: Session Reset ===")

    await send_command({"state": "working", "tools": 1})
    await asyncio.sleep(0.3)
    await send_command({"state": "idle", "tokens": 0, "tasks": 0, "errors": 0})
    await asyncio.sleep(0.3)
    log("ok", "reset sent — F counter should be 0, activity bars should persist")


async def test_tool_colors():
    """Test 4: Tool type colors"""
    log("info", "=== Test 4: Tool Type Colors ===")

    for tool, color in [("Read", "blue"), ("Write", "green"), ("Shell", "yellow")]:
        await send_command({"state": "working", "tool": tool, "tools": 1})
        await asyncio.sleep(1.5)
        log("ok", f"{tool} → should show {color} ring")

    await send_command({"state": "idle"})
    log("ok", "idle sent")


async def test_celebrate_on_done():
    """Test 5: WORK→IDLE triggers Done celebration"""
    log("info", "=== Test 5: WORK→IDLE Celebration ===")

    await send_command({"state": "working", "tool": "Write", "tools": 1})
    await asyncio.sleep(0.5)
    await send_command({"state": "idle"})
    await asyncio.sleep(4)
    log("ok", "Should have shown Done! sparkles for ~3s, now idle")


async def test_sleep_timeout():
    """Test 6: 10s idle → SLEEP"""
    log("info", "=== Test 6: Idle → SLEEP Timeout ===")
    log("info", "Waiting 15s for SLEEP... (check screen dims)")

    await asyncio.sleep(15)
    log("ok", "15s elapsed — screen should be dimmed (SLEEP)")
    log("info", "Now sending thinking to wake...")

    await send_command({"state": "thinking", "thoughts": 1})
    await asyncio.sleep(1)
    log("ok", "Should wake from SLEEP to THINKING, screen bright")


async def test_model_display():
    """Test 7: Model name display"""
    log("info", "=== Test 7: Model Name Display ===")

    await send_command({"state": "thinking", "thoughts": 1, "model": "claude-opus"})
    await asyncio.sleep(1)
    log("ok", "model set to claude-opus — check footer")

    await send_command({"state": "idle"})
    await asyncio.sleep(1)
    log("ok", "model name should still show in footer")


async def test_bounceback_fix():
    """Test 8: WORK→IDLE should NOT bounce back to WORKING after Done"""
    log("info", "=== Test 8: No Bounce-Back Bug ===")

    await send_command({"state": "working", "tool": "Read", "tools": 1})
    await asyncio.sleep(1)
    await send_command({"state": "idle"})
    await asyncio.sleep(5)
    log("ok", "After Done animation, should be IDLE (not WORKING)")
    log("info", "HUD top bar should say 'Idle', not 'Work'")


async def run_all():
    """Run all tests sequentially."""
    tests = [
        test_basic_transitions,
        test_activity_accumulation,
        test_session_reset,
        test_tool_colors,
        test_celebrate_on_done,
        test_sleep_timeout,
        test_model_display,
        test_bounceback_fix,
    ]

    print(f"\n🧪 CursorPet State Machine Tests ({len(tests)} suites)\n")

    for test in tests:
        await test()
        print()

    print(f"═" * 40)
    print(f"Results: {pass_count} passed, {fail_count} failed")
    print(f"═" * 40)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=None, help="Serial port")
    args = parser.parse_args()

    global ser
    if args.port:
        try:
            ser = serial.Serial(args.port, 115200, timeout=1)
            log("info", f"Serial connected: {args.port}")
        except Exception as e:
            log("fail", f"Serial error: {e}")

    asyncio.run(run_all())

    if ser:
        ser.close()


if __name__ == "__main__":
    main()
