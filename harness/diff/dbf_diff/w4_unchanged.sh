#!/bin/sh
# L1 read-only contract (HELP / smoke), DEC-03a.10, initech-9nzpr.
# Run with two extracted trees, or in a w4_trace output with before/ and files/.
set -eu
if ! diff -r "${1:-before}" "${2:-files}"; then
    echo 'RED: L1 read-only trace changed file names or bytes'; exit 1
fi
echo 'GREEN: L1 read-only: no file changes (all names and bytes)'
