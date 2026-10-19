#!/bin/sh
# Rule 6: run the SAME oracle legs against isolated one-fault fixtures.
# No tracked publication, executable or allow-list is ever changed here.
set -eu
gate=$1; replay=$2; out=$3; built=$4; allow=$5; sdisk=${6:-}; sinv=${7:-}
root=$(pwd)
m=$out/mutants
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
# A real manual entry emits its own machine index through the house class.
cat > "$m/fake.tex" <<'TEX'
\documentclass[ibm]{initech-manual}
\begin{document}
\begin{CommandEntry}{PUBSFAKE}{Internal}
\Purpose Does nothing.
\Syntax \SyntaxLine{PUBSFAKE}
\Remarks A nonexistent command.
\Examples No example.
\SeeAlso
\end{CommandEntry}
\end{document}
TEX
SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
TEXINPUTS="$root/pubs/class//:$root/pubs/common//:" \
"${LUALATEX:-lualatex}" -interaction=batchmode -halt-on-error -output-directory="$m" "$m/fake.tex" > "$m/fake-typeset.log" 2>&1
# The fake entry's own main.cmds line joins the real books' lists.
grep -q '^PUBSFAKE|internal|GATED|PUBSFAKE$' "$m/fake.cmds" || {
    cat "$m/fake.cmds"; printf '!!! test-pubs-mutant FAIL: the class did not register PUBSFAKE\n'; exit 1; }
red FAKE_COMMAND DOC-NOT-SHIPPED "$gate" commands "$out/shipped" "$allow" "$m/fake.cmds" \
    "$built/initechdos-reference/main.cmds" "$built/initechdos-users-guide/main.cmds"
# The first dated row represents a genuinely shipped undocumented item.
sed '0,/^[^#]/ { /^[^#]/d; }' "$allow" > "$m/removed.allow"
red REMOVED_ALLOWANCE SHIPPED-NOT-DOCUMENTED "$gate" commands "$out/shipped" "$m/removed.allow" \
    "$built/initechdos-reference/main.cmds" "$built/initechdos-users-guide/main.cmds"
"$replay" pubs/examples/samir/salary.prg > "$m/salary.actual"
sed 's/1225/1226/' pubs/examples/samir/salary.txt > "$m/changed.txt"
red CHANGED_TRANSCRIPT TRANSCRIPT-DRIFT "$gate" same TRANSCRIPT-DRIFT "$m/changed.txt" "$m/salary.actual"
printf 'A chapter with www.\n' > "$m/banned.tex"
red BANNED_WORD BANNED-WORD "$gate" lint pubs/denylist.txt "$m/banned.tex"
printf '\200\n' > "$m/ascii.tex"
red NON_ASCII ASCII "$gate" lint pubs/denylist.txt "$m/ascii.tex"
printf 'Copyright 1994.\n' > "$m/year.tex"
red LATE_YEAR PERIOD-YEAR "$gate" lint pubs/denylist.txt "$m/year.tex"
# The decimal-places exemption (ipu5.8) must not let a year after a point
# through: ".1994" with no digit before the point is still a year.
printf 'Edition .1994 reprinted.\n' > "$m/year2.tex"
red LATE_YEAR_POINT PERIOD-YEAR "$gate" lint pubs/denylist.txt "$m/year2.tex"
printf '\\PartNumber{1024-bad-11}\n\\PrintCode{A1-3/92}\n' > "$m/part.tex"
red BAD_PART METADATA "$gate" lint pubs/denylist.txt "$m/part.tex"
printf '\\PartNumber{1024-3300-11}\n\\PrintCode{A1-13/92}\n' > "$m/print.tex"
red BAD_PRINT_CODE METADATA "$gate" lint pubs/denylist.txt "$m/print.tex"
red ALLOWLIST_GROWTH ALLOWLIST-GROWTH "$gate" monotonic "$allow" "$m/removed.allow"
# A real overfull TeX box, graded by the build gate's cap check.
cat > "$m/overfull.tex" <<'TEX'
\documentclass{article}
\begin{document}
\noindent\hbox{\rule{300mm}{1pt}}
\end{document}
TEX
TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
"${LUALATEX:-lualatex}" -interaction=batchmode -halt-on-error -output-directory="$m" "$m/overfull.tex" > "$m/overfull-typeset.log" 2>&1
red OVERFULL_BOX OVERFULL-CAP sh harness/pubs/check_log.sh "$m/overfull.log" 0
# Captured text is the printed screen source; changing it must break replay.
sed 's/InitechDOS/InitechXOS/' pubs/manuals/initechdos-users-guide/screens/startup.txt > "$m/startup.txt"
red CHANGED_SCREEN SCREEN-DRIFT "$gate" same SCREEN-DRIFT "$m/startup.txt" "$root/build/pubs/screens-check/initechdos-users-guide/startup.txt"
# ---- InitechBase (bead initech-ipu5.8): one named mutant per new rule ----
[ -n "$sdisk" ] && [ -d "$sdisk" ] || { printf '!!! test-pubs-mutant FAIL: no InitechBase sample diskette\n'; exit 1; }
v1=$built/initechbase-volume-one; v2dir=$root/pubs/manuals/initechbase-volume-two
# A real XbaseEntry of the house class registers what it documents.
cat > "$m/xfake.tex" <<'TEX'
\documentclass[ashtontate]{initech-manual}
\begin{document}
\begin{XbaseEntry}{PUBSFAKE}{command}
Does nothing.
\Syntax \SyntaxLine{PUBSFAKE}
\Usage A nonexistent command.
\end{XbaseEntry}
\end{document}
TEX
SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" \
TEXINPUTS="$root/pubs/class//:$root/pubs/common//:" \
"${LUALATEX:-lualatex}" -interaction=batchmode -halt-on-error -output-directory="$m" "$m/xfake.tex" > "$m/xfake-typeset.log" 2>&1
grep -q '^PUBSFAKE|command|GATED|PUBSFAKE$' "$m/xfake.cmds" || {
    cat "$m/xfake.cmds"; printf '!!! test-pubs-mutant FAIL: XbaseEntry did not register PUBSFAKE\n'; exit 1; }
