#!/bin/sh
# Operator ATA A: terminology; genuine DOS 3.30 diskcopy-break supplies
# Ctrl-C/exit3/no-write semantics. Whole-image mtools-readable seeds grade IO.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?programs} out=${4:?out}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
fail(){ echo "FAIL ATA volume smoke: $*" >&2; exit 1; }
for cpu in qemu bochs; do
 for program in DISKCOPY DISKCOMP; do
  d=$out/$cpu-$program; mkdir -p "$d"
  truncate -s 1474560 "$d/data.img"
  mformat -i "$d/data.img" -f 1440 ::
  nasm -f bin dos/tests/runtime/done.asm -o "$d/DONE.COM"
  mcopy -i "$d/data.img" "$programs/$program.COM" "$programs/FORMAT.COM" "$d/DONE.COM" ::
  printf 'Independent ATA data\r\n' > "$d/known"
  mcopy -i "$d/data.img" "$d/known" ::KNOWN.TXT
  printf '@echo off\r\necho ATA-FORMAT-BEGIN\r\nA:\\FORMAT A:\r\nif errorlevel 5 echo ATA-FORMAT-BAD\r\nif not errorlevel 4 echo ATA-FORMAT-BAD\r\necho ATA-FORMAT-END\r\nA:\\%s A: A:\r\nif errorlevel 4 echo ATA-CANCEL-BAD\r\nif not errorlevel 3 echo ATA-CANCEL-BAD\r\nA:\\DONE\r\n' "$program" > "$d/AUTOEXEC.BAT"
  mcopy -i "$d/data.img" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
  cp "$d/data.img" "$d/before.img"
  case $program in DISKCOPY) first='SOURCE'; second='TARGET'; preposition=in;; DISKCOMP) first='FIRST'; second='SECOND'; preposition=into;; esac
  "build/factory/${cpu}_harness" --disk "$boot" --disk2 "$d/data.img" \
   --keys-after "Insert $first volume $preposition drive A:" --keys 'x,ctrl-c' \
   --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 60000 \
   --out "$d" --name run > "$d/driver.log" 2> "$d/report" || { cat "$d/report"; fail "$cpu $program transport"; }
  grep -F 'OK=1' "$d/report" >/dev/null || fail "$cpu $program completion"
  grep -F 'triple_fault=0' "$d/report" >/dev/null || fail "$cpu $program CPU faults"
  tr -d '\r' < "$d/run.serial" > "$d/text"
  grep -Fx 'Format not supported on drive A:' "$d/text" >/dev/null || fail "$cpu running A FORMAT guard"
  grep -Fx "Insert $first volume $preposition drive A:" "$d/text" >/dev/null || fail "$cpu $program first prompt"
  grep -Fx "Insert $second volume into drive A:" "$d/text" >/dev/null || fail "$cpu $program second prompt"
  grep -Fx '^C' "$d/text" >/dev/null || fail "$cpu $program cancellation"
  if grep -E 'ATA-(FORMAT|CANCEL)-BAD|[Dd]iskette' "$d/text"; then fail "$cpu $program wording or status"; fi
  actual=$d/data.img
  if [ "$cpu" = bochs ]; then
   private=$(sed -n 's/^\[bochs\] private_disk2=//p' "$d/report")
   [ -z "$private" ] || actual=$private
  fi
  cmp "$d/before.img" "$actual" || fail "$cpu $program wrote data volume"
  mtype -i "$actual" ::KNOWN.TXT > "$d/readback"
  cmp "$d/known" "$d/readback" || fail "$cpu $program independent data readback"
  fsck.fat -n "$actual" > "$d/fsck" || fail "$cpu $program filesystem consistency"
  echo "GREEN $cpu $program: ATA volume prompts, Ctrl-C exit3, running-A FORMAT refusal, whole-image identity and mtools"
 done
done
