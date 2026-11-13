#!/bin/sh
# Rule 2/6: real valid stream, malformed stream, read error, and close error.
set -eu
ok=$1; close_fault=$2; root=$3
mkdir -p "$root"
printf '\000\000\000\000' > "$root/rel"
printf '0123456789abcdef' > "$root/module"
"$ok" "$root/module" "$root/good.EXE" --reloc-file "$root/rel" --header-relocs
[ "$(od -An -tu2 -j6 -N2 "$root/good.EXE" | tr -d ' ')" = 1 ]
if "$ok" "$root/module" "$root/bad.EXE" --reloc-file "$root" > "$root/read.log" 2>&1; then
 echo 'RED mzlink accepted a relocation read error'; exit 1
fi
grep -q -- '--reloc-file: read failed' "$root/read.log"
printf 'abc' > "$root/short"
if "$ok" "$root/module" "$root/bad.EXE" --reloc-file "$root/short" > "$root/short.log" 2>&1; then exit 1; fi
grep -q 'length is not a multiple of 4' "$root/short.log"
if "$close_fault" "$root/module" "$root/bad.EXE" --reloc-file "$root/rel" > "$root/close.log" 2>&1; then
 echo 'RED mzlink accepted a relocation close error'; exit 1
fi
grep -q -- '--reloc-file: close failed' "$root/close.log"
echo 'GREEN mzlink relocation I/O: valid stream preserved, read/short/close errors rejected'
