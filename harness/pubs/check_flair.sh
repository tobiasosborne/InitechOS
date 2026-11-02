#!/bin/sh
# check_flair.sh -- the FLAIR books' figure and operation proofs (bead
# initech-ipu5.11; PUB-REF-0001 GC-03/GC-04, NT-20).
#
# Usage: check_flair.sh <gate build dir> <book>...
# The books' sources are read from $FLAIR_BOOKS_ROOT (default pubs/manuals;
# the Rule-6 mutants point it at an isolated copy).
# For every FLAIR book:
#   SCREEN-PAIR       every printed \FlairScreen / \FlairInline / \FlairText
#                     (registry <job>.flairscreens) has its committed capture
#                     screens/<name>.grey or .console and its trace
#                     screens/<name>.mouse
#   SCREEN-UNREPLAYED every printed figure has a capture row in screens/FLAIR
#   SCREEN-UNPRINTED  every committed capture is printed
# and, over the FLAIR books together:
#   OPERATION-UNSHIPPED  every documented desktop operation (\Operation{M},
#                     .cmds KIND operation) is a serial marker the shipped
#                     source prints ("M in os/)
#   OPERATION-UNPROVED   ... and some capture row of these books settles on
#                     it or lists it in its proves field, so the operation is
#                     replayed on the running desktop by `make
#                     test-pubs-flair-screens` (flair_capture.sh checks the
#                     proves markers on the serial capture)
# Thin shell (Law 3).
set -eu
built=$1; shift
rows=$(mktemp)
trap 'rm -f "$rows"' EXIT
for b in "$@"; do
    dir=${FLAIR_BOOKS_ROOT:-pubs/manuals}/$b/screens
    reg=$built/$b/main.flairscreens
    [ -f "$reg" ] || { printf '!!! test-pubs RED SCREEN-ROUTE: %s missing\n' "$reg"; exit 1; }
    [ -f "$dir/FLAIR" ] && sed -e '/^#/d' -e '/^$/d' "$dir/FLAIR" >> "$rows"
    while IFS= read -r fig; do
        [ -n "$fig" ] || continue
        name=${fig#screens/}; name=${name%.*}
        [ -f "$dir/${fig#screens/}" ] && [ -f "$dir/$name.mouse" ] || {
            printf '!!! test-pubs RED SCREEN-PAIR: %s/%s (capture and .mouse)\n' "$dir" "$name"; exit 1; }
        grep -q "^$name|" "$dir/FLAIR" || { printf '!!! test-pubs RED SCREEN-UNREPLAYED: %s/%s\n' "$dir" "$name"; exit 1; }
    done < "$reg"
    for g in "$dir"/*.grey "$dir"/*.console; do
        [ -f "$g" ] || continue
        grep -qxF "screens/$(basename "$g")" "$reg" || { printf '!!! test-pubs RED SCREEN-UNPRINTED: %s\n' "$g"; exit 1; }
    done
done
n=0
for b in "$@"; do
    grep '|operation|' "$built/$b/main.cmds" | cut -d'|' -f1
done | sort -u | while IFS= read -r m; do
    [ -n "$m" ] || continue
    grep -rqF "\"$m" os/ || { printf '!!! test-pubs RED OPERATION-UNSHIPPED: %s\n' "$m"; exit 1; }
    { cut -d'|' -f2 "$rows" | sed 's/#[0-9]*$//'; cut -d'|' -f4 "$rows" | tr ',' '\n'; } | grep -q "^$m" || {
        printf '!!! test-pubs RED OPERATION-UNPROVED: %s (no capture settles on it)\n' "$m"; exit 1; }
done
ops=$(for b in "$@"; do grep -c '|operation|' "$built/$b/main.cmds" || true; done | awk '{s+=$1} END {print s+0}')
figs=$(cat "$rows" | wc -l)
printf '>>> test-pubs-flair-screens-pairs: green (%s capture rows; %s operation entries proved by a capture)\n' "$figs" "$ops"
