#!/bin/sh
# GC-11 for InitechBase (bead initech-ipu5.8): the month-end programs of the
# Initech Accounting System are documented as they are.  Two legs:
#   SAMPLE-PROGRAM    the body of each sample program -- every line from its
#                     first "? 'INITECH SYSTEMS CORP" on -- equals the body of
#                     its reference program in harness/diff/dbf_diff/canon,
#                     the programs the SAMIR gates run in the emulator;
#   REFERENCE-OUTPUT  the transcript printed in Programming with InitechBase
#                     holds every non-empty line of the reference output
#                     (canon/*.out), in order, character for character.
# Usage: samir_programs.sh <disk dir> <canon dir> <book dir>
set -eu
disk=$1; ref=$2; book=$3
body() { sed -n "/^? 'INITECH SYSTEMS CORP/,\$p" "$1"; }
check() { prg=$1; refprg=$2; refout=$3; shown=$4
    [ -f "$disk/$prg" ] && [ -f "$ref/$refprg" ] || { printf '!!! test-pubs RED SAMPLE-PROGRAM: %s or %s missing\n' "$prg" "$refprg"; exit 1; }
    [ -n "$(body "$ref/$refprg")" ] || { printf '!!! test-pubs RED SAMPLE-PROGRAM: %s has no body\n' "$refprg"; exit 1; }
    body "$disk/$prg" > "$tmp.a"; body "$ref/$refprg" > "$tmp.b"
    cmp -s "$tmp.a" "$tmp.b" || { diff "$tmp.b" "$tmp.a" | head -5
        printf '!!! test-pubs RED SAMPLE-PROGRAM: %s differs from %s\n' "$disk/$prg" "$refprg"; exit 1; }
    grep -v '^$' "$ref/$refout" > "$tmp.c"
    grep -Fxf "$tmp.c" "$book/$shown" > "$tmp.d" || true
    cmp -s "$tmp.c" "$tmp.d" || { diff "$tmp.c" "$tmp.d" | head -5
        printf '!!! test-pubs RED REFERENCE-OUTPUT: %s does not show %s\n' "$book/$shown" "$refout"; exit 1; }
    printf '>>> test-pubs-samir-programs: %s == %s body, %s shows all %s lines of %s\n' "$prg" "$refprg" "$shown" "$(wc -l < "$tmp.c")" "$refout"
}
tmp=$(mktemp)
trap 'rm -f "$tmp" "$tmp".*' EXIT
check Y2KACCT.PRG y2k_accounting.prg y2k_accounting.out examples/p9-aging.txt
check INTPOST.PRG salami.prg salami.out examples/p10-intpost.txt
printf '>>> test-pubs-samir-programs: green (the month-end programs printed as the reference programs, with their reference output)\n'
