set -eu
for tool in mformat mcopy mmd mdel; do command -v "$tool" >/dev/null; done
mkdir -p build/conservation
image=build/conservation/fixture.img
dd if=/dev/zero of="$image" bs=512 count=2880 status=none
mformat -i "$image" -f 1440 ::
# Allocate, remove, then replace alternating files so OLD.BIN is fragmented.
dd if=/dev/zero of=build/conservation/one.bin bs=512 count=1 status=none
dd if=kernel/tests/fixtures/chain.txt of=build/conservation/old.bin bs=1 count=1537 status=none
# Ensure several clusters even if the text fixture changes.
cat kernel/tests/fixtures/chain.txt >> build/conservation/old.bin
for n in 1 2 3 4 5 6 7 8; do mcopy -i "$image" build/conservation/one.bin ::F$n; done
for n in 1 3 5 7; do mdel -i "$image" ::F$n; done
mcopy -i "$image" build/conservation/old.bin ::OLD.BIN
mmd -i "$image" ::SUB
mcopy -i "$image" build/conservation/old.bin ::SUB/OLD.BIN
failed=0
for op in create unlink; do
 if ! build/test_file_conservation "$image" build/conservation "$op"; then failed=1; fi
done
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py build/conservation/*.img
for file in build/conservation/create-*.img build/conservation/unlink-*.img; do
 case "$file" in *-0-0.img) continue;; esac
 # All reported errors leave both originals under their previous names.
 for name in OLD.BIN SUB/OLD.BIN; do
  mcopy -o -i "$file" "::$name" build/conservation/readback.bin
  cmp build/conservation/old.bin build/conservation/readback.bin
 done
 cmp "$image" "$file"
done
for file in build/conservation/*-0-0.img; do
 case "$file" in *-sub-*) name=SUB/OLD.BIN;; *) name=OLD.BIN;; esac
 case "$file" in
  */create-*)
   mcopy -o -i "$file" "::$name" build/conservation/readback.bin
   test ! -s build/conservation/readback.bin;;
  */unlink-*)
   if mcopy -o -i "$file" "::$name" build/conservation/readback.bin 2>/dev/null; then
    echo 'FAIL: successful unlink retained the target'; exit 1
   fi;;
 esac
done
printf 'PASS mtools: original bytes survive every reported metadata error\n'
test "$failed" -eq 0
