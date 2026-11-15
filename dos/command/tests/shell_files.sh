#!/bin/sh
# Independent file bytes + mcopy/mren inventories on identical FAT seeds.
# MS-DOS 3.3 Reference pp.50-52,104; Guide p.17. No artifact computes expected.
set -efu
check_suite=shell-files
. dos/tests/assertions.sh
export LC_ALL=C TZ=UTC INITECH_QMP_STDIO=1
mode=$1 runner=$2 tag=${3:-green}
out=build/shell-files-$mode-$tag
mkdir -p "$out"
dd if=/dev/zero of="$out/data.img" bs=512 count=2880 status=none
mformat -i "$out/data.img" -f 1440 -N 102 ::
printf 'alpha\032tail\r\n' > "$out/a"
printf 'beta\r\n' > "$out/b"
printf 'gamma\032ignored' > "$out/c"
printf 'other\r\n' > "$out/other"
for d in SRC DST REN SUB SUB/CHILD; do mmd -i "$out/data.img" "::$d"; done
for n in ONE TWO; do mcopy -i "$out/data.img" "$out/a" "::SRC/$n.TXT"; done
mcopy -i "$out/data.img" "$out/b" ::B.TXT
mcopy -i "$out/data.img" "$out/c" ::C.TXT
mcopy -i "$out/data.img" "$out/a" ::APP.TXT
mcopy -i "$out/data.img" "$out/other" ::REN/A1.TXT
mcopy -i "$out/data.img" "$out/b" ::REN/A2.TXT
mcopy -i "$out/data.img" "$out/other" ::REN/KEEP.BIN
mcopy -i "$out/data.img" "$out/other" ::REN/CLASH.DAT
cat > "$out/commands" <<'CMDS'
copy src\one.txt src\two.txt extra.txt
copy /b b.txt+src\one.txt src\one.txt
copy src\one.txt /a ascii.txt /b
copy /b src\one.txt bin.txt
copy src\one.txt text.txt /a
copy /b src\one.txt+b.txt joined.bin /b /v
copy src\one.txt + b.txt joined.txt
copy src\one.txt + b.txt + c.txt three.txt /b
copy app.txt + b.txt
copy src\*.txt all.txt
copy src\*.txt dst
copy /b src\*.txt dst\*.bin
ren ren\*.txt *.bak
ren ren\*.bak B?.dat
ren ren\B?.dat CLASH.DAT
md bad?name
md con
md aux.txt
md sub\lpt1
md legal
CMDS
{ printf '@echo off\n'; cat "$out/commands"; printf 'echo SHELL-FILES-DONE\n'; } > "$out/autoexec"
mcopy -i "$out/data.img" "$out/autoexec" ::AUTOEXEC.BAT
cp -f "$out/data.img" "$out/reference.img"
mcopy -i "$out/reference.img" '::SRC/*.TXT' "$out"
for n in ONE TWO; do
    mcopy -i "$out/reference.img" "$out/$n.TXT" "::DST/$n.TXT"
    mcopy -i "$out/reference.img" "$out/$n.TXT" "::DST/$n.BIN"
done
# mren has no DOS destination-wildcard mapping; these independent manual
# p.104 substitutions are applied to each reference leaf explicitly.
for n in 1 2; do
    mren -i "$out/reference.img" "::REN/A$n.TXT" "::REN/A$n.BAK"
    mren -i "$out/reference.img" "::REN/A$n.BAK" "::REN/B$n.DAT"
done
cp -f "$out/reference.img" "$out/collision-before.img"
for n in 1 2; do
    if mren -D s -i "$out/reference.img" "::REN/B$n.DAT" ::REN/CLASH.DAT; then fail 'reference collision unexpectedly succeeded'; fi
done
check 'mren collision changed image' cmp -s "$out/reference.img" "$out/collision-before.img"
if [ "$mode" = host ]; then
    cp -f "$out/data.img" "$out/namespace.img"
    "$runner" "$out/namespace.img" --namespace > "$out/namespace-log" 2>&1 || { rc=$?; cat "$out/namespace-log"; exit "$rc"; }
    check 'rejected raw names changed image' cmp -s "$out/namespace.img" "$out/data.img"
    cp -f "$out/data.img" "$out/refused.img"
    printf 'copy app.txt + b.txt\n' > "$out/refused-command"
    SHELL_REFUSE_TRUNCATE=1 "$runner" "$out/refused.img" "$out/refused-command" > "$out/refused-text" 2> "$out/refused-report" || { rc=$?; cat "$out/refused-report"; exit "$rc"; }
    check 'COPY ignored truncation error' grep -q 'Access denied' "$out/refused-text"
    check 'COPY wrote after truncation error' cmp -s "$out/refused.img" "$out/data.img"
    "$runner" "$out/data.img" "$out/commands" > "$out/text" 2> "$out/report" || { rc=$?; cat "$out/report"; exit "$rc"; }
