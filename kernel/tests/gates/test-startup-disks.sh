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
cp "$w/c-base.img" "$w/c-plain.img"
# The old two-file master (factory/image/mk_boot_system.sh): hidden system
# IO.SYS and INITDOS.SYS, no COMMAND.COM. Only this layout may start from A:.
printf 'loader\n' > "$w/io"
printf 'kernel\n' > "$w/initdos"
mcopy -i "$w/c-base.img@@32256" "$w/io" ::IO.SYS
mcopy -i "$w/c-base.img@@32256" "$w/initdos" ::INITDOS.SYS
mattrib -i "$w/c-base.img@@32256" +h +s +r -a ::IO.SYS ::INITDOS.SYS
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

# stage2 boots C: with up to 16 sectors per FAT. mtools pads this FAT12 C:
# to 13; the system cache, CONFIG/EXEC binding and FAT writes must hold it.
cp "$w/c-base.img" "$w/c-padded.img"
mformat -i "$w/c-padded.img@@32256" -T 16384 -h 16 -s 63 -c 8 -L 13 -H 63 ::
assert test "$(od -An -tu1 -j $((32256 + 22)) -N 1 "$w/c-padded.img" | tr -d ' ')" = 13
mcopy -i "$w/c-padded.img@@32256" "$w/c-root" ::ROOT.BIN
mcopy -i "$w/c-padded.img@@32256" "$w/command" ::COMMAND.COM
cp "$w/c-padded.img" "$w/padded-run.img"
echo "startup scenario: c-padded-fat"
case_status=0
build/test_startup_disks "$w/padded-run.img" - 2 write-new || case_status=$?
[ "$case_status" -eq 0 ]
printf 'padded FAT write\r\n' > "$w/new-want"
assert sh -c "mtype -i '$w/padded-run.img@@32256' ::NEW.TXT | cmp - '$w/new-want'"
assert sh -c "mtype -i '$w/padded-run.img@@32256' ::ROOT.BIN | cmp - '$w/c-root'"
dd if="$w/padded-run.img" of="$w/padded-c.raw" bs=512 skip=63 count=16384 2>/dev/null
assert fsck.fat -n "$w/padded-c.raw"

# A read error on the installed C: must not hand startup to data A:.
c_vbr=$(od -An -tu1 -j32256 -N24 "$w/c-installed.img" | tr -s ' \n' ' ')
set -- $c_vbr
c_root=$((63 + ${15} + ${16}*256 + ${17} * (${23} + ${24}*256)))
# A clean C: with neither COMMAND.COM nor the old system files is no system.
for fault in c-root-fault:$c_root c-vbr-fault:63 legacy-root-fault:$c_root c-unidentified:0; do
    scenario=${fault%%:*}
    master=c-installed
    if [ "$scenario" = legacy-root-fault ]; then master=c-base; fi
    if [ "$scenario" = c-unidentified ]; then master=c-plain; fi
    cp "$w/$master.img" "$w/$scenario-c.img"
    cp "$w/a-base.img" "$w/$scenario-a.img"
    echo "startup scenario: $scenario (LBA ${fault#*:})"
    case_status=0
    build/test_startup_disks "$w/$scenario-c.img" "$w/$scenario-a.img" 2 "c-fault=${fault#*:}" ||
        case_status=$?
    assert cmp "$w/$master.img" "$w/$scenario-c.img"
    assert cmp "$w/a-base.img" "$w/$scenario-a.img"
    [ "$case_status" -eq 0 ]
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
cp "$w/c-fat16.img" "$w/d-healthy.img"

echo "startup scenario: d-fat16"
case_status=0
build/test_startup_disks "$w/d-healthy.img" - 2 "fat16-d=$d_base" || case_status=$?
assert cmp "$w/c-fat16.img" "$w/d-healthy.img"
[ "$case_status" -eq 0 ]

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
