#!/bin/sh
# Thin mtools/dosfstools driver; factory logic and rendering stay in C.
# Ref: PRD 6.1, ADR-0003 Appendix A; MS-DOS 3.3 pp.127-129,324;
# IBM DOS 3.3/4.0 Operation Training p.D-6 (pre-4.0 TREE sample).
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1
mode=$1; runner=$2; work=$3
mkdir -p "$work"
seed=$work/seed.img
fail() { echo "XCOPY/TREE RED: $*"; exit 1; }
run() { "$runner" --run "$work/art.img" "$@" > "$work/output" 2> "$work/run.log"; }
fresh() { cp -f "$seed" "$work/art.img"; cp -f "$seed" "$work/ref.img"; }
compare() {
    # FAT namespace membership ignores allocation/order; TREE below grades
    # directory order separately against the independent mdir inventory.
    mdir -/ -b -i "$work/art.img" ::DST | sort > "$work/art.list"
    mdir -/ -b -i "$work/ref.img" ::DST | sort > "$work/ref.list"
    cmp -s "$work/art.list" "$work/ref.list" || fail "$1 directory membership vs mcopy -s"
    while IFS= read -r name; do
        case $name in */) continue;; esac
        mtype -i "$work/art.img" "$name" > "$work/art.bytes"
        mtype -i "$work/ref.img" "$name" > "$work/ref.bytes"
        cmp -s "$work/art.bytes" "$work/ref.bytes" || fail "$1 bytes $name vs mcopy"
        mattrib -i "$work/art.img" "$name" > "$work/art.attr"
        mattrib -i "$work/ref.img" "$name" > "$work/ref.attr"
        cmp -s "$work/art.attr" "$work/ref.attr" || fail "$1 attributes $name vs mcopy"
        mdir -i "$work/art.img" "$name" | sed -n '5p' > "$work/art.date"
        mdir -i "$work/ref.img" "$name" | sed -n '5p' > "$work/ref.date"
        cmp -s "$work/art.date" "$work/ref.date" || fail "$1 date/time $name vs mcopy -m"
    done < "$work/ref.list"
    fsck.fat -n "$work/art.img" > "$work/fsck.log" || fail "$1 dosfstools filesystem consistency"
}
dd if=/dev/zero of="$seed" bs=512 count=2880 status=none
mformat -i "$seed" -f 1440 -N 0x12345678 -v TESTVOL ::
mmd -i "$seed" ::SRC ::SRC/BRANCH ::SRC/BRANCH/DEEP ::SRC/EMPTY ::SRC/HOLLOW ::SRC/HOLLOW/EMPTY2 ::DST
printf 'first file\r\n' > "$work/old.txt"
printf 'second file\032binary\000tail' > "$work/plain.bin"
dd if=/dev/zero of="$work/new.bin" bs=512 count=13 status=none
touch -t 198001010000 "$work/old.txt" "$work/plain.bin"
touch -t 198807151200 "$work/new.bin"
mcopy -m -i "$seed" "$work/old.txt" ::SRC/ROOT.TXT
mcopy -m -i "$seed" "$work/plain.bin" ::SRC/PLAIN.BIN
mcopy -m -i "$seed" "$work/old.txt" ::SRC/BRANCH/LEAF.TXT
mcopy -m -i "$seed" "$work/new.bin" ::SRC/BRANCH/DEEP/DEEP.BIN
mattrib -a -i "$seed" ::SRC/PLAIN.BIN

