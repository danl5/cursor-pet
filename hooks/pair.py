#!/usr/bin/env python3
"""
One-time pairing script for CursorPet.
Run this once, press A on the device to authorize, done.

Usage:
  python3 pair.py

The device will show 'Allow Connection?' — press A on the StickS3.
After this, Cursor hooks connect without interruption.
"""

import asyncio
import sys

DEVICE_NAME = "CursorPet"
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"


async def pair():
    try:
        from bleak import BleakScanner, BleakClient
    except ImportError:
        print("pip install bleak")
        return False

    print(f"Scanning for {DEVICE_NAME}...")
    device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)

    if not device:
        print(f"\n{DEVICE_NAME} not found.")
        print("Make sure:")
        print("  1. StickS3 is powered on")
        print("  2. Screen shows the pet (not blank)")
        return False

    print(f"Found! Connecting to {device.address}...")
    print(f"\n>>> PRESS A ON THE STICKS3 TO AUTHORIZE <<<\n")

    try:
        async with BleakClient(device, timeout=25.0) as client:
            print("Paired! Device authorized.")
            print("Cursor hooks will now connect without interruption.")
            return True
    except Exception as e:
        if "timeout" in str(e).lower():
            print("\nTimed out - did you press A on the device?")
            print("Run this script again to try once more.")
        else:
            print(f"Error: {e}")
        return False


if __name__ == "__main__":
    success = asyncio.run(pair())
    sys.exit(0 if success else 1)
