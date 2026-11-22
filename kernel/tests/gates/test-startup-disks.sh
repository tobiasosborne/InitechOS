set -eu
. "$SOURCE_ROOT/kernel/tests/assertions.sh"
w=build/startup-disks
mkdir -p "$w"
truncate -s $(((16384+71)*512)) "$w/c-base.img"
printf 'drive z: file="%s/%s/c-base.img" partition=1\n' "$PWD" "$w" > "$w/mtoolsrc"
export MTOOLSRC="$PWD/$w/mtoolsrc"
mpartition -I z:
mpartition -c -b 63 -l 16384 -T 1 z:
mpartition -a z:
mformat -i "$w/c-base.img@@32256" -T 16384 -h 16 -s 63 -c 8 -H 63 ::
printf 'system C bytes\n' > "$w/c-root"
mcopy -o -i "$w/c-base.img@@32256" "$w/c-root" ::ROOT.BIN
cp "$w/c-base.img" "$w/c-installed.img"
printf 'resident command entry\n' > "$w/command"
mcopy -i "$w/c-installed.img@@32256" "$w/command" ::COMMAND.COM
truncate -s 1474560 "$w/a-base.img"
mformat -i "$w/a-base.img" -f 1440 ::
printf 'legacy A bytes\n' > "$w/a-root"
mcopy -i "$w/a-base.img" "$w/a-root" ::ROOT.BIN

for scenario in c-alone c-with-data a-legacy; do
    master=c-installed
    want=2
    if [ "$scenario" = a-legacy ]; then master=c-base; want=0; fi
    cp "$w/$master.img" "$w/$scenario-c.img"
    cp "$w/a-base.img" "$w/$scenario-a.img"
    slave=$w/$scenario-a.img
    if [ "$scenario" = c-alone ]; then slave=-; fi

    echo "startup scenario: $scenario"
    case_status=0
    build/test_startup_disks "$w/$scenario-c.img" "$slave" "$want" || case_status=$?
    assert cmp "$w/$master.img" "$w/$scenario-c.img"
    assert cmp "$w/a-base.img" "$w/$scenario-a.img"
    [ "$case_status" -eq 0 ] || exit "$case_status"
done

# Keep C intact and mint a FAT16 data partition with one sector per cluster.
d_base=20160
d_size=16384
cp "$w/c-installed.img" "$w/c-fat16.img"
truncate -s "$(((d_base+d_size+8)*512))" "$w/c-fat16.img"
printf 'drive y: file="%s/%s/c-fat16.img" partition=2\n' "$PWD" "$w" >> "$MTOOLSRC"
mpartition -c -b "$d_base" -l "$d_size" -T 4 y:
d="$w/c-fat16.img@@$((d_base*512))"
mformat -i "$d" -T "$d_size" -h 16 -s 63 -c 1 -H "$d_base" ::
printf 'independent imported bytes\n' > "$w/d-root"
mcopy -i "$d" "$w/d-root" ::KEEP.TXT
cp "$w/c-fat16.img" "$w/d-probe-fails.img"

echo "startup scenario: d-probe-fails"
case_status=0
build/test_startup_disks "$w/d-probe-fails.img" - 2 "$d_base" || case_status=$?
assert cmp "$w/c-fat16.img" "$w/d-probe-fails.img"
[ "$case_status" -eq 0 ]

truncate -s "$((d_size*512))" "$w/a-fat16.img"
mformat -i "$w/a-fat16.img" -T "$d_size" -h 16 -s 63 -c 1 ::
mcopy -i "$w/a-fat16.img" "$w/d-root" ::KEEP.TXT
cp "$w/a-fat16.img" "$w/a-probe-fails.img"
cp "$w/c-installed.img" "$w/with-failed-a.img"

echo "startup scenario: a-probe-fails"
case_status=0
build/test_startup_disks "$w/with-failed-a.img" "$w/a-probe-fails.img" 2 fail-a || case_status=$?
assert cmp "$w/c-installed.img" "$w/with-failed-a.img"
assert cmp "$w/a-fat16.img" "$w/a-probe-fails.img"
[ "$case_status" -eq 0 ]

# mtools refuses to create overlapping partitions ("Partition would overlap
# with partition 1"). Mint D: on its own disk, then copy only its table entry
# so that FAT16 D: starts inside the installed FAT12 C:.
o_base=8255
truncate -s "$(((o_base+d_size+8)*512))" "$w/d-entry.img"
printf 'drive x: file="%s/%s/d-entry.img" partition=2\n' "$PWD" "$w" >> "$MTOOLSRC"
mpartition -I x:
mpartition -c -b "$o_base" -l "$d_size" -T 4 x:
mpartition -d x:
cp "$w/c-installed.img" "$w/c-overlap.img"
truncate -s "$(((o_base+d_size+8)*512))" "$w/c-overlap.img"
dd if="$w/d-entry.img" of="$w/c-overlap.img" bs=1 skip=462 seek=462 count=16 conv=notrunc 2>/dev/null
o="$w/c-overlap.img@@$((o_base*512))"
mformat -i "$o" -T "$d_size" -h 16 -s 63 -c 1 -H "$o_base" ::
mcopy -i "$o" "$w/d-root" ::KEEP.TXT
assert sh -c "mtype -i '$w/c-overlap.img@@32256' ::ROOT.BIN | cmp - '$w/c-root'"
assert sh -c "mtype -i '$o' ::KEEP.TXT | cmp - '$w/d-root'"
cp "$w/c-overlap.img" "$w/overlap-run.img"

echo "startup scenario: c-d-overlap"
case_status=0
build/test_startup_disks "$w/overlap-run.img" - 2 overlap || case_status=$?
assert cmp "$w/c-overlap.img" "$w/overlap-run.img"
assert sh -c "mtype -i '$w/overlap-run.img@@$((o_base*512))' ::KEEP.TXT | cmp - '$w/d-root'"
[ "$case_status" -eq 0 ]
