#!/bin/sh
# ABI/fake/container checks under the shared timeout runner.
set -eu
out=$1
case $2 in
 abi) "$out/tests/audit" tests/traps.tsv include/toolbox_gate.h include/cp_dialog.h rt/tbx_rt.c "$TOP/flair/tbxgate.c";;
 layout) echo 'ABI: target i386 layouts pinned (compile-only)';;
 link) "$out/tests/link"; echo 'ABI: all bindings link against shared fake';;
 host) "$out/tests/host";;
 search33) "$out/tests/search33" "$TOP/kernel/tests/fixtures/search33/matrix.bin";;
 host32) sh tests/host32.sh "${HOSTCC:-cc}" "$out/tests/host32";;
 mz) make --no-print-directory -C "$TOP/factory/mz" test;;
 *) exit 1;;
esac
