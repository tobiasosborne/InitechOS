#!/bin/sh
# iword_mutants.sh -- Rule 6 for test-pubs-iword (bead initech-ipu5.10): the
# SAME oracle legs against isolated one-fault fixtures; nothing tracked is
# changed.  Each case must fail with its own named diagnostic.
# usage: iword_mutants.sh <iword_probe> <iword_probe NO_BLOCK_ON>
#                         <iword_probe FKEY_DROP> <pubs_gate> <built books>
#                         <out dir> <allow file>
set -eu
probe=$1; noblk=$2; fkdrop=$3; gate=$4; built=$5; out=$6; allow=$7
root=$(pwd)
m=$out/iword-mutants
rm -rf "$m"; mkdir -p "$m"
shipped=$out/iword/shipped
ref=$built/initechword-reference/main.cmds
[ -s "$shipped" ] || { printf '!!! test-pubs-iword-mutant FAIL: run test-pubs-iword first (no %s)\n' "$shipped"; exit 1; }
red() {
    name=$1; tag=$2; shift 2
    if "$@" > "$m/$name.log" 2>&1; then printf '!!! test-pubs-iword-mutant FAIL: %s survived\n' "$name"; exit 1; fi
    grep -F "$tag" "$m/$name.log" || { cat "$m/$name.log"; printf '!!! test-pubs-iword-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-iword-mutant: %s rejected\n' "$name"
}
# 1 a documented key the build does not have (Shift-F11 does nothing in
#   WORD.EXE: Reference p. 966 note 4, spec/iword/fkeys.json "unmapped")
cp "$ref" "$m/fake.cmds"; printf 'Shift-F11|wpkey|GATED|Fake\n' >> "$m/fake.cmds"
red FAKE_KEY 'test-pubs RED DOC-NOT-SHIPPED:' "$gate" commands "$shipped" "$allow" "$m/fake.cmds"
# 2 an entry's registration dropped: Reveal Codes (Alt-F3) undocumented
grep -v '^Alt-F3|wpkey|' "$ref" > "$m/dropped.cmds"
red DROPPED_ENTRY 'test-pubs RED SHIPPED-NOT-DOCUMENTED:' "$gate" commands "$shipped" "$allow" "$m/dropped.cmds"
# 3 the template strip loses its Exit (F7) cell
grep -E '^((Shift|Alt|Ctrl)-)?F[0-9]+(\|wpkey|=[^|]*\|wpgray)$' "$shipped" > "$m/fkeys"
grep -v '^F7|wpkey|' "$built/initechword-install/strip.cmds" > "$m/strip.cmds"
red TEMPLATE_CELL 'test-pubs RED SHIPPED-NOT-DOCUMENTED:' "$gate" commands "$m/fkeys" "$allow" "$m/strip.cmds"
# 4 running text names a key that does nothing (Shift-F11)
printf 'Shift-F8\nShift-F11\n' > "$m/text.wpkeys"
red UNSHIPPED_KEY 'test-pubs RED DOC-NOT-SHIPPED:' "$gate" keys "$shipped" "$m/text.wpkeys"
# 5 a printed screen changed by one character
got=$(ls "$out/iword/got-initechword-workbook/"*.scr | head -1)
sc=pubs/manuals/initechword-workbook/screens/$(basename "$got")
sed '1s/^\(.\)/X/' "$sc" > "$m/changed.scr"
red CHANGED_SCREEN 'test-pubs RED SCREEN-DRIFT:' "$gate" same SCREEN-DRIFT "$m/changed.scr" "$got"
# 6 a printed keystroke changed: the replay no longer makes the printed screen
k=$(ls "$out/iword/initechword-workbook-"*.keys | head -1)
first=$(sed -n 's/^screen //p' "$k" | head -1)
awk 'BEGIN{d=0} /^type ./ && !d { sub(/^type ./, "type Q"); d=1 } { print }' "$k" > "$m/changed.keys"
mkdir -p "$m/fs" "$m/got"
"$probe" replay "$m/changed.keys" "$m/fs" "$m/got"
red CHANGED_KEYSTROKE 'test-pubs RED SCREEN-DRIFT:' "$gate" same SCREEN-DRIFT "pubs/manuals/initechword-workbook/screens/$first.scr" "$m/got/$first.scr"
# 7 a status-line message added to WORD.EXE without a probe scenario.
#   face_a.c's shared retrieve path now passes `back`, not M_GET_NAME:
#   mutate the message itself, independently of its caller's return mode.
sed 's|error("ERROR: File Not Found",|error("ERROR: Unprobed Message",|' os/iword/app/face_a.c > "$m/face_a.c"
grep -q 'ERROR: Unprobed Message' "$m/face_a.c" || { printf '!!! test-pubs-iword-mutant FAIL: the message mutant did not apply\n'; exit 1; }
mkdir -p "$m/inv"
red UNPROBED_MESSAGE 'test-pubs RED MESSAGE-UNPROBED:' "$probe" inventory "$m/inv" "$m/face_a.c"
# 8 a WORD.EXE that never shows "Block on" (face_a.c IWORD_MUT_NO_BLOCK_ON):
#   its measured inventory lacks the message the Reference documents
mkdir -p "$m/inv2"
"$noblk" inventory "$m/inv2" > "$m/noblk.shipped"
red NO_BLOCK_ON 'test-pubs RED DOC-NOT-SHIPPED:' "$gate" commands "$m/noblk.shipped" "$allow" "$ref"
# 9 a lesson lists a feature the Reference has no entry for
cp "$built/initechword-workbook/main.wpfeatures" "$m/feats"; printf 'Spell\n' >> "$m/feats"
red FOREIGN_FEATURE 'test-pubs RED FEATURES:' sh harness/pubs/iword_features.sh "$built/initechword-reference/main.wpentries" "$m/feats"
# 10 two Reference entries out of letter-by-letter order (AN-11), typeset
cat > "$m/order.tex" <<'TEX'
\documentclass[wordperfect]{initech-manual}
\usepackage{initech-wpc}
\begin{document}
\begin{Feature}{Save}Saves.\end{Feature}
\begin{Feature}{Block}Blocks.\end{Feature}
\end{document}
TEX
red ENTRY_ORDER 'out of letter-by-letter order' env SOURCE_DATE_EPOCH=699408000 FORCE_SOURCE_DATE=1 TZ=UTC \
    TEXMFVAR="$root/build/pubs-tex-cache" TEXMFCACHE="$root/build/pubs-tex-cache" TEXINPUTS="$root/pubs/class//:$root/pubs/common//:" \
    "${LUALATEX:-lualatex}" -interaction=nonstopmode -halt-on-error -output-directory="$m" "$m/order.tex"
# 11 the template prints a grey key under the wrong name (Shift-F7 is
#    Print on the status line): the grey cells are held to the measured
#    names, name for name
sed 's/^Shift-F7=Print|wpgray|/Shift-F7=Printer|wpgray|/' "$built/initechword-install/strip.cmds" > "$m/greyname.cmds"
grep -q '^Shift-F7=Printer|wpgray|' "$m/greyname.cmds" || { printf '!!! test-pubs-iword-mutant FAIL: the grey-name mutant did not apply\n'; exit 1; }
red GREY_NAME 'test-pubs RED DOC-NOT-SHIPPED:' "$gate" commands "$m/fkeys" "$allow" "$m/greyname.cmds"
# 12 a WORD.EXE whose F9 no longer shows End Field (face_a.c
#    IWORD_MUT_FKEY_DROP, the P3 lane's own knob): the measured inventory
#    lacks the grey key the Reference documents
mkdir -p "$m/inv3"
"$fkdrop" inventory "$m/inv3" > "$m/fkdrop.shipped"
red FKEY_DROP 'test-pubs RED DOC-NOT-SHIPPED:' "$gate" commands "$m/fkdrop.shipped" "$allow" "$ref"
# 13 a key of the Search string left undocumented (F6 puts [BOLD] in it)
grep -v '^Srch:F6|wpkey|' "$ref" > "$m/srch.cmds"
red SEARCH_KEY 'test-pubs RED SHIPPED-NOT-DOCUMENTED:' "$gate" commands "$shipped" "$allow" "$m/srch.cmds"
# 14 a message in the SHIPPED branch of a mutant knob (#ifdef
#    IWORD_MUT_SEARCH_NOTFOUND ... #else) is still demanded: only the
#    mutant's own branch is left out of the literal scan
sed 's|            error("\* Not found \*", M_EDIT);|            error("* Nowhere *", M_EDIT);|' os/iword/app/face_a.c > "$m/face_b.c"
grep -q 'error("\* Nowhere \*", M_EDIT);' "$m/face_b.c" || { printf '!!! test-pubs-iword-mutant FAIL: the shipped-branch mutant did not apply\n'; exit 1; }
mkdir -p "$m/inv4"
red SHIPPED_BRANCH_MESSAGE 'test-pubs RED MESSAGE-UNPROBED:' "$probe" inventory "$m/inv4" "$m/face_b.c"
printf '>>> test-pubs-iword-mutant: green (14 named mutants rejected by their gate legs)\n'
