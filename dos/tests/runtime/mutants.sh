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
 if [ ! -f "$baseline/green" ]; then
  mkdir -p "$baseline"
  if sh -x "$root/dos/tests/runtime/$case_name.sh" "$root" "$root/build/image/boot.img" "$root/build/dos" "$baseline" > "$baseline/verdict" 2>&1; then touch "$baseline/green";
  else echo "ERROR baseline $case_name: $baseline/verdict"; exit 1; fi
 fi
 d=$out/$name; mkdir -p "$d/component"
 cp -R "$root/dos/." "$d/component/"
 if ! patch --batch --fuzz=0 -p1 -d "$d/component" < "$patch_file" > "$d/patch.log" 2>&1; then
  echo "ERROR patch $name";failed=$((failed+1));continue
 fi
 if ! nice make -j2 --no-print-directory -C "$d/component" TOP="$root" OUT="$d/build" all > "$d/build.log" 2>&1; then
  echo "ERROR build $name";failed=$((failed+1));continue
 fi
 boot=$root/build/image/boot.img
 case "$patch_file" in command/*|*/command/*)
  objects=$(make --no-print-directory -s -C "$root/kernel" object-list)
  if ! ld -m elf_i386 --build-id=none -T "$root/kernel/kernel.ld" -o "$d/system.elf" $objects \
   "$d/build/command-resident.o" "$d/build/batch.o" "$d/build/env.o" --start-group --whole-archive \
   "$root/build/flair/desktop/libdesktop.a" "$root/build/flair/libflair.a" --no-whole-archive \
   "$root/build/flair/region/target/region.o" --end-group > "$d/link.log" 2>&1; then
   echo "ERROR link $name";failed=$((failed+1));continue
  fi
  objcopy -O binary "$d/system.elf" "$d/INITDOS.SYS"
  boot=$d/boot.img
  sh "$root/factory/image/mk_boot_system.sh" "$boot" "$root/build/boot/mbr.bin" "$root/build/boot/stage2.bin" \
   "$root/build/boot/IO.SYS" "$d/INITDOS.SYS" "$root/build/factory/boot_system" > "$d/image.log"
 esac
 verdict=0
 for pass in 1 2; do
  if sh -x "$root/dos/tests/runtime/$case_name.sh" "$root" "$boot" "$d/build" "$d/pass-$pass" > "$d/verdict-$pass" 2>&1; then :;
  else verdict=$((verdict+1)); fi
 done
 if [ "$verdict" -eq 2 ]; then echo "killed $name ($case_name; twice)";killed=$((killed+1));
 else echo "SURVIVED/UNSTABLE $name ($case_name)";survived=$((survived+1));fi
done
echo "dos: $total mutants, $killed killed, $survived survived, $failed errors; runtime sessions"
test "$failed" -eq 0 && test "$survived" -eq 0
