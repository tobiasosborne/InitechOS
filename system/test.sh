#!/bin/sh
set -eu
# Independent contract: one strong FLAIR entry in product, weak in DOS-only;
# no GUI symbols in standalone kernel, no old ABI/overlay inputs in this link.
nm ../build/system/system.elf | grep -E ' T kernel_graphical_environment$' >/dev/null
nm ../build/kernel/kernel.elf | grep -E ' W kernel_graphical_environment$' >/dev/null
if nm ../build/kernel/kernel.elf | grep -E ' (flair_|tbx_|finder_)'; then exit 1; fi
if grep -E 'attic/|mregparm|mrtd|flto|--gc-sections|COLD' Makefile; then exit 1; fi
test -s ../build/flair/libflair.a
test -s ../build/flair/desktop/libdesktop.a
# Exercise factory input additions without starting an emulator.
../build/factory/trace-input-test
sh tests/manifest.sh
sh tests/applist.sh
sh tests/inputs.sh
echo 'system: composition and standalone boundary green'
