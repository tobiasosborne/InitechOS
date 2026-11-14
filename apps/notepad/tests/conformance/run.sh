#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/apps/notepad/tests/conformance
out=$top/build/apps/notepad/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Iapps/notepad/include -Isdk/include -Isdk/host -Iflair/include -Iflair/include/assets -Iflair/region/include -Ibuild/flair/generated"
links="flair/textedit.c flair/scrap.c build/sdk/host/libtbx_host.a"
cc $flags "$src/observe.c" $links -o "$out/observe"
for f in "$src"/fixtures/N*.DAT; do timeout 30 "$out/observe" "$f"; done
cc $flags -DSCRAPBOOK "$src/observe.c" $links -o "$out/scrapbook"
for f in "$src"/fixtures/S*.DAT; do timeout 30 "$out/scrapbook" "$f"; done
