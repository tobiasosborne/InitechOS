#!/bin/sh
# Thin mtools/dosfstools driver; factory logic and rendering stay in C.
# Ref: PRD 6.1, ADR-0003 Appendix A; MS-DOS 3.3 pp.127-129,324;
# IBM DOS 3.3/4.0 Operation Training p.D-6 (pre-4.0 TREE sample).
set -eu
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1
mode=$1; runner=$2; work=$3
mkdir -p "$work"
seed=$work/seed.img
fail() { echo "FAIL XCOPY/TREE: $*"; echo "INITECH_CHECKS 1 failures=1"; exit 1; }
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
    # initech-wz16r secondary: a copy that fails verification is a failed
    # output; it may not remain in place stamped like a good copy.
    if mtype -i "$work/art.img" ::DST/ROOT.TXT > "$work/art.bytes" 2>/dev/null; then fail '/V failure left the bad copy in place'; fi
    mdir -a -b -i "$work/art.img" ::DST > "$work/private.list"
    if grep -E 'XCOPY[0-9A-F]{3}\.(TMP|BAK)$' "$work/private.list"; then fail '/V failure leaked stage'; fi
    fresh
    if run XCOPY 'SRC DST /E' ; then fail '/E accepted without /S'; fi
    if run XCOPY 'SRC SRC\BRANCH /S'; then fail 'cyclic copy accepted'; fi
    grep -q 'Cannot perform a cyclic copy' "$work/output" || fail 'cyclic diagnostic'
    for flag in '' /F; do
        fresh
        # Genuine DOS 3.30 TREEPATH/TREEF return zero with this refusal.
        run TREE "SRC $flag" || fail "TREE directory operand status"
        printf 'Invalid parameter\r\n' > "$work/expected"
        cmp -s "$work/output" "$work/expected" || fail "TREE directory operand refusal"
        run TREE "A: $flag" || fail "TREE $flag execution"
        mdir -/ -b -i "$seed" :: > "$work/mdir.list"
        files=0; [ "$flag" != /F ] || files=1
        "$runner" --tree-golden "$work/mdir.list" '' "$files" "$work/expected"
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
    # The same default exclusion applies while descending directories.
    # Source: MS-DOS 3.3 XCOPY reference (pp.127-129,324 above). The reference
    # image contains only the ordinary source tree; mcopy supplies its bytes,
    # membership, attributes and timestamps independently of XCOPY's filters.
    fresh
    mmd -i "$work/art.img" ::SRC/HIDDEN ::SRC/SYSTEM
    mcopy -m -i "$work/art.img" "$work/old.txt" ::SRC/HIDDEN/LEAF.TXT
    mcopy -m -i "$work/art.img" "$work/old.txt" ::SRC/SYSTEM/LEAF.TXT
    mattrib +h -i "$work/art.img" ::SRC/HIDDEN
    mattrib +s -i "$work/art.img" ::SRC/SYSTEM
    run XCOPY 'SRC DST /S /E' || fail 'hidden/system directory execution'
    mcopy -s -m -i "$work/ref.img" '::SRC/*' ::DST
    compare 'hidden/system directories excluded recursively'
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
    # self-alias capture: 8.3 leaves and resolved directory aliases.
    for target in 'SRC\LONGNAMEX.TXT' 'SRC\.\LONGNAMEX.TXT' 'LONGDIRX\LONGNAMEX.TXT'; do
        fresh
        # Fragmented allocation makes truncation corrupt the source reliably.
        dd if=/dev/zero of="$work/hole" bs=3000 count=1 status=none
        mcopy -i "$work/art.img" "$work/hole" ::HOLE.BIN
        dd if=/dev/zero bs=4893 count=1 status=none | tr "\000" "L" > "$work/long"
        mcopy -i "$work/art.img" "$work/long" ::SRC/LONGNAME.TXT
        mdel -i "$work/art.img" ::HOLE.BIN
        if [ "$target" = 'LONGDIRX\LONGNAMEX.TXT' ]; then
            mmd -i "$work/art.img" ::LONGDIRX
            mcopy -i "$work/art.img" "$work/long" ::LONGDIRX/LONGNAME.TXT
            target='LONGDIRXX\LONGNAMEX.TXT'
            source='LONGDIRX\LONGNAME.TXT'
        else source='SRC\LONGNAME.TXT'; fi
        cp "$work/art.img" "$work/before.img"
        if run XCOPY "$source $target"; then fail '8.3 self-copy accepted'; fi
        grep -q 'File cannot be copied onto itself' "$work/output" || fail '8.3 self-copy diagnostic'
        mtype -i "$work/art.img" "::$(printf '%s' "$source" | tr '\\' '/')" > "$work/art.bytes" || fail '8.3 source deleted'
        cmp "$work/long" "$work/art.bytes" || fail '8.3 source changed'
        cmp "$work/before.img" "$work/art.img" || fail '8.3 self-copy wrote the volume'
    done
    # Failure may unlink a new entry only, never a pre-existing destination.
    # initech-wz16r: retention means the BYTES. The destination seed differs
    # from the source bytes, so an overwrite (or a truncation to zero) cannot
    # masquerade as retention; mtools supplies the independent readback.
    fresh
    printf 'PRE-EXISTING DESTINATION - DO NOT LOSE\r\n' > "$work/previous.txt"
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/ROOT.TXT
    if XCOPY_FAIL_READ=1 run XCOPY 'SRC\ROOT.TXT DST'; then fail 'read error ignored'; fi
    mdir -b -i "$work/art.img" ::DST/ROOT.TXT > "$work/entry" || fail 'failure deleted an existing entry'
    mtype -i "$work/art.img" ::DST/ROOT.TXT > "$work/art.bytes"
    cmp "$work/previous.txt" "$work/art.bytes" || fail 'read failure destroyed pre-existing destination bytes'
    mtype -i "$work/art.img" ::SRC/ROOT.TXT > "$work/art.bytes"
    cmp "$work/old.txt" "$work/art.bytes" || fail 'read failure damaged source'
    # initech-q2w4b: the same rule when the read failure lands past the
    # first 2 KiB chunk of a larger source. No DOS 3.30 XCOPY capture
    # exists (initech-0e5gu); the safe rule is that a source read failure
    # never destroys the previous destination.
    fresh
    dd if=/dev/zero bs=5000 count=1 status=none | tr '\000' 'S' > "$work/bigsrc.bin"
    mcopy -i "$work/art.img" "$work/bigsrc.bin" ::SRC/BIGSRC.BIN
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/BIGSRC.BIN
    if XCOPY_FAIL_READ_AT=3 run XCOPY 'SRC\BIGSRC.BIN DST'; then fail 'mid-file read error ignored'; fi
    mtype -i "$work/art.img" ::DST/BIGSRC.BIN > "$work/art.bytes" || fail 'mid-file read failure deleted pre-existing destination'
    cmp "$work/previous.txt" "$work/art.bytes" || fail 'mid-file read failure destroyed pre-existing destination bytes'
    mtype -i "$work/art.img" ::SRC/BIGSRC.BIN > "$work/art.bytes"
    cmp "$work/bigsrc.bin" "$work/art.bytes" || fail 'mid-file read failure damaged source'
    # gze1f: a transient failure on the old second source pass must not
    # destroy the old destination. Retained bytes can finish the copy;
    # otherwise the previous destination must survive byte for byte.
    for read_at in 6 7; do
        fresh
        mcopy -i "$work/art.img" "$work/bigsrc.bin" ::SRC/BIGSRC.BIN
        mcopy -i "$work/art.img" "$work/previous.txt" ::DST/BIGSRC.BIN
        if XCOPY_FAIL_READ_AT=$read_at run XCOPY 'SRC\BIGSRC.BIN DST'; then
            expected=$work/bigsrc.bin
        else expected=$work/previous.txt; fi
        mtype -i "$work/art.img" ::DST/BIGSRC.BIN > "$work/art.bytes" || fail "read $read_at deleted previous destination"
        cmp "$expected" "$work/art.bytes" || fail "read $read_at corrupted destination"
    done
    # Genuine DOS 3.30 XCOPY /V copies this 200000-byte source and the
    # following file (tests/compat33-large). Neither a chunk workspace nor
    # the old 128 KiB buffer is a file-size ceiling.
    for size in 200000; do
        fresh
        dd if=/dev/zero bs=$size count=1 status=none | tr '\000' 'B' > "$work/large.bin"
        mcopy -i "$work/art.img" "$work/large.bin" ::SRC/LARGE.BIN
        mcopy -i "$work/art.img" "$work/previous.txt" ::SRC/ZZZ.TXT
        mcopy -i "$work/art.img" "$work/previous.txt" ::DST/LARGE.BIN
        run XCOPY 'SRC DST /V' || fail "large directory copy $size execution"
        mtype -i "$work/art.img" ::DST/LARGE.BIN > "$work/art.bytes" || fail "large $size target missing"
        cmp "$work/large.bin" "$work/art.bytes" || fail "large $size destination bytes"
        mtype -i "$work/art.img" ::DST/ZZZ.TXT > "$work/art.bytes" || fail 'file following large source was skipped'
        cmp "$work/previous.txt" "$work/art.bytes" || fail 'file following large source bytes'
    done
    # /V re-reads the source while the old name still holds the old bytes.
    for read_at in 6 7 70 100; do
        fresh
        mcopy -i "$work/art.img" "$work/large.bin" ::SRC/LARGE.BIN
        mcopy -i "$work/art.img" "$work/previous.txt" ::DST/LARGE.BIN
        if XCOPY_FAIL_READ_AT=$read_at run XCOPY 'SRC\LARGE.BIN DST /V'; then fail "read $read_at /V failure ignored"; fi
        mtype -i "$work/art.img" ::DST/LARGE.BIN > "$work/art.bytes" || fail "read $read_at /V deleted target"
        cmp "$work/previous.txt" "$work/art.bytes" || fail "read $read_at /V changed target"
        mdir -a -b -i "$work/art.img" ::DST > "$work/private.list"
        if grep -E 'XCOPY[0-9A-F]{3}\.(TMP|BAK)$' "$work/private.list"; then fail "read $read_at leaked private file"; fi
    done
    # A foreign candidate is never acquired, truncated or deleted. A source
    # read-only bit and full timestamp survive publication by rename.
    fresh
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/XCOPY000.TMP
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/XCOPY000.BAK
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/ROOT.TXT
    mattrib +r -i "$work/art.img" ::SRC/ROOT.TXT
    run XCOPY 'SRC\ROOT.TXT DST /V' || fail 'stage collision execution'
    for ext in TMP BAK; do
        mtype -i "$work/art.img" "::DST/XCOPY000.$ext" > "$work/art.bytes" || fail "foreign $ext deleted"
        cmp "$work/previous.txt" "$work/art.bytes" || fail "foreign $ext changed"
    done
    mtype -i "$work/art.img" ::DST/ROOT.TXT > "$work/art.bytes"
    cmp "$work/old.txt" "$work/art.bytes" || fail 'stage collision destination bytes'
    mattrib -i "$work/art.img" ::DST/ROOT.TXT > "$work/readonly"
    grep -Eq '^[^:]*R[^:]*::' "$work/readonly" || fail 'published source read-only bit lost'
    mdir -a -b -i "$work/art.img" ::DST > "$work/private.list"
    if grep -E 'XCOPY00[1-9A-F]\.(TMP|BAK)$' "$work/private.list"; then fail 'owned stage or backup survived success'; fi
    # An absent long destination can alias a private 8.3 candidate.
    fresh
    run XCOPY 'SRC\ROOT.TXT DST\XCOPY000.TMPX' F || fail 'private-name destination alias execution'
    mtype -i "$work/art.img" ::DST/XCOPY000.TMP > "$work/art.bytes"
    cmp "$work/old.txt" "$work/art.bytes" || fail 'private-name destination alias bytes'
    # Publication failures restore the old name. If the device also refuses
    # rollback, retain the only old bytes under the disclosed backup name.
    for rename_at in 1 2 persistent; do
        fresh
        mcopy -i "$work/art.img" "$work/previous.txt" ::DST/ROOT.TXT
        if [ "$rename_at" = persistent ]; then
            if XCOPY_FAIL_RENAME_FROM=2 run XCOPY 'SRC\ROOT.TXT DST'; then fail 'rename failure ignored'; fi
            old=::DST/XCOPY000.BAK
            grep -F 'A:\DST\XCOPY000.BAK' "$work/output" >/dev/null || fail 'rollback recovery path not disclosed'
        else
            if XCOPY_FAIL_RENAME_AT=$rename_at run XCOPY 'SRC\ROOT.TXT DST'; then fail "rename $rename_at failure ignored"; fi
            old=::DST/ROOT.TXT
        fi
        mtype -i "$work/art.img" "$old" > "$work/art.bytes" || fail "rename $rename_at lost old bytes"
        cmp "$work/previous.txt" "$work/art.bytes" || fail "rename $rename_at changed old bytes"
        mdir -a -b -i "$work/art.img" ::DST > "$work/private.list"
        if grep -E '\.TMP$' "$work/private.list"; then fail "rename $rename_at leaked stage"; fi
        fsck.fat -n "$work/art.img" > "$work/fsck.log" || fail "rename $rename_at filesystem consistency"
    done
    fresh
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/ROOT.TXT
    mattrib +r -i "$work/art.img" ::DST/ROOT.TXT
    cp "$work/art.img" "$work/before.img"
    if run XCOPY 'SRC\ROOT.TXT DST'; then fail 'read-only destination replaced'; fi
    cmp "$work/before.img" "$work/art.img" || fail 'read-only destination altered volume'
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
    run TREE 'A: /F' || fail 'volume label execution'
    tr -d '\r' < "$work/output" | grep -qx 'DIRECTORY PATH LISTING FOR VOLUME TEST VOLUME' || fail 'volume label spaces preserved'
    # Disk full mid-copy (initech-w33id): Appendix F assigns
    # "Insufficient disk space" to Xcopy; the captured COPY disk-full
    # ruling removes the failed output. No partial, source-stamped
    # destination may survive.
    fresh
    dd if=/dev/zero bs=1000 count=20 status=none | tr "\000" "B" > "$work/big.bin"
    touch -t 198807151200 "$work/big.bin"
    mcopy -m -i "$work/art.img" "$work/big.bin" ::SRC/BIG.BIN
    free=$(mdir -i "$work/art.img" :: | sed -n 's/^ *\([0-9 ]*\) bytes free$/\1/p' | tr -d ' ')
    dd if=/dev/zero of="$work/fill.bin" bs=1 count=$((free-10000)) status=none
    mcopy -i "$work/art.img" "$work/fill.bin" ::FILL.BIN
    if run XCOPY 'SRC\BIG.BIN DST'; then fail 'disk full accepted'; fi
    grep -q 'exit=5' "$work/run.log" || fail 'disk full exit'
    tr -d '\r' < "$work/output" | grep -qx 'Insufficient disk space' || fail 'disk full message'
    if tr -d '\r' < "$work/output" | grep -qx 'Access denied'; then fail 'disk full reported as Access denied'; fi
    tr -d '\r' < "$work/output" | grep -qx '0 File(s) copied' || fail 'disk full count'
    if mtype -i "$work/art.img" ::DST/BIG.BIN > "$work/art.bytes" 2>/dev/null; then fail 'disk full left a partial destination'; fi
    mdir -a -b -i "$work/art.img" ::DST > "$work/private.list"
    if grep -E 'XCOPY[0-9A-F]{3}\.(TMP|BAK)$' "$work/private.list"; then fail 'disk full leaked partial stage'; fi
    fsck.fat -n "$work/art.img" > "$work/fsck.log" || fail 'disk full fsck'
    # The staged XCOPY transaction preserves an older destination on full
    # disk. The genuine COPY disk-full deletion ruling remains in COPY.
    fresh
    mcopy -m -i "$work/art.img" "$work/big.bin" ::SRC/BIG.BIN
    mcopy -i "$work/art.img" "$work/previous.txt" ::DST/BIG.BIN
    free=$(mdir -i "$work/art.img" :: | sed -n 's/^ *\([0-9 ]*\) bytes free$/\1/p' | tr -d ' ')
    dd if=/dev/zero of="$work/fill.bin" bs=1 count=$((free-10000)) status=none
    mcopy -i "$work/art.img" "$work/fill.bin" ::FILL.BIN
    if run XCOPY 'SRC\BIG.BIN DST'; then fail 'disk full over a pre-existing destination accepted'; fi
    grep -q 'exit=5' "$work/run.log" || fail 'disk full pre-existing exit'
    mtype -i "$work/art.img" ::DST/BIG.BIN > "$work/art.bytes" || fail 'disk full deleted previous destination'
    cmp "$work/previous.txt" "$work/art.bytes" || fail 'disk full changed previous destination'
    mtype -i "$work/art.img" ::SRC/BIG.BIN > "$work/art.bytes"
    cmp "$work/big.bin" "$work/art.bytes" || fail 'disk full damaged source'
    echo 'XCOPY GREEN: disk full reports Insufficient disk space and leaves no partial destination'
    echo 'INITECH_CHECKS 1 failures=0'
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
    printf '@echo off\r\nPATH=A:\\DOS\r\nXCOPY SRC DST /S /E /V /W /P\r\nTREE A: /F > TREEF.TXT\r\nTREE A: > TREE.TXT\r\necho ZUVO-DONE\r\n' > "$work/autoexec.bat"
    # Pre-create the two redirected files so both listings see the same root.
    : > "$work/TREEF.TXT"; : > "$work/TREE.TXT"
    mcopy -i "$work/art.img" "$work/TREEF.TXT" "$work/TREE.TXT" ::
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
mdir -/ -b -i "$work/art.img" :: > "$work/mdir.list"
for files in 0 1; do
    "$runner" --tree-golden "$work/mdir.list" '' "$files" "$work/expected"
    p=::TREE.TXT; [ "$files" = 0 ] || p=::TREEF.TXT
    mtype -i "$work/art.img" "$p" > "$work/output" || fail 'QEMU TREE output missing'
    cmp -s "$work/output" "$work/expected" || fail 'QEMU TREE manual layout / mdir inventory'
done
echo 'XCOPY/TREE EMU GREEN: external flat EXEC; injected /W /P keys; FAT differential; TREE byte-exact; SHELL-EXIT'
