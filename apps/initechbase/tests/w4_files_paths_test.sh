#!/bin/sh
# Exercise the real file gate under both corpus spellings and missing refs.
# Private repo-shaped root supplies the default ../dbase3-decomp sibling in
# worktrees whose actual sibling is absent. Nothing outside build/ is written.
set -eu
root=$(pwd)
gate=$root/tests/w4_files_test.sh
sh "$gate"
[ "${SKIP_DBASE3_DECOMP:-0}" != 1 ] || exit 0
ref=$(realpath -e -- "${DBASE3_DECOMP:?corpus required}")
tmp=$(mktemp -d "$root/build/w4-paths.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir "$tmp/repo"
ln -s "$root/build" "$tmp/repo/build"
ln -s "$ref" "$tmp/dbase3-decomp"
(cd "$tmp/repo" && DBASE3_DECOMP=../dbase3-decomp sh "$gate") > "$tmp/relative.log" 2>&1 || {
    cat "$tmp/relative.log"; echo 'RED: default relative W4 corpus rejected'; exit 1;
}
(cd "$tmp/repo" && DBASE3_DECOMP="$ref" sh "$gate") > "$tmp/absolute.log" 2>&1 || {
    cat "$tmp/absolute.log"; echo 'RED: absolute W4 corpus rejected'; exit 1;
}
if (cd "$tmp/repo" && DBASE3_DECOMP=../missing sh "$gate") > "$tmp/missing.log" 2>&1; then
    echo 'RED: missing W4 corpus accepted'; exit 1
fi
grep -q 'DBASE3_DECOMP FAIL:.*SKIP_DBASE3_DECOMP=1' "$tmp/missing.log" || {
    cat "$tmp/missing.log"; exit 1;
}
(cd "$tmp/repo" && DBASE3_DECOMP=../missing SKIP_DBASE3_DECOMP=1 sh "$gate") > "$tmp/skip.log" 2>&1
grep -q 'DBASE3_DECOMP SKIPPED (SKIP_DBASE3_DECOMP=1 opt-out)' "$tmp/skip.log"
cat "$tmp/missing.log" "$tmp/skip.log"
echo 'GREEN: W4 corpus paths: default relative and absolute; missing RED; explicit opt-out shouts'
