#!/bin/sh
# GC-03 for the Initech 123 books (bead initech-ipu5.9): every printed
# example (\I123Example, initech-lotus.sty, listed in <job>.i123ex) is
# replayed through the SHIPPED application on the host (harness/pubs/
# i123_face.c replay: os/i123/app/face_a.c typed into, key by key) and must
# equal the printed .txt byte for byte (pubs_gate same, TRANSCRIPT-DRIFT).
# Every .i23 in a book is printed somewhere (TRANSCRIPT-UNPRINTED).
#   i123_replay.sh check <i123_face> <pubs_gate> <built gate dir> <out dir>
#   i123_replay.sh write <i123_face> - - <out dir>   (rewrites the .txt:
#                                     a deliberate act, review the diff)
#   i123_replay.sh canon <quarterly.txt>   GC-11: the share of quarterly
#                                     results, as printed, totals 116%
#                                     (CANON-116); run by check mode too.
set -eu
mode=$1
canon() {
    grep -qx 'B9: @SUM(B4\.\.B8) = 116%' "$1" || {
        printf '!!! test-pubs RED CANON-116: %s: the quarterly shares must total 116%% (PUB-REF-0001 GC-11, NT-36)\n' "$1"; exit 1; }
}
if [ "$mode" = canon ]; then canon "$2"; printf '>>> test-pubs-i123-canon: green (the quarterly shares total 116%%)\n'; exit 0; fi
face=$2; gate=$3; builds=$4; out=$5
mkdir -p "$out/i123-replay"
n=0
if [ "$mode" = write ]; then
    for ex in pubs/manuals/initech123-*/examples/*.i23; do
        [ -f "$ex" ] || continue
        "$face" replay "$ex" > "${ex%.i23}.txt"
        n=$((n+1))
    done
    printf '>>> pubs-i123-examples: %s printed examples written from the shipped application -- review `git diff`\n' "$n"
    exit 0
fi
for index in "$builds"/initech123-*/main.i123ex; do
    [ -f "$index" ] || continue
    manual=$(basename "$(dirname "$index")")
    while IFS= read -r stem; do
        [ -n "$stem" ] || continue
        s=pubs/manuals/$manual/$stem
        [ -f "$s.i23" ] && [ -f "$s.txt" ] || { printf '!!! test-pubs RED TRANSCRIPT-PAIR: %s\n' "$s"; exit 1; }
        n=$((n+1))
        "$face" replay "$s.i23" > "$out/i123-replay/$n.txt"
        "$gate" same TRANSCRIPT-DRIFT "$s.txt" "$out/i123-replay/$n.txt"
    done < "$index"
    for ex in pubs/manuals/$manual/examples/*.i23; do
        [ -f "$ex" ] || continue
        stem=${ex#pubs/manuals/$manual/}; stem=${stem%.i23}
        grep -qxF "$stem" "$index" || { printf '!!! test-pubs RED TRANSCRIPT-UNPRINTED: %s\n' "$ex"; exit 1; }
    done
done
[ "$n" -gt 0 ] || { printf '!!! test-pubs RED TRANSCRIPT-EMPTY: no Initech 123 examples replayed\n'; exit 1; }
canon pubs/manuals/initech123-setting-up/examples/quarterly.txt
printf '>>> test-pubs-i123-canon: green (the quarterly shares total 116%%, GC-11)\n'
printf '>>> test-pubs-i123-examples: green (%s printed examples replayed through the shipped application, byte for byte)\n' "$n"
