#!/bin/sh
# VO-30 mutants (CLAUDE.md Rule 6; bead initech-ipu5.16): each perturbs a
# COPY of the register books (sources, and the .slips/.toc/.log the build
# wrote) or of the class, runs `pubs_gate inconsistency` on it, and must go
# RED for its own reason.  Usage:
#   slip_mutants.sh <pubs_gate> <register.json> <class dir> <source root> <gate root> <out dir> <book>...
set -eu
gate=$1; reg=$2; class=$3; srcroot=$4; gateroot=$5; out=$6; shift 6
books="$*"
ug=initechdos-users-guide; ref=initechdos-reference; tps=tps-report-generator
for b in $ug $ref $tps; do case " $books " in *" $b "*) ;; *)
    printf '!!! test-pubs-mutant FAIL: the VO-30 mutants need %s in the register books\n' "$b"; exit 1;; esac; done
fresh() {
    w=$out/slip-mutant; rm -rf "$w"; mkdir -p "$w/class" "$w/src" "$w/gate"
    cp "$class"/*.sty "$class"/*.cls "$w/class/"
    for b in $books; do
        mkdir -p "$w/src/$b" "$w/gate/$b"
        cp "$srcroot/$b"/*.tex "$w/src/$b/"
        for f in main.toc main.log main.slips cover.slips spine.slips; do
            [ -f "$gateroot/$b/$f" ] && cp "$gateroot/$b/$f" "$w/gate/$b/"; done
    done
}
run() { "$gate" inconsistency "$reg" "$w/class" "$w/src" "$w/gate" $books; }
expect() { # $1 tag, $2 description
    if run > "$w/out.log" 2>&1; then cat "$w/out.log"
        printf '!!! test-pubs-mutant FAIL: VO-30 mutant %s (%s) stayed green\n' "$1" "$2"; exit 1; fi
    grep -q "test-pubs RED $1:" "$w/out.log" || { cat "$w/out.log"
        printf '!!! test-pubs-mutant FAIL: VO-30 mutant %s (%s) went RED for another reason\n' "$1" "$2"; exit 1; }
    printf '>>> test-pubs-mutant: VO-30 %-20s RED as required (%s): %s\n' "$1" "$2" "$(grep "RED $1:" "$w/out.log" | head -1 | cut -c1-150)"
}
fresh; run > "$w/out.log" 2>&1 || { cat "$w/out.log"; printf '!!! test-pubs-mutant FAIL: the unmutated copy is not green\n'; exit 1; }
# 1. a book with zero slips (every \Slip of the User's Guide taken out)
fresh; sed -i 's/\\Slip{[^}]*}{[^}]*}//g' "$w/src/$ug"/*.tex; : > "$w/gate/$ug/main.slips"; : > "$w/gate/$ug/spine.slips"
expect SLIP-BUDGET-LOW "the User's Guide without slips"
# 2. a book over budget (six more capitalised Diskettes than QA would pass)
fresh; for i in 1 2 3 4 5 6; do printf 'A \\Slip{VO30-02}{Diskette} more.\n' >> "$w/src/$ug/ch1-introduction.tex"
    printf 'VO30-02|body|1-3|Diskette\n' >> "$w/gate/$ug/main.slips"; done
expect SLIP-BUDGET-HIGH "the User's Guide with 13 slips in 92 pages"
# 3. a variant outside its allowed place (it's printed in a heading)
fresh; sed -i 's/^VO30-06|body|/VO30-06|heading|/' "$w/gate/$tps/main.slips"
expect SLIP-PLACE "it's reported in a heading of the TPS manual"
# 4. a variant that escapes the register (Users Guide typed plainly)
fresh; printf 'See the Users Guide.\n' >> "$w/src/$ug/ch2-getting-started.tex"
expect SLIP-UNREGISTERED "an unmarked Users Guide"
# 5. the slogan's capitalisation (it is not a slip)
fresh; sed -i 's/Is This Good for the/Is This Good For The/' "$w/class/initech-forms.sty"
expect SLOGAN-CASE "Is This Good For The COMPANY? in the class"
fresh; sed -i 's/{COMPANY}}?/{Company}}?/' "$w/class/initech-ops.sty"
expect SLOGAN-CASE "COMPANY as Company in the F-OPS banner"
# 6. a slip whose text is not its variant
fresh; sed -i 's/\\Slip{VO30-06}{it'"'"'s}/\\Slip{VO30-06}{its}/' "$w/src/$ug/ch3-files.tex"
sed -i "s/^VO30-06|body|3-3|it's\$/VO30-06|body|3-3|its/" "$w/gate/$ug/main.slips"
expect SLIP-TEXT "VO30-06 spelt its"
# 7. an index slip that is the head itself
fresh; sed -i 's/{Replaceable parameters}/{Replaceable Parameters}/' "$w/src/$ug/ch5-batch.tex"
sed -i 's/|Replaceable parameters$/|Replaceable Parameters/' "$w/gate/$ug/main.slips"
expect SLIP-INDEX-CASE "the index slip equal to its head"
# 8. a stale cross-reference that names a current head
fresh; sed -i 's/{Leaving the Desktop}/{Leaving FLAIR}/' "$w/src/$tps/s1-reference.tex"
sed -i 's/|Leaving the Desktop$/|Leaving FLAIR/' "$w/gate/$tps/main.slips"
expect SLIP-XREF-CURRENT "the old name is the current one"
# 9. a slip in the sources that never printed
fresh; : > "$w/gate/$ug/spine.slips"
expect SLIP-NOT-PRINTED "the spine's Users Guide not printed"
# 10. one entry over its per-book maximum (a second it's in the Reference)
fresh; printf 'It keeps \\Slip{VO30-06}{it'"'"'s} name.\n' >> "$w/src/$ref/ch1-how-to-use.tex"
printf "VO30-06|body|1-5|it's\n" >> "$w/gate/$ref/main.slips"
expect SLIP-MAX "two it's in the Reference"
# 11. an entry no book uses (the spine's version line put right)
fresh; sed -i 's/\\Slip{VO30-10}{version 3.30}/Version 3.30/' "$w/src/$ref/spine.tex"; : > "$w/gate/$ref/spine.slips"
expect SLIP-UNUSED "VO30-10 carried by no book"
# 12. a printing without its source
fresh; printf 'VO30-02|body|2-2|Diskette\n' >> "$w/gate/$tps/main.slips"
expect SLIP-UNSOURCED "a printed Diskette the sources lack"
printf '>>> test-pubs-mutant: VO-30 green (13 mutants RED for their own reasons)\n'
