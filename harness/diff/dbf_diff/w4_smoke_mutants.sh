#!/bin/sh
# Mutate observed behavior / injected trace, never the asserted contract.
set -eu
root=$(pwd)
case_dir=harness/diff/dbf_diff/xbase_prog_diff/w4/smoke
out=build/w4/smoke
build/w4_screen_check --mutants "$out/prompt.ppm.screen" "$case_dir/ASSERTIONS" "$out"
# A changed data record must be rejected by the complete CHECK.
cp -f "$out/files/CLIENTS.DBF" "$out/original.dbf"
build/w4_mutate --xor 130 "$out/original.dbf" "$out/files/CLIENTS.DBF"
rc=0
(cd "$out" && W4_ROOT="$root" W4_CASE="$root/$case_dir" sh "$root/$case_dir/CHECK") > "$out/data-mutant.log" 2>&1 || rc=$?
cp -f "$out/original.dbf" "$out/files/CLIENTS.DBF"
[ "$rc" = 1 ] && grep -q 'RED: L1 smoke changed files' "$out/data-mutant.log" || {
    cat "$out/data-mutant.log"; echo 'RED: data effect mutant survived'; exit 1;
}
echo 'RED proven: L1 changed DBF record rejected by CHECK'
# Locked trace mutation: omit LIST, leave every assertion intact. A full boot
# must fail TEXT at row 1, not timeout or missing capture. Private app name.
mut=build/w4-smoke-mutants
mkdir -p "$mut/smoke-no-list"
cp -rf "$case_dir/SETUP" "$mut/smoke-no-list/"
cp -f "$case_dir/CHECK" "$case_dir/ASSERTIONS" "$case_dir/CELLS" "$mut/smoke-no-list/"
sed '/^TEXT LIST$/,+2d' "$case_dir/KEYS" > "$mut/smoke-no-list/KEYS"
rc=0
W4_TRACE_ROOT="$mut" sh harness/diff/dbf_diff/w4_trace.sh smoke-no-list > "$mut/no-list.log" 2>&1 || rc=$?
[ "$rc" = 1 ] && grep -q 'RED: L2 TEXT assertion 2 at (1,0)' "$mut/no-list.log" || {
    cat "$mut/no-list.log"; echo 'RED: omit-LIST trace mutant survived/wrong reason'; exit 1;
}
echo 'RED proven: L2 omit-LIST replay rejected at expected text region'
echo 'GREEN: test-w4-structure-mutant: text/cursor/highlight/blank + data and trace mutants RED'
