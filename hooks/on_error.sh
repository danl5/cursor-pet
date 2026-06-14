#!/bin/bash
# Cursor hook: error handler - shake the pet and increment error count
# Called by postToolUseFailure

STICK_IP="${CURSOR_PET_IP:-192.168.1.100}"
STICK_PORT="${CURSOR_PET_PORT:-80}"
BASE_URL="http://${STICK_IP}:${STICK_PORT}"

# Read input from stdin
INPUT=$(cat)
ERROR_MSG=$(echo "$INPUT" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d.get('error_message','unknown error'))" 2>/dev/null || echo "unknown error")

# Notify stick of error
curl -s -X POST "${BASE_URL}/api/stats" \
    -H "Content-Type: application/json" \
    -d "{\"errors\":1}" > /dev/null 2>&1

curl -s -X POST "${BASE_URL}/api/state" \
    -H "Content-Type: application/json" \
    -d "{\"state\":\"error\",\"message\":\"${ERROR_MSG}\"}" > /dev/null 2>&1

exit 0
