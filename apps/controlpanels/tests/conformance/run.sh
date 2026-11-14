#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/apps/controlpanels/tests/conformance
out=$top/build/apps/controlpanels/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Isdk/include"
links=""
cc $flags "$src/observe.c" $links -o "$out/observe"
for f in "$src"/fixtures/P*.DAT; do timeout 30 "$out/observe" "$f"; done
