#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/apps/initechword/tests/conformance
out=$top/build/apps/initechword/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Iapps/initechword/include -Isdk/include -Isdk/host"
links="apps/initechword/core/*.c apps/initechword/fs/wp5.c build/sdk/host/libtbx_host.a -lm"
cc $flags "$src/observe.c" $links -o "$out/observe"
for f in "$src"/fixtures/W*.WP5; do timeout 30 "$out/observe" "$f" "$out/${f##*/}.save"; done