if [ "$mode" = mint ]; then exit 0; fi
if [ "$mode" = host ]; then
    fresh
    run XCOPY 'SRC DST /S /E /V' || fail '/S /E /V execution'
    mcopy -s -m -i "$work/ref.img" '::SRC/*' ::DST
    compare '/S /E /V'
    tr -d '\r' < "$work/output" | grep -qx '4 File(s) copied' || fail 'copied count'
    fresh
    run XCOPY 'SRC DST /S' || fail '/S execution'
    mcopy -s -m -i "$work/ref.img" '::SRC/*' ::DST
    mrd -i "$work/ref.img" ::DST/EMPTY ::DST/HOLLOW/EMPTY2 ::DST/HOLLOW
    compare '/S excludes empty trees'
    fresh
    run XCOPY 'SRC\*.TXT DST /S' || fail 'wildcard execution'
    mcopy -m -i "$work/ref.img" ::SRC/ROOT.TXT ::DST
    mmd -i "$work/ref.img" ::DST/BRANCH
    mcopy -m -i "$work/ref.img" ::SRC/BRANCH/LEAF.TXT ::DST/BRANCH
    compare 'wildcards recur with /S'
    fresh
    run XCOPY 'SRC DST' || fail 'nonrecursive execution'
    mcopy -m -i "$work/ref.img" ::SRC/ROOT.TXT ::SRC/PLAIN.BIN ::DST
    compare 'default excludes subdirectories'
    for flag in A M; do
        fresh
        run XCOPY "SRC DST /S /$flag" || fail "/$flag execution"
        mcopy -s -m -i "$work/ref.img" '::SRC/*' ::DST
        mdel -i "$work/ref.img" ::DST/PLAIN.BIN
        mrd -i "$work/ref.img" ::DST/EMPTY ::DST/HOLLOW/EMPTY2 ::DST/HOLLOW
        compare "/$flag archive selection"
        if [ "$flag" = M ]; then
            mattrib -a -i "$work/ref.img" ::SRC/ROOT.TXT ::SRC/BRANCH/LEAF.TXT ::SRC/BRANCH/DEEP/DEEP.BIN
        fi
        for p in SRC/ROOT.TXT SRC/PLAIN.BIN SRC/BRANCH/LEAF.TXT SRC/BRANCH/DEEP/DEEP.BIN; do
            mattrib -i "$work/art.img" "::$p" > "$work/art.attr"
            mattrib -i "$work/ref.img" "::$p" > "$work/ref.attr"
            cmp -s "$work/art.attr" "$work/ref.attr" || fail "/$flag source archive $p"
        done
    done
    fresh
    run XCOPY 'SRC DST /S /D:07-15-1988' || fail '/D execution'
    mmd -i "$work/ref.img" ::DST/BRANCH ::DST/BRANCH/DEEP
    mcopy -m -i "$work/ref.img" ::SRC/BRANCH/DEEP/DEEP.BIN ::DST/BRANCH/DEEP
    compare '/D inclusive date'
    fresh
    run XCOPY 'SRC DST /W /P' ' yn' || fail '/W /P execution'
    grep -q 'Press any key when ready to start' "$work/output" || fail '/W wait prompt'
    grep -q 'copying files' "$work/output" || fail '/W second line'
    grep -q '(Y/N?)' "$work/output" || fail '/P confirmation prompt'
    mcopy -m -i "$work/ref.img" ::SRC/ROOT.TXT ::DST
    compare '/P decline leaves no target'
    for choice in F D; do
        fresh
        run XCOPY 'SRC\ROOT.TXT NEW' "$choice" || fail "target $choice execution"
        grep -q 'Does NEW specify a file name or directory name on the target (F = file, D = directory)?' "$work/output" || fail 'F/D prompt'
        mdir -b -i "$work/art.img" :: > "$work/target.list"
        if [ "$choice" = F ]; then entry=::/NEW; else entry=::/NEW/; fi
        grep -qxF "$entry" "$work/target.list" || fail "target $choice file/directory kind"
        if [ "$choice" = F ]; then p=::NEW; else p=::NEW/ROOT.TXT; fi
        mtype -i "$work/art.img" "$p" > "$work/art.bytes" || fail "target $choice path"
        cmp -s "$work/art.bytes" "$work/old.txt" || fail "target $choice bytes"
    done
    fresh
    if run XCOPY 'SRC\ROOT.TXT DST /V' '' corrupt; then fail '/V failed to detect read-back corruption'; fi
    grep -q 'exit=5 verify_reads=[1-9]' "$work/run.log" || fail '/V error exit / actual read-back'
    fresh
    if run XCOPY 'SRC DST /E'; then fail '/E accepted without /S'; fi
    if run XCOPY 'SRC SRC\BRANCH /S'; then fail 'cyclic copy accepted'; fi
    grep -q 'Cannot perform a cyclic copy' "$work/output" || fail 'cyclic diagnostic'
    for flag in '' /F; do
        fresh
        run TREE "SRC $flag" || fail "TREE $flag execution"
        mdir -/ -b -i "$seed" ::SRC > "$work/mdir.list"
        files=0; [ "$flag" != /F ] || files=1
        "$runner" --tree-golden "$work/mdir.list" /SRC "$files" "$work/expected"
        cmp -s "$work/output" "$work/expected" || fail "TREE $flag manual layout / mdir inventory"
    done
    fresh
    mmd -i "$work/art.img" ::ONLY ::ONLY/CHILD
    mcopy -m -i "$work/art.img" "$work/old.txt" ::ONLY/CHILD/ONE.TXT
    run XCOPY 'ONLY DST /S' || fail '/S source with files only below root'
    mmd -i "$work/ref.img" ::DST/CHILD
    mcopy -m -i "$work/ref.img" "$work/old.txt" ::DST/CHILD/ONE.TXT
    compare '/S source with files only below root'
    fresh
    run XCOPY 'SRC\PL?IN.BIN DST' || fail '? wildcard execution'
    mcopy -m -i "$work/ref.img" ::SRC/PLAIN.BIN ::DST
    compare '? wildcard binary copy'
    fresh
    mcopy -m -i "$work/art.img" "$work/old.txt" ::SRC/HIDDEN.TXT
    mcopy -m -i "$work/art.img" "$work/old.txt" ::SRC/SYSTEM.TXT
    mattrib +h -i "$work/art.img" ::SRC/HIDDEN.TXT
    mattrib +s -i "$work/art.img" ::SRC/SYSTEM.TXT
    run XCOPY 'SRC DST' || fail 'hidden/system execution'
    mcopy -m -i "$work/ref.img" ::SRC/ROOT.TXT ::SRC/PLAIN.BIN ::DST
    compare 'hidden/system excluded'
    fresh
    mmd -i "$work/art.img" ::WORK
    ZUVO_CWD=WORK run XCOPY '\SRC\ROOT.TXT' || fail 'omitted target execution'
    mtype -i "$work/art.img" ::WORK/ROOT.TXT > "$work/art.bytes" 2>/dev/null || fail 'omitted target directory'
    cmp -s "$work/art.bytes" "$work/old.txt" || fail 'omitted target bytes'
    fresh
    if run XCOPY 'SRC\ROOT.TXT SRC\.\ROOT.TXT'; then fail 'same-file alias accepted'; fi
    mtype -i "$work/art.img" ::SRC/ROOT.TXT > "$work/art.bytes"
    cmp -s "$work/art.bytes" "$work/old.txt" || fail 'same-file alias destroyed source'
    for tail in 'SRC DST /D:02-30-1988' 'SRC DST /D:07-15-1979' 'SRC DST /D:01-01-2080' 'SRC DST /Z' ''; do
        if run XCOPY "$tail"; then fail "invalid syntax accepted: $tail"; fi
        grep -q 'exit=4' "$work/run.log" || fail 'invalid syntax exit code'
    done
    for date in 02-29-00 01-01-2079 07/15/88; do
        fresh
        if run XCOPY "SRC DST /D:$date"; then fail 'future date unexpectedly copied old root files'; fi
        grep -q 'exit=1' "$work/run.log" || fail '/D DOS 3.3 date syntax/year window'
    done
    fresh
    if run XCOPY 'SRC DST /W' "$(printf '\003')"; then fail '/W Ctrl-C accepted'; fi
    grep -q 'exit=2' "$work/run.log" || fail '/W Ctrl-C exit'
    mlabel -i "$work/art.img" '::TEST VOLUME'
    run TREE 'SRC /F' || fail 'volume label execution'
    tr -d '\r' < "$work/output" | grep -qx 'DIRECTORY PATH LISTING FOR VOLUME TEST VOLUME' || fail 'volume label spaces preserved'
    echo 'XCOPY/TREE GREEN: mcopy FAT differential; /S /E /A /M /D /P /V /W F/D; TREE mdir/manual rendering; fsck.fat'
    exit 0
