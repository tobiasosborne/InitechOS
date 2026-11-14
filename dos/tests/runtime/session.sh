#!/bin/sh
# Drive the product image externally. Oracles: DOS manual cited in each core,
# pinned captures, supplied bytes and independent mtools on persisted files.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?program directory} out=${4:?output}
mkdir -p "$out"
out=$(CDPATH= cd -- "$out" && pwd)
. "$root/dos/tests/runtime/verdict.sh"
short=$(mktemp -d /tmp/dos-session.XXXXXX)
scenario_out=$out
rmdir "$short"
ln -s "$out" "$short"
trap 'rc=$?; out=$scenario_out; rm -f "$short"; finish_scenario "$rc"' 0
out=$short
export LC_ALL=C TZ=UTC INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
fail() { semantic "$*"; }
img=$out/data.img
cp "$boot" "$out/boot.img"
truncate -s 1474560 "$img"
mformat -i "$img" -f 1440 -v INITECH ::
mcopy -i "$img" "$programs"/*.COM ::
nasm -f bin "$root/dos/tests/runtime/done.asm" -o "$out/DONE.COM"
mcopy -i "$img" "$out/DONE.COM" ::DONE.COM
mmd -i "$img" ::SRC ::SUB ::EMPTY
printf 'apple\r\nzebra\r\n' > "$out/words"
printf 'retained pipe file\r\n' > "$out/sentinel"
touch -t 198707061330.58 "$out/words"
mcopy -m -i "$img" "$out/words" ::SRC/ONE.TXT
mcopy -i "$img" "$out/words" ::WORDS.TXT
mcopy -i "$img" "$out/sentinel" '::PIPE1.$$$'
# The captured hexadecimal pipe namespace is based on the guest clock. Seed
# every whole-second candidate in this run's 13:30/13:31 observation window;
# CREATE NEW must preserve them rather than truncate a stranger's bytes.
: > "$out/foreign-names"
for minute in 30 31;do
 second=0
 while [ "$second" -lt 60 ];do
  name=$(printf '%02X%02X%02X00' 13 "$minute" "$second")
  printf '%s\n' "$name" >> "$out/foreign-names"
  mcopy -i "$img" "$out/sentinel" "::$name"
  second=$((second+1))
 done
done
printf '@ECHO OFF\r\nSHIFT\r\nECHO %%0 >SHIFT.OUT\r\n' > "$out/child"
mcopy -i "$img" "$out/child" ::CHILD.BAT
cat > "$out/autoexec" <<'BAT'
@ECHO OFF
ECHO AUTOEXEC-RAN
COMMAND /C ECHO secondary >SEC.TXT
SET PRIVATE=parent
COMMAND /C SET PRIVATE=child
ECHO %PRIVATE% >ENV.TXT
CALL CHILD argument
IF x==x FOR %%f IN (one) DO IF y==y ECHO IF-BODY
FOR %%f IN (SRC\*.TXT) DO ECHO %%f
ECHO redirect-data >REDIR.TXT
ECHO RESTORED
ECHO pipe-data | MORE >PIPE.OUT
DIR SRC\*.TXT /W >DIR.OUT
COPY SRC\ONE.TXT STAMP.TXT
SORT <WORDS.TXT >SORTED.TXT
MORE <WORDS.TXT >MORE.TXT
FIND "apple" <WORDS.TXT >FIND.TXT
RD EMPTY
CD A:
Z:
EXIT
ECHO PERMANENT-SURVIVED
PROMPT $P$G$$$D
ECHO SESSION-READY
DONE
BAT
if [ "${DOS_INTERACTIVE:-1}" = 1 ]; then sed '$d' "$out/autoexec" > "$out/interactive-bat"; mv "$out/interactive-bat" "$out/autoexec";fi
mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
printf "\n\n\ncd sub\n" | "$root/build/factory/keys" > "$out/keys"
printf "f,o,r,spc,shift-5,f,spc,i,n,spc,shift-9,o,n,e,shift-0,spc,d,o,spc,e,c,h,o,spc,shift-f,shift-o,shift-r,minus,shift-p,shift-r,shift-o,shift-m,shift-p,shift-t,ret," >> "$out/keys"
printf "cd ..\nDone\n" | "$root/build/factory/keys" >> "$out/keys"
keys=ctrl-z,$(tr '\n' ', ' < "$out/keys"); marker=TRANSPORT-DONE
if [ "${DOS_INTERACTIVE:-1}" = 0 ]; then keys=ctrl-z,ret,ret,ret,d,o,n,e,ret;marker=TRANSPORT-DONE;fi
if [ "${DOS_EXIT_CONTROL:-0}" = 1 ]; then
 sed '$d' "$out/autoexec" > "$out/exit-bat"
 printf 'COMMAND /C EXIT\n' >> "$out/exit-bat"
 mcopy -o -i "$img" "$out/exit-bat" ::AUTOEXEC.BAT
 marker=SHELL-EXIT
fi
if ! timeout 90 "$root/build/factory/qemu_harness" --disk "$out/boot.img" --boot-rw --disk2 "$img" --keys-after SHELL-READY \
 --keys "$keys" --expect "$marker" --quit-after "$marker" \
 --rtc-base 1987-07-06T13:30:58 --timeout-ms 15000 --name session --out "$out" \
 > "$out/stdout" 2> "$out/report"; then error 'session did not finish'; fi
transport_pass "$out/report"
tr -d '\r' < "$out/session.serial" > "$out/text"
mdir -i "$img" :: > "$out/volume-after" || error 'cannot read result volume'
for s in AUTOEXEC-RAN IF-BODY 'SRC\ONE.TXT' RESTORED PERMANENT-SURVIVED 'A:\' 'Invalid drive specification'; do
 require "missing [$s]" grep -Fx "$s" "$out/text" >/dev/null;
done
if [ "${DOS_INTERACTIVE:-1}" = 1 ]; then
require 'FOR at prompt' grep -Fx FOR-PROMPT "$out/text" >/dev/null
require 'CWD/literal dollar prompt' grep -F 'A:\SUB>$' "$out/text" >/dev/null
require 'live date prompt' grep -F '07-06-1987' "$out/text" >/dev/null
fi
extract() {
 if mtype -i "$img" "::$1" > "$out/got" 2>/dev/null; then :;else
  rc=$?;[ "$rc" -eq 1 ] || error "mtype failed ($rc)"
  semantic "missing $1"
 fi
 require "$1 bytes" cmp "$out/got" "$2"
}
printf 'secondary\r\n' > "$out/expected"; extract SEC.TXT "$out/expected"
printf 'parent\r\n' > "$out/expected"; extract ENV.TXT "$out/expected"
printf 'argument\r\n' > "$out/expected"; extract SHIFT.OUT "$out/expected"
printf 'redirect-data\r\n' > "$out/expected"; extract REDIR.TXT "$out/expected"
printf 'pipe-data\r\n' > "$out/expected"; extract PIPE.OUT "$out/expected"
printf 'apple\r\n' > "$out/expected"; extract FIND.TXT "$out/expected"
extract MORE.TXT "$out/words"; extract SORTED.TXT "$out/words"; extract STAMP.TXT "$out/words"
extract 'PIPE1.$$$' "$out/sentinel"
while IFS= read -r name;do
 # A guest-clock candidate may differ between runs; the independent property
 # and assertion identity are the same for every preexisting member.
 require 'foreign clock namespace member survives' grep -E "^$name +" "$out/volume-after" >/dev/null
 mtype -i "$img" "::$name" > "$out/got" || error "cannot read foreign $name"
 require 'foreign clock namespace bytes' cmp "$out/got" "$out/sentinel"
done < "$out/foreign-names"
mtype -i "$img" ::DIR.OUT > "$out/dir"
require 'DIR volume identity' grep -F 'Volume in drive A is INITECH' "$out/dir" >/dev/null
require 'DIR operands' grep -F 'ONE      TXT' "$out/dir" >/dev/null
if grep -F 'WORDS    TXT' "$out/dir" >/dev/null; then fail 'DIR ignored mask'; fi
if mdir -i "$img" ::EMPTY >/dev/null 2>&1; then fail 'RD did not remove directory'; fi
require 'COPY timestamp' "$root/build/dos/host/tool_dos_parity_host" --compare "$img" STAMP.TXT "$img" SRC/ONE.TXT > "$out/stamp"
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || fail 'CPU fault'
echo 'GREEN DOS session: product commands, batch control, handles, filters, timestamps and prompt'
