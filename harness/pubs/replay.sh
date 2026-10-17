#!/bin/sh
# Replay every emitted Transcript and DotExample (PUB-REF-0001 GC-03).
# Two routes, chosen by the files beside the printed stem.txt:
#   stem.prg  the program route (ipu5.6): samir_replay <prg>, samir_do;
#   stem.dot  the dot-prompt route (ipu5.8, the InitechBase manuals):
#             samir_replay session <scratch> <dot>, the shipped samir_repl,
#             in a FRESH copy of the Sample Programs and Utilities diskette
#             (<disk dir>, harness/pubs/samir_sample_disk.c) for every
#             example, so no example sees what an earlier one changed.
# The printed .txt is the expected output; it is never regenerated here.
# Usage: replay.sh <samir_replay> <pubs_gate> <gate builds> <out> [<disk dir>]
set -eu
runner=$1; gate=$2; builds=$3; out=$4; disk=${5:-}
mkdir -p "$out/replay"
n=0; s=0
for index in "$builds"/*/main.replays; do
    manual=$(basename "$(dirname "$index")")
    while IFS= read -r example; do
        [ -n "$example" ] || continue
        case $example in *'..'* ) : ;; esac
        stem=pubs/manuals/$manual/$example
        [ -f "$stem.txt" ] || { printf '!!! test-pubs RED TRANSCRIPT-PAIR: %s.txt\n' "$stem"; exit 1; }
        n=$((n+1))
        if [ -f "$stem.dot" ]; then
            [ ! -f "$stem.prg" ] || { printf '!!! test-pubs RED TRANSCRIPT-PAIR: %s has both .prg and .dot\n' "$stem"; exit 1; }
            [ -n "$disk" ] && [ -d "$disk" ] || { printf '!!! test-pubs RED SAMPLE-DISK: no sample diskette for %s\n' "$stem"; exit 1; }
            work=$out/replay/disk-$n
            rm -rf "$work"; cp -r "$disk" "$work"; rm -f "$work/.minted"
            "$runner" session "$work" "$stem.dot" > "$out/replay/$n.txt" || {
                printf '!!! test-pubs RED SAMIR-REPLAY: %s.dot did not replay\n' "$stem"; exit 1; }
            rm -rf "$work"; s=$((s+1))
        elif [ -f "$stem.prg" ]; then
            "$runner" "$stem.prg" > "$out/replay/$n.txt"
        else
            printf '!!! test-pubs RED TRANSCRIPT-PAIR: %s has neither .prg nor .dot\n' "$stem"; exit 1
        fi
        "$gate" same TRANSCRIPT-DRIFT "$stem.txt" "$out/replay/$n.txt"
    done < "$index"
done
[ "$n" -gt 0 ] || { printf '!!! test-pubs RED TRANSCRIPT-EMPTY: no examples replayed\n'; exit 1; }
printf '>>> test-pubs-examples: green (%s examples replayed byte for byte: %s dot-prompt sessions, %s programs; screen examples use test-pubs-screens)\n' "$n" "$s" "$((n - s))"
