#!/bin/sh
# Operator ATA A: terminology; DOS 3.30 diskcopy-break supplies Ctrl-C,
# exit status 3 and no-write semantics. mtools/fsck verify each medium.
set -eu
root=${1:?root} out=${2:?output}
cd "$root"
mkdir -p "$out"; out=$(realpath "$out")
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
fail(){ echo "FAIL ATA volume smoke $cpu $scenario: $*" >&2; exit 1; }
for cpu in ${DOS_ATA_EMULATORS:-qemu bochs}; do
 for scenario in alone format DISKCOPY DISKCOMP; do
  d=$(mktemp -d "$out/$cpu-$scenario.XXXXXX")
  case $scenario in alone) attachment=alone;; *) attachment=with-data;; esac
  media_prepare "${DOS_TEST_MEDIA:-staged}" "$attachment" "$d"
  [ "$media_system" = C ] || fail 'installed C: source required'
  if [ -n "$media_data" ]; then
   printf 'Independent ATA data\r\n' > "$d/known"
   mcopy -i "$media_data" "$d/known" ::KNOWN.TXT
   cp "$media_data" "$d/a-before.img"
  fi
  {
   printf '%s\n' '@ECHO OFF' 'FORMAT C:' 'IF ERRORLEVEL 5 ECHO ATA-BAD' 'IF NOT ERRORLEVEL 4 ECHO ATA-BAD'
   case $scenario in
    format)
     printf '%s\n' 'FORMAT A:' 'IF ERRORLEVEL 1 ECHO ATA-BAD' \
      'IF EXIST A:\KNOWN.TXT ECHO ATA-BAD' 'ECHO Newly formatted ATA data>A:\FRESH.TXT';;
    DISKCOPY|DISKCOMP)
     printf '%s\n' "$scenario A: A:" 'IF ERRORLEVEL 4 ECHO ATA-BAD' 'IF NOT ERRORLEVEL 3 ECHO ATA-BAD';;
   esac
   printf '%s\n' 'ECHO ATA-DONE'
  } | sed 's/$/\r/' > "$d/AUTOEXEC.BAT"
  mcopy -o -i "$media_work" "$d/AUTOEXEC.BAT" ::AUTOEXEC.BAT
  cp "$media_boot" "$d/c-before.img"
  case $scenario in
   alone) set -- ;;
   format) set -- --keys-after 'Insert new volume for drive A:' --keys 'ret,n';;
   DISKCOPY) first='SOURCE'; second='TARGET'; preposition=in;;
   DISKCOMP) first='FIRST'; second='SECOND'; preposition=into;;
  esac
  case $scenario in
   DISKCOPY|DISKCOMP) set -- --keys-after "Insert $first volume $preposition drive A:" --keys 'x,ctrl-c';;
  esac
  media_run "$cpu" "build/factory/${cpu}_harness" "$@" \
   --expect ATA-DONE --quit-after ATA-DONE --timeout-ms "${DOS_ATA_TIMEOUT_MS:-60000}" \
   --out "$d" --name run > "$d/driver.log" 2> "$d/report" || { cat "$d/report"; fail transport; }
  grep -F 'OK=1' "$d/report" >/dev/null || fail completion
  grep -F 'triple_fault=0' "$d/report" >/dev/null || fail 'CPU faults'
  tr -d '\r' < "$d/run.serial" > "$d/text"
  grep -Fx 'Format not supported on drive C:' "$d/text" >/dev/null || fail 'missing running C: refusal'
  if grep -E 'ATA-BAD|[Dd]iskette' "$d/text"; then fail 'wording or status'; fi
  media_readback "$cpu" "$d/report"
  cmp "$d/c-before.img" "$media_written" || fail 'whole C: disk changed'
  case $scenario in
   alone) ;;
   format)
    grep -Fx 'Insert new volume for drive A:' "$d/text" >/dev/null || fail 'FORMAT A: prompt'
    grep -Fx 'Format complete' "$d/text" >/dev/null || fail 'FORMAT A: verdict'
    mdir -a -b -i "$media_data" :: > "$d/a-list"
    if grep -Fxi '::/KNOWN.TXT' "$d/a-list"; then fail 'stale A: file after FORMAT'; fi
    printf 'Newly formatted ATA data\r\n' > "$d/fresh"
    mtype -i "$media_data" ::FRESH.TXT > "$d/readback"
    cmp "$d/fresh" "$d/readback" || fail 'new file on A: readback';;
   DISKCOPY|DISKCOMP)
    grep -Fx "Insert $first volume $preposition drive A:" "$d/text" >/dev/null || fail 'first prompt'
    grep -Fx "Insert $second volume into drive A:" "$d/text" >/dev/null || fail 'second prompt'
    grep -Fx '^C' "$d/text" >/dev/null || fail 'cancellation'
    cmp "$d/a-before.img" "$media_data" || fail 'whole A: changed on cancellation'
    mtype -i "$media_data" ::KNOWN.TXT > "$d/readback"
    cmp "$d/known" "$d/readback" || fail 'A: readback';;
  esac
  if [ -n "$media_data" ]; then
   fsck.fat -n "$media_data" > "$d/fsck" || fail 'A: filesystem consistency'
  fi
  echo "GREEN ATA $cpu $scenario: running C: refused, media and DOS results verified"
 done
done
