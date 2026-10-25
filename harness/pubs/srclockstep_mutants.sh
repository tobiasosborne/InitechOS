#!/bin/sh
# srclockstep_mutants.sh -- Rule 6 for `pubs_gate srclockstep` (bead
# initech-7opiw; make test-pubs-lockstep-src-mutant).  The baseline copy of
# the three InitechBase books must be GREEN; then each fault is injected into
# a fresh copy (no tracked file is touched), run through the SAME gate, and
# must fail with its specific diagnostic; restoring (a fresh copy) is GREEN.
# Usage: srclockstep_mutants.sh <pubs_gate> <shipped> <allow> <manuals dir> <work dir> <generated dir>
set -eu
gate=$1; shipped=$2; allow=$3; man=$4; w=$5; gen=$6
vo=initechbase-volume-one; qr=initechbase-quick-reference; v2=initechbase-volume-two
fresh() {
    rm -rf "$w"; mkdir -p "$w/manuals" "$w/examples" "$w/gen"
    for b in $vo $qr $v2; do cp -rf "$man/$b" "$w/manuals/$b"; done
    cp -rf "$man/../class" "$man/../common" "$w/"
    cp -rf "$man/../examples/samir" "$w/examples/"
    for t in limits formats coercion messages; do cp -f "$gen/samir_$t.tex" "$w/gen/"; done
}
run() {
    "$gate" srclockstep "$shipped" "$allow" "$w/manuals/$vo" "$w/manuals/$qr" "$w/manuals/$v2" "$w/gen"
}
red() {
    name=$1; tag=$2
    if run > "$w/$name.log" 2>&1; then
        printf '!!! test-pubs-lockstep-src-mutant FAIL: %s survived\n' "$name"; exit 1
    fi
    grep -F "test-pubs RED $tag:" "$w/$name.log" > /dev/null || {
        cat "$w/$name.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-lockstep-src-mutant: %s rejected (%s)\n' "$name" "$tag"
    faults=$((faults + 1))
}
faults=0
fresh
run > "$w/baseline.log" 2>&1 || { cat "$w/baseline.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: baseline is not green\n'; exit 1; }
# 1. Volume One loses the whole ABS function entry (the name is still shipped).
fresh
awk '/^\\begin{XbaseEntry}{ABS}{function}/{skip=1} !skip{print} skip && /^\\end{XbaseEntry}/{skip=0}' "$man/$vo/u6-entries.tex" > "$w/manuals/$vo/u6-entries.tex"
cmp -s "$man/$vo/u6-entries.tex" "$w/manuals/$vo/u6-entries.tex" && { printf '!!! mutant 1 did not apply\n'; exit 1; }
red vo-entry-missing SHIPPED-NOT-DOCUMENTED
# 2. The Quick Reference loses the ABS line.
fresh
sed '/^\\QRFunction{ABS}/d' "$man/$qr/qr-functions.tex" > "$w/manuals/$qr/qr-functions.tex"
cmp -s "$man/$qr/qr-functions.tex" "$w/manuals/$qr/qr-functions.tex" && { printf '!!! mutant 2 did not apply\n'; exit 1; }
red qr-entry-missing SHIPPED-NOT-DOCUMENTED
# 3. An entry for a name the shipped inventory lacks (a non-DRAFT QR line).
fresh
printf '\\QRFunction{NOSUCHFN}{Not shipped}\n' >> "$w/manuals/$qr/qr-functions.tex"
red qr-entry-unshipped DOC-NOT-SHIPPED
# 4. A DotExample whose transcript is gone.
fresh
f=$(sed -n 's/.*\\DotExample{\(examples\/[^}]*\)}.*/\1/p' "$man/$vo/l1-quick-start.tex" | head -1)
rm -f "$w/manuals/$vo/$f.txt"
red example-txt-missing EXAMPLE-PAIR
# 5. A DotExample whose script is gone.
fresh
rm -f "$w/manuals/$vo/$f.dot"
red example-dot-missing EXAMPLE-PAIR
# 6. A duplicated entry in one book (the class's "documented twice" error).
fresh
printf '\\QRFunction{ABS}{Again}\n' >> "$w/manuals/$qr/qr-functions.tex"
red qr-entry-twice DOCUMENTED-TWICE
# Review of 31b286e4: each of these used to be a false GREEN against the
# real class/main.cmds oracle. No unknown TeX is guessed to be harmless.
fresh
sed 's/^\\QRFunction{ABS}{Absolute value of a number}$/\\iffalse\\QRFunction{ABS}{Absolute value of a number}\\fi/' "$man/$qr/qr-functions.tex" > "$w/manuals/$qr/qr-functions.tex"
red conditional-abs SRC-CONDITIONAL
fresh
sed 's/^\\QRFunction{ABS}{Absolute value of a number}$/\\newcommand{\\UnusedAbs}{\\QRFunction{ABS}{Absolute value of a number}}/' "$man/$qr/qr-functions.tex" > "$w/manuals/$qr/qr-functions.tex"
red unused-macro-abs SRC-DEFINITION
fresh
printf '\\def\\UnusedAbs{\\QRFunction{ABS}{Unused}}\n' >> "$w/manuals/$qr/qr-functions.tex"
red def-body SRC-DEFINITION
fresh
awk '/^\\QRFunction\{ABS\}/{print "\\begin{TypeIn}";print;print "\\end{TypeIn}";next} {print}' "$man/$qr/qr-functions.tex" > "$w/manuals/$qr/qr-functions.tex"
red verbatim-abs SRC-VERBATIM
fresh
sed '/^\\QRFunction{ABS}/d' "$man/$qr/qr-functions.tex" > "$w/manuals/$qr/qr-functions.tex"
printf '\\QRFunction{ABS}{After end}\n' >> "$w/manuals/$qr/main.tex"
red entry-after-document SRC-AFTER-END
fresh
printf '\\input stealth\n' >> "$w/manuals/$qr/qr-functions.tex"
printf '\\QRFunction{NOSUCHFN}{Not shipped}\n' > "$w/manuals/$qr/stealth.tex"
red unbraced-input SRC-INPUT
fresh
printf '\\include{stealth}\n' >> "$w/manuals/$qr/qr-functions.tex"
printf '\\QRFunction{NOSUCHFN}{Not shipped}\n' > "$w/manuals/$qr/stealth.tex"
red include-input SRC-UNSUPPORTED
fresh
printf '\\RegisterCommand{NOSUCHFN}{function}{GATED}{NOSUCHFN}\n' >> "$w/manuals/$qr/qr-functions.tex"
red direct-register SRC-UNSUPPORTED
fresh
printf '\\csname QRFunction\\endcsname{NOSUCHFN}{Not shipped}\n' >> "$w/manuals/$qr/qr-functions.tex"
red csname-register SRC-UNSUPPORTED
fresh
printf '\\begin {XbaseEntry}{NOSUCHFN}{function}\\Syntax X\\Usage X\\end{XbaseEntry}\n' >> "$w/manuals/$qr/qr-functions.tex"
red spaced-environment SRC-ENVIRONMENT
fresh
printf '\\input{samir_extra}\n' >> "$w/manuals/$qr/qr-functions.tex"
printf '\\QRFunction{NOSUCHFN}{From common}\n' > "$w/common/samir_extra.tex"
red common-input SRC-INPUT
fresh
printf '\\input{../../common/stealth}\n' >> "$w/manuals/$qr/qr-functions.tex"
printf '\\QRFunction{NOSUCHFN}{From common}\n' > "$w/common/stealth.tex"
red outside-input SRC-INPUT
fresh
sed 's/\\RegisterCommand{#1}{function}{GATED}{#1}/\\RegisterCommand{#1}{command}{GATED}{#1}/' "$man/../class/initech-xbase.sty" > "$w/class/initech-xbase.sty"
red class-registration SRC-SUPPORT
fresh
printf '\\QREntry[review]{NOSUCHFN}{bogus}{NOSUCHFN}{Not yet implemented}\n' >> "$w/manuals/$qr/qr-functions.tex"
red draft-invalid-kind SRC-KIND
fresh
rm -f "$w/examples/samir/salary.txt"
red transcript-txt-missing TRANSCRIPT-PAIR
fresh
rm -f "$w/examples/samir/salary.prg"
red transcript-prg-missing TRANSCRIPT-PAIR
fresh
printf '? 9999\n' >> "$w/manuals/$vo/examples/fn-abs.dot"
red example-typed-line EXAMPLE-TYPED
fresh
printf '\000? 9999\n' >> "$w/manuals/$vo/examples/fn-abs.dot"
red example-hidden-typed-line EXAMPLE-TYPED
fresh
printf '\t\n' >> "$w/manuals/$vo/examples/fn-abs.txt"
red example-unprintable-byte EXAMPLE-BYTES
fresh
printf '\\UnknownEntryWrapper{ABS}\n' >> "$w/manuals/$qr/qr-functions.tex"
red unknown-macro SRC-UNKNOWN
fresh
printf '\\input{stealth}\n' >> "$w/manuals/$qr/qr-functions.tex"
ln -s ../../common/stealth.tex "$w/manuals/$qr/stealth.tex"
printf '\\QRFunction{NOSUCHFN}{From common}\n' > "$w/common/stealth.tex"
red symlink-input SRC-INPUT
fresh
printf '\\ifnum1=0 \\QRFunction{ABS}{Skipped}\\fi\n' >> "$w/manuals/$qr/qr-functions.tex"
red numeric-conditional SRC-CONDITIONAL
fresh
printf 'Not part of the document\n' >> "$w/manuals/$qr/main.tex"
red text-after-document SRC-AFTER-END
fresh
sed 's/\\end{document}/\\end {document}/' "$man/$qr/main.tex" > "$w/manuals/$qr/main.tex"
red spaced-end SRC-ENVIRONMENT
fresh
printf '\\texttt{\\QRFunction{NOSUCHFN}{Hidden}}\n' >> "$w/manuals/$qr/qr-functions.tex"
red entry-in-argument SRC-CONTEXT
fresh
printf '\\xmsg{1}{\\QRFunction{NOSUCHFN}{Hidden}}\n' >> "$w/manuals/$qr/qr-functions.tex"
red entry-in-message SRC-CONTEXT
fresh
printf '\\QRFunction{NOSUCHFN}{Generated entry}\n' >> "$w/gen/samir_messages.tex"
red generated-registration SRC-CONTEXT
fresh
cp -f "$w/gen/samir_messages.tex" "$w/manuals/$qr/samir_messages.tex"
red generated-shadow SRC-INPUT
fresh
rm -f "$w/gen/samir_messages.tex"
red generated-missing SRC-INPUT
fresh
printf '\\input{samir_nonexistent}\n' >> "$w/manuals/$qr/qr-functions.tex"
red unlisted-generated-input SRC-INPUT
fresh
printf '\\QRFunction{AB|S}{Malformed name}\n' >> "$w/manuals/$qr/qr-functions.tex"
red index-delimiter SRC-NAME
fresh
printf '\\QREntry{}{compound}{X}{Empty entry field}\n' >> "$w/manuals/$qr/qr-functions.tex"
red empty-compound-title SRC-NAME
fresh
printf '\\QREntry{BAD|TITLE}{compound}{X}{Extra index field}\n' >> "$w/manuals/$qr/qr-functions.tex"
red compound-delimiter SRC-NAME
fresh
printf '\000\\QRFunction{NOSUCHFN}{Hidden after NUL}\n' >> "$w/manuals/$qr/qr-functions.tex"
red embedded-nul SRC-SYNTAX
fresh
printf '\\directlua{tex.sprint("injected")}\n' >> "$w/manuals/$qr/qr-functions.tex"
red unaudited-lua SRC-UNSUPPORTED
fresh
printf '\\^^51RFunction{NOSUCHFN}{Encoded control word}\n' >> "$w/manuals/$qr/qr-functions.tex"
red encoded-control-word SRC-SYNTAX
fresh
printf '\\string\\QRFunction{NOSUCHFN}{Quoted entry}\n' >> "$w/manuals/$qr/qr-functions.tex"
red quoted-entry SRC-UNKNOWN
# Comments containing entry macros remain comments, never phantom entries.
fresh
printf '%% \\QRFunction{NOSUCHFN}{Comment only}\n' >> "$w/manuals/$qr/qr-functions.tex"
run > "$w/comment.log" 2>&1 || { cat "$w/comment.log"; exit 1; }
# Restore: a fresh copy is GREEN again.
fresh
run > "$w/restored.log" 2>&1 || { cat "$w/restored.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: restored copy is not green\n'; exit 1; }
rm -rf "$w"
printf '>>> test-pubs-lockstep-src-mutant: green (%s faults rejected with their diagnostics; comment and restored copies green)\n' "$faults"
