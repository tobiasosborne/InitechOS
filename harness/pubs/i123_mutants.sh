#!/bin/sh
# Rule 6 for the Initech 123 gates (bead initech-ipu5.9): each new rule is
# shown RED for its own reason on an isolated one-fault fixture.  No tracked
# file is changed.
#   i123_mutants.sh <i123_gate> <i123_face> <pubs_gate> <flair_shot>
#                   <check dir> <built gate dir> <i123_face MENU_SPELLING mutant>
set -eu
gate=$1; face=$2; pgate=$3; shot=$4; out=$5; built=$6; facemut=$7
m=$out/i123-mutants; mkdir -p "$m"
ref=$built/initech123-reference/main.cmds
ship=$out/i123.shipped; refused=$out/i123.refused
red() {
    name=$1; tag=$2; shift 2
    if "$@" > "$m/$name.log" 2>&1; then printf '!!! test-pubs-i123-mutant FAIL: %s survived\n' "$name"; exit 1; fi
    grep -F "test-pubs RED $tag:" "$m/$name.log" || {
        cat "$m/$name.log"; printf '!!! test-pubs-i123-mutant FAIL: %s failed for the wrong reason\n' "$name"; exit 1; }
    printf '>>> test-pubs-i123-mutant: %s rejected\n' "$name"
}
# 1. a documented command the build lacks
{ cat "$ref"; printf '/Worksheet Frobnicate|menu|GATED|/Worksheet\n'; } > "$m/fake.cmds"
red FAKE_MENU_ENTRY DOC-NOT-SHIPPED "$gate" commands "$ship" "$refused" "menu+function+key=$m/fake.cmds"
# 2. one shipped item left undocumented (the first registered @function)
first=$(grep -m1 '|function|' "$ref")
grep -vxF "$first" "$ref" > "$m/dropped.cmds"
red DROPPED_ENTRY SHIPPED-NOT-DOCUMENTED "$gate" commands "$ship" "$refused" "menu+function+key=$m/dropped.cmds"
# 3. a refused leaf documented as working
leaf=$(grep -m1 '|menu|DRAFT|' "$ref")
awk -v l="$leaf" '$0 == l { sub(/\|DRAFT\|/, "|GATED|") } { print }' "$ref" > "$m/gated.cmds"
red GATED_REFUSED_LEAF DOC-GATED-REFUSED "$gate" commands "$ship" "$refused" "menu+function+key=$m/gated.cmds"
# 4. a working command still marked DRAFT
work=$(grep -m1 '^/Copy|menu|GATED|' "$ref")
sed "s#^/Copy|menu|GATED|#/Copy|menu|DRAFT|#" "$ref" > "$m/stale.cmds"
[ -n "$work" ] || { printf '!!! test-pubs-i123-mutant FAIL: /Copy is not registered GATED\n'; exit 1; }
red STALE_DRAFT STALE-DRAFT "$gate" commands "$ship" "$refused" "menu+function+key=$m/stale.cmds"
# 5. an inline mention of a command the build lacks
printf '/Range Frobnicate|menu\n' > "$m/fake.mentions"
red FAKE_MENTION MENTION-NOT-SHIPPED "$gate" mentions "$ship" "$m/fake.mentions"
# 6. one printed result digit changed
ex=$(ls pubs/manuals/initech123-reference/examples/*.i23 | head -1)
"$face" replay "$ex" > "$m/replayed.txt"
sed '0,/[0-9]/ s/\([0-9]\)/X\1/' "${ex%.i23}.txt" > "$m/changed.txt"
red CHANGED_EXAMPLE TRANSCRIPT-DRIFT "$pgate" same TRANSCRIPT-DRIFT "$m/changed.txt" "$m/replayed.txt"
# 7. a FLAIR figure one pixel off its crop (a row)
dump=$(ls "$out"/../flair-check/initech123-reference/*.ppm | head -1)
name=$(basename "$dump" .ppm)
row=$(grep "^$name|" pubs/manuals/initech123-reference/screens/FLAIR)
set -- $(printf '%s' "$row" | cut -d'|' -f2) ; x0=$1; y0=$2; x1=$3; y1=$4
sc=$(printf '%s' "$row" | cut -d'|' -f3)
# one row down, or one row up when the crop already reaches the bottom of
# the 640 x 480 screen (a full-width crop cannot move sideways)
if [ "$y1" -lt 480 ]; then dy=1; else dy=-1; fi
"$shot" png "$dump" "$x0" $((y0 + dy)) "$x1" $((y1 + dy)) "$sc" "$m/shifted.png" spec/assets/clut.json spec/assets/color_canon.json
"$shot" unb64 "pubs/manuals/initech123-reference/screens/$name.b64" "$m/committed.png"
"$shot" same "$m/committed.png" "$(dirname "$dump")/$name.png"
red SHIFTED_FIGURE SCREEN-DRIFT "$shot" same "$m/committed.png" "$m/shifted.png"
# 8. the inventory really is read from the shipped menu resource: a build
#    whose first menu word is misspelled (I123_MUT_MENU_SPELLING, the
#    test-i123-menu-tree-mutant knob) ships /Vorksheet, which no book
#    documents (the gate's first finding; /Worksheet no longer ships)
"$facemut" inventory > "$m/mut.shipped"
red MENU_RESOURCE_MISSPELLED SHIPPED-NOT-DOCUMENTED "$gate" commands "$m/mut.shipped" "$refused" "menu+function+key=$ref"
# 9. canon: the quarterly shares printed as totalling 100%
sed 's/= 116%$/= 100%/' pubs/manuals/initech123-setting-up/examples/quarterly.txt > "$m/quarterly.txt"
red CANON_100 CANON-116 sh harness/pubs/i123_replay.sh canon "$m/quarterly.txt"
# 10. CP-03: a defined term missing from the glossary
ref_dir=$built/initech123-reference
{ cat "$ref_dir/main.terms"; printf 'frobnication\n'; } > "$m/main.terms"
red TERM_NOT_IN_GLOSSARY TERM-NOT-IN-GLOSSARY "$gate" glossary "$m/main.terms" "$ref_dir/main.gloss"
# 11. a printed message (an example's "Message:" line, the replay's try
#     directive) changed by one word
msgs=pubs/manuals/initech123-reference/examples/messages
"$face" replay "$msgs.i23" > "$m/messages.replayed"
sed 's/^Message: Illegal number$/Message: Illegal value/' "$msgs.txt" > "$m/messages.txt"
red CHANGED_MESSAGE TRANSCRIPT-DRIFT "$pgate" same TRANSCRIPT-DRIFT "$m/messages.txt" "$m/messages.replayed"
# 12. a printed refusal the application does not make: try keys that the
#     application accepts (no message, no EDIT)
printf 'try 5~\n' > "$m/accepted.i23"
red TRY_ACCEPTED I123-PROBE "$face" replay "$m/accepted.i23"
# 13. GC-02: a printed message one character off the shipped catalogue
msgs_ref=$built/initech123-reference/main.msgs
{ cat "$msgs_ref"; printf 'Invalid ranges\n'; } > "$m/changed.msgs"
red MESSAGE_NOT_SHIPPED MESSAGE-NOT-SHIPPED "$gate" messages os/i123/app/face_a.c "complete=$m/changed.msgs"
# 14. GC-02: one shipped message left out of the Reference
grep -vx 'Menu nesting error' "$msgs_ref" > "$m/dropped.msgs"
red MESSAGE_UNDOCUMENTED MESSAGE-UNDOCUMENTED "$gate" messages os/i123/app/face_a.c "complete=$m/dropped.msgs"
printf '>>> test-pubs-i123-mutant: green (14 named mutants rejected by their gate legs)\n'
