#!/bin/sh
# Exercise the real file gate under both corpus spellings and missing refs.
# Private repo-shaped root supplies the default ../dbase3-decomp sibling in
# worktrees whose actual sibling is absent. Nothing outside build/ is written.
set -eu
. "${INITECH_SHELL_CHECKS:?factory shell-checks.sh}"
. tests/check_status.sh
root=$(pwd)
gate=$root/tests/w4_files_test.sh
check "w4-files: file gate" sh "$gate"
[ "${SKIP_DBASE3_DECOMP:-0}" != 1 ] || { summary; exit 0; }
ref=$(realpath -e -- "${DBASE3_DECOMP:?corpus required}")
tmp=$(mktemp -d "$root/build/w4-paths.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir "$tmp/repo"
ln -s "$root/build" "$tmp/repo/build"
ln -s "$ref" "$tmp/dbase3-decomp"
gate_in() { (cd "$tmp/repo" && DBASE3_DECOMP=$1 sh "$gate") > "$2" 2>&1 || { rc=$?; cat "$2"; return "$rc"; }; }
check 'w4-files: default relative W4 corpus rejected' gate_in ../dbase3-decomp "$tmp/relative.log"
check 'w4-files: absolute W4 corpus rejected' gate_in "$ref" "$tmp/absolute.log"
if (cd "$tmp/repo" && DBASE3_DECOMP=../missing sh "$gate") > "$tmp/missing.log" 2>&1; then
    fail 'w4-files: missing W4 corpus accepted'
fi; ok
grep -q 'DBASE3_DECOMP FAIL:.*SKIP_DBASE3_DECOMP=1' "$tmp/missing.log" || {
    cat "$tmp/missing.log"; fail 'w4-files: missing corpus did not name the opt-out';
}; ok
gate_in_skip() { (cd "$tmp/repo" && DBASE3_DECOMP=../missing SKIP_DBASE3_DECOMP=1 sh "$gate") > "$tmp/skip.log" 2>&1; }
check 'w4-files: explicit opt-out run failed' gate_in_skip
grep -q 'DBASE3_DECOMP SKIPPED (SKIP_DBASE3_DECOMP=1 opt-out)' "$tmp/skip.log" || fail 'w4-files: opt-out did not shout'; ok
cat "$tmp/missing.log" "$tmp/skip.log"
echo 'GREEN: W4 corpus paths: default relative and absolute; missing RED; explicit opt-out shouts'
summary
