#!/bin/sh
set -eu
for component do
    out=$(pwd)/build/$component
    mkdir -p "$out"
    host=red
    if make --no-print-directory -C "$component" test-run > "$out/status-host.log" 2>&1; then host=host-green; fi
    state=$host
    if make --no-print-directory -C "$component" -f Makefile -f "$(pwd)/mk/import-check.mk" all import-check > "$out/status-import.log" 2>&1; then
        [ "$host" != host-green ] || state=imported
    elif ! grep -q '^FAIL ' "$out/status-import.log"; then
        echo 'FAIL target: build/audit error (status-import.log)' >> "$out/status-import.log"
    fi
    echo "$component: $state"
    [ "$host" = host-green ] || echo '  FAIL host (status-host.log)'
    sed -n '/^FAIL /p' "$out/status-import.log" | sort -u | sed 's/^/  /'
done
