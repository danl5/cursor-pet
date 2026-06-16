#!/usr/bin/env python3
"""
Cursor Pet BLE bridge.
Sends hook events to the CursorPet BLE device via Nordic UART Service.

Usage:
  notify_ble.py <action>
  notify_ble.py thinking
  notify_ble.py working
  notify_ble.py idle
  notify_ble.py reset
  notify_ble.py error

Prerequisites:
  pip install bleak
"""

import asyncio
import json
import sys
import os

DEVICE_NAME = "CursorPet"
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"


async def send_command(data: dict, timeout: float = 5.0) -> bool:
    try:
        from bleak import BleakScanner, BleakClient
    except ImportError:
        print("bleak not installed. Run: pip install bleak", file=sys.stderr)
        return False

    device = await BleakScanner.find_device_by_name(
        DEVICE_NAME, timeout=timeout
    )

    if not device:
        print(f"Device '{DEVICE_NAME}' not found", file=sys.stderr)
        return False

    try:
        async with BleakClient(device, timeout=timeout) as client:
            payload = json.dumps(data).encode()
            await client.write_gatt_char(RX_CHAR_UUID, payload, response=True)
            await asyncio.sleep(0.3)
            return True
    except Exception as e:
        print(f"BLE write failed: {e}", file=sys.stderr)
        return False


def main():
    action = sys.argv[1] if len(sys.argv) > 1 else "idle"

    data = {}

    if action == "reset":
        data = {"state": "idle", "tokens": 0, "tasks": 0, "errors": 0}
    elif action == "thinking":
        data = {"state": "thinking", "thoughts": 1}
    elif action == "working":
        data = {"state": "working", "tools": 1}
    elif action == "idle":
        data = {"state": "idle"}
    elif action == "error":
        data = {"state": "error"}
    elif action == "sleep":
        data = {"state": "sleep"}
    elif action.startswith("{"):
        try:
            data = json.loads(action)
        except json.JSONDecodeError:
            print(f"Invalid JSON: {action}", file=sys.stderr)
            sys.exit(1)
    else:
        print(f"Unknown action: {action}", file=sys.stderr)
        sys.exit(1)

    success = asyncio.run(send_command(data))
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
