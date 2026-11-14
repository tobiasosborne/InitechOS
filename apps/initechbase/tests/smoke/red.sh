#!/bin/sh
# Prove that a booted, normally returning REPLACE can fail disk readback.
set -eu
cd "$(dirname "$0")/../../../.."
root=$PWD
out=$root/build/apps/initechbase/red
mkdir -p "$out"
source=apps/initechbase/cmd/mutate.c
cp "$source" "$out/mutate.c"
sha256sum build/apps/initechbase/INITBASE.EXE build/apps/initechbase/INITBASE.OVL > "$out/baseline.sha256"
restore() {
 cp "$out/mutate.c" "$source"
 make -C apps/initechbase package > "$out/restore.log" 2>&1
 make image >> "$out/restore.log" 2>&1
}
trap restore EXIT HUP INT TERM
patch --batch --fuzz=0 -p1 -d apps/initechbase < apps/initechbase/mutants/drop-replace-write.patch > "$out/patch.log"
make -C apps/initechbase package > "$out/build.log" 2>&1
make image >> "$out/build.log" 2>&1
if IBASE_SMOKE_OUT="$out/session" IBASE_SMOKE_CPUS=qemu sh apps/initechbase/tests/smoke/run.sh > "$out/session.log" 2>&1; then
 echo 'FAIL drop-write mutant survived'; exit 1
fi
grep -Fx 'FAIL persisted REPLACE/APPEND/DELETE' "$out/session.log"
cp -R "$out/session/qemu" "$out/qemu"
restore
trap - EXIT HUP INT TERM
sha256sum -c "$out/baseline.sha256"
echo 'RED CONTROL: dropped REPLACE write caught by independent disk readback; restored'
