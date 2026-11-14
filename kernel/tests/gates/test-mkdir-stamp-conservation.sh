set -eu
# Only a completed inequality is semantic evidence. cmp I/O/setup errors stay
# errors under the shared mutation classifier (c9619fba).
same_bytes() {
 result=0
 cmp "$1" "$2" || result=$?
 case $result in
  0) :;;
  1) printf 'FAIL MKDIR invariant: %s\nINITECH_CHECKS 1 failures=1\n' "$3"; exit 1;;
  *) exit 2;;
 esac
}
truncate -s 1474560 build/fixture.img
mformat -i build/fixture.img -f 1440 ::
printf 'last good document\n' > build/good
mcopy -i build/fixture.img build/good ::TARGET.BIN
mmd -i build/fixture.img ::SUB
mcopy -i build/fixture.img build/good ::SUB/KEEP.BIN
mdir -b -i build/fixture.img ::/ ::SUB/ | sort > build/expected.list
for parent in root sub; do
 mode=mkdir-stamp
 test "$parent" = root || mode=mkdir-stamp-sub
 cp build/fixture.img build/control-$parent.img
 build/test_metadata_errors build/control-$parent.img "$mode" 0 > build/control.log
 grep -qx 'MKDIR_RESULT success' build/control.log
 "$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_mkdir_stamp.py build/control-$parent.img "$parent"
 writes=$(sed -n 's/^WRITE_COUNT //p' build/control.log)
 test "$writes" -gt 0
 for persistence in before after; do
  fault=1
  while [ "$fault" -le "$writes" ]; do
   image=build/$parent-$persistence-$fault.img
   cp build/fixture.img "$image"
   build/test_metadata_errors "$image" "$mode" "$fault" "$persistence" > build/result.log
   grep -qx 'MKDIR_RESULT failed' build/result.log
   mdir -b -i "$image" ::/ ::SUB/ | sort > build/actual.list
   same_bytes build/actual.list build/expected.list 'failed creation preserves directory names'
   for original in TARGET.BIN SUB/KEEP.BIN; do
    mcopy -o -i "$image" "::$original" build/readback
    same_bytes build/good build/readback 'failed creation preserves original payload'
   done
   fault=$((fault+1))
  done
 done
done
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py build/root-*.img build/sub-*.img build/control-*.img