fi

# QEMU end-to-end: same seed, real external EXEC and keyboard injected after
# /W's prompt. AUTOEXEC writes TREE into files for exact byte grading.
boot=$4; harness=$5; xcopy=$6; tree=$7
cp -f "$seed" "$work/art.img"
if [ "$mode" = cancel ]; then
    printf '@echo off\r\nPATH=A:\\DOS\r\nXCOPY SRC DST /W\r\nif errorlevel 2 echo ZUVO-CANCEL2\r\nif errorlevel 3 echo ZUVO-BAD-CANCEL\r\necho ZUVO-DONE\r\n' > "$work/autoexec.bat"
    trace='ctrl-c,ret,e,x,i,t,ret'
else
    printf '@echo off\r\nPATH=A:\\DOS\r\nXCOPY SRC DST /S /E /V /W /P\r\nTREE SRC /F > TREEF.TXT\r\nTREE SRC > TREE.TXT\r\necho ZUVO-DONE\r\n' > "$work/autoexec.bat"
    trace='spc,y,y,y,y,ret,e,x,i,t,ret'
fi
mmd -i "$work/art.img" ::DOS
mcopy -i "$work/art.img" "$xcopy" ::DOS/XCOPY.COM
mcopy -i "$work/art.img" "$tree" ::DOS/TREE.COM
mcopy -i "$work/art.img" "$work/autoexec.bat" ::AUTOEXEC.BAT
name=$(basename "$work")
"$harness" --disk "$boot" --disk2 "$work/art.img" --name "$name" --out "$work" --timeout-ms 30000 \
    --keys "$trace" --keys-after 'copying files' \
    --keys-after-ms 20000 --quit-after SHELL-DONE > "$work/harness.log" 2> "$work/report" || true
