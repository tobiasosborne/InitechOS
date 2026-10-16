#!/bin/sh
# Shared by the build gate and the OVERFULL mutant (GC-07).
set -eu
log=$1; cap=$2
over=$(grep -c '^Overfull \\[hv]box' "$log" || true)
if [ "$over" -gt "$cap" ]; then
    printf '!!! test-pubs RED OVERFULL-CAP: %s: %s overfull boxes (cap %s)\n' "$log" "$over" "$cap"
    exit 1
fi
