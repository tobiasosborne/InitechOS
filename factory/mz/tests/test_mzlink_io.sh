#!/bin/sh
# Rule 2/6: real valid stream, malformed stream, read error, and close error.
set -eu
. "$(dirname "$0")/../../testlib/shell-checks.sh"
ok=$1; close_fault=$2; root=$3
t=factory/mz/tests/test_mzlink_io.sh
mkdir -p "$root"
printf '\000\000\000\000' > "$root/rel"
printf '0123456789abcdef' > "$root/module"
"$ok" "$root/module" "$root/good.EXE" --reloc-file "$root/rel" --header-relocs || fail "$t: mzlink refused a valid relocation stream"
[ "$(od -An -tu2 -j6 -N2 "$root/good.EXE" | tr -d ' ')" = 1 ] || fail "$t: header relocation count is not 1"; ok
if "$ok" "$root/module" "$root/bad.EXE" --reloc-file "$root" > "$root/read.log" 2>&1; then
 fail "$t: mzlink accepted a relocation read error"
fi
grep -q -- '--reloc-file: read failed' "$root/read.log" || fail "$t: read error not reported"; ok
printf 'abc' > "$root/short"
if "$ok" "$root/module" "$root/bad.EXE" --reloc-file "$root/short" > "$root/short.log" 2>&1; then fail "$t: mzlink accepted a short stream"; fi
grep -q 'length is not a multiple of 4' "$root/short.log" || fail "$t: short stream not reported"; ok
if "$close_fault" "$root/module" "$root/bad.EXE" --reloc-file "$root/rel" > "$root/close.log" 2>&1; then
 fail "$t: mzlink accepted a relocation close error"
fi
grep -q -- '--reloc-file: close failed' "$root/close.log" || fail "$t: close error not reported"; ok
echo 'GREEN mzlink relocation I/O: valid stream preserved, read/short/close errors rejected'
summary
