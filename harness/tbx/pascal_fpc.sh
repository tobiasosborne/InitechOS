#!/bin/sh
# harness/tbx/pascal_fpc.sh -- the Free Pascal leg of the binding
# differentials (factory shell; beads initech-tdnl.22 R5.2, initech-tdnl.23
# R5.3).
#
#   pascal_fpc.sh <TPSIN.PAS> <workdir> <events> <trace.out>
#
# Compiles the SAME composed program (os/tps/FLAIR.PAS + an application) with
# fpc on the host -- {$MODE DELPHI}{$B+}{$H-} come from FLAIR.PAS's first
# line, the ADR-0007 DEC-02 dialect pins -- links it with the C print
# harness (harness/tbx/tbxhost64.c + tbxmodel.c: FLAIR.PAS's externals under
# their declared names), and runs it with the event script as
# tbxevents.txt in the working directory. The trace goes to <trace.out>.
# Environment: FPC TBXHOST64_OBJS (the two harness objects, space-separated).
set -eu
src=$1; w=$2; ev=$3; out=$4
: "${FPC:?}" "${TBXHOST64_OBJS:?}"
rm -rf "$w"; mkdir -p "$w"
cp -f "$src" "$w/app.pas"
k=""
for o in $TBXHOST64_OBJS; do k="$k -k$o"; done
# shellcheck disable=SC2086
(cd "$w" && "$FPC" -B -O- -v0 $k app.pas > fpc.log 2>&1) \
    || { cat "$w/fpc.log" >&2; echo "!!! pascal_fpc: fpc rejected $src" >&2; exit 1; }
cp -f "$ev" "$w/tbxevents.txt"
(cd "$w" && ./app > trace.txt) \
    || { echo "!!! pascal_fpc: the fpc-built program exited non-zero" >&2; exit 1; }
cp -f "$w/trace.txt" "$out"
