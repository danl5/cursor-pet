#!/usr/bin/env python3
"""
Send a command to the CursorPet BLE daemon via Unix socket.

Usage: same as before, e.g.:
  notify_ble.py thinking
  notify_ble.py idle
  notify_ble.py '{"state":"working","tools":1}'
"""

import json
import socket
import sys
import os

SOCKET_PATH = "/tmp/cursorpet.sock"


def main():
    action = sys.argv[1] if len(sys.argv) > 1 else "idle"

    data = {}
    if action == "reset":
        data = {"state": "idle", "tokens": 0, "tasks": 0, "errors": 0}
    elif action == "thinking":
        data = {"state": "thinking", "thoughts": 1}
    elif action == "working":
        data = {"state": "working", "tools": 1}
    elif action in ("idle", "error", "sleep"):
        data = {"state": action}
    elif action.startswith("{"):
        try:
            data = json.loads(action)
        except json.JSONDecodeError:
            print(f"Invalid JSON: {action}", file=sys.stderr)
            sys.exit(1)
    else:
        print(f"Unknown action: {action}", file=sys.stderr)
        sys.exit(1)

    payload = json.dumps(data)

    try:
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.settimeout(2)
        sock.connect(SOCKET_PATH)
        sock.sendall(payload.encode())
        sock.close()
        sys.exit(0)
    except Exception as e:
        print(f"daemon unreachable: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
