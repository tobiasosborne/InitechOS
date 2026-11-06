#!/bin/sh
set -eu
out=build/w4-inventory-mutants
mkdir -p "$out"
tool=build/w4_inventory
src=spec/samir/screen_regrade.json
red() {
    rc=0; "$tool" "$1" > "$out/$2.log" 2>&1 || rc=$?
    [ "$rc" = 1 ] && grep -q "$3" "$out/$2.log" || { cat "$out/$2.log"; echo "RED: inventory mutant $2 survived/wrong reason"; exit 1; }
    printf 'RED proven: inventory %s: ' "$2"; tail -1 "$out/$2.log"
}
rm -f "$out/missing.json"
red "$out/missing.json" missing 'missing JSON'
"$tool" --remove-first "$src" > "$out/row.json"
red "$out/row.json" drop-row 'expected ten W4 app groups'
"$tool" --remove-mutant "$src" > "$out/mutant.json"
red "$out/mutant.json" drop-mutant 'missing/orphan structural mutant'
"$tool" --remove-l1 "$src" > "$out/l1.json"
red "$out/l1.json" drop-dbt 'required L1 kind/tool dropped'
"$tool" --remove-pair "$src" > "$out/pair.json"
red "$out/pair.json" drop-pair 'required assertion/mutant pair dropped'
"$tool" --remove-retained "$src" > "$out/retained.json"
red "$out/retained.json" drop-retained 'required retained screen dropped'
sed 's@harness/diff/dbf_diff/dbf_ref.py@harness/diff/dbf_diff/ndx_ref.py@g' "$src" > "$out/wrong-pair.json"
red "$out/wrong-pair.json" wrong-tool-pair 'required L1 kind/tool dropped'
sed 's/"id": "edit"/"id": "browse"/' "$src" > "$out/duplicate.json"
red "$out/duplicate.json" duplicate 'missing/duplicate/unknown W4 group'
sed 's/byte-exact-screens/structural-screens/' "$src" > "$out/w3.json"
red "$out/w3.json" weaken-w3 'W3 exact grading lost'
sed 's@build/frm_diff@build/no-such-w4-tool@' "$src" > "$out/tool.json"
red "$out/tool.json" missing-tool 'file absent/empty'
sed 's@create-retained/create-empty.screen@structure-retained/no-such.screen@' "$src" > "$out/dump.json"
red "$out/dump.json" missing-dump 'file absent/empty'
printf '%s\n' '{"apps":[],}' > "$out/bad.json"
red "$out/bad.json" malformed 'expected object key'
printf '%s\n' '{"status":"locked","status":"unlocked"}' > "$out/keys.json"
red "$out/keys.json" duplicate-key 'duplicate JSON key'
"$tool" "$src"
echo 'GREEN: test-w4-inventory-mutant: row/L1/pair/retained/tool and schema/evidence mutations RED'
