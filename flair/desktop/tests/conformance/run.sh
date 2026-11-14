#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/flair/desktop/tests/conformance
out=$top/build/flair/desktop/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Iflair/desktop -Ikernel/include -ffunction-sections -fdata-sections"
links="flair/desktop/desktop_db.c -Wl,--gc-sections"
cc $flags "$src/observe.c" $links -o "$out/observe"
for f in "$src"/fixtures/D*.DB; do timeout 30 "$out/observe" "$f"; done
