#!/bin/sh
# Run from a clean source state. Each patch is restored even on interruption.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/red-desktop
active=
patch_dir=.
restore() {
 if [ -n "$active" ]; then
  if [ "$patch_dir" = . ]; then
   patch --no-backup-if-mismatch -R -p1 < "$active"
  else
   patch --batch --fuzz=0 --no-backup-if-mismatch -R -p1 -d "$patch_dir" < "$active"
  fi
  active=
 fi
}
trap restore EXIT
trap 'exit 1' HUP INT TERM
for mutation in save-empty loader-cap word-save; do
 patch_dir=.
 active="factory/mutants/desktop-$mutation.patch"
 if [ "$mutation" = word-save ]; then
  active=apps/initechword/mutants/save-nothing.patch
  patch_dir=apps/initechword
  patch --batch --fuzz=0 --no-backup-if-mismatch -p1 -d "$patch_dir" < "$active"
 else
  patch --no-backup-if-mismatch -p1 < "$active"
 fi
 make image > "build/red-desktop/$mutation-build.log" 2>&1
 if DESKTOP_SMOKE_OUT="build/red-desktop/$mutation" sh factory/smoke-desktop.sh > "build/red-desktop/$mutation.log" 2>&1; then
  echo "FAIL: desktop $mutation survived" >&2; exit 1
 fi
 case $mutation in
 save-empty) grep -F 'FAIL desktop: QEMU worksheet content' "build/red-desktop/$mutation.log";;
 loader-cap) grep -F 'TENANT-LOAD-FAIL name=123.EXE' "build/red-desktop/$mutation/qemu.serial"
             grep -F 'FAIL desktop: QEMU session' "build/red-desktop/$mutation.log";;
 word-save) grep -F 'FAIL desktop: QEMU initechword saved content' "build/red-desktop/$mutation.log";;
 esac
 echo "ok red-once: desktop $mutation rejected"
 restore
done
make image > build/red-desktop/restored-build.log 2>&1
echo 'DESKTOP RED: 3 mutations rejected; production sources restored'
