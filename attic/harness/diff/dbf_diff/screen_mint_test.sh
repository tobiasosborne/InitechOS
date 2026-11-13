#!/bin/sh
set -eu
tool=build/screen_tool
base=${1:-harness/diff/dbf_diff/xbase_prog_diff/screen-mint}
work=$(mktemp -d build/screen-test.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM
"$tool" codec-test "$work/codec.screen"
grep -q '^CURSOR 25 00$' "$work/codec.screen"
"$tool" decode "$work/codec.screen" "$work/offpage.bin"
printf '\032' | dd of="$work/offpage.bin" bs=1 seek=4000 conv=notrunc status=none
rc=0
"$tool" encode "$work/offpage.bin" "$work/invalid.screen" > "$work/bad-cursor" 2>&1 || rc=$?
[ "$rc" = 2 ] && grep -q 'cursor outside' "$work/bad-cursor"
printf 'GREEN: screen cursor row 25 round-trips; row 26 rejected\n'
n=0
for case_dir in "$base"/*; do
    [ -f "$case_dir/DUMPS" ] || continue
    while IFS= read -r name; do
        case $name in ''|'#'*) continue ;; esac
        g="$case_dir/$name.screen"
        "$tool" compare "$g" "$g"
        "$tool" decode "$g" "$work/page.bin"
        "$tool" compare-raw "$g" "$work/page.bin"
        "$tool" encode "$work/page.bin" "$work/page.screen"
        cmp "$g" "$work/page.screen"
        n=$((n+1))
    done < "$case_dir/DUMPS"
    (cd "$case_dir" && sha256sum -c SHA256SUMS && sha256sum -c INPUTS.sha256) >/dev/null
    sh harness/diff/dbf_diff/screen_mint_oracle.sh "$case_dir"
done
[ "$n" -gt 0 ]
# Use the same verifier as the ordinary gate, against an isolated altered pin.
cp -rf "$base/dot" "$work/oracle-mutant"
sed '1s/^[0-9a-f]\{64\}/0000000000000000000000000000000000000000000000000000000000000000/' "$work/oracle-mutant/ORACLE.sha256" > "$work/altered"
mv -f "$work/altered" "$work/oracle-mutant/ORACLE.sha256"
rc=0
sh harness/diff/dbf_diff/screen_mint_oracle.sh "$work/oracle-mutant" > "$work/oracle-red" 2>&1 || rc=$?
[ "$rc" = 1 ] && grep -q '^RED: oracle provenance mismatch:' "$work/oracle-red"
printf 'RED: altered oracle hash rejected (exit 1)\n'
g="$base/dot/structure.screen"
"$tool" compare "$g" "$g" > "$work/self"
cat "$work/self"
printf 'GREEN: comparator self-test golden vs itself\n'
for plane in cell attr cursor; do
    "$tool" "mutate-$plane" "$g" "$work/mutant.screen"
    rc=0
    "$tool" compare "$g" "$work/mutant.screen" > "$work/diff" || rc=$?
    cat "$work/diff"
    [ "$rc" = 1 ] || { printf 'FAIL: %s mutation did not produce a difference\n' "$plane"; exit 1; }
    printf 'RED: comparator self-test one-%s perturbation rejected (exit 1)\n' "$plane"
done
cp -f "$g" "$work/malformed.screen"
printf 'trailing garbage\n' >> "$work/malformed.screen"
rc=0
"$tool" compare "$g" "$work/malformed.screen" > "$work/bad" 2>&1 || rc=$?
[ "$rc" = 2 ]
printf 'GREEN: screen format rejects trailing data (exit 2)\n'
printf 'WAIT 1\nDUMP absent\n' > "$work/KEYS"
printf 'present\n' > "$work/DUMPS"
rc=0
"$tool" compile "$work/KEYS" "$work/DUMPS" "$work/EVENTS" > "$work/bad-keys" 2>&1 || rc=$?
[ "$rc" = 2 ]
printf 'GREEN: key compiler rejects undeclared dump (exit 2)\n'
printf 'GREEN: screen-mint self-test (%s real dumps; text/attribute/cursor mutation gates)\n' "$n"
