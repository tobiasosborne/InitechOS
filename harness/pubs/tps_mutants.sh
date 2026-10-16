#!/bin/sh
# tps_mutants.sh -- Rule 6 for the Turbo Initech manual gates (bead
# initech-ipu5.12).  Each named mutant is a one-fault copy of a real input
# under build/; the SAME gate leg must go RED with its own tag.  No tracked
# file is changed.
set -eu
: "${GATE:?}" "${BUILT:?}" "${RAILROAD:?}" "${GRAMMAR:?}" "${SHIPPED:?}" "${MSGS:?}" "${OUT:?}" "${TPS_BIN:?}" \
  "${PARSE_RICH:?}" "${FIXTURES:?}"
m=$OUT; rm -rf "$m"; mkdir -p "$m"
red() {
    name=$1; tag=$2; shift 2
    if "$@" > "$m/$name.log" 2>&1; then
        printf '!!! test-pubs-tps-mutant FAIL: %s survived\n' "$name"; exit 1
    fi
    grep -m1 -F "test-pubs RED $tag:" "$m/$name.log" || {
        cat "$m/$name.log"; printf '!!! test-pubs-tps-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-tps-mutant: %s rejected\n' "$name"
}
cmds=$(ls "$BUILT"/turbo-initech-*/main.cmds)
msgs=$(ls "$BUILT"/turbo-initech-*/main.msgs)
# GC-01, documented => shipped: an entry for a routine the RTL lacks.
cat $cmds > "$m/all.cmds"
printf 'Halt|routine|GATED|Halt\n' >> "$m/all.cmds"
red FAKE_ROUTINE DOC-NOT-SHIPPED "$GATE" commands "$SHIPPED" "$m/all.cmds"
# GC-01, shipped => documented: the entry for BlockWrite deleted.
cat $cmds | grep -v '^blockwrite|routine|' > "$m/drop.cmds"
red DROPPED_ENTRY SHIPPED-NOT-DOCUMENTED "$GATE" commands "$SHIPPED" "$m/drop.cmds"
# GC-01 limits: a capacity printed with the wrong value.
cat $cmds | sed 's/^SYMBOLMAX=1,024|limit|/SYMBOLMAX=1,025|limit|/' > "$m/limit.cmds"
red WRONG_LIMIT DOC-NOT-SHIPPED "$GATE" commands "$SHIPPED" "$m/limit.cmds"
# GC-02: one character of a printed diagnostic changed.
cat $msgs | sed 's/^LEX-2|unterminated string/LEX-2|unterminated strinG/' > "$m/changed.msgs"
red CHANGED_MESSAGE MESSAGE-TEXT "$GATE" msgcheck "$MSGS" "$m/changed.msgs"
# CP-18, grammar => parser: an alternative the parser does not have.
sed 's/^factor = "-" factor/factor = "@" factor | "-" factor/' "$GRAMMAR" > "$m/extra.ebnf"
grep -q '"@"' "$m/extra.ebnf" || { printf '!!! test-pubs-tps-mutant FAIL: grammar mutant not applied\n'; exit 1; }
red EXTRA_ALTERNATIVE GRAMMAR-PARSER sh harness/pubs/tps_grammar.sh "$RAILROAD" "$m/extra.ebnf" "$TPS_BIN" "$m/g1" 300 "$PARSE_RICH"
# CP-18, parser => grammar: `downto' dropped from the for statement.
sed 's/( "to" | "downto" )/"to"/' "$GRAMMAR" > "$m/missing.ebnf"
red MISSING_ALTERNATIVE GRAMMAR-INCOMPLETE sh harness/pubs/tps_grammar.sh "$RAILROAD" "$m/missing.ebnf" "$TPS_BIN" "$m/g2" 300 \
    "$FIXTURES/gen_control_deep.pas"
# GC-03: one digit of a printed program output changed.
book=$(ls -d "$BUILT"/turbo-initech-* | head -1); book=$(basename "$book")
first=$(sed -n 's/^program|//p' "$BUILT/$book/main.tpsexamples" | head -1)
[ -n "$first" ] || { printf '!!! test-pubs-tps-mutant FAIL: %s prints no program\n' "$book"; exit 1; }
mkdir -p "$m/tree/$book/examples" "$m/built/$book"
cp "pubs/manuals/$book/examples/$first.pas" "$m/tree/$book/examples/"
tr '0123456789' '1234567890' < "pubs/manuals/$book/examples/$first.out" > "$m/tree/$book/examples/$first.out"
cmp -s "pubs/manuals/$book/examples/$first.out" "$m/tree/$book/examples/$first.out" && {
    printf '!!! test-pubs-tps-mutant FAIL: %s.out has no digit to change\n' "$first"; exit 1; }
printf 'program|%s\noutput|%s\n' "$first" "$first" > "$m/built/$book/main.tpsexamples"
red CHANGED_OUTPUT TRANSCRIPT-DRIFT env EXROOT="$m/tree" BUILT="$m/built" OUT="$m/replay" sh harness/pubs/tps_replay.sh
# GC-09: a chapter cites a gate that does not exist.
printf 'test-pubs-build, test-tps-no-such-gate\n' > "$m/bad.gates"
red FAKE_GATE GATE-CITATION sh harness/pubs/tps_gates_cited.sh "$m/bad.gates"
printf '>>> test-pubs-tps-mutant: green (8 named mutants rejected by their gate legs)\n'
