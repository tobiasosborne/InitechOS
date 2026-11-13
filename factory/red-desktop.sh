#!/bin/sh
# Run from a clean source state. Each patch is restored even on interruption.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/red-desktop
active=
restore() {
 if [ -n "$active" ]; then patch --no-backup-if-mismatch -R -p1 < "$active"; active=; fi
}
trap restore EXIT HUP INT TERM
for mutation in save-empty loader-cap; do
 active="factory/mutants/desktop-$mutation.patch"
 patch --no-backup-if-mismatch -p1 < "$active"
 make image > "build/red-desktop/$mutation-build.log" 2>&1
 if DESKTOP_SMOKE_OUT="build/red-desktop/$mutation" sh factory/smoke-desktop.sh > "build/red-desktop/$mutation.log" 2>&1; then
  echo "FAIL: desktop $mutation survived" >&2; exit 1
 fi
 case $mutation in
 save-empty) grep -F 'FAIL desktop: QEMU worksheet content' "build/red-desktop/$mutation.log";;
 loader-cap) grep -F 'TENANT-LOAD-FAIL name=123.EXE' "build/red-desktop/$mutation/qemu.serial"
             grep -F 'FAIL desktop: QEMU session' "build/red-desktop/$mutation.log";;
 esac
 echo "ok red-once: desktop $mutation rejected"
 restore
done
make image > build/red-desktop/restored-build.log 2>&1
echo 'DESKTOP RED: 2 mutations rejected; production sources restored'
