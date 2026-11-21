#!/bin/bash
# Использование: server_start.sh <путь к weather_server> <файл для PID>
SERVER="$1"
PIDFILE="$2"

# Вывод сервера уходит в лог: иначе CTest будет ждать закрытия потока вывода
"$SERVER" > "$PIDFILE.log" 2>&1 &
echo $! > "$PIDFILE"

# Ждём до 10 секунд, пока сервер начнёт отвечать
for i in $(seq 1 50); do
    if curl -s -o /dev/null --max-time 1 "http://127.0.0.1:8080/ping"; then
        exit 0
    fi
    sleep 0.2
done

echo "Сервер не запустился за 10 секунд (порт 8080 уже занят?)"
exit 1