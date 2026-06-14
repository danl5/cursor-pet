#!/bin/bash
# Cursor hook: notify StickS3 of state changes
# Usage: ./notify.sh <state> [extra_json]
# States: idle, thinking, working

STICK_IP="${CURSOR_PET_IP:-192.168.1.100}"
STICK_PORT="${CURSOR_PET_PORT:-80}"
BASE_URL="http://${STICK_IP}:${STICK_PORT}"

STATE="${1:-idle}"
EXTRA="${2:-}"

case "$STATE" in
    idle)
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"idle\"}" > /dev/null 2>&1
        ;;
    thinking)
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"thinking\"}" > /dev/null 2>&1
        ;;
    working)
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"working\"}" > /dev/null 2>&1
        ;;
    stats)
        curl -s -X POST "${BASE_URL}/api/stats" \
            -H "Content-Type: application/json" \
            -d "${EXTRA}" > /dev/null 2>&1
        ;;
esac

exit 0
