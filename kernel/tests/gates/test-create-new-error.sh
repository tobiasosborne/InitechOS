set -eu
truncate -s 1474560 build/fixture.img
mformat -i build/fixture.img -f 1440 ::
printf 'last good document\n' > build/good
mcopy -i build/fixture.img build/good ::TARGET.BIN
for fault in 0 1; do
 cp build/fixture.img build/new-$fault.img
 build/test_metadata_errors build/new-$fault.img new "$fault"
 cmp build/fixture.img build/new-$fault.img
 mcopy -o -i build/new-$fault.img ::TARGET.BIN build/readback
 cmp build/good build/readback
done
