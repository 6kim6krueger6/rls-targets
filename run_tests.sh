#!/usr/bin/env bash
set -u

FAILURES=()
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

record_failure() {
    FAILURES+=("$1")
}

expect_exit() {
    local expected="$1"
    shift
    local actual=0

    "$@"
    actual=$?
    if [ "$actual" -ne "$expected" ]; then
        record_failure "ожидался код ${expected}, получен ${actual}: $*"
    fi
}

expect_output_contains() {
    local needle="$1"
    shift
    local output=""

    output="$("$@" 2>/dev/null)"
    if ! printf '%s\n' "$output" | grep -Fq "$needle"; then
        record_failure "строка не найдена в выводе: ${needle} ($*)"
    fi
}

echo "Сборка проекта..." >&2
if ! make clean >/dev/null 2>&1 || ! make >/dev/null 2>&1; then
    record_failure "make или make clean завершились с ошибкой"
fi

if [ ! -x "./rls_analyze" ]; then
    record_failure "исполняемый файл ./rls_analyze не найден"
else
    expect_exit 1 ./rls_analyze "несуществующий_файл.csv"

    EMPTY_FILE="$(mktemp /tmp/rls_empty.XXXXXX.csv)"
    : > "$EMPTY_FILE"
    expect_exit 2 ./rls_analyze "$EMPTY_FILE"
    rm -f "$EMPTY_FILE"

    expect_exit 3 ./rls_analyze test_data/sample.csv --top abc
    expect_exit 3 ./rls_analyze test_data/sample.csv --sector 10
    expect_exit 3 ./rls_analyze

    expect_exit 0 ./rls_analyze test_data/sample.csv
    expect_output_contains "=== Отчёт по журналу: sample.csv ===" ./rls_analyze test_data/sample.csv
    expect_output_contains "Ближайшая цель:" ./rls_analyze test_data/sample.csv --top 2
    expect_output_contains "Фильтр: сектор" ./rls_analyze test_data/sample.csv --sector 0.0 180.0
fi

if [ "${#FAILURES[@]}" -eq 0 ]; then
    echo "Все тесты пройдены"
    exit 0
fi

for failure in "${FAILURES[@]}"; do
    echo "FAIL: ${failure}"
done
exit 1
