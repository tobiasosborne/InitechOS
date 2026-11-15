#!/bin/sh
set -eu
out=$1
. "$TOP/turbo/tests/checks.sh"
scratch=$out/runs/unnamed
mint=$TURBOPASCAL_DECOMP/goldens/minted/MINT08
setup sh -c 'cd "$1" && sha256sum --quiet -c SHA256SUMS' sh "$mint"
setup rm -rf "$scratch"
setup mkdir -p "$scratch"
setup "$out/keys" tests/unnamed.steps "$scratch/IDEKEYS.BIN" "$scratch/shots.list"
setup sh -c 'cd "$1" && "$2" > stdout' sh "$scratch" "$out/idehost"
assert 'MINT08 unnamed Verify, Rename, cancel and retry screens' "$out/screens" "$mint" "$scratch" tests/unnamed.expect
assert 'MINT08 saved unnamed bytes' test -f "$scratch/UNNAMED.PAS"
assert 'MINT08 saved unnamed bytes' cmp "$mint/disk/UNNAMED.PAS" "$scratch/UNNAMED.PAS"
assert 'successful save completes Quit' grep -Fx 'IDE-HOST-MODE=5' "$scratch/stdout"
finish
