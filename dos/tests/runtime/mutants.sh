#!/bin/sh
# Strict patch -> target build -> external session, with a green baseline for
# each case and repeatable runtime failures. Build errors never count as kills.
set -eu
root=$1; shift
[ "$#" -gt 0 ] || { echo 'FAIL: no runtime mutation patches selected' >&2;exit 1; }
base=$root/build/dos/session-mutants
mkdir -p "$base"
out=$(mktemp -d "$base/run.XXXXXX")
mkdir -p "$out"
failed=0; survived=0; killed=0; total=0
for patch_file do
 total=$((total+1))
 name=$(basename "$patch_file" .patch)
 case_name=$(sed -n 's/^# session: //p' "$patch_file")
 test -n "$case_name" || { echo "ERROR $name: no session selection"; exit 1; }
 baseline=$out/baseline-$case_name
 if [ ! -f "$baseline/status" ]; then
  mkdir -p "$baseline"
  rc=0
  sh -x "$root/dos/tests/runtime/$case_name.sh" "$root" "$root/build/image/boot.img" "$root/build/dos" "$baseline" > "$baseline/verdict" 2>&1 || rc=$?
  sh "$root/dos/tests/runtime/classify.sh" "$baseline" "$rc" > "$baseline/status"
 fi
 if ! grep -Fx PASS "$baseline/status" >/dev/null; then
  echo "ERROR baseline $name ($case_name): $baseline/verdict";failed=$((failed+1));continue
 fi
 d=$out/$name; mkdir -p "$d/component"
 if ! cp -R "$root/dos/." "$d/component/"; then
  echo "ERROR setup $name";failed=$((failed+1));continue
 fi
 if ! patch --batch --fuzz=0 -p1 -d "$d/component" < "$patch_file" > "$d/patch.log" 2>&1; then
  echo "ERROR patch $name";failed=$((failed+1));continue
 fi
 if ! nice make -j2 --no-print-directory -C "$d/component" TOP="$root" OUT="$d/build" all > "$d/build.log" 2>&1; then
  echo "ERROR build $name";failed=$((failed+1));continue
 fi
 boot=$root/build/image/boot.img
 case $(sed -n 's|^--- a/||p' "$patch_file") in command/*)
  if ! objects=$(make --no-print-directory -s -C "$root/kernel" object-list); then
   echo "ERROR objects $name";failed=$((failed+1));continue
  fi
  if ! ld -m elf_i386 --build-id=none -T "$root/kernel/kernel.ld" -o "$d/system.elf" $objects \
   "$d/build/command-resident.o" "$d/build/batch.o" "$d/build/env.o" --start-group --whole-archive \
   "$root/build/flair/desktop/libdesktop.a" "$root/build/flair/libflair.a" --no-whole-archive \
   "$root/build/flair/region/target/region.o" --end-group > "$d/link.log" 2>&1; then
   echo "ERROR link $name";failed=$((failed+1));continue
  fi
  if ! objcopy -O binary "$d/system.elf" "$d/INITDOS.SYS"; then
   echo "ERROR binary $name";failed=$((failed+1));continue
  fi
  boot=$d/boot.img
  if ! sh "$root/factory/image/mk_boot_system.sh" "$boot" "$root/build/boot/mbr.bin" "$root/build/boot/stage2.bin" \
   "$root/build/boot/IO.SYS" "$d/INITDOS.SYS" "$root/build/factory/boot_system" > "$d/image.log"; then
   echo "ERROR image $name";failed=$((failed+1));continue
  fi
 esac
 for pass in 1 2; do
  rc=0
  sh -x "$root/dos/tests/runtime/$case_name.sh" "$root" "$boot" "$d/build" "$d/pass-$pass" > "$d/verdict-$pass" 2>&1 || rc=$?
  printf '%s\n' "$rc" > "$d/exit-$pass"
  sh "$root/dos/tests/runtime/classify.sh" "$d/pass-$pass" "$rc" > "$d/status-$pass"
 done
 if grep '^SEMANTIC ' "$d/status-1" >/dev/null && cmp -s "$d/status-1" "$d/status-2"; then
  echo "killed $name ($case_name; twice; $(cat "$d/status-1"))";killed=$((killed+1))
 elif grep -Fx PASS "$d/status-1" >/dev/null && grep -Fx PASS "$d/status-2" >/dev/null; then
  echo "SURVIVED $name ($case_name)";survived=$((survived+1))
 else
  echo "ERROR scenario $name ($case_name): $(cat "$d/status-1") / $(cat "$d/status-2")";failed=$((failed+1))
 fi
done
echo "dos: $total mutants, $killed killed, $survived survived, $failed errors; runtime sessions"
test "$failed" -eq 0 && test "$survived" -eq 0