else
    build/qemu_harness --disk "$runner" --disk2 "$out/data.img" --out "$out" --name run \
      --timeout-ms 25000 --keys-after SHELL-READY --keys 'e,x,i,t,ret' --quit-after SHELL-DONE \
      --rtc-base 1987-07-06T13:30:58 --rtc-deterministic 2> "$out/report" || true
    check 'injected EXIT did not finish' grep -q quit_after_found=1 "$out/report"
    check 'injected trace incomplete' grep -q keys_sent=5 "$out/report"
    ! grep -q triple_fault=1 "$out/report" || fail 'triple fault'
    tr -d '\r' < "$out/run.serial" > "$out/text"
    check 'batch did not finish' grep -qx SHELL-FILES-DONE "$out/text"
fi
extract() { read_product "$out/data.img" "$1" "$out/got"; check "$1 bytes" cmp -s "$out/got" "$2"; }
printf alpha > "$out/ascii"
printf 'alpha\032tail\r\n\032' > "$out/text-eof"
cat "$out/a" "$out/b" > "$out/joined-bin"
printf 'alphabeta\r\n\032' > "$out/joined-text"
extract ASCII.TXT "$out/ascii"
extract BIN.TXT "$out/a"
extract TEXT.TXT "$out/text-eof"
extract JOINED.BIN "$out/joined-bin"
extract JOINED.TXT "$out/joined-text"
extract APP.TXT "$out/joined-text"
printf 'alphabeta\r\ngamma' > "$out/three"
extract THREE.TXT "$out/three"
printf 'alphaalpha' > "$out/all"
extract ALL.TXT "$out/all"
extract SRC/ONE.TXT "$out/a"
if [ "$mode" = host ]; then
    printf 'copy /b src\\one.txt verify.bin /v\n' > "$out/verify-command"
    SHELL_CORRUPT_VERIFY=1 "$runner" "$out/data.img" "$out/verify-command" > "$out/verify-text" 2> "$out/verify-report" || { rc=$?; cat "$out/verify-report"; exit "$rc"; }
    check 'COPY /V accepted corrupt read-back' grep -q 'Bad command or file name' "$out/verify-text"
    check 'COPY /V did not read recorded data' grep -q 'verify_reads=[1-9]' "$out/verify-report"
    if grep -q 'copied' "$out/verify-text"; then fail 'COPY /V printed success after corruption'; fi
fi
mdir -b -i "$out/data.img" ::EXTRA.TXT >/dev/null 2>&1 && fail 'surplus COPY created destination'
for dir in SRC DST REN; do
    mdir -b -i "$out/data.img" "::$dir" > "$out/raw-names" || error "list $dir"
    sort "$out/raw-names" > "$out/actual"
    mdir -b -i "$out/reference.img" "::$dir" > "$out/raw-names" || error "list reference $dir"
    sort "$out/raw-names" > "$out/expected"
    check "$dir membership != mtools" cmp -s "$out/actual" "$out/expected"
done
check 'REN collision diagnostic differs from manual p.325' grep -q 'Duplicate file name' "$out/text"
for n in DST/ONE.TXT DST/TWO.TXT DST/ONE.BIN DST/TWO.BIN; do extract "$n" "$out/a"; done
for n in REN/B1.DAT REN/B2.DAT REN/KEEP.BIN REN/CLASH.DAT; do
    mtype -i "$out/reference.img" "::$n" > "$out/want"
    extract "$n" "$out/want"
done
for n in 'BAD?NAME' CON AUX.TXT SUB/LPT1; do
    mdir -b -i "$out/data.img" "::$n" >/dev/null 2>&1 && fail 'invalid created name persisted'
done
mdir -b -i "$out/data.img" ::LEGAL >/dev/null 2>&1 || fail 'legal MD rejected'
check_log 'dosfstools damage' "$out/fsck" fsck.fat -n "$out/data.img"
echo "GREEN test-shell-files-$mode: COPY modes/lists/wildcards, REN patterns/collisions, MD namespace; mtools and fsck.fat"
complete
