set -eu
. kernel/tests/assertions.sh
mkdir -p build/conservation
image=build/conservation/fixture.img
truncate -s 1474560 "$image"
mformat -i "$image" -f 1440 ::
printf 'last good document\n' > build/conservation/good
: > build/conservation/empty
mcopy -i "$image" build/conservation/good ::OLD.BIN
mcopy -i "$image" build/conservation/empty ::ZERO.BIN
mmd -i "$image" ::SUB ::EMPTY ::SUB/EMPTY
mcopy -i "$image" build/conservation/good ::SUB/OLD.BIN
mcopy -i "$image" build/conservation/empty ::SUB/ZERO.BIN
for op in partial append mkdir rmdir; do
 build/test_file_conservation "$image" build/conservation "$op"
done
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py build/conservation/*.img
for file in build/conservation/*-*.img; do
 case "$file" in *-0-0.img) continue;; esac
 # One mtools traversal per image; fresh destinations cannot retain a file
 # omitted by this image. Keep every original byte and directory assertion.
 extracted=$file.files
 mkdir "$extracted"
 mcopy -s -i "$file" ::OLD.BIN ::ZERO.BIN ::SUB ::EMPTY "$extracted/"
 for parent in '' SUB/; do
  assert cmp build/conservation/good "$extracted/${parent}OLD.BIN"
  assert test -f "$extracted/${parent}ZERO.BIN"
  assert test ! -s "$extracted/${parent}ZERO.BIN"
  assert test -d "$extracted/${parent}EMPTY"
 done
done
dd if=/dev/zero bs=1024 count=1 status=none | tr '\000' X > build/conservation/payload
cat build/conservation/good build/conservation/payload > build/conservation/appended
for file in build/conservation/*-0-0.img; do
 case "$file" in *-sub-*) parent=SUB/;; *) parent=;; esac
 case "$file" in
  */partial-*)
   mcopy -o -i "$file" "::${parent}ZERO.BIN" build/conservation/readback
   assert cmp build/conservation/payload build/conservation/readback;;
  */append-*)
   mcopy -o -i "$file" "::${parent}OLD.BIN" build/conservation/readback
   assert cmp build/conservation/appended build/conservation/readback;;
  */mkdir-*) mdir -i "$file" "::${parent}NEWDIR" > /dev/null;;
  */rmdir-*)
   if mdir -i "$file" "::${parent}EMPTY" > /dev/null 2>&1; then
    echo 'FAIL: successful RMDIR retained the name'; exit 1
   fi;;
 esac
done
printf 'PASS: failed extension/MKDIR/RMDIR conserves allocation and original bytes\n'
finish
