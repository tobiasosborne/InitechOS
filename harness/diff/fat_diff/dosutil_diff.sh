#!/bin/sh
# Expected attribute bytes: mattrib; labels: mlabel + mdir + fsck.fat.
# Printed DOS text: IBM DOS 3.10 Reference 7-18,7-127/128; MS 3.3 pp.33,86/87.
set -eu
mode=$1
runner=$2
tag=$3
b=build/dosutil_$tag
fail() { echo "FAIL: $1"; exit 1; }
sh harness/diff/fat_diff/dosutil_fixture.sh "$b.seed"
cp -f "$b.seed" "$b.img"
cp -f "$b.seed" "$b.ref"
run() { "$runner" "$b.img" "$mode" "$1" "${2-}" > "$b.out"; }
attrs() {
    for p in ONE.TXT TWO.TXT OTHER.BIN SUB/ONE.TXT SUB/DEEP/ONE.TXT OTHER/ONE.TXT; do
        mattrib -i "$1" "::$p"
    done
}
checkattrs() {
    attrs "$b.img" > "$b.actual"
    attrs "$b.ref" > "$b.expected"
    cmp -s "$b.actual" "$b.expected" || fail "$1"
}
checklabel() {
    mlabel -s -i "$b.img" :: > "$b.actual"
    mlabel -s -i "$b.ref" :: > "$b.expected"
    cmp -s "$b.actual" "$b.expected" || fail "$1"
    fsck.fat -n "$b.img" > "$b.fsck" 2>&1 || fail 'L2 BPB/root label agreement'
    mdir -i "$b.img" :: > "$b.mdir"
}
if [ "$mode" = attrib ]; then
    run '+R ONE.TXT'; mattrib -i "$b.ref" +r ::ONE.TXT
    checkattrs 'A1 set R'
    run '-R ONE.TXT'; mattrib -i "$b.ref" -r ::ONE.TXT
    checkattrs 'A1 clear R'
    run '-A ONE.TXT'; mattrib -i "$b.ref" -a ::ONE.TXT
    checkattrs 'A2 clear A'
    run '+A ONE.TXT'; mattrib -i "$b.ref" +a ::ONE.TXT
    checkattrs 'A2 set A'
    run '+R TWO.TXT'; mattrib -i "$b.ref" +r ::TWO.TXT
    checkattrs 'A3 preserve H/S and all nonattribute bytes'
    run '+R A:\*.TXT'; mattrib -i "$b.ref" +r '::/*.TXT'
    checkattrs 'A4 wildcard excludes BIN/subdirectories'
    run '+R -A A:\ONE.TXT /S'
    for p in ONE.TXT SUB/ONE.TXT SUB/DEEP/ONE.TXT OTHER/ONE.TXT; do mattrib -i "$b.ref" +r -a "::$p"; done
    checkattrs 'A5 /S traverses every branch and depth'
    run 'A:\SUB\ONE.TXT'
    printf 'R        A:\\SUB\\ONE.TXT\r\n' > "$b.expected"
    cmp -s "$b.out" "$b.expected" || fail 'A6 display R/A columns and full path'
    DOSUTIL_CWD=SUB "$runner" "$b.img" attrib 'DEEP\..\ONE.TXT' > "$b.out"
    cmp -s "$b.out" "$b.expected" || fail 'A6 relative CWD and dot normalization'
    cp -f "$b.img" "$b.before"
    for arg in '+H ONE.TXT' '+S ONE.TXT' 'SUB' 'ONE.TXT TWO.TXT' '/X ONE.TXT'; do
        if run "$arg"; then fail 'A7 invalid switches/operand/directory accepted'; fi
    done
    cmp -s "$b.img" "$b.before" || fail 'A7 rejected input wrote disk'
    # CHMOD must change no other byte, including timestamps, FATs and payloads.
    cp -f "$b.seed" "$b.img"
    run '+R ONE.TXT'
    "$runner" "$b.img" attrib '-R ONE.TXT' > "$b.out"
    cmp -s "$b.img" "$b.seed" || fail 'A3 CHMOD touched other disk bytes'
else
    run 'a:two words'; mlabel -i "$b.ref" ::'TWO WORDS'
    checklabel 'L1 create/replace uppercase label with spaces'
    run 'A:123456789012345'; mlabel -i "$b.ref" ::12345678901
    checklabel 'L4 first eleven characters'
    run 'A:' '
N
'
    checklabel 'L3 non-Y preserves current label'
    grep -Fq 'Volume in drive A is 12345678901' "$b.out" || fail 'L5 current label dialogue'
    grep -Fq 'Volume label (11 characters, ENTER for none)?' "$b.out" || fail 'L5 input prompt'
    grep -Fq 'Delete current volume label (Y/N)?' "$b.out" || fail 'L5 delete prompt'
    run 'A:' '
y
'
    mlabel -c -i "$b.ref" ::
    checklabel 'L3 Y deletes label'
    run 'A:' 'new label
'
    mlabel -i "$b.ref" ::'NEW LABEL'
    checklabel 'L1 interactive creation'
    cp -f "$b.img" "$b.before"
    for arg in 'A:BAD.LABEL' 'A:BAD?LABEL' 'B:WRONG'; do
        if run "$arg"; then fail 'L6 invalid name/drive accepted'; fi
        case "$arg" in
            B:*) grep -Fq 'Invalid drive specification' "$b.out" || fail 'L6 drive diagnostic';;
            *) grep -Fq 'Invalid characters in volume label' "$b.out" || fail 'L6 label diagnostic';;
        esac
    done
    cmp -s "$b.img" "$b.before" || fail 'L6 invalid name/drive wrote disk'
    # IBM Reference A-75 / MS 3.3 p.348: root exhaustion must be diagnosed.
    mlabel -c -i "$b.img" ::
    i=0
    while mcopy -i "$b.img" "${b}.seed.txt" "::F$i" 2>/dev/null; do i=$((i+1)); done
    cp -f "$b.img" "$b.before"
    if run 'A:FULL'; then fail 'L7 full root accepted label'; fi
    tr -d '\r' < "$b.out" | grep -Fxq 'No room in root directory' || fail 'L7 root-full diagnostic'
    cmp -s "$b.img" "$b.before" || fail 'L7 full-root failure wrote disk'
fi
echo "PASS: $mode host differential (mattrib/mlabel/mdir/fsck.fat; identical source images)"
