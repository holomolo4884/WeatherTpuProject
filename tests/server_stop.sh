#!/bin/bash
# Использование: server_stop.sh <файл с PID>
PIDFILE="$1"
if [ -f "$PIDFILE" ]; then
    kill "$(cat "$PIDFILE")" 2>/dev/null
    rm -f "$PIDFILE"
fi
exit 0