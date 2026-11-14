#!/bin/sh
# Tier 1: execute direct seed, hosted Pascal native, and resident Pascal output
# on the product image. FPC is the independent stdout/file oracle. Selectors
# allow red-first probes and tier-2 semantic mutation catchers without changing
# the complete default matrix. CLAUDE.md: The oracle is the truth.
set -eu
cd "$(dirname "$0")/../../.."
. factory/testlib/smoke-lock.sh
smoke_lock
root=$PWD
out=${TURBO_EXEC_OUT:-$root/build/turbo/smoke/execution}
hosted=${TURBO_HOSTED:-$root/build/turbo/compiler/turbo_fpc}
paths=${TURBO_EXEC_PATHS:-seed hosted hosted-flat resident resident-flat}
emulators=${TURBO_EXEC_EMULATORS:-qemu bochs}
sources=${TURBO_EXEC_SOURCES:-$(echo turbo/compiler/fixtures/gen*.pas turbo/tests/execution/*.pas)}
mkdir -p "$out"
out=$(CDPATH= cd -- "$out" && pwd)
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
failed=0
: > "$out/cases"
n=0
for source in $sources; do
 name=$(basename "$source" .pas)
 ref=$out/reference/$name
 rm -rf "$ref"
 mkdir -p "$ref"
 case $name in
 for_max|for_min)
  # The product's Integer is 32-bit; TP Integer is 16-bit. Change only
  # the declaration, keeping the endpoint arithmetic and loop identical.
  sed 's/i,count: integer/i,count: longint/' "$source" > "$ref/input.pas" ;;
 *) cp "$source" "$ref/input.pas" ;;
 esac
 fpc -Mtp -B -O- -v0 -FU"$ref" -FE"$ref" -o"$ref/reference" "$ref/input.pas" > "$ref/fpc.log"
 (cd "$ref" && timeout 5 ./reference > stdout)
 if [ -f "${source%.pas}.golden" ]; then diff -u "${source%.pas}.golden" "$ref/stdout"; fi
 for path in $paths; do
  n=$((n+1)); id=$(printf 'P%03d' "$n")
  dir=$out/programs/$id
  rm -rf "$dir"
  mkdir -p "$dir"
  cp "$source" "$dir/TURBOIN.PAS"
  case $path in
  seed)
   if ! build/turbo/seed/initechc --emit-asm -o "$dir/program.s" "$source" > "$dir/build.log" 2>&1; then
    echo "FAIL $name $path compilation"; failed=1; continue
   fi
   sh turbo/compiler/tests/package.sh "$dir/program.s" "$dir/package" >> "$dir/build.log" 2>&1
   cp "$dir/package/program.exe" "$dir/PROGRAM.COM" ;;
  hosted|hosted-flat)
   (cd "$dir" && "$hosted" > build.log)
   if grep -E 'TURBO-.*(ERROR|SKIP)' "$dir/build.log" || [ ! -s "$dir/TURBOOUT.EXE" ]; then
    echo "FAIL $name $path compilation"; failed=1; continue
   fi
   if [ "$path" = hosted-flat ]; then artifact=TURBOOUT.COM; else artifact=TURBOOUT.EXE; fi
   cp "$dir/$artifact" "$dir/PROGRAM.COM" ;;
  resident|resident-flat) ;;
  *) echo "Unknown path: $path" >&2; exit 1 ;;
  esac
  echo "$id $name $path" >> "$out/cases"
 done
done
for emulator in $emulators; do
 dir=$out/$emulator
 mkdir -p "$dir"
 disk=$dir/test.img
 dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
 mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
 mcopy -i "$disk" build/dos/COMMAND.COM ::
 # Share the resident compiler, keeping both format matrices within the
 # existing 8 MiB TEST volume. EXEC preserves each case's working directory.
 case " $paths " in
 *" resident "*|*" resident-flat "*) mcopy -i "$disk" build/turbo/compiler/TURBO.COM ::TURBO.COM ;;
 esac
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 mcopy -i "$disk" "$dir/CONFIG.SYS" ::
 batch=$dir/RUN.BAT
 segment=0; in_batch=0
 printf '@echo off\r\n' > "$batch"
 while read -r id name path; do
  # COMMAND.COM accepts at most 4096 bytes per batch. Chain small batches
  # at root rather than truncating a large matrix or changing the shell.
  if [ "$in_batch" -eq 24 ]; then
   segment=$((segment+1)); next=$(printf 'B%03d.BAT' "$segment")
   printf '%s\r\n' "$next" >> "$batch"
   mcopy -i "$disk" "$batch" ::
   batch=$dir/$next; in_batch=0
   printf '@echo off\r\n' > "$batch"
  fi
  in_batch=$((in_batch+1))
  mmd -i "$disk" ::"$id"
  printf 'cd %s\r\n' "$id" >> "$batch"
  if [ "$path" = resident ] || [ "$path" = resident-flat ]; then
   mcopy -i "$disk" build/turbo/smoke/TIMER.COM ::"$id"/TIMER.COM
   mcopy -i "$disk" "$out/programs/$id/TURBOIN.PAS" ::"$id"/TURBOIN.PAS
   printf 'echo compile-%s\r\ntimer\r\n\\turbo.com\r\ntimer\r\n' "$id" >> "$batch"
   if [ "$path" = resident-flat ]; then program=turboout.com; else program=turboout.exe; fi
  else
   mcopy -i "$disk" "$out/programs/$id/PROGRAM.COM" ::"$id"/PROGRAM.COM
   program=program.com
  fi
  printf 'echo output-%s\r\n%s\r\necho end-%s\r\ncd \\\r\n' "$id" "$program" "$id" >> "$batch"
 done < "$out/cases"
 printf 'echo turbo-execution-complete\r\n' >> "$batch"
 mcopy -i "$disk" "$batch" ::
 printf '\n\nrun\n' | build/factory/keys > "$dir/keys"
 extra=
 if [ "$emulator" = bochs ]; then extra='--vga-ext none'; fi
 if ! build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" \
  $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" \
  --expect turbo-execution-complete --quit-after turbo-execution-complete \
  --timeout-ms "${TURBO_EXEC_TIMEOUT_MS:-300000}" --name run --out "$dir" \
  > "$dir/stdout" 2> "$dir/report"; then
  cat "$dir/report"; failed=1
 fi
 tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 if [ "$emulator" = qemu ]; then
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report"
  test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2
 else grep -Fx '[bochs] triple_fault=0' "$dir/report"; fi
 if grep -E 'TURBO-.*(ERROR|SKIP)|PANIC|Bad command|File not found' "$dir/serial.txt"; then failed=1; fi
 count=0
 while read -r id name path; do
  scratch=$dir/$id
  mkdir -p "$scratch"
  sed -n "/^output-$id\$/,/^end-$id\$/p" "$dir/serial.txt" | sed '1d;$d' > "$scratch/stdout"
  if ! grep -qx "end-$id" "$dir/serial.txt" || ! diff -u "$out/reference/$name/stdout" "$scratch/stdout"; then
   echo "FAIL $emulator $name $path stdout/completion"; failed=1
  else echo "PASS $emulator $name $path"; fi
  for file in "$out/reference/$name"/*.BIN; do
   test -f "$file" || continue
   mcopy -o -i "$disk" ::"$id"/"$(basename "$file")" "$scratch/$(basename "$file")"
   cmp "$file" "$scratch/$(basename "$file")"
  done
  if [ "$path" = resident ] || [ "$path" = resident-flat ]; then
   sed -n "/^compile-$id\$/,/^output-$id\$/p" "$dir/serial.txt" |
    awk -v name="$name" '/^TICK=/ {t=substr($0,6)+0;if(n++==0)start=t;else {d=t-start;if(d<0)d+=8640000;printf "%s compile=%.2fs (guest clock, 1s resolution)\n",name,d/100}} END{if(n!=2)exit 1}'
   mcopy -o -i "$disk" ::"$id"/TURBOOUT.EXE "$scratch/program.exe"
   build/factory/mz/mzcheck "$scratch/program.exe" > "$scratch/mzcheck.log"
   if [ "$path" = resident-flat ]; then
    mcopy -o -i "$disk" ::"$id"/TURBOOUT.COM "$scratch/program.com"
   fi
  fi
  # All registered corpus/regression cases fit the conventional arena. A
  # missing flat file must fail, and a renamed extended EXE is no substitute.
  case $path in
  hosted-flat) flat=$out/programs/$id/PROGRAM.COM ;;
  resident-flat) flat=$scratch/program.com ;;
  *) flat= ;;
  esac
  if [ -n "$flat" ]; then
   test -s "$flat"
   test "$(od -An -tx1 -N2 "$flat" | tr -d ' \n')" != 4d5a
  fi
  count=$((count+1))
 done < "$out/cases"
 echo "$emulator: $count generated executions compared to independent FPC"
done
exit "$failed"
