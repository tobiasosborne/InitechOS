#!/bin/sh
# Manual oracle, independent literals: MS-DOS 3.3 Reference pp.41-42,
# 101,132,142,144-149,152,159. mtools acts on an identical FAT fixture.
set -eu
export INITECH_QMP_STDIO=1 LC_ALL=C TZ=UTC
mode=$1
boot=${2:-unused}
tag=${3:-green}
out=build/batch2_${mode}_run_${tag}
mkdir -p "$out"
fail() { printf 'batch2-%s RED: %s\n' "$mode" "$*" >&2; exit 1; }
img=$out/data.img
rm -f "$out/run.ppm"
dd if=/dev/zero of="$img" bs=512 count=2880 status=none
mformat -i "$img" -f 1440 -N 102 ::
mmd -i "$img" ::DIR ::DIR/SUB
printf 'one\r\n' > "$out/one"
for leaf in A.TXT B.TXT SKIP.BIN; do mcopy -i "$img" "$out/one" "::DIR/SUB/$leaf"; done
cp -f "$img" "$out/reference.img"
if [ "$mode" = host ]; then
    "${BATCH2_HOST:-build/batch2_host}" "$img" > "$out/host.log" 2>&1 || { cat "$out/host.log"; fail 'host assertions'; }
    printf 'root\r\n' > "$out/root"
    mcopy -i "$out/reference.img" "$out/root" ::ROOT.TXT
    for leaf in ROOT.TXT DIR/SUB/A.TXT DIR/SUB/B.TXT; do
        mcopy -o -i "$img" "::$leaf" "$out/actual"
        mcopy -o -i "$out/reference.img" "::$leaf" "$out/expected"
        cmp "$out/actual" "$out/expected" || fail 'CWD bytes differ from mcopy'
    done
    mdir -b -i "$out/reference.img" '::DIR/SUB/*.TXT' | sed 's,^.*/,MATCH ,' | sort > "$out/expected.names"
    sed -n '/^MATCH /p' "$out/host.log" | sort > "$out/actual.names"
    diff -u "$out/expected.names" "$out/actual.names" || fail 'find selection differs from mdir'
    cat "$out/host.log"
    fsck.fat -n "$img" > "$out/fsck" 2>&1 || fail 'fsck damage'
    printf 'PASS batch2-host: identical-image mcopy/mdir differential; fsck.fat clean\n'
    exit 0
fi
keys='e,x,i,t,ret'
case "$mode" in
body)
    cat > "$out/child" <<'BAT'
@echo CHILD-OK
BAT
    mcopy -i "$img" "$out/child" ::CHILD.BAT
    printf '@echo off\r\nfor %%%%v in (one) do shift\r\necho SHIFT-%%0-%%1\r\n' > "$out/shifter"
    mcopy -i "$img" "$out/shifter" ::SHIFTER.BAT
    cat > "$out/loop" <<'BAT'
@echo off
:again
if "%1"=="" goto done
echo ARG-%1
if x==x shift
goto again
:done
if x==x call child
if x==x call shifter first second
for %%v in (one) do call child
for %%v in (one) do shift
for %%v in (one two) do goto jumped
echo BAD-FOR-GOTO
:jumped
if x==x if x==x echo NESTED-OK
if x==x for %%v in (one) do echo IF-FOR-%%v
for %%v in (one) do if x==x echo FOR-IF-%%v
if x==x pause
for %%v in (one) do pause
if x==x set TEST=INTERNAL
if x==x echo %TEST%
for %%v in (one) do cd \dir\sub
if x==x cd
echo BODY-DONE
BAT
    mcopy -i "$img" "$out/loop" ::LOOP.BAT
    printf '@echo off\r\ncall loop first second\r\necho TRACE-DONE\r\n' > "$out/autoexec"
    keys='spc,spc,e,x,i,t,ret'
    ;;
cwd)
    cat > "$out/autoexec" <<'BAT'
