#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/apps/initech123/tests/conformance
out=$top/build/apps/initech123/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Iapps/initech123/include -Isdk/include -Isdk/host"
links="apps/initech123/core/*.c apps/initech123/fs/wk1.c build/sdk/host/libtbx_host.a -lm"
cc $flags "$src/observe.c" $links -o "$out/observe"
for f in "$src"/fixtures/L*.WK1; do timeout 30 "$out/observe" "$f" "$out/${f##*/}.save"; done
