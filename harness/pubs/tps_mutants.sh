#!/bin/sh
# tps_mutants.sh -- Rule 6 for the TPS Report Generator manual's gates
# (bead initech-ipu5.14; test-pubs-tps, the flair screen route).  Each case
# is ONE fault in an isolated copy, run through the SAME gate leg, and must
# fail with its specific diagnostic.  No tracked file is changed.
# Usage: tps_mutants.sh <tps_inventory> <pubs_gate> <check dir> <book gate dir> <gen dir>
set -eu
inv=$1; gate=$2; out=$3; book=$4; gen=$5
root=$(pwd)
m=$out/tps-mutants
mkdir -p "$m"
red() {
    name=$1; tag=$2; shift 2
    if "$@" > "$m/$name.log" 2>&1; then
        printf '!!! test-pubs-mutant FAIL: %s survived\n' "$name"; exit 1
    fi
    grep -F "test-pubs RED $tag:" "$m/$name.log" || {
        cat "$m/$name.log"; printf '!!! test-pubs-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-mutant: %s rejected\n' "$name"
}
"$inv" inventory spec/tps_ritual.json > "$m/tps.inventory"
# The real book is green against the real inventory (the baseline).
"$inv" check "$m/tps.inventory" "$book/main.cmds" "$out/shipped" > /dev/null
# 1. A documented message the program lacks: the spec (the program's locked
#    table) renames TPS-008; the book still documents TPS-008.
sed 's/"TPS-008 TPSREPT.TXT/"TPS-009 TPSREPT.TXT/' spec/tps_ritual.json > "$m/renamed.json"
"$inv" inventory "$m/renamed.json" > "$m/renamed.inventory"
red PROGRAM_LACKS_MESSAGE DOC-NOT-SHIPPED "$inv" check "$m/renamed.inventory" "$book/main.cmds" "$out/shipped"
# 2. A message typeset in a page the program does not have (the real class,
#    the real generated table, \TpsMsg of an id outside the inventory).
cat > "$m/fake.tex" <<'TEX'
\documentclass[operations]{initech-manual}
\usepackage{initech-ops}
\begin{document}
\TpsMsg{TPS-003}
\TpsMsg{TPS-099}
\end{document}
TEX
SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
TEXINPUTS="$root/pubs/class//:$gen//:" \
"${LUALATEX:-lualatex}" -interaction=batchmode -halt-on-error -output-directory="$m" "$m/fake.tex" > "$m/fake-typeset.log" 2>&1
grep -q '^TPS-099|tps|GATED|TPS-099$' "$m/fake.cmds" || {
    cat "$m/fake.cmds"; printf '!!! test-pubs-mutant FAIL: the class did not register TPS-099\n'; exit 1; }
cat "$book/main.cmds" "$m/fake.cmds" > "$m/fake-book.cmds"
red TYPESET_UNKNOWN_MESSAGE DOC-NOT-SHIPPED "$inv" check "$m/tps.inventory" "$m/fake-book.cmds" "$out/shipped"
# 3. A shipped message the book does not document (both directions).
grep -v '^TPS-008|tps|' "$book/main.cmds" > "$m/dropped.cmds"
red UNDOCUMENTED_MESSAGE SHIPPED-NOT-DOCUMENTED "$inv" check "$m/tps.inventory" "$m/dropped.cmds" "$out/shipped"
# 4. The seven presses left out of the final quiz (bead initech-5igf: the
#    book is the only place the count is written).
grep -v '^QUIZ|tpscount|' "$book/main.cmds" > "$m/nocount.cmds"
red COUNT_NOT_IN_QUIZ COUNT-PLACEMENT "$inv" check "$m/tps.inventory" "$m/nocount.cmds" "$out/shipped"
# 5. An inline command the build does not ship.
{ cat "$book/main.mentions"; printf 'TPSPRINT\n'; } > "$m/mentions"
red UNSHIPPED_MENTION DOC-NOT-SHIPPED "$gate" mentions "$out/shipped" "$m/mentions"
# 6. One byte of a printed journey-route screen against its live re-capture.
sed 's/RETURN CODE 0\./RETURN CODE 9./' pubs/manuals/tps-report-generator/screens/tpsdone.txt > "$m/tpsdone.txt"
cmp -s "$m/tpsdone.txt" pubs/manuals/tps-report-generator/screens/tpsdone.txt && {
    printf '!!! test-pubs-mutant FAIL: the screen mutation changed nothing\n'; exit 1; }
red CHANGED_TPS_SCREEN SCREEN-DRIFT "$gate" same SCREEN-DRIFT "$m/tpsdone.txt" "$root/build/pubs/screens-check/tps-report-generator/tpsdone.txt"
# 7. A printed program file that no gate compares with the running program.
{ cat "$book/main.reports"; printf '../../../harness/diff/tps/old_tpsrept.txt|1|4\n'; } > "$m/reports"
red UNPROVED_REPORT_FILE REPORT-UNPROVED "$inv" reports "$m/reports" \
    ../../../harness/diff/tps/tpsrept_expected.txt ../../../harness/diff/tps/tpsparm.dat ../../../spec/dos_banner.txt
printf '>>> test-pubs-mutant: green (7 TPS-manual mutants rejected by their gate legs)\n'
