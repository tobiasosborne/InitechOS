#!/bin/sh
# GC-03/04: every printed figure has replay input and expected text.
set -eu
builds=$1
for index in "$builds"/*/main.screens; do
    manual=$(basename "$(dirname "$index")")
    dir=pubs/manuals/$manual
    while IFS= read -r figure; do
        [ -n "$figure" ] || continue
        case $figure in screens/*.txt) ;; *) printf '!!! test-pubs RED SCREEN-ROUTE: %s\n' "$figure"; exit 1;; esac
        stem=${figure%.txt}
        [ -f "$dir/$figure" ] && [ -f "$dir/$stem.keys" ] || {
            printf '!!! test-pubs RED SCREEN-PAIR: %s/%s\n' "$dir" "$stem"; exit 1; }
        name=${stem#screens/}
        grep -qF "$name|" "$dir/screens/SCREENS" || {
            printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s\n' "$figure"; exit 1; }
    done < "$index"
    for expected in "$dir"/screens/*.txt; do
        [ -f "$expected" ] || continue
        grep -qxF "screens/$(basename "$expected")" "$index" || {
            printf '!!! test-pubs RED SCREEN-UNPRINTED: %s\n' "$expected"; exit 1; }
    done
done
printf '>>> test-pubs-screen-pairs: green (each printed Screen has keys, expected text and a capture route)\n'
