#!/bin/bash
# Cursor hook: notify StickS3 of state changes and activity
# Usage: ./notify.sh <action> [extra]

STICK_IP="${CURSOR_PET_IP:-192.168.1.100}"
STICK_PORT="${CURSOR_PET_PORT:-80}"
BASE_URL="http://${STICK_IP}:${STICK_PORT}"

ACTION="${1:-idle}"
EXTRA="${2:-}"

send_activity() {
    local thoughts="${1:-0}"
    local tools="${2:-0}"
    curl -s -X POST "${BASE_URL}/api/activity" \
        -H "Content-Type: application/json" \
        -d "{\"thoughts\":${thoughts},\"tools\":${tools}}" > /dev/null 2>&1
}

case "$ACTION" in
    reset)
        curl -s -X POST "${BASE_URL}/api/stats" \
            -H "Content-Type: application/json" \
            -d "{\"tokens\":0,\"tasks\":0,\"errors\":0}" > /dev/null 2>&1
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"idle\"}" > /dev/null 2>&1
        ;;
    idle)
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"idle\"}" > /dev/null 2>&1
        ;;
    thinking)
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"thinking\"}" > /dev/null 2>&1
        send_activity 1 0
        ;;
    working)
        curl -s -X POST "${BASE_URL}/api/state" \
            -H "Content-Type: application/json" \
            -d "{\"state\":\"working\"}" > /dev/null 2>&1
        send_activity 0 1
        ;;
    stats)
        curl -s -X POST "${BASE_URL}/api/stats" \
            -H "Content-Type: application/json" \
            -d "${EXTRA}" > /dev/null 2>&1
        ;;
esac

exit 0
