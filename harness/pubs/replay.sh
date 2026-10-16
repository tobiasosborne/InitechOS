#!/bin/sh
# Replay every emitted Transcript, including the real InitechBase specimen.
set -eu
runner=$1; gate=$2; builds=$3; out=$4
mkdir -p "$out/replay"
n=0
for index in "$builds"/*/main.replays; do
    manual=$(basename "$(dirname "$index")")
    while IFS= read -r example; do
        [ -n "$example" ] || continue
        case $example in *'..'* ) : ;; esac
        stem=pubs/manuals/$manual/$example
        [ -f "$stem.prg" ] && [ -f "$stem.txt" ] || { printf '!!! test-pubs RED TRANSCRIPT-PAIR: %s\n' "$stem"; exit 1; }
        n=$((n+1))
        "$runner" "$stem.prg" > "$out/replay/$n.txt"
        "$gate" same TRANSCRIPT-DRIFT "$stem.txt" "$out/replay/$n.txt"
        printf '>>> test-pubs-examples: %s.prg == printed transcript\n' "$stem"
    done < "$index"
done
[ "$n" -gt 0 ] || { printf '!!! test-pubs RED TRANSCRIPT-EMPTY: no examples replayed\n'; exit 1; }
printf '>>> test-pubs-examples: green (%s SAMIR programs replayed byte for byte; screen examples use test-pubs-screens)\n' "$n"
