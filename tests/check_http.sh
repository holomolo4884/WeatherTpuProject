#!/bin/bash
# Использование: check_http.sh <ожидаемый код> <аргументы curl...>
expected="$1"
shift

code=$(curl -s -o /dev/null -w "%{http_code}" --max-time 30 "$@")

if [ "$code" = "$expected" ]; then
    exit 0
fi
echo "Ожидали $expected, получили $code"
exit 1