set -eu
. kernel/tests/assertions.sh
fixture=kernel/tests/fixtures/fragmented-append33
for kind in tail middle blocked; do
    image=build/$kind.img
    sh "$fixture/mint.sh" "$image" "$kind"
    mcopy -i "$image" ::TARGET.BIN build/"$kind".expected
    printf X >> build/"$kind".expected
    producer=0
    build/test_fat12_write_partial "$image" --append || producer=$?
    [ "$producer" -lt 2 ] || exit 2
    mcopy -i "$image" ::TARGET.BIN build/"$kind".actual
    assert cmp build/"$kind".expected build/"$kind".actual
    [ "$producer" = 0 ] || exit 2
done
mcopy -i build/blocked.img ::GUARD.BIN build/guard.actual
printf 'neighboring file\n' > build/guard.expected
assert cmp build/guard.expected build/guard.actual
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py build/tail.img build/middle.img build/blocked.img
finish
