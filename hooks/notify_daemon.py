#!/usr/bin/env python3
"""
Cursor Pet BLE daemon — maintains persistent BLE connection.
Listens on a Unix socket for commands from hook scripts.

Start once:
  python3 notify_daemon.py &

Then hooks write to the socket:
  echo '{"state":"thinking","thoughts":1}' | nc -U /tmp/cursorpet.sock

Stop:
  echo 'quit' | nc -U /tmp/cursorpet.sock

The daemon keeps BLE connected; no advertising needed between hooks.
"""

import asyncio
import json
import os
import sys
import signal

DEVICE_NAME = "CursorPet"
SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
SOCKET_PATH = "/tmp/cursorpet.sock"


async def handle_client(reader, writer, client, lock):
    try:
        data = await reader.read(4096)
        if not data:
            return
        text = data.decode().strip()
        if text == "quit":
            print("daemon: quitting")
            sys.exit(0)
        async with lock:
            await client.write_gatt_char(
                RX_CHAR_UUID, text.encode(), response=True
            )
    except Exception as e:
        print(f"daemon: error: {e}", file=sys.stderr)


async def socket_server(client, lock):
    if os.path.exists(SOCKET_PATH):
        os.unlink(SOCKET_PATH)

    server = await asyncio.start_unix_server(
        lambda r, w: handle_client(r, w, client, lock),
        path=SOCKET_PATH,
    )
    print(f"daemon: listening on {SOCKET_PATH}")
    async with server:
        await server.serve_forever()


async def ble_connect():
    from bleak import BleakScanner, BleakClient

    while True:
        print(f"daemon: scanning for {DEVICE_NAME}...")
        device = await BleakScanner.find_device_by_name(
            DEVICE_NAME, timeout=15.0
        )
        if not device:
            print("daemon: device not found, retrying in 5s...")
            await asyncio.sleep(5)
            continue

        print(f"daemon: connecting to {device.address}...")
        try:
            async with BleakClient(device, timeout=30.0) as client:
                print("daemon: connected")
                lock = asyncio.Lock()
                await socket_server(client, lock)
        except Exception as e:
            print(f"daemon: disconnected: {e}, reconnecting...")
            await asyncio.sleep(2)


def main():
    print(f"Cursor Pet BLE daemon (pid={os.getpid()})")
    print(f"  socket: {SOCKET_PATH}")
    print(f"  send: echo '{{\"state\":\"thinking\",\"thoughts\":1}}' | nc -U {SOCKET_PATH}")
    print(f"  quit: echo quit | nc -U {SOCKET_PATH}")
    asyncio.run(ble_connect())


if __name__ == "__main__":
    main()
