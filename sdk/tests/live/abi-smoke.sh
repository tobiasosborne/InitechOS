#!/bin/sh
# GDB observes the existing production dispatcher after its stack-to-a[] copy.
# Only debug metadata changes: -g adds no loadable bytes to INITDOS.SYS.
set -eu
cd "$(dirname "$0")/../../.."
out=${SDK_ABI_OUT:-build/sdk/abi-smoke}
tenant=${SDK_ABI_TENANT:-build/sdk/abi-live}
mkdir -p "$out"
exec 9>/tmp/initech-sdk-gdb.lock
flock 9
cp build/image/data.img "$out/data.img"
mcopy -o -i "$out/data.img" "$tenant/SDKABI.EXE" ::APPS/SDKCHECK.EXE
. factory/desktop-trace.sh
emulator=qemu; markers=$out/markers; : > "$markers"
trace=$(
 inject "$open_disk";await_marker FINDER-OPEN-VOLUME
 inject "$open_apps";await_marker 'FINDER-OPEN-FOLDER name=APPS'
 # finder_windows.c:fw_key selects by name and reveals an offscreen icon.
 inject 'ks,kd,kk,kctrl-o';await_marker 'FLAIR-DISPATCH app=SDKCHECK'
 inject kctrl-k
)
trace=${trace%,}
offset() { nm "$tenant/sdkabi.base.elf" | awk -v s="$1" '$3==s {print "0x"$1}'; }
rec=$(offset g_rec); phase=$(offset sdk_phase); evbuf=$(offset g_evbuf); proc=$(offset tbx_event_proc)
exit_return=$(offset sdk_exit_return)
test -n "$rec" && test -n "$phase" && test -n "$evbuf" && test -n "$proc"
# Fail visibly if the gate is refactored: do not silently observe the wrong PC.
line=$(awk '/switch [(]ax[)]/{print NR-1;exit}' flair/tbxgate.c)
test -n "$line"
{
 cat <<GDB
set pagination off
set confirm off
set tcp auto-retry on
set tcp connect-timeout 120
set remotetimeout 10
file build/system/system.elf
set architecture i386
target remote :1234
set \$index=0
break flair/tbxgate.c:$line
commands
silent
set \$base=g_slot.plan.load_base
if ax==1
 if a[0]!=\$base+$rec || *(unsigned*)(a[0]+16)!=\$base+$evbuf || *(unsigned*)(a[0]+20)!=\$base+$proc
  echo ABI registration pointer identity FAIL\n
  quit 1
 end
end
if *(unsigned*)(\$base+$phase)==1
GDB
 awk '
 /^[#]/||NF==0 {next}
 {printf " if $index==%d\n  if ax!=0x%s",n,$1;
  for(i=1;i<=$2;i++)printf " || a[%d]!=%s",i-1,($3=="REGISTER"?"$base+"rec:"0x"$(i+2));
  printf "\n   echo ABI argument identity FAIL at row %d\\n\n   quit 1\n  end\n end\n",n;n++}
 END {printf " set $index=$index+1\n printf \"ABI checked %%u\\n\", $index\n"}
 ' rec="$rec" sdk/tests/live/abi.tsv
 cat <<GDB
end
continue
end
break disk_tenant_event
commands
silent
printf "CALLBACK %08x %08x %08x %08x %08x %08x\n", ev->what,ev->message,ev->modifiers,ev->when,(int)ev->where.h,(int)ev->where.v
continue
end
break finish
commands
silent
if g_slot.exit_req && g_slot.exit_rc==79225
 if *(int*)(g_slot.plan.load_base+$exit_return)!=0
  echo ABI exit return identity FAIL\n
  quit 1
 end
end
continue
end
continue
if \$index!=37
 echo ABI missing calls FAIL\n
 quit 1
end
GDB
} > "$out/check.gdb"
INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1 nice -n 10 build/factory/qemu_harness \
 --disk build/image/boot.img --disk2 "$out/data.img" --keys-after FLAIR-LIVE-READY \
 --mouse "$trace" --expect 'TENANT-EXIT rc=79225 via=exit' --quit-after 'TENANT-EXIT rc=79225 via=exit' \
 --gdb --timeout-ms 90000 --out "$out" --name abi > "$out/stdout" 2> "$out/report" &
pid=$!
cleanup() {
 children=$(ps -o pid= --ppid "$pid" || :)
 [ -z "$children" ] || kill $children 2>/dev/null || :
 wait "$pid" 2>/dev/null || :
}
trap cleanup EXIT HUP INT TERM
rc=0
timeout 100 gdb -q -batch -x "$out/check.gdb" > "$out/gdb.log" 2>&1 || rc=$?
if [ "$rc" -ne 0 ]; then cat "$out/gdb.log";exit 1;fi
wait "$pid"
trap - EXIT HUP INT TERM
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report"
# Last pushed event must survive all six flat fields through the SDK callback.
mcopy -o -i "$out/data.img" ::SDKEVT.BIN "$out/event.bin"
awk '/^CALLBACK /{last=$0}END{sub(/^CALLBACK /,"",last);print last}' "$out/gdb.log" > "$out/event.expected"
od -An -v -tx4 "$out/event.bin" | xargs > "$out/event.actual"
cmp "$out/event.expected" "$out/event.actual"
printf 'live ABI: 37 bindings, all argument dwords; registration and six callback fields intact\n'