@echo off
cd \dir\sub
cd
cd ..
cd
cd \dir\sub
cd \
cd
echo root>root.txt
cd \dir\sub
cd
cd ..
cd ..
cd ..
cd
echo TRACE-DONE
BAT
    ;;
prompt)
    printf '@echo off\r\nprompt $$MID-$D$G\r\n@echo on\r\nver\r\n@echo off\r\n' > "$out/autoexec"
    keys='d,a,t,e,spc,0,2,minus,2,9,minus,1,9,8,8,ret,e,c,h,o,spc,p,r,o,m,p,t,o,k,ret,e,x,i,t,ret'
    ;;
for)
    cat > "$out/autoexec" <<'BAT'
@echo off
cd \dir\sub
for %%f in (*.txt) do echo BATCH-%%f
for %%f in (\dir\sub\*.txt) do echo QUAL-%%f
for %%f in (no*.zzz) do echo BAD-NOMATCH
for %%f in (literal missing) do echo LITERAL-%%f
for %%f in (a.txt) do dir *.bin
for %%f in (*.txt) do for %%g in (*.bin) do echo NEST-%%f-%%g
BAT
    keys='f,o,r,spc,shift-5,f,spc,i,n,spc,shift-9,shift-8,dot,t,x,t,shift-0,spc,d,o,spc,e,c,h,o,spc,p,r,o,m,p,t,minus,shift-5,f,ret,e,x,i,t,ret'
    ;;
console)
    printf '@echo off\r\ncls\r\n' > "$out/autoexec"
    keys='c,o,p,y,spc,c,o,n,spc,t,dot,t,x,t,ret,h,e,l,l,o,ret,ctrl-z,ret,e,c,h,o,spc,b,a,d,ctrl-c,c,o,p,y,spc,c,o,n,spc,u,dot,t,x,t,ret,m,o,r,e,ret,ctrl-c,e,c,h,o,spc,c,o,n,s,o,l,e,o,k,ret,e,x,i,t,ret'
    ;;
*) fail 'unknown mode' ;;
esac
mcopy -i "$img" "$out/autoexec" ::AUTOEXEC.BAT
capture=
[ "$mode" != console ] || capture="--screendump --screendump-after SHELL-DONE"
build/qemu_harness --disk "$boot" --disk2 "$img" --out "$out" --name run \
    --rtc-base 1987-07-06T13:30:58 --rtc-deterministic --timeout-ms 25000 \
    --keys-after SHELL-READY --keys "$keys" --quit-after SHELL-DONE \
    $capture 2> "$out/report" || true
[ -s "$out/run.serial" ] || fail 'no serial'
! grep -q triple_fault=1 "$out/report" || fail 'triple fault'
tr -d '\r' < "$out/run.serial" > "$out/text"
# Check body failure before completion, so the original infinite GOTO loop is
# distinguished from a boot/injection failure.
if [ "$mode" = body ]; then
    ! grep -qx 'Bad command or file name' "$out/text" || fail 'IF/FOR body bypassed batch dispatcher'
fi
grep -q quit_after_found=1 "$out/report" || fail 'trace incomplete'
case "$mode" in
body)
    for s in ARG-first ARG-second NESTED-OK IF-FOR-one FOR-IF-one SHIFT-first-second INTERNAL BODY-DONE TRACE-DONE; do
        grep -qx "$s" "$out/text" || fail "missing $s"
    done
    [ "$(grep -c '^CHILD-OK$' "$out/text")" = 2 ] || fail 'CALL continuation'
    [ "$(grep -c '^Strike a key when ready . . .$' "$out/text")" = 2 ] || fail 'PAUSE body/manual message'
    ! grep -q BAD-FOR-GOTO "$out/text" || fail 'FOR GOTO did not leave loop'
    grep -qxF 'A:\DIR\SUB' "$out/text" || fail 'ordinary internal CD body'
    ;;
