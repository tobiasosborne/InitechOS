#!/bin/sh
# Rule 6: run the SAME oracle legs against isolated one-fault fixtures.
# No tracked publication, executable or allow-list is ever changed here.
set -eu
gate=$1; replay=$2; out=$3; built=$4; allow=$5
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
TEXINPUTS="$root/pubs/class//:" \
"${LUALATEX:-lualatex}" -interaction=batchmode -halt-on-error -output-directory="$m" "$m/fake.tex" > "$m/fake-typeset.log" 2>&1
cat "$built/initechdos-users-guide/main.commands" "$m/fake.commands" > "$m/fake.index"
red FAKE_COMMAND DOC-NOT-SHIPPED "$gate" commands "$out/shipped" "$m/fake.index" "$allow"
# The first dated row represents a genuinely shipped undocumented item.
sed '0,/^[^#]/ { /^[^#]/d; }' "$allow" > "$m/removed.allow"
red REMOVED_ALLOWANCE SHIPPED-NOT-DOCUMENTED "$gate" commands "$out/shipped" "$built/initechdos-users-guide/main.commands" "$m/removed.allow"
"$replay" pubs/examples/samir/salary.prg > "$m/salary.actual"
sed 's/1225/1226/' pubs/examples/samir/salary.txt > "$m/changed.txt"
red CHANGED_TRANSCRIPT TRANSCRIPT-DRIFT "$gate" same TRANSCRIPT-DRIFT "$m/changed.txt" "$m/salary.actual"
printf 'A chapter with www.\n' > "$m/banned.tex"
red BANNED_WORD BANNED-WORD "$gate" lint pubs/denylist.txt "$m/banned.tex"
printf '\200\n' > "$m/ascii.tex"
red NON_ASCII ASCII "$gate" lint pubs/denylist.txt "$m/ascii.tex"
printf 'Copyright 1994.\n' > "$m/year.tex"
red LATE_YEAR PERIOD-YEAR "$gate" lint pubs/denylist.txt "$m/year.tex"
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
printf '>>> test-pubs-mutant: green (11 named mutants rejected by their gate legs)\n'
