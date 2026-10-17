#!/bin/sh
# iword_check.sh -- the InitechWord manuals' correctness gate (bead
# initech-ipu5.10; PUB-REF-0001 GC-01, GC-02, GC-03, CP-16, CP-17, PK-14).
#
# usage: iword_check.sh check|update <iword_probe> <pubs_gate> <built books>
#                       <out dir> <allow file>
# The books are built first (test-pubs-build); the class wrote, beside each
# PDF, the lists this gate reads (pubs/class/initech-wpc.sty):
#   main.cmds       NAME|KIND|STATE|ENTRY, the ONE registration list
#   *.wpkeys        keys named in running text (\K)
#   main.wpreplay   the keystrokes the book prints, BEGIN <name> .. END per
#                   launch of InitechWord, with "screen <name>" where a
#                   printed screen must be what the replay shows
#   main.wpscreens  the screens the book prints
#   main.wpentries  the Reference's feature entries; main.wpfeatures the
#                   Workbook's lesson feature lists
# Legs (each RED with its own tag):
#   INVENTORY   iword_probe inventory: the shipped surface of WORD.EXE
#   COMMANDS    the Reference documents every shipped key, menu choice,
#               pull-down item, code and message, and nothing else (both
#               directions, pubs_gate commands; GC-01, GC-02, CP-16)
#   TEMPLATE    the function-key template's cells == the shipped F-key
#               bindings, both directions (PK-14: "generated from the shipped
#               key map"), the grey cells == the keys that show a grey name,
#               name for name
#   KEYS        every key named in running text is a shipped key
#   FEATURES    every feature a Workbook lesson lists is a Reference entry
#               (CP-17: the Feature List appendices are built from them)
#   REPLAY      every printed keystroke sequence replayed on the real
#               WORD.EXE (iword_probe replay); every printed screen equals
#               the screen the replay produced, byte for byte (GC-03)
#   PRINTED     every committed screen is printed, every printed one is
#               committed, no screen was missing at typesetting
# `update` re-runs the replays and writes the screens into the source tree
# instead of comparing (a deliberate act: review the diff).
# Thin shell (Law 3).
set -eu
mode=$1; probe=$2; gate=$3; built=$4; out=$5; allow=$6
red() { printf '!!! test-pubs RED %s: %s\n' "$1" "$2"; exit 1; }
w=$out/iword
rm -rf "$w"; mkdir -p "$w/inv-fs"
books="initechword-reference initechword-workbook initechword-install"
for b in $books; do [ -d "$built/$b" ] || [ "$mode" = update ] || red IWORD-BOOKS "$built/$b not built"; done

# INVENTORY
"$probe" inventory "$w/inv-fs" > "$w/shipped"
[ "$(wc -l < "$w/shipped")" -ge 100 ] || red INVENTORY "fewer than 100 shipped items -- the probe did not run"
printf '>>> test-pubs-iword: inventory %s shipped items (%s keys, %s grey keys, %s menu choices, %s pull-down items, %s codes, %s messages)\n' \
    "$(wc -l < "$w/shipped")" "$(grep -c '|wpkey$' "$w/shipped")" "$(grep -c '|wpgray$' "$w/shipped")" "$(grep -c '|wpmenu$' "$w/shipped")" \
    "$(grep -c '|wppull$' "$w/shipped")" "$(grep -c '|wpcode$' "$w/shipped")" "$(grep -c '|wpmsg$' "$w/shipped")"

if [ "$mode" = check ]; then
    # COMMANDS
    "$gate" commands "$w/shipped" "$allow" "$built/initechword-reference/main.cmds"
    # TEMPLATE
    grep -E '^((Shift|Alt|Ctrl)-)?F[0-9]+(\|wpkey|=[^|]*\|wpgray)$' "$w/shipped" > "$w/fkeys"
    "$gate" commands "$w/fkeys" "$allow" "$built/initechword-install/strip.cmds"
    # the other documents register nothing (the Reference is the one list)
    for c in "$built/initechword-workbook/main.cmds" "$built/initechword-install/main.cmds"; do
        [ -z "$(grep -v '^#' "$c" || true)" ] || red COMMAND-SURFACE "$c registers entries; the Reference owns them"
    done
    # KEYS
    "$gate" keys "$w/shipped" "$built"/initechword-*/*.wpkeys
    # FEATURES
    sh harness/pubs/iword_features.sh "$built/initechword-reference/main.wpentries" "$built/initechword-workbook/main.wpfeatures"
fi

# REPLAY
n=0; s=0
for b in $books; do
    src=pubs/manuals/$b
    idx=$built/$b/main.wpreplay
    [ -f "$idx" ] || continue
    fs=$w/fs-$b; got=$w/got-$b
    mkdir -p "$fs" "$got"
    awk -v d="$w" -v b="$b" '
        /^BEGIN / { if (open) { print "nested BEGIN" > "/dev/stderr"; exit 1 } open = 1; k++; f = sprintf("%s/%s-%03d.keys", d, b, k); printf "" > f; next }
        /^END$/   { if (!open) { print "END without BEGIN" > "/dev/stderr"; exit 1 } open = 0; close(f); next }
        { if (!open) { print "keys outside a replay" > "/dev/stderr"; exit 1 } print > f }
        END { if (open) { print "unterminated replay" > "/dev/stderr"; exit 1 } }' "$idx" || red REPLAY "$idx is not well formed"
    for k in "$w/$b"-*.keys; do
        [ -f "$k" ] || continue
        "$probe" replay "$k" "$fs" "$got" || red REPLAY "$k died"
        n=$((n + 1))
    done
    for sc in $(sed -n 's/^screen //p' "$idx"); do
        [ -f "$got/$sc.scr" ] || red REPLAY "$b: the replay made no screen $sc"
        if [ "$mode" = update ]; then
            mkdir -p "$src/screens"; cp -f "$got/$sc.scr" "$src/screens/$sc.scr"
        else
            [ -f "$src/screens/$sc.scr" ] || red PRINTED "$src/screens/$sc.scr is printed but not committed"
            "$gate" same SCREEN-DRIFT "$src/screens/$sc.scr" "$got/$sc.scr" || { diff -u "$src/screens/$sc.scr" "$got/$sc.scr" | head -20 || true; exit 1; }
        fi
        s=$((s + 1))
    done
    [ "$(sed -n 's/^screen //p' "$idx" | sort | uniq -d | wc -l)" -eq 0 ] || red REPLAY "$b: a screen name is used twice"
    if [ "$mode" = check ]; then
        for c in "$src"/screens/*.scr; do
            [ -f "$c" ] || continue
            grep -qxF "$(basename "$c" .scr)" "$built/$b/main.wpscreens" || red PRINTED "$c is committed but not printed"
        done
        if grep -q 'initech-wpc: MISSING SCREEN' "$built/$b/main.log"; then red PRINTED "$b was typeset with a missing screen"; fi
    fi
done
[ "$n" -gt 0 ] || red REPLAY "no keystroke sequences replayed"
if [ "$mode" = update ]; then
    printf '>>> pubs-iword-screens: %s replays, %s screens written into pubs/manuals -- review the diff, then typeset again\n' "$n" "$s"
else
    printf '>>> test-pubs-iword: green (%s launches of WORD.EXE replayed, %s printed screens == the replay byte for byte)\n' "$n" "$s"
fi
