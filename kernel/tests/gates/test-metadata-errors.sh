set -eu
mkdir -p build/metadata
truncate -s 1474560 build/metadata/blank.img
mformat -i build/metadata/blank.img -f 1440 ::
printf 'last good document\n' > build/metadata/good
: > build/metadata/empty
cp build/metadata/blank.img build/metadata/readerr.img
mcopy -i build/metadata/readerr.img build/metadata/good ::TARGET.BIN
failed=0
if ! build/test_metadata_errors build/metadata/readerr.img readerr 1; then failed=1; fi
for fault in 0 5; do
 cp build/metadata/blank.img build/metadata/refresh-$fault.img
 mcopy -i build/metadata/refresh-$fault.img build/metadata/empty ::TARGET.BIN
 if ! build/test_metadata_errors build/metadata/refresh-$fault.img refresh "$fault"; then failed=1; fi
 mcopy -o -i build/metadata/refresh-$fault.img ::TARGET.BIN build/metadata/readback
 dd if=/dev/zero bs=512 count=1 status=none | tr '\000' X > build/metadata/written
 cmp build/metadata/written build/metadata/readback
done
test "$failed" -eq 0