grep -q 'triple_fault=0' "$work/report" || fail 'QEMU triple fault/report'
if [ "$mode" = cancel ] && grep -q '^INT23-BREAK' "$work/$name.serial" &&
   grep -q 'Terminate batch job (Y/N)?' "$work/$name.serial" &&
   ! grep -q '^ZUVO-CANCEL2' "$work/$name.serial"; then
    # A /W that reads the key with break checking (AH=08h) leaves ^C to the
    # kernel's INT 23h abort; inside a batch real DOS 3.3 then asks
    # "Terminate batch job (Y/N)?" (MS-DOS 3.3 Users Guide, Reference PAUSE,
    # printed p.149 / PDF p.234; MSG-DOS-0063; EXEC-child path 119b454e),
    # which reads the rest of the typed keys, so no exit 2 and no ZUVO-DONE.
    fail "QEMU /W Ctrl-C ignored by XCOPY (INT 23h abort reached the batch prompt 'Terminate batch job (Y/N)?'; no exit 2)"
fi
grep -q '^ZUVO-DONE' "$work/$name.serial" || fail 'QEMU external EXEC completion'
grep -q '^SHELL-EXIT' "$work/$name.serial" || fail 'QEMU post-program shell exit'
if [ "$mode" = cancel ]; then
    grep -q '^ZUVO-CANCEL2' "$work/$name.serial" || fail 'QEMU /W Ctrl-C exit must be 2'
    if grep -q '^ZUVO-BAD-CANCEL' "$work/$name.serial"; then fail 'QEMU /W Ctrl-C exit exceeds 2'; fi
    mdir -/ -b -i "$work/art.img" ::DST > "$work/art.list"
    [ ! -s "$work/art.list" ] || fail 'QEMU /W Ctrl-C copied files'
    echo 'XCOPY/TREE CANCEL GREEN: injected Ctrl-C; exit=2; destination unchanged; SHELL-EXIT'
    exit 0
fi
cp -f "$seed" "$work/ref.img"
mcopy -s -m -i "$work/ref.img" '::SRC/*' ::DST
compare 'QEMU /S /E /V /W /P'
mdir -/ -b -i "$seed" ::SRC > "$work/mdir.list"
for files in 0 1; do
    "$runner" --tree-golden "$work/mdir.list" /SRC "$files" "$work/expected"
    p=::TREE.TXT; [ "$files" = 0 ] || p=::TREEF.TXT
    mtype -i "$work/art.img" "$p" > "$work/output" || fail 'QEMU TREE output missing'
    cmp -s "$work/output" "$work/expected" || fail 'QEMU TREE manual layout / mdir inventory'
done
echo 'XCOPY/TREE EMU GREEN: external flat EXEC; injected /W /P keys; FAT differential; TREE byte-exact; SHELL-EXIT'
