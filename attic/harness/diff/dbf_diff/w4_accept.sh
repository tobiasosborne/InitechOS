#!/bin/sh
# Future application acceptance, DEC-03a.10. No not-started app instantiated.
set -eu
app=${1:?app required}
unset W4_TRACE_ROOT
build/w4_inventory spec/samir/screen_regrade.json --ready "$app"
sh harness/diff/dbf_diff/w4_trace.sh "$app"
# Scripts are explicit, named once per structural assertion in the inventory.
# Each script must itself prove a failing behavior and return 0 on detection.
# --ready validates their existence; the list is read by the C inventory tool.
build/w4_inventory spec/samir/screen_regrade.json --mutants "$app" > "build/w4/$app/mutants.list"
while IFS= read -r mutant; do sh "$mutant"; done < "build/w4/$app/mutants.list"
build/w4_inventory spec/samir/screen_regrade.json --retained "$app" > "build/w4/$app/retained.list"
while IFS= read -r golden; do
    name=$(basename "$golden" .screen)
    build/screen_tool compare "$golden" "build/w4/$app/$name.ppm.screen"
done < "build/w4/$app/retained.list"
