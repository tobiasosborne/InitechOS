#!/bin/sh
# DOS 3.30 sys-clean and MS-DOS 3.3 User's Guide pp. 74–76, 119.
# InitechDOS transfers its resident shell's COMMAND.COM forwarder as well.
set -eu
root=$(realpath "${1:?root}")
out=$(realpath -m "${2:?output}")
cd "$root"
mkdir -p "$out"
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
fail(){ echo "FAIL system transfer $cpu $scenario: $*" >&2; exit 1; }
for cpu in ${DOS_TRANSFER_CPUS:-qemu bochs}; do
 for scenario in SYS-A FORMAT-A SYS-D FORMAT-D; do
  utility=${scenario%-*} letter=${scenario##*-}
  run=$(mktemp -d "$out/$cpu-$scenario.XXXXXX")
  case $letter in A) attachment=with-data;; D) attachment=alone;; esac
  media_prepare "${DOS_TEST_MEDIA:-staged}" "$attachment" "$run"
  [ "$media_system" = C ] || fail 'installed C: source required'
  if [ "$letter" = D ]; then
   truncate -s $((40320*512)) "$media_boot"
   printf 'label: dos\nunit: sectors\nstart=63,size=16384,type=1,bootable\nstart=20160,size=2880,type=1\n' |
    sfdisk --no-reread --no-tell-kernel "$media_boot" > "$run/sfdisk.log" 2>&1
   target=$media_boot@@$((20160*512))
   mformat -i "$target" -T 2880 -h 2 -s 18 -c 1 -H 20160 ::
  fi
  case $scenario in
   SYS-A|FORMAT-A)
    set -- --keys-after 'Insert new volume for drive A:' --keys 'ret,n';;
   SYS-D)
    # Leave two deleted slots so SYS can transfer while preserving KEEP.
    printf 'This file is the unrelated third root entry.\r\n' > "$run/KEEP.TXT"
    for file in FIRST SECOND KEEP; do mcopy -i "$target" "$run/KEEP.TXT" "::$file.TXT"; done
    mdel -i "$target" ::FIRST.TXT ::SECOND.TXT
    set --;;
   FORMAT-D)
    set -- --keys-after TRANSFER-START --keys "$(printf '\ny\n' | build/factory/keys)" ;;
  esac
  {
   printf '@ECHO OFF\r\nECHO TRANSFER-START\r\n'
   if [ "$scenario" = SYS-A ]; then
    printf 'C:\\FORMAT.COM A:\r\nIF ERRORLEVEL 1 ECHO TRANSFER-ERROR\r\n'
   fi
   printf 'C:\\%s.COM %s:' "$utility" "$letter"
   if [ "$utility" = FORMAT ]; then printf ' /S'; fi
   printf '\r\nIF ERRORLEVEL 1 ECHO TRANSFER-ERROR\r\nECHO TRANSFER-DONE\r\n'
  } > "$run/AUTOEXEC.BAT"
  mcopy -o -i "$media_work" "$run/AUTOEXEC.BAT" ::AUTOEXEC.BAT
  driver=build/factory/${cpu}_harness
  media_run "$cpu" "$driver" "$@" --expect TRANSFER-DONE --quit-after TRANSFER-DONE \
   --timeout-ms "${DOS_TRANSFER_TIMEOUT_MS:-30000}" --out "$run" --name transfer \
   > "$run/stdout" 2> "$run/report" || { cat "$run/report"; fail 'transfer session did not complete'; }
  tr -d '\r' < "$run/transfer.serial" > "$run/text"
  grep -Fqx 'System transferred' "$run/text" || fail 'missing DOS transfer verdict'
  if grep -Fx TRANSFER-ERROR "$run/text"; then fail 'nonzero exit status'; fi
  media_readback "$cpu" "$run/report"
  if [ "$letter" = A ]; then
   target=$media_data
  else
   target=$run/target.fat
   dd if="$media_written" of="$target" bs=512 skip=20160 count=2880 status=none
  fi
  fsck.fat -n "$target" > "$run/fsck" 2>&1 || fail 'target filesystem'
  for file in IO.SYS INITDOS.SYS COMMAND.COM; do
   mcopy -i "$media_work" "::$file" - > "$run/source.bin"
   mcopy -i "$target" "::$file" - > "$run/target.bin" || fail "$file missing"
   cmp "$run/source.bin" "$run/target.bin" || fail "$file bytes"
  done
  if [ "$letter" = A ]; then
   echo "GREEN system transfer $cpu $scenario: all three files match the C: source"
   continue
  fi
  if [ "$scenario" = SYS-D ]; then
   mtype -i "$target" ::KEEP.TXT > "$run/keep"
   cmp "$run/KEEP.TXT" "$run/keep" || fail 'SYS changed unrelated file'
  fi

  # Retain the MBR code and stage2, remove the old system partition and
  # point the first partition slot at the transferred volume in place.
  dd if="$media_written" of="$run/target.img" bs=512 count=17 status=none
  truncate -s $((40320*512)) "$run/target.img"
  dd if="$run/target.fat" of="$run/target.img" bs=512 seek=20160 conv=notrunc status=none
  printf 'label: dos\nunit: sectors\nstart=20160,size=2880,type=1,bootable\n' |
   sfdisk --no-reread --no-tell-kernel "$run/target.img" > "$run/target-sfdisk.log" 2>&1
  "$driver" --disk "$run/target.img" --keys-after SHELL-READY --keys 'ret,ret,v,e,r,ret' \
   --expect 'InitechDOS Version 3.30' --quit-after 'InitechDOS Version 3.30' \
   --timeout-ms "${DOS_TRANSFER_TIMEOUT_MS:-30000}" \
   --out "$run" --name boot > "$run/boot.stdout" 2> "$run/boot.report" \
   || { cat "$run/boot.report"; fail 'transferred target did not boot on its own'; }
  grep -F 'C:\>ver' "$run/boot.serial" >/dev/null || fail 'missing C: prompt on target'
  echo "GREEN system transfer $cpu $scenario: all three files match the source and the target boots"
 done
done
