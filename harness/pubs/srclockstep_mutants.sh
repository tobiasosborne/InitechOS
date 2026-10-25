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
    cp -f "$man/../entry-index.sh" "$w/"
    cp -rf "$man/../examples/samir" "$w/examples/"
    for t in limits formats coercion messages; do cp -f "$gen/samir_$t.tex" "$w/gen/"; done
    for b in $vo $qr; do
        for idx in "$gen"/entries-"$b"-*.tex; do cp -f "$idx" "$w/gen/"; done
    done
}
refresh() { sh "$w/entry-index.sh" "$w/manuals/$1/entries/$2" > "$w/gen/entries-$1-$2.tex"; }
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
# 1. Volume One loses ABS's file; a refreshed index must expose the missing entry.
fresh
rm -f "$w/manuals/$vo/entries/fn/abs.tex"
refresh "$vo" fn
red vo-entry-missing SHIPPED-NOT-DOCUMENTED
# 2. The Quick Reference loses ABS's file, with its index refreshed too.
fresh
rm -f "$w/manuals/$qr/entries/fn/abs.tex"
refresh "$qr" fn
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
sed 's/^\\QRFunction{ABS}{Absolute value of a number}$/\\iffalse\\QRFunction{ABS}{Absolute value of a number}\\fi/' "$man/$qr/entries/fn/abs.tex" > "$w/manuals/$qr/entries/fn/abs.tex"
red conditional-abs SRC-CONDITIONAL
fresh
sed 's/^\\QRFunction{ABS}{Absolute value of a number}$/\\newcommand{\\UnusedAbs}{\\QRFunction{ABS}{Absolute value of a number}}/' "$man/$qr/entries/fn/abs.tex" > "$w/manuals/$qr/entries/fn/abs.tex"
red unused-macro-abs SRC-DEFINITION
fresh
printf '\\def\\UnusedAbs{\\QRFunction{ABS}{Unused}}\n' >> "$w/manuals/$qr/qr-functions.tex"
red def-body SRC-DEFINITION
fresh
awk '/^\\QRFunction\{ABS\}/{print "\\begin{TypeIn}";print;print "\\end{TypeIn}";next} {print}' "$man/$qr/entries/fn/abs.tex" > "$w/manuals/$qr/entries/fn/abs.tex"
red verbatim-abs SRC-VERBATIM
fresh
rm -f "$w/manuals/$qr/entries/fn/abs.tex"
refresh "$qr" fn
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
# F4 integration: compare against the DIRECTORY-derived index before any
# command coverage check. Wrong-group moves regenerate both indexes so the
# kind/group check, rather than a stale-index diagnostic, must reject them.
fresh
printf '\\QRFunction{STRAY}{Omitted by index}\n' > "$w/manuals/$qr/entries/fn/stray.tex"
red omitted-entry-file SRC-ENTRY-INDEX
fresh
printf '\\input{entries/fn/phantom}%%\n' >> "$w/gen/entries-$qr-fn.tex"
red phantom-index-line SRC-ENTRY-INDEX
fresh
mv -f "$w/manuals/$qr/entries/fn/abs.tex" "$w/manuals/$qr/entries/cmd/abs.tex"
refresh "$qr" fn
refresh "$qr" cmd
red wrong-entry-group SRC-ENTRY-GROUP
fresh
awk 'NR==1{print;next} {a[NR]=$0} END{for(i=NR;i>=2;i--)print a[i]}' "$gen/entries-$qr-fn.tex" > "$w/gen/entries-$qr-fn.tex"
red reordered-entry-index SRC-ENTRY-INDEX
fresh
printf '\\QRFunction{HIDDEN}{Hidden entry}\n' > "$w/manuals/$qr/entries/fn/.hidden.tex"
red hidden-entry-file SRC-ENTRY-INDEX
# Comments containing entry macros remain comments, never phantom entries.
fresh
printf '%% \\QRFunction{NOSUCHFN}{Comment only}\n' >> "$w/manuals/$qr/qr-functions.tex"
run > "$w/comment.log" 2>&1 || { cat "$w/comment.log"; exit 1; }
# Restore: a fresh copy is GREEN again.
fresh
run > "$w/restored.log" 2>&1 || { cat "$w/restored.log"; printf '!!! test-pubs-lockstep-src-mutant FAIL: restored copy is not green\n'; exit 1; }
rm -rf "$w"
printf '>>> test-pubs-lockstep-src-mutant: green (%s faults rejected with their diagnostics; comment and restored copies green)\n' "$faults"
