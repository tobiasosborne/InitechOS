#!/bin/sh
set -eu
cd "$(dirname "$0")/../../.."
root=$PWD
out=$root/build/turbo/smoke/corpus
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1
for emulator in qemu bochs; do
 dir=$out/$emulator
 mkdir -p "$dir"
 disk=$dir/test.img
 dd if=/dev/zero of="$disk" bs=512 count=16384 2>/dev/null
 mformat -i "$disk" -T 16384 -c 8 -h 16 -s 63 -N 0x54555242 -v TEST ::
 mcopy -i "$disk" build/dos/COMMAND.COM ::
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 mcopy -i "$disk" "$dir/CONFIG.SYS" ::
 printf '@echo off\r\n' > "$dir/RUN.BAT"
 : > "$dir/cases"
 n=0
 for source in turbo/compiler/fixtures/gen*.pas; do
  name=$(basename "$source" .pas)
  if [ "$emulator" = bochs ]; then
   case $name in gen_tiny|gen_control_deep|gen_file_shared) ;; *) continue ;; esac
  fi
  n=$((n+1)); id=$(printf 'P%02d' "$n")
  echo "$id $name" >> "$dir/cases"
  mmd -i "$disk" ::"$id"
  mcopy -i "$disk" build/turbo/compiler/TURBO.COM ::"$id"/TURBO.COM
  mcopy -i "$disk" build/turbo/smoke/TIMER.COM ::"$id"/TIMER.COM
  mcopy -i "$disk" "$source" ::"$id"/TURBOIN.PAS
  printf 'cd %s\r\necho compile-%s\r\ntimer\r\nturbo\r\ntimer\r\necho output-%s\r\nturboout.com\r\necho end-%s\r\ncd \\\r\n' \
     "$id" "$id" "$id" "$id" >> "$dir/RUN.BAT"
 done
 printf 'echo turbo-smoke-complete\r\n' >> "$dir/RUN.BAT"
 mcopy -i "$disk" "$dir/RUN.BAT" ::
 printf '\n\nrun\n' | build/factory/keys > "$dir/keys"
 extra=
 if [ "$emulator" = bochs ]; then extra='--vga-ext none'; fi
 if ! build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" \
     $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" \
     --expect turbo-smoke-complete --quit-after turbo-smoke-complete \
     --timeout-ms 300000 --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"; then
     cat "$dir/report"; exit 1
 fi
 tr -d '\r' < "$dir/run.serial" > "$dir/serial.txt"
 if [ "$emulator" = qemu ]; then
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report"
  test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2
 else
  grep -Fx '[bochs] triple_fault=0' "$dir/report"
 fi
 if grep -E 'TURBO-.*(ERROR|SKIP)|PANIC|Bad command|File not found' "$dir/serial.txt"; then exit 1; fi
 test "$(grep -c '^TURBO-GEN-OK ' "$dir/serial.txt")" -eq "$n"
 : > "$dir/times.txt"
 while read -r id name; do
  scratch=$dir/$id
  mkdir -p "$scratch"
  sed -n "/^output-$id\$/,/^end-$id\$/p" "$dir/serial.txt" | sed '1d;$d' > "$scratch/stdout"
  diff -u "turbo/compiler/fixtures/$name.golden" "$scratch/stdout"
  sed -n "/^compile-$id\$/,/^output-$id\$/p" "$dir/serial.txt" |
   awk -v name="$name" '/^TICK=/ {t=substr($0,6)+0;if(n++==0)start=t;else {d=t-start;if(d<0)d+=8640000;printf "%s compile=%.2fs (guest DOS clock; 1s resolution)\n",name,d/100}} END{if(n!=2)exit 1}' >> "$dir/times.txt"
  mcopy -o -i "$disk" ::"$id"/TURBOOUT.COM "$scratch/program.com"
  mcopy -o -i "$disk" ::"$id"/TURBOOUT.EXE "$scratch/program.exe"
  build/factory/mz/mzcheck "$scratch/program.exe" > "$scratch/mzcheck.log"
  # FPC execution remains independently checked against the authored golden.
  cp "turbo/compiler/fixtures/$name.pas" "$scratch/reference.pas"
  fpc -B -O- -v0 -FU"$scratch" -FE"$scratch" -o"$scratch/reference" "$scratch/reference.pas" > "$scratch/fpc.log"
  (cd "$scratch" && ./reference > reference.stdout)
  diff -u "turbo/compiler/fixtures/$name.golden" "$scratch/reference.stdout"
  for file in "$scratch"/*.BIN; do
   test -f "$file" || continue
   mcopy -o -i "$disk" ::"$id"/"$(basename "$file")" "$file.guest"
   cmp "$file" "$file.guest"
  done
 done < "$dir/cases"
 cat "$dir/times.txt"
 echo "$emulator: $n/$n native in-OS compile/run/FPC comparisons; separate TEST disk"
done
