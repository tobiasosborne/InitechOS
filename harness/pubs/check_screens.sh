#!/bin/sh
# GC-03/04: every printed figure has replay input and expected text.
set -eu
builds=$1
for index in "$builds"/*/main.screens; do
    manual=$(basename "$(dirname "$index")")
    dir=pubs/manuals/$manual
    while IFS= read -r figure; do
        [ -n "$figure" ] || continue
        case $figure in
        screens/*.txt) ;;
        screens/*.png)
            # FLAIR pixel route (initech-ipu5.9): the PNG, its key script and
            # a FLAIR row (pubs/screens/flair_capture.sh re-captures it).
            stem=${figure%.png}; name=${stem#screens/}
            [ -f "$dir/$figure" ] && [ -f "$dir/$stem.k123" ] || {
                printf '!!! test-pubs RED SCREEN-PAIR: %s/%s\n' "$dir" "$stem"; exit 1; }
            grep -q "^$name|" "$dir/screens/FLAIR" 2>/dev/null || {
                printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s\n' "$figure"; exit 1; }
            continue ;;
        *) printf '!!! test-pubs RED SCREEN-ROUTE: %s\n' "$figure"; exit 1;; esac
        stem=${figure%.txt}
        [ -f "$dir/$figure" ] && [ -f "$dir/$stem.keys" ] || {
            printf '!!! test-pubs RED SCREEN-PAIR: %s/%s\n' "$dir" "$stem"; exit 1; }
        name=${stem#screens/}
        grep -qF "$name|" "$dir/screens/SCREENS" || {
            printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s\n' "$figure"; exit 1; }
    done < "$index"
    for expected in "$dir"/screens/*.png; do
        [ -f "$expected" ] || continue
        grep -qxF "screens/$(basename "$expected")" "$index" || {
            printf '!!! test-pubs RED SCREEN-UNPRINTED: %s\n' "$expected"; exit 1; }
    done
    for expected in "$dir"/screens/*.txt; do
        [ -f "$expected" ] || continue
        grep -qxF "screens/$(basename "$expected")" "$index" || {
            printf '!!! test-pubs RED SCREEN-UNPRINTED: %s\n' "$expected"; exit 1; }
    done
done
printf '>>> test-pubs-screen-pairs: green (each printed Screen has keys, expected text and a capture route)\n'
