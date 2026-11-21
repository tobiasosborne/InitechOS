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