red FAKE_XBASE_ENTRY DOC-NOT-SHIPPED "$gate" commands "$out/samir.shipped" harness/pubs/samir.allow "$m/xfake.cmds" "$v1/main.cmds"
# One shipped command whose entry is removed from Using InitechBase.
grep -v '^ZAP|' "$v1/main.cmds" > "$m/v1-nozap.cmds"
red REMOVED_XBASE_ENTRY SHIPPED-NOT-DOCUMENTED "$gate" commands "$out/samir.shipped" harness/pubs/samir.allow "$m/v1-nozap.cmds"
# One printed character of a replayed dot-prompt session.
rm -rf "$m/session-disk"; cp -r "$sdisk" "$m/session-disk"
"$replay" session "$m/session-disk" pubs/manuals/initechbase-volume-one/examples/c-list.dot > "$m/session.actual"
sed 's/LUMBERGH/LUMBERGX/' pubs/manuals/initechbase-volume-one/examples/c-list.txt > "$m/session.changed"
red CHANGED_SESSION TRANSCRIPT-DRIFT "$gate" same TRANSCRIPT-DRIFT "$m/session.changed" "$m/session.actual"
# One character of a printed catalogue message, and one message left out.
sed 's/^12|Variable not found\.|/12|Variable not founds.|/' "$v1/main.msgs" > "$m/changed.msgs"
red CHANGED_MESSAGE MESSAGE-TEXT "$gate" messages "$out/samir.catalogue" "$m/changed.msgs"
grep -v '^12|Variable not found\.|GATED$' "$v1/main.msgs" > "$m/missing.msgs"
red MISSING_MESSAGE MESSAGE-NOT-DOCUMENTED "$gate" messages "$out/samir.catalogue" "$m/missing.msgs"
# An inline mention of a command the dot prompt does not ship.  JOIN is a
# dBASE III PLUS command InitechBase lacks (INDEX, the old fault, ships
# since samircore/samirfns, so the mutant survived; initech-090x).
printf 'JOIN WITH DEPTS TO STAFFDEP\n' > "$m/unshipped.mentions"
red UNSHIPPED_MENTION DOC-NOT-SHIPPED "$gate" samirmentions "$out/samir.shipped" "$m/unshipped.mentions"
# A month-end program changed on the sample diskette, and its printed
# output changed in the book (GC-11, harness/pubs/samir_programs.sh).
rm -rf "$m/disk-src"; cp -r pubs/examples/samir/disk "$m/disk-src"
sed -i 's/STR(AGE, 6, 0)/STR(AGE, 7, 0)/' "$m/disk-src/Y2KACCT.PRG"
red SAMPLE_PROGRAM_DRIFT SAMPLE-PROGRAM sh harness/pubs/samir_programs.sh "$m/disk-src" harness/diff/dbf_diff/canon "$v2dir"
rm -rf "$m/v2"; mkdir -p "$m/v2"; cp -r "$v2dir/examples" "$m/v2/"
sed -i 's/-36477/-36478/' "$m/v2/examples/p9-aging.txt"
red REFERENCE_OUTPUT_DRIFT REFERENCE-OUTPUT sh harness/pubs/samir_programs.sh pubs/examples/samir/disk harness/diff/dbf_diff/canon "$m/v2"
# The inventory probe must refuse a word the dot prompt refuses.
[ -n "$sinv" ] && [ -x "$sinv" ] || { printf '!!! test-pubs-mutant FAIL: no InitechBase inventory probe\n'; exit 1; }
sinv=$(cd "$(dirname "$sinv")" && pwd)/$(basename "$sinv")
rm -rf "$m/probe"; mkdir -p "$m/probe"
printf 'PUBSFAKE\nLIST\nUPPER\n' | (cd "$m/probe" && "$sinv") > "$m/probe.list"
grep -qx 'LIST|command' "$m/probe.list" && grep -qx 'UPPER|function' "$m/probe.list" && ! grep -q '^PUBSFAKE|' "$m/probe.list" || {
    cat "$m/probe.list"; printf '!!! test-pubs-mutant FAIL: the inventory probe cannot tell a shipped word from PUBSFAKE\n'; exit 1; }
printf '>>> test-pubs-mutant: inventory probe refuses PUBSFAKE, accepts LIST and UPPER()\n'
printf '>>> test-pubs-mutant: green (20 named mutants rejected by their gate legs)\n'
