#!/bin/sh
# Drive the product image externally. Oracles: DOS manual cited in each core,
# pinned captures, supplied bytes and independent mtools on persisted files.
set -eu
root=${1:?root} boot=${2:?boot} programs=${3:?program directory} out=${4:?output}
mkdir -p "$out"
out=$(CDPATH= cd -- "$out" && pwd)
short=$(mktemp -d /tmp/dos-session.XXXXXX)
rmdir "$short"
ln -s "$out" "$short"
trap 'rm -f "$short"' 0 HUP INT TERM
out=$short
export LC_ALL=C TZ=UTC INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
fail() { echo "RED DOS session: $*" >&2; exit 1; }
img=$out/data.img
cp "$boot" "$out/boot.img"
truncate -s 1474560 "$img"
mformat -i "$img" -f 1440 -v INITECH ::
mcopy -i "$img" "$programs"/*.COM ::
mmd -i "$img" ::SRC ::SUB ::EMPTY
printf 'apple\r\nzebra\r\n' > "$out/words"
printf 'retained pipe file\r\n' > "$out/sentinel"
touch -t 198707061330.58 "$out/words"
mcopy -m -i "$img" "$out/words" ::SRC/ONE.TXT
mcopy -i "$img" "$out/words" ::WORDS.TXT
mcopy -i "$img" "$out/sentinel" '::PIPE1.$$$'
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
BAT
mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
printf "\n\ncd sub\n" | "$root/build/factory/keys" > "$out/keys"
printf "f,o,r,spc,shift-5,f,spc,i,n,spc,shift-9,o,n,e,shift-0,spc,d,o,spc,e,c,h,o,spc,shift-f,shift-o,shift-r,minus,shift-p,shift-r,shift-o,shift-m,shift-p,shift-t,ret," >> "$out/keys"
printf "cd ..\necho SESSION-DONE\n" | "$root/build/factory/keys" >> "$out/keys"
keys=$(tr '\n' ', ' < "$out/keys"); marker=session-done
if [ "${DOS_INTERACTIVE:-1}" = 0 ]; then keys=;marker=SESSION-READY;fi
if ! timeout 90 "$root/build/factory/qemu_harness" --disk "$out/boot.img" --boot-rw --disk2 "$img" --keys-after SHELL-READY \
 --keys "$keys" --expect "$marker" --quit-after "$marker" \
 --rtc-base 1987-07-06T13:30:58 --timeout-ms 15000 --name session --out "$out" \
 > "$out/stdout" 2> "$out/report"; then fail 'session did not finish'; fi
tr -d '\r' < "$out/session.serial" > "$out/text"
for s in AUTOEXEC-RAN IF-BODY 'SRC\ONE.TXT' RESTORED PERMANENT-SURVIVED 'A:\' 'Invalid drive specification'; do
 grep -Fx "$s" "$out/text" >/dev/null || fail "missing [$s]";
done
if [ "${DOS_INTERACTIVE:-1}" = 1 ]; then
grep -Fx FOR-PROMPT "$out/text" >/dev/null || fail 'FOR at prompt'
grep -F 'A:\SUB>$' "$out/text" >/dev/null || fail 'CWD/literal dollar prompt'
grep -F '07-06-1987' "$out/text" >/dev/null || fail 'live date prompt'
fi
extract() { mtype -i "$img" "::$1" > "$out/got" 2>/dev/null || fail "missing $1"; cmp "$out/got" "$2" || fail "$1 bytes"; }
printf 'secondary\r\n' > "$out/expected"; extract SEC.TXT "$out/expected"
printf 'parent\r\n' > "$out/expected"; extract ENV.TXT "$out/expected"
printf 'argument\r\n' > "$out/expected"; extract SHIFT.OUT "$out/expected"
printf 'redirect-data\r\n' > "$out/expected"; extract REDIR.TXT "$out/expected"
printf 'pipe-data\r\n' > "$out/expected"; extract PIPE.OUT "$out/expected"
printf 'apple\r\n' > "$out/expected"; extract FIND.TXT "$out/expected"
extract MORE.TXT "$out/words"; extract SORTED.TXT "$out/words"; extract STAMP.TXT "$out/words"
extract 'PIPE1.$$$' "$out/sentinel"
mtype -i "$img" ::DIR.OUT > "$out/dir"
grep -F 'Volume in drive A is INITECH' "$out/dir" >/dev/null || fail 'DIR volume identity'
grep -F 'ONE.TXT' "$out/dir" >/dev/null || fail 'DIR operands'
if grep -F 'WORDS.TXT' "$out/dir" >/dev/null; then fail 'DIR ignored mask'; fi
if mdir -i "$img" ::EMPTY >/dev/null 2>&1; then fail 'RD did not remove directory'; fi
"$root/build/dos/host/tool_dos_parity_host" --compare "$img" STAMP.TXT "$img" SRC/ONE.TXT > "$out/stamp" || fail 'COPY timestamp'
grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$out/report" >/dev/null || fail 'CPU fault'
echo 'GREEN DOS session: product commands, batch control, handles, filters, timestamps and prompt'
