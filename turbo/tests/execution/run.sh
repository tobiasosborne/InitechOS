#!/bin/sh
# Tier 1: execute direct seed, hosted Pascal native, and resident Pascal output
# on the product image. FPC is the independent stdout/file oracle. Selectors
# allow red-first probes and tier-2 semantic mutation catchers without changing
# the complete default matrix. CLAUDE.md: The oracle is the truth.
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
. "$root/turbo/tests/checks.sh"
setup mkdir -p "$root/build/turbo/smoke/execution"
out=${TURBO_EXEC_OUT:-$(mktemp -d "$root/build/turbo/smoke/execution/run.XXXXXX")}
hosted=${TURBO_HOSTED:-$root/build/turbo/compiler/turbo_fpc}
paths=${TURBO_EXEC_PATHS:-seed hosted hosted-flat resident resident-flat}
emulators=${TURBO_EXEC_EMULATORS:-qemu bochs}
sources=${TURBO_EXEC_SOURCES:-$(echo turbo/compiler/fixtures/gen*.pas turbo/tests/execution/*.pas)}
setup mkdir -p "$out"
out=$(CDPATH= cd -- "$out" && pwd)
echo "Turbo execution smoke evidence: $out"
export MTOOLS_SKIP_CHECK=1 INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
failed=0
: > "$out/cases"
n=0
for source in $sources; do
 name=$(basename "$source" .pas)
 ref=$out/reference/$name
 setup rm -rf "$ref"
 setup mkdir -p "$ref"
 case $name in
 for_max|for_min)
  # The product's Integer is 32-bit; TP Integer is 16-bit. Change only
  # the declaration, keeping the endpoint arithmetic and loop identical.
  setup sed 's/i,count: integer/i,count: longint/' "$source" > "$ref/input.pas" ;;
 *) cp "$source" "$ref/input.pas" ;;
 esac
 setup fpc -Mtp -B -O- -v0 -FU"$ref" -FE"$ref" -o"$ref/reference" "$ref/input.pas" > "$ref/fpc.log"
 (cd "$ref" && setup timeout 5 ./reference > stdout)
 if [ -f "${source%.pas}.golden" ]; then assert "$name:FPC authored stdout" diff -u "${source%.pas}.golden" "$ref/stdout"; fi
 for path in $paths; do
  n=$((n+1)); id=$(printf 'P%03d' "$n")
  dir=$out/programs/$id
  setup rm -rf "$dir"
  setup mkdir -p "$dir"
  setup cp "$source" "$dir/TURBOIN.PAS"
  case $path in
  seed)
   setup build/turbo/seed/initechc --emit-asm -o "$dir/program.s" "$source" > "$dir/build.log" 2>&1
   assert "$name:$path cdecl frame header conserved" awk -f "$root/turbo/tests/frame-layout.awk" "$dir/program.s"
   setup sh turbo/compiler/tests/package.sh "$dir/program.s" "$dir/package" >> "$dir/build.log" 2>&1
   setup cp "$dir/package/program.exe" "$dir/PROGRAM.COM" ;;
  hosted|hosted-flat)
   (cd "$dir" && setup "$hosted" > build.log)
   assert_not "$name:$path compile diagnostic" grep -E 'TURBO-.*(ERROR|SKIP)' "$dir/build.log"
   assert "$name:$path native image exists" test -s "$dir/TURBOOUT.EXE"
   assert "$name:$path cdecl frame header conserved" awk -f "$root/turbo/tests/frame-layout.awk" "$dir/TURBOOUT.S"
   if [ "$path" = hosted-flat ]; then artifact=TURBOOUT.COM; else artifact=TURBOOUT.EXE; fi
   assert "$name:$path selected image exists" test -s "$dir/$artifact"
   setup cp "$dir/$artifact" "$dir/PROGRAM.COM" ;;
  resident|resident-flat) ;;
  *) echo "Unknown path: $path" >&2; exit 1 ;;
  esac
  echo "$id $name $path" >> "$out/cases"
 done
done
for emulator in $emulators; do
 dir=$out/$emulator
 setup mkdir -p "$dir"
 disk=$dir/test.img
 setup dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
 setup mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
 setup mcopy -i "$disk" build/dos/COMMAND.COM ::
 # Share the resident compiler, keeping both format matrices within the
 # existing 8 MiB TEST volume. EXEC preserves each case's working directory.
 case " $paths " in
 *" resident "*|*" resident-flat "*) mcopy -i "$disk" build/turbo/compiler/TURBO.COM ::TURBO.COM ;;
 esac
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 setup mcopy -i "$disk" "$dir/CONFIG.SYS" ::
 batch=$dir/RUN.BAT
 segment=0; in_batch=0
 printf '@echo off\r\n' > "$batch"
 while read -r id name path; do
  # COMMAND.COM accepts at most 4096 bytes per batch. Chain small batches
  # at root rather than truncating a large matrix or changing the shell.
  if [ "$in_batch" -eq 24 ]; then
   segment=$((segment+1)); next=$(printf 'B%03d.BAT' "$segment")
   printf '%s\r\n' "$next" >> "$batch"
   setup mcopy -i "$disk" "$batch" ::
   batch=$dir/$next; in_batch=0
   printf '@echo off\r\n' > "$batch"
  fi
  in_batch=$((in_batch+1))
  setup mmd -i "$disk" ::"$id"
  printf 'cd %s\r\n' "$id" >> "$batch"
  if [ "$path" = resident ] || [ "$path" = resident-flat ]; then
   setup mcopy -i "$disk" build/turbo/smoke/TIMER.COM ::"$id"/TIMER.COM
   setup mcopy -i "$disk" "$out/programs/$id/TURBOIN.PAS" ::"$id"/TURBOIN.PAS
   printf 'echo compile-%s\r\ntimer\r\n\\turbo.com\r\ntimer\r\n' "$id" >> "$batch"
   if [ "$path" = resident-flat ]; then program=turboout.com; else program=turboout.exe; fi
  else
   setup mcopy -i "$disk" "$out/programs/$id/PROGRAM.COM" ::"$id"/PROGRAM.COM
   program=program.com
  fi
  printf 'echo output-%s\r\n%s\r\necho end-%s\r\ncd \\\r\n' "$id" "$program" "$id" >> "$batch"
 done < "$out/cases"
 printf 'echo turbo-execution-complete\r\n' >> "$batch"
 setup mcopy -i "$disk" "$batch" ::
 printf '\n\nrun\n' | build/factory/keys > "$dir/keys"
 extra=
 if [ "$emulator" = bochs ]; then extra='--vga-ext none'; fi
 setup build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" \
  $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" \
  --expect turbo-execution-complete --quit-after turbo-execution-complete \
  --timeout-ms "${TURBO_EXEC_TIMEOUT_MS:-300000}" --name run --out "$dir" \
  > "$dir/stdout" 2> "$dir/report"
 setup test -s "$dir/run.serial"
 setup tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 if [ "$emulator" = qemu ]; then
  setup grep -Fq 'launched=1 timed_out=0 exit=0 signal=0' "$dir/report"
  setup grep -Fq 'quit_after_found=1' "$dir/report"
  setup grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report"
  setup test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2
 else
  setup grep -Fx '[bochs] triple_fault=0' "$dir/report"
  setup grep -Fx '[bochs] OK=1' "$dir/report"
 fi
 assert_not "$emulator:compiler and loader diagnostics" grep -E 'TURBO-.*(ERROR|SKIP)|PANIC|Bad command|File not found' "$dir/serial.txt"
 count=0
 while read -r id name path; do
  scratch=$dir/$id
  setup mkdir -p "$scratch"
  setup sed -n "/^output-$id\$/,/^end-$id\$/p" "$dir/serial.txt" > "$scratch/bracket"
  setup sed '1d;$d' "$scratch/bracket" > "$scratch/stdout"
  assert "$emulator:$name:$path completion" grep -qx "end-$id" "$dir/serial.txt"
  assert "$emulator:$name:$path FPC stdout" diff -u "$out/reference/$name/stdout" "$scratch/stdout"
  for file in "$out/reference/$name"/*.BIN; do
   test -f "$file" || continue
   setup mdir -b -i "$disk" ::"$id"/ > "$scratch/list"
   leaf=$(basename "$file")
   assert "$emulator:$name:$path $leaf exists" grep -iEq "(^|/)$leaf\$" "$scratch/list"
   setup mcopy -o -i "$disk" ::"$id"/"$(basename "$file")" "$scratch/$(basename "$file")"
   assert "$emulator:$name:$path FPC file bytes" cmp "$file" "$scratch/$(basename "$file")"
  done
  if [ "$path" = resident ] || [ "$path" = resident-flat ]; then
   setup sed -n "/^compile-$id\$/,/^output-$id\$/p" "$dir/serial.txt" > "$scratch/compile-bracket"
   setup awk -v name="$name" '/^TICK=/ {t=substr($0,6)+0;if(n++==0)start=t;else {d=t-start;if(d<0)d+=8640000;printf "%s compile=%.2fs (guest clock, 1s resolution)\n",name,d/100}} END{if(n!=2)exit 1}' "$scratch/compile-bracket"
   setup mcopy -o -i "$disk" ::"$id"/TURBOOUT.EXE "$scratch/program.exe"
   assert "$emulator:$name:$path native container format" build/factory/mz/mzcheck "$scratch/program.exe" > "$scratch/mzcheck.log"
   if [ "$path" = resident-flat ]; then
    setup mcopy -o -i "$disk" ::"$id"/TURBOOUT.COM "$scratch/program.com"
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
   assert "$emulator:$name:$path flat image exists" test -s "$flat"
   assert "$emulator:$name:$path actual flat form" test "$(od -An -tx1 -N2 "$flat" | tr -d ' \n')" != 4d5a
  fi
  checks=$((checks+1))
  count=$((count+1))
 done < "$out/cases"
 echo "$emulator: $count generated executions compared to independent FPC"
done
finish
