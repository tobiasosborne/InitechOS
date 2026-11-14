#!/bin/sh
# GM2 immutable FAT16 volume, reached through the shipped DOS programs.
# All fixtures are disk data, never a kernel scenario. mtools is the oracle.
set -eu
root=$(realpath "${1:?root}")
out=${2:-$root/build/dos/smoke-fat16}
mkdir -p "$out"
out=$(realpath "$out")
cd "$root"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C
fail(){ echo "FAIL FAT16 smoke: $*" >&2;exit 1; }
truncate -s $((16384*512)) "$out/import.img"
mformat -i "$out/import.img" -T 16384 -c 1 -h 16 -s 63 -H 4032 -N 0x494e4954 -v IMPORT ::
printf 'Imported accounts\r\n' > "$out/expected"
mcopy -i "$out/import.img" "$out/expected" ::KEEP.TXT
mmd -i "$out/import.img" ::SUB
mcopy -i "$out/import.img" "$out/expected" ::SUB/NESTED.TXT
mdir -a -b -i "$out/import.img" :: > "$out/before.list"
cat > "$out/AUTOEXEC.BAT" <<'BAT'
@ECHO OFF
PATH=A:\
ECHO FAT16-BEGIN
DIR D:
TYPE D:\KEEP.TXT
TYPE D:\SUB\NESTED.TXT
COPY D:\KEEP.TXT A:\OUT.TXT
CHKDSK D:
ECHO REFUSE-FORMAT
FORMAT D:
ECHO REFUSE-SYS
SYS D:
ECHO REFUSE-LABEL
LABEL D: CHANGED
ECHO REFUSE-CHKDSK
CHKDSK D: /F
ECHO REFUSE-DISKCOPY
DISKCOPY A: D:
ECHO REFUSE-DISKCOMP
DISKCOMP A: D:
ECHO REFUSE-COPY
COPY A:\OUT.TXT D:\NEW.TXT
ECHO REFUSE-ATTRIB
ATTRIB +R D:\KEEP.TXT
ECHO REFUSE-DEL
DEL D:\KEEP.TXT
ECHO REFUSE-REN
REN D:\KEEP.TXT LOST.TXT
ECHO REFUSE-MD
MD D:\NEW
ECHO FAT16-FDISK
ECHO REFUSE-FDISK-ACTIVE
FDISK
ECHO FAT16-DONE
BAT
if [ "${DOS_POLICY_WRITE_CONTROL:-0}" = 1 ];then
 # Oracle control: a legitimate FAT12 write on the boot disk MUST make the
 # whole-image assertion fail, including Bochs's private writable boot clone.
 sed -i '/^ECHO FAT16-DONE/i ECHO changed>C:\\CONTROL.TXT' "$out/AUTOEXEC.BAT"
fi
# FDISK changes the mounted FAT16 partition active flag; its guard returns
# immediately on refusal. Host coverage separately checks deletion.
printf '2\n2\n' | build/factory/keys > "$out/keys"
for cpu in ${DOS_POLICY_EMULATORS:-qemu bochs}; do
 run=$out/$cpu
 mkdir -p "$run"
 cp build/image/boot.img "$run/boot.img"
 truncate -s $((20480*512)) "$run/boot.img"
 printf 'label: dos\nunit: sectors\nstart=63,size=2880,type=1,bootable\nstart=4032,size=16384,type=4\n' | sfdisk --no-reread --no-tell-kernel "$run/boot.img" > "$run/sfdisk.log" 2>&1
 dd if="$out/import.img" of="$run/boot.img" bs=512 seek=4032 conv=notrunc status=none
 cp "$run/boot.img" "$run/before.img"
 cp build/image/data.img "$run/data.img"
 mcopy -o -i "$run/data.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
 # QEMU's raw boot writes must reach this private copy; Bochs opens its
 # private raw image directly. No product image is modified.
 case $cpu in qemu) set -- --boot-rw;; bochs) set --;; *) fail "unknown emulator $cpu";; esac
 if ! "build/factory/${cpu}_harness" --disk "$run/boot.img" --disk2 "$run/data.img" "$@" \
  --keys-after FAT16-FDISK --keys "$(cat "$out/keys")" \
  --expect FAT16-DONE --quit-after FAT16-DONE --timeout-ms "${DOS_POLICY_TIMEOUT_MS:-90000}" \
  --out "$run" --name policy > "$run/stdout" 2> "$run/report";then cat "$run/report";fail "$cpu transport";fi
 tr -d '\r' < "$run/policy.serial" > "$run/text"
 grep -Fx "[$cpu] OK=1" "$run/report" >/dev/null || grep -Fx '[harness] OK=1' "$run/report" >/dev/null || fail "$cpu incomplete transport"
 grep -F 'bytes total disk space' "$run/text" >/dev/null || fail "$cpu plain CHKDSK report"
 grep -Fx 'Imported accounts' "$run/text" >/dev/null || fail "$cpu TYPE"
 # Each maintenance command has its own delimited refusal, so one diagnostic
 # cannot hide a skipped utility.
 for command in FORMAT SYS LABEL CHKDSK DISKCOPY DISKCOMP FDISK-ACTIVE;do
  awk -v mark="REFUSE-$command" '$0==mark{p=1;next} p&&/^REFUSE-|^FAT16-/{exit} p{print}' "$run/text" > "$run/$command.txt"
  grep -Fx 'Write protect error writing drive D:' "$run/$command.txt" >/dev/null || fail "$cpu $command refusal"
 done
 for command in COPY ATTRIB DEL;do
  awk -v mark="REFUSE-$command" '$0==mark{p=1;next} p&&/^REFUSE-|^FAT16-/{exit} p{print}' "$run/text" > "$run/$command.txt"
  grep -F 'Access denied' "$run/$command.txt" >/dev/null || fail "$cpu $command file refusal"
 done
 # Existing REN and MD command diagnostics use their captured utility rows.
 for spec in "REN|Duplicate file name" "MD|Unable to create directory";do
  command=${spec%%|*};message=${spec#*|}
  awk -v mark="REFUSE-$command" '$0==mark{p=1;next} p&&/^REFUSE-|^FAT16-/{exit} p{print}' "$run/text" > "$run/$command.txt"
  grep -F "$message" "$run/$command.txt" >/dev/null || fail "$cpu $command refusal"
 done
 actual=$run/boot.img
 if [ "$cpu" = bochs ];then
  actual=$(sed -n 's/^\[bochs\] private_boot=//p' "$run/report")
  [ -f "$actual" ] || fail 'Bochs writable boot clone is missing'
 fi
 cmp "$run/before.img" "$actual" || fail "$cpu whole physical image changed"
 dd if="$actual" of="$run/after.img" bs=512 skip=4032 count=16384 status=none
 cmp "$out/import.img" "$run/after.img" || fail "$cpu volume changed"
 mdir -a -b -i "$run/after.img" :: > "$run/after.list"
 cmp "$out/before.list" "$run/after.list" || fail "$cpu inventory changed"
 mtype -i "$run/after.img" ::KEEP.TXT > "$run/keep"
 mtype -i "$run/after.img" ::SUB/NESTED.TXT > "$run/nested"
 mtype -i "$run/data.img" ::OUT.TXT > "$run/copied"
 for file in keep nested copied;do cmp "$out/expected" "$run/$file" || fail "$cpu $file bytes";done
 echo "GREEN FAT16 $cpu: reads/copy-out, seven disk refusals, five file refusals, whole-image identity and mtools readback"
done
