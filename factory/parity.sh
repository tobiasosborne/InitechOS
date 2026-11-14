#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
legacy=${INITECH_LEGACY:-${LEGACY_TREE:-$HOME/Projects/InitechOS-legacy}}
mkdir -p build/parity
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
build/factory/keys < factory/session.txt > build/parity/keys
finish=$(printf '\nsmoke-complete\r\n')
for variant in legacy product; do
 boot=build/image/boot.img
 [ "$variant" != legacy ] || boot=$legacy/build/tracer_boot.img
 cp build/image/data.img "build/parity/$variant-data.img"
 build/factory/qemu_harness --disk "$boot" --disk2 "build/parity/$variant-data.img" \
  --keys-after SHELL-READY --keys "$(cat build/parity/keys)" --expect "$finish" \
  --quit-after "$finish" --timeout-ms 150000 --name "$variant" --out build/parity \
  > "build/parity/$variant.stdout" 2> "build/parity/$variant.report"
 tr -d '\r' < "build/parity/$variant.serial" | sed -n '/^SHELL-READY$/,$p' > "build/parity/$variant-session.txt"
done
diff -u build/parity/legacy-session.txt build/parity/product-session.txt > build/parity/session.diff
diff -u build/parity/legacy.serial build/parity/product.serial > build/parity/boot.diff || :
echo 'PARITY: identical complete interactive session (legacy-final shell vs product)'
echo 'Boot diagnostics differ; inspect build/parity/boot.diff and factory/ACCEPTANCE.md.'
