#!/bin/sh
set -eu
cd "$(dirname "$0")/../../../.."
root=$PWD
out=$root/build/apps/initechbase/smoke
src=$root/apps/initechbase
mkdir -p "$out"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 MTOOLS_SKIP_CHECK=1
checks=0
fail() { echo "FAIL $*" >&2; echo "InitechBase SMOKE: $checks checks completed, 1 failure" >&2; exit 1; }
line() { grep -Fx -- "$2" "$1" >/dev/null || fail "missing [$2] in $1"; echo "ok $emulator: $2"; checks=$((checks+1)); }
cpus=${IBASE_SMOKE_CPUS-qemu bochs}
sh factory/smoke-selection.sh InitechBase-emulators "$cpus" 'qemu bochs'
for emulator in $cpus; do
 dir=$out/$emulator
 mkdir -p "$dir"
 disk=$dir/test.img
 # Fresh TEST disk replaces the working volume. Copy installed product files
 # from that volume, never compile test scenarios into the boot image.
 dd if=/dev/zero of="$disk" bs=512 count=2880 2>/dev/null
 mformat -i "$disk" -f 1440 -N 0x49424153 -v TEST ::
 mmd -i "$disk" ::APPS
 for ext in EXE OVL; do
  mcopy -o -i build/image/data.img ::APPS/INITBASE.$ext "$dir/INITBASE.$ext"
  mcopy -i "$disk" "$dir/INITBASE.$ext" ::APPS/
 done
 mcopy -i "$disk" build/dos/COMMAND.COM ::
 printf 'FILES=20\r\nSHELL=COMMAND.COM /P\r\n' > "$dir/CONFIG.SYS"
 mcopy -i "$disk" "$dir/CONFIG.SYS" ::
 build/apps/initechbase/host/prog_diff --mint-parts "$dir/PARTS.DBF"
 build/apps/initechbase/host/mint_clients_dbf "$dir/BASE.DBF"
 build/apps/initechbase/smoke/fixtures "$dir/INVOICE.DBF"
 cp "$src/tests/xbase_prog_diff/corpus/dml/index.prg" "$dir/IDXTEST.PRG"
 cp "$src/tests/canon/y2kacct_driver.prg" "$dir/Y2KACCT.PRG"
 cp "$src/tests/smoke/values.prg" "$dir/VALUES.PRG"
 # C21 is the independent real-III+ stale-index repair capture. Only fixture
 # names change to coexist with this journey's existing BASE/NAMES pair.
 cp "$src/tests/conformance/cases/C21/BASE.DBF" "$dir/SHORT.DBF"
 cp "$src/tests/conformance/cases/C21/OLD.NDX" "$dir/STALE.NDX"
 tr -d '\r' < "$src/tests/conformance/cases/C21/MINT.PRG" |
  sed 's/USE BASE INDEX OLD/USE SHORT INDEX STALE/; s/COPY FILE BASE.DBF TO SAVED.DBF/COPY FILE SHORT.DBF TO REPAIRED.DBF/; s/RESULT.OUT/C21.OUT/; /^QUIT$/d' > "$dir/C21TEST.PRG"
 for file in PARTS.DBF BASE.DBF INVOICE.DBF IDXTEST.PRG Y2KACCT.PRG VALUES.PRG SHORT.DBF STALE.NDX C21TEST.PRG; do
  mcopy -i "$disk" "$dir/$file" ::APPS/
 done
 build/factory/keys < "$src/tests/smoke/session.txt" > "$dir/keys"
 extra=
 if [ "$emulator" = bochs ]; then
  extra='--vga-ext none --rtc-time0 699440400'
  # Pace commands, but leave the final completion command terminal. The
  # driver correctly rejects any trace step after its quit marker is seen.
  pause=${IBASE_SMOKE_WAIT_MS:-1000}
  sed -i "s/ret/ret,w$pause/g; s/,w$pause\$//" "$dir/keys"
  # Child teardown can outlast the ordinary command gap on Bochs. Its DOS
  # input reset discards an echo queued before COMMAND.COM resumes.
  exit_pause=5000
  if [ "$pause" -gt "$exit_pause" ]; then exit_pause=$pause; fi
  sed -i "s/q,u,i,t,ret,w$pause,/q,u,i,t,ret,w$exit_pause,/" "$dir/keys"
 fi
 if ! build/factory/${emulator}_harness --disk build/image/boot.img --disk2 "$disk" \
    $extra --keys-after SHELL-READY --keys "$(cat "$dir/keys")" \
    --expect initechbasesmokecomplete --quit-after initechbasesmokecomplete \
    --timeout-ms "${IBASE_SMOKE_TIMEOUT_MS:-90000}" --name run --out "$dir" > "$dir/stdout" 2> "$dir/report"; then
  cat "$dir/report"; fail "$emulator session incomplete";
 fi
 tr -d "\r" < "$dir/run.serial" | sed "s/\[DML-BEGIN\]//g; s/\[DML-END\].*//; s/\. [a-z?].*//" > "$dir/serial.txt"
 if [ "$emulator" = qemu ]; then
  grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$dir/report" || fail 'QEMU faults'
  test "$(grep -c 'CPU Reset' "$dir/run.qemu.log")" -eq 2 || fail 'QEMU resets'
  if grep -Ei 'triple fault|check_exception old:' "$dir/run.qemu.log"; then fail 'QEMU fault trace'; fi
 else
  grep -Fx '[bochs] triple_fault=0' "$dir/report" || fail 'Bochs fault'
 fi
 # Literal lines come from real III+ captures; remove only transport CR/EOF.
 checks=$((checks+1)) # the emulator fault verdict above
 tr -d '\r\032' < "$src/tests/xbase_prog_diff/mint/prog-mint/query/REAL.OUT" |
  sed -n '/^Record#/,/^      4/p' > "$dir/list.expected"
 while IFS= read -r expected; do line "$dir/serial.txt" "$expected"; done < "$dir/list.expected"
 tr -d '\r\032' < "$src/tests/xbase_prog_diff/mint/golden/index/GOLD.OUT" |
  sed 's/\[DML-BEGIN\]//; s/\[DML-END\]//' | sed '/^$/d' > "$dir/index.expected"
 printf "\n" >> "$dir/index.expected"
 while IFS= read -r expected; do test -z "$expected" || line "$dir/serial.txt" "$expected"; done < "$dir/index.expected"
 while IFS= read -r expected; do
  test -z "$expected" || line "$dir/serial.txt" "$expected"
 done < "$src/tests/canon/y2k_accounting.out"
 line "$dir/serial.txt" 'initechbasesmokecomplete'
 # Read back AFTER exit; both parsers are independent of the product codecs.
 for ext in DBF; do mcopy -o -i "$disk" ::APPS/PARTS.$ext "$dir/RESULT.$ext"; done
 python3 "$src/tests/dbf_ref.py" --records "$dir/RESULT.DBF" > "$dir/records.txt"
 diff -u "$src/tests/xbase_prog_diff/mint/prog-mint/mutate/PARTS.records" "$dir/records.txt" || fail 'persisted REPLACE/APPEND/DELETE'
 echo "ok $emulator: mtools + independent DBF reader: all five records (REPLACE/APPEND/DELETE)"
 checks=$((checks+1))
 mcopy -o -i "$disk" ::APPS/NAMES.NDX "$dir/NAMES.NDX"
 python3 "$src/tests/ndx_ref.py" --index-dump "$dir/NAMES.NDX" > "$dir/index.txt"
 python3 "$src/tests/ndx_ref.py" --index-dump "$src/tests/xbase_prog_diff/mint/golden/index/NAMES.NDX" > "$dir/index.want"
 diff -u "$dir/index.want" "$dir/index.txt" || fail 'persisted INDEX'
 echo "ok $emulator: mtools + independent NDX reader: index matches real III+"
 checks=$((checks+1))
 mcopy -o -i "$disk" ::APPS/C21.OUT "$dir/C21.OUT"
 cmp "$src/tests/conformance/reference/C21/RESULT.OUT" "$dir/C21.OUT" || fail 'C21 real III+ transcript'
 mcopy -o -i "$disk" ::APPS/REPAIRED.DBF "$dir/REPAIRED.DBF"
 python3 "$src/tests/conformance/read_dbf.py" "$dir/REPAIRED.DBF" > "$dir/repaired.txt"
 python3 "$src/tests/conformance/read_dbf.py" "$src/tests/conformance/reference/C21/SAVED.DBF" > "$dir/repaired.want"
 cmp "$dir/repaired.want" "$dir/repaired.txt" || fail 'C21 wrong-record write'
 mcopy -o -i "$disk" ::APPS/STALE.NDX "$dir/STALE.NDX"
 python3 "$src/tests/ndx_ref.py" --index-dump "$dir/STALE.NDX" > "$dir/stale.txt"
 python3 "$src/tests/ndx_ref.py" --index-dump "$src/tests/conformance/reference/C21/OLD.NDX" > "$dir/stale.want"
 cmp "$dir/stale.want" "$dir/stale.txt" || fail 'C21 stale index entries after REINDEX'
 checks=$((checks+3))
 echo "ok $emulator: C21 transcript, repaired DBF and rebuilt NDX match real III+"
 echo "$emulator: InitechBase smoke GREEN (8 MiB)"
done
# GM5: a fresh genuine III+ session must open serialized target output.
core=$out/real-reopen/core
mkdir -p "$core/build"
(cd "$core" && "$root/build/apps/initechbase/host/test_ibase_core" "$core") > "$out/real-core.log" 2>&1
mkdir -p "$core/build/dbsafe"
(cd "$core" && "$root/build/apps/initechbase/host/test_ibase_dbsafe" flush) > "$out/real-flush.log" 2>&1
sh "$src/tests/write-safety/reopen.sh" "$out/real-reopen" "$dir" "$core"
echo "InitechBase SMOKE: $checks checks, 0 failures (emulators: $cpus at 8 MiB)"
