#!/bin/bash
# Parse Cursor hook stdin and forward to BLE device.
# Usage: hook_parse.sh <default_action> [fallback_state]

ACTION="${1:-idle}"
STDIN=$(cat 2>/dev/null)

# Try to extract fields from stdin JSON
TOOL_NAME=$(echo "$STDIN" | python3 -c "
import sys,json
try:
    d=json.load(sys.stdin)
    print(d.get('tool_name',''))
except: pass
" 2>/dev/null)

MODEL=$(echo "$STDIN" | python3 -c "
import sys,json
try:
    d=json.load(sys.stdin)
    print(d.get('model',''))
except: pass
" 2>/dev/null)

# Build JSON payload
PAYLOAD="{\"state\":\"$ACTION\""

# Map tool name to display label
if [ -n "$TOOL_NAME" ]; then
    case "$TOOL_NAME" in
        *read*|*Read*|*grep*|*Glob*|*Grep*)
            LABEL="Read" ;;
        *write*|*Write*|*Edit*|*edit*)
            LABEL="Write" ;;
        *shell*|*Shell*|*Bash*|*bash*|*Terminal*)
            LABEL="Shell" ;;
        *)
            LABEL="Tool" ;;
    esac
    PAYLOAD="$PAYLOAD,\"tool\":\"$LABEL\",\"tools\":1"
elif [ "$ACTION" = "thinking" ]; then
    PAYLOAD="$PAYLOAD,\"thoughts\":1"
fi

if [ -n "$MODEL" ]; then
    PAYLOAD="$PAYLOAD,\"model\":\"$MODEL\""
fi

PAYLOAD="$PAYLOAD}"

DIR="$(dirname "$0")"
exec "$DIR/notify_ble.py" "$PAYLOAD"
