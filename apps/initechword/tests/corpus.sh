#!/bin/sh
# Required independent WordPerfect captures; never turn absence into a skip.
set -eu
root=$1
missing() { echo "FAIL: WordPerfect corpus missing or incomplete at $root; set WORDPERFECT51_DECOMP to wordperfect51-decomp ($1)" >&2; exit 1; }
for path in tools/wp5_ref.py goldens/minted/MINT02.WP5 mint/work/LISTF/lf005.img mint/work/LISTF/grids; do
    test -e "$root/$path" || missing "$path"
done
for mint in 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 24A 25 26 27 28 30 31 32; do
    test -f "$root/goldens/minted/MINT$mint.WP5" || test -f "$root/mint/work/WP51/MINT$mint" || missing "MINT$mint"
done
for tool in python3 mcopy mtype mformat fsck.fat md5sum sha256sum; do
    command -v "$tool" >/dev/null || { echo "FAIL: $tool required for WordPerfect independent checks" >&2; exit 1; }
done
manifest=$(pwd)/tests/diff/listfiles_mint005.md5
(cd "$root/mint/work/LISTF" && md5sum -c --quiet "$manifest")
(cd tests/fixtures/iword-listfiles-ops && sha256sum -c --quiet SHA256SUMS)
