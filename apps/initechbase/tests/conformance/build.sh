#!/bin/sh
set -eu
src=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
repo=$(CDPATH= cd -- "$src/../.." && pwd)
out=${1:-"$repo/build/apps/initechbase/conformance"}
mkdir -p "$out"
cc -std=c11 -O2 -Wall -Wextra -Werror "$src/tests/conformance/fixtures.c" -o "$out/fixtures"
# Build the existing object set; no registration/change to the component gate.
nice make -j2 -C "$src" "$repo/build/apps/initechbase/host/test_w2_host" > "$out/build.log" 2>&1
objects=$repo/build/apps/initechbase/host-core
set --
for file in cmd/workarea cmd/nav cmd/flow cmd/query cmd/mutate cmd/set cmd/proc cmd/report \
 fs/dbf fs/dbt fs/ndx core/eval core/parse core/lex core/value core/rt core/fn_builtins pal/pal_host; do
 set -- "$@" "$objects/$file.o"
done
cc -std=c11 -O2 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 -I"$src/include" -I"$src/tests" "$src/tests/conformance/host.c" "$src/tests/w2_host.c" \
 "$@" -o "$out/host"
