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
# 7. a FLAIR figure one pixel off its crop
dump=$(ls "$out"/../flair-check/initech123-reference/*.ppm | head -1)
name=$(basename "$dump" .ppm)
row=$(grep "^$name|" pubs/manuals/initech123-reference/screens/FLAIR)
set -- $(printf '%s' "$row" | cut -d'|' -f2) ; x0=$1; y0=$2; x1=$3; y1=$4
sc=$(printf '%s' "$row" | cut -d'|' -f3)
"$shot" png "$dump" $((x0 + 1)) "$y0" $((x1 + 1)) "$y1" "$sc" "$m/shifted.png" spec/assets/clut.json spec/assets/color_canon.json
"$shot" unb64 "pubs/manuals/initech123-reference/screens/$name.b64" "$m/committed.png"
"$shot" same "$m/committed.png" "$(dirname "$dump")/$name.png"
red SHIFTED_FIGURE SCREEN-DRIFT "$shot" same "$m/committed.png" "$m/shifted.png"
# 8. the inventory really is read from the shipped menu resource: a build
#    whose first menu word is misspelled (I123_MUT_MENU_SPELLING, the
#    test-i123-menu-tree-mutant knob) no longer ships /Worksheet
"$facemut" inventory > "$m/mut.shipped"
red MENU_RESOURCE_MISSPELLED DOC-NOT-SHIPPED "$gate" commands "$m/mut.shipped" "$refused" "menu+function+key=$ref"
printf '>>> test-pubs-i123-mutant: green (8 named mutants rejected by their gate legs)\n'
