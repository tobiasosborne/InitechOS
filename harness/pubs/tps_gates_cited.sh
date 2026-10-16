#!/bin/sh
# tps_gates_cited.sh -- GC-09 for the Turbo Initech manuals (bead
# initech-ipu5.12): every gate a chapter cites in \Gates{} (initech-
# borland.sty writes each list to <job>.gates) must be a real target of the
# Makefile or of an included makefile, and every chapter cites at least one.
# Usage: tps_gates_cited.sh BOOK.gates...
set -eu
targets=$(cat Makefile pubs/*.mk spec/*.mk 2>/dev/null | awk '
  /^[A-Za-z0-9_.%$(][^:=]*:([^=]|$)/ { sub(/:.*/, ""); for (i = 1; i <= NF; i++) print $i }' | sort -u)
n=0
for list in "$@"; do
    [ -s "$list" ] || { printf '!!! test-pubs RED GATE-CITATION: %s cites no gates\n' "$list" >&2; exit 1; }
    tr ',' '\n' < "$list" | tr -d ' ' | grep -v '^$' | sort -u > "$list.each"
    while read -r g; do
        printf '%s\n' "$targets" | grep -qx -- "$g" || { printf '!!! test-pubs RED GATE-CITATION: %s: no Makefile target %s\n' "$list" "$g" >&2; exit 1; }
        n=$((n + 1))
    done < "$list.each"
done
printf '>>> test-pubs-tps-gates: green (%s distinct gate citations per book, every one a real target)\n' "$n"
