#!/bin/sh
# Run from anywhere. SDK host and generated font headers must already exist
# (make -j2 test builds them). This is a study, not a registered green gate.
set -eu
top=$(CDPATH= cd -- "$(dirname "$0")/../../../.." && pwd)
cd "$top"
src=$top/apps/initechtext/tests/conformance
out=$top/build/apps/initechtext/conformance
mkdir -p "$out"
(cd "$src/fixtures" && sha256sum -c ../SHA256SUMS >&2)
flags="-std=c11 -D_DEFAULT_SOURCE -O2 -Wall -Wextra -Wno-unused-function -Wno-unused-variable -Iapps/initechtext/include -Isdk/include -Isdk/host -Iflair/include -Iflair/include/assets -Iflair/region/include -Ibuild/flair/generated"
links="flair/textedit.c flair/scrap.c build/sdk/host/libtbx_host.a"
cc $flags "$src/observe.c" $links -o "$out/observe"
rc=0
for f in "$src"/fixtures/T*.TXT; do timeout 30 "$out/observe" "$f" 0 || rc=1; done
timeout 30 "$out/observe" "$src/fixtures/T04.TXT" 4 || rc=1
exit "$rc"
