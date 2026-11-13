#!/bin/sh
# Patch complete isolated trees: the production sources and outputs stay intact.
set -eu
cd "$(dirname "$0")/.."
root=$PWD
mkdir -p build/factory/mutants
work=$(mktemp -d "$root/build/factory/mutants/run.XXXXXX")
start=$(date +%s)
killed=0 survived=0 errors=0
snapshot() {
 mkdir -p "$1"
 tar -cf - --exclude=./build --exclude=./.git --exclude=./attic . | tar -xf - -C "$1"
}
baseline=$work/baseline
snapshot "$baseline"
if ! (cd "$baseline" && make image && sh factory/smoke-desktop.sh) > "$work/baseline.log" 2>&1; then
 echo "factory: 3 mutants, 0 killed, 0 survived, 1 errors; baseline failed ($work/baseline.log)"
 exit 1
fi
for mutation in save-empty loader-cap word-save; do
 scratch=$work/$mutation
 # Reuse baseline outputs; command stamps rebuild against the new source root.
 cp -R "$baseline" "$scratch"
 patch_root=$scratch patch_file=factory/mutants/desktop-$mutation.patch
 if [ "$mutation" = word-save ]; then
  patch_root=$scratch/apps/initechword
  patch_file=apps/initechword/mutants/save-nothing.patch
 fi
 if ! patch --batch --fuzz=0 --no-backup-if-mismatch -p1 -d "$patch_root" < "$patch_file" > "$work/$mutation-patch.log" 2>&1 ||
    ! (cd "$scratch" && make image) > "$work/$mutation-build.log" 2>&1; then
  echo "!!! factory desktop-$mutation: patch/build failure"; errors=$((errors+1)); continue
 fi
 good=1
 for pass in 1 2; do
  rc=0
  (cd "$scratch" && DESKTOP_SMOKE_OUT="build/red-$pass" sh factory/smoke-desktop.sh) > "$work/$mutation-$pass.log" 2>&1 || rc=$?
  if [ "$rc" -eq 0 ]; then good=0; continue; fi
  case $mutation in
   save-empty) grep -Fq 'FAIL desktop: QEMU worksheet content' "$work/$mutation-$pass.log" || good=0;;
   word-save) grep -Fq 'FAIL desktop: QEMU initechword saved content' "$work/$mutation-$pass.log" || good=0;;
   loader-cap) grep -Fq 'TENANT-LOAD-FAIL name=123.EXE' "$scratch/build/red-$pass/qemu.serial" &&
               grep -Fq 'FAIL desktop: QEMU session' "$work/$mutation-$pass.log" || good=0;;
  esac
 done
 if [ "$good" -eq 1 ]; then
  echo "  killed desktop-$mutation (two matching semantic failures)"; killed=$((killed+1)); rm -rf "$scratch"
 else
  echo "!!! factory desktop-$mutation survived/unstable"; survived=$((survived+1))
 fi
done
echo "factory: 3 mutants, $killed killed, $survived survived, $errors errors; $(($(date +%s)-start))s"
if [ "$survived" -eq 0 ] && [ "$errors" -eq 0 ]; then rm -rf "$work"; else echo "scratch retained: $work"; exit 1; fi
