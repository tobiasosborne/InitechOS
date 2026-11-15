set -eu
mkdir -p build/mkdir-rollback
w=build/mkdir-rollback
: > "$w/empty"
for mode in flush space; do
 image="$w/$mode.img"
 truncate -s 1474560 "$image"
 mformat -i "$image" -f 1440 ::
 mmd -i "$image" ::SUB
 for n in 01 02 03 04 05 06 07 08 09 10 11 12 13 14; do
  mcopy -i "$image" "$w/empty" "::SUB/FILL$n"
 done
 if [ "$mode" = space ]; then
  free=$(mdir -i "$image" :: | awk '/bytes free/ {gsub(/[^0-9]/, ""); print}')
  test "$free" -gt 512
  dd if=/dev/zero of="$w/keep-$mode" bs=512 count=$((free/512-1)) status=none
 else
  printf 'Original named bytes survive a failed MKDIR.\n' > "$w/keep-$mode"
 fi
 mcopy -i "$image" "$w/keep-$mode" ::KEEP.BIN
 # Both real fault paths retain a valid filesystem and every prior named byte.
 build/test_mkdir_rollback "$image" "$mode"
 "$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py "$image"
 fsck.fat -n "$image"
 mcopy -i "$image" ::KEEP.BIN "$w/readback-$mode"
 cmp "$w/keep-$mode" "$w/readback-$mode"
 if mdir -i "$image" ::SUB/NEWDIR >/dev/null 2>&1; then
  printf 'FAIL: failed MKDIR published NEWDIR\nINITECH_CHECKS 1 failures=1\n'; exit 1
 fi
 mdir -i "$image" ::SUB
 printf 'INITECH_CHECKS 1 failures=0\n'
done