cwd)
    sed -n '/^A:\\/p' "$out/text" | sed '/>/d' > "$out/cwd.actual"
    printf '%s\n' 'A:\DIR\SUB' 'A:\DIR' 'A:\' 'A:\DIR\SUB' 'A:\' > "$out/cwd.expected"
    diff -u "$out/cwd.expected" "$out/cwd.actual" || fail 'CD root/parent/absolute/getcwd'
    printf 'root\r\n' > "$out/expected"
    mcopy -o -i "$img" ::ROOT.TXT "$out/actual" || fail 'relative write after CD root'
    cmp "$out/actual" "$out/expected" || fail 'root write bytes'
    ;;
prompt)
    grep -qE '^\$MID-[0-9]{2}-[0-9]{2}-[0-9]{4}>date ' "$out/text" || fail 'literal dollar truncated prompt'
    grep -qF '$MID-07-06-1987>date 02-29-1988' "$out/text" || fail 'prompt date did not read clock'
    grep -qF '$MID-02-29-1988>echo promptok' "$out/text" || fail 'prompt did not follow date seam'
    grep -qF '$MID-07-06-1987>ver' "$out/text" || fail 'batch echo prompt date/dollar'
    ;;
for)
    mdir -b -i "$out/reference.img" '::DIR/SUB/*.TXT' | sed 's,^.*/,,' | sort > "$out/expected.names"
    sed -n 's/^BATCH-//p' "$out/text" | sort > "$out/batch.names"
    sed -n 's/^prompt-//p' "$out/text" | sort > "$out/prompt.names"
    diff -u "$out/expected.names" "$out/batch.names" || fail 'batch wildcard set differs from mdir'
    diff -u "$out/expected.names" "$out/prompt.names" || fail 'interactive FOR differs from mdir'
    for s in 'QUAL-\dir\sub\A.TXT' 'QUAL-\dir\sub\B.TXT'; do grep -qxF "$s" "$out/text" || fail 'qualified FOR prefix'; done
    ! grep -q BAD-NOMATCH "$out/text" || fail 'unmatched wildcard executed'
    grep -qx LITERAL-literal "$out/text" && grep -qx LITERAL-missing "$out/text" || fail 'literal set'
    for s in NEST-A.TXT-SKIP.BIN NEST-B.TXT-SKIP.BIN; do grep -qx "$s" "$out/text" || fail 'nested FOR search state'; done
    ;;
console)
    build/console_text "${VGABIOS:-/usr/share/seabios/vgabios-stdvga.bin}" "$out/run.ppm" > "$out/console" || fail 'console contains an undecodable control glyph'
    grep -qxF '^Z' "$out/console" || fail 'Ctrl-Z not painted as ^Z'
    grep -qxF 'A:\>echo bad^C' "$out/console" || fail 'Ctrl-C did not cancel input with ^C'
    grep -qxF '^C' "$out/console" || fail 'COPY CON abort caret'
    ! grep -q INT23-BREAK "$out/console" || fail 'INT23 marker leaked to console'
    [ "$(grep -c INT23-BREAK "$out/text")" = 2 ] || fail 'serial break marker lost'
    printf 'hello\r\n' > "$out/expected"
    mcopy -o -i "$img" ::T.TXT "$out/actual"
    cmp "$out/actual" "$out/expected" || fail 'COPY CON EOF bytes'
    printf 'more\r\n' > "$out/expected"
    mcopy -o -i "$img" ::U.TXT "$out/actual"
    cmp "$out/actual" "$out/expected" || fail 'aborted COPY bytes'
    grep -qx consoleok "$out/console" || fail 'abort recovery'
    ;;
esac
fsck.fat -n "$img" > "$out/fsck" 2>&1 || fail 'fsck damage'
printf 'PASS batch2-%s: manual transcript + injected QEMU + FAT oracle; fsck.fat clean\n' "$mode"
