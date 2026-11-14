#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/apps/initechcalc/tests/conformance
out=$top/build/apps/initechcalc/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Iapps/initechcalc/include -Iapps/initech123/include -Isdk/include -Isdk/host"
links="apps/initech123/core/sheet.c apps/initech123/core/fmt.c apps/initech123/core/formula.c apps/initech123/core/range.c apps/initech123/core/eval.c apps/initech123/core/recalc.c apps/initech123/fs/wk1.c build/sdk/host/libtbx_host.a -lm"
cc $flags "$src/observe.c" $links -o "$out/observe"
for f in "$src"/fixtures/C*.TXT; do timeout 30 "$out/observe" "$f"; done
