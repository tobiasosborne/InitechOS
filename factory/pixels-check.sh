#!/bin/sh
# Enforced independent masks are part of image-smoke in cert, without a
# CERT_STRICT switch. Legacy equality is expressly informational.
set -eu
cd "$(dirname "$0")/.."
out=$1
shift
corpus=${SYSTEM7_DECOMP:-${INITECH_CORPUS:-$HOME/Projects}/system7-decomp}
root=$PWD
(cd "$corpus" && sha256sum -c "$root/factory/references/desktop-icons.sha256")
build/factory/desktop-reference "$corpus/goldens/captures" "$out/icons-reference.ppm" "$out/icons-mask.ppm"
build/factory/desktop-reference "$corpus/goldens/captures" "$out/icons-selected.ppm" "$out/icons-mask.ppm" selected-volume
if [ "$#" -eq 0 ]; then set -- desktop disk apps qemu; fi
for screen do
 reference=$out/icons-reference.ppm
 case $screen in disk|apps) reference=$out/icons-selected.ppm;; esac
 build/factory/pixels-desktop "$reference" "$out/$screen.ppm" "$out/icons-mask.ppm" || {
  echo "FAIL independent desktop pixels: $screen" >&2; exit 1;
 }
done
