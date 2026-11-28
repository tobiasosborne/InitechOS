#!/bin/sh
# No scenario may classify a failed emulator invocation as a semantic result.
set -eu
root=$(CDPATH= cd -- "$1" && pwd)
boot=$2 out=$3 keys=$4 marker=$5
out=$(CDPATH= cd -- "$out" && pwd)
short=$(mktemp -d /tmp/dos-run.XXXXXX); rmdir "$short"; ln -s "$out" "$short"
trap 'rm -f "$short"' 0 HUP INT TERM
. "$root/dos/tests/runtime/session-media.sh"
prepare_session_boot "$boot" "$short/data.img" "$short/boot.img"
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 TZ=UTC LC_ALL=C
set -- --disk "$short/boot.img" --disk2 "$short/data.img"
case ${DOS_SESSION_EMULATOR:-qemu} in
 qemu)
  tag=harness
  driver=$root/build/factory/qemu_harness
  set -- "$@" --boot-rw --timeout-ms 20000
  if [ -n "${DOS_SESSION_RTC:-}" ]; then set -- "$@" --rtc-base "$DOS_SESSION_RTC"; fi
  ;;
 bochs)
  tag=bochs
  driver=$root/build/factory/desktop-bochs
  make -s -C "$root" -f factory/desktop-tools.mk "$driver"
  export DESKTOP_SERIAL="$short/run.serial"
  set -- "$@" --timeout-ms 45000
  if [ -n "${DOS_SESSION_RTC:-}" ]; then
   set -- "$@" --rtc-time0 "$(TZ=UTC date -d "$DOS_SESSION_RTC" +%s)"
  fi
  if [ -n "$keys" ]; then keys=$(printf '%s\n' "$keys" | sed 's/,$//; s/^/k/; s/,/,k/g'); fi
  ;;
 *) echo "Unknown DOS test emulator: $DOS_SESSION_EMULATOR" >&2; exit 2 ;;
esac
if [ -n "$keys" ]; then
 set -- "$@" --keys-after "${DOS_SESSION_KEYS_AFTER:-SHELL-READY}" --keys "$keys"
fi
if ! timeout -k 5 60 "$driver" "$@" --expect "$marker" --quit-after "$marker" \
 --name run --out "$short" > "$out/stdout" 2> "$out/report"; then exit 2; fi
tr -d '\r' < "$out/run.serial" > "$out/text"
grep -Fx "[$tag] OK=1" "$out/report" >/dev/null || exit 2
grep -F 'timed_out=0' "$out/report" >/dev/null || exit 2
grep -F 'triple_fault=0' "$out/report" >/dev/null || exit 2
if [ "$tag" = harness ]; then
 grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || exit 2
fi
printf '%s\n' "$out/report" > "$out/transport.ok"
