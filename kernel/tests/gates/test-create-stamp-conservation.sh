set -eu
truncate -s 1474560 build/fixture.img
mformat -i build/fixture.img -f 1440 ::
printf 'last good document\n' > build/good
mcopy -i build/fixture.img build/good ::TARGET.BIN
for fault in 0 1 2 3 4; do
 cp build/fixture.img build/stamp-$fault.img
 build/test_metadata_errors build/stamp-$fault.img creat-stamp "$fault" > build/stamp-$fault.log
 mcopy -o -i build/stamp-$fault.img ::TARGET.BIN build/readback
 if grep -qx 'CREATE_RESULT failed' build/stamp-$fault.log; then
  cmp build/good build/readback
  cmp build/fixture.img build/stamp-$fault.img
 else
  grep -qx 'CREATE_RESULT success' build/stamp-$fault.log
  test ! -s build/readback
 fi
done
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py build/stamp-*.img
