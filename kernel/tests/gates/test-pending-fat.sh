set -eu
for tool in mformat mcopy mmd mdel mlabel mattrib mdir fsck.fat; do command -v "$tool" >/dev/null; done
mkdir -p build/pending-fat
w=build/pending-fat
truncate -s 1474560 "$w/base.img"
mformat -i "$w/base.img" -f 1440 ::
printf 'root bytes\n' > "$w/root"
printf 'deep bytes\n' > "$w/deep"
mcopy -i "$w/base.img" "$w/root" ::ROOT.BIN
mmd -i "$w/base.img" ::SUB ::SUB/DEEP
mcopy -i "$w/base.img" "$w/deep" ::SUB/DEEP/DOC.BIN
mlabel -i "$w/base.img" '::ROOT LABEL'
for mode in names hide rename cwd space labels; do
 cp "$w/base.img" "$w/$mode.img"
 build/test_pending_fat "$mode" "$w/$mode.img"
done
cmp "$w/base.img" "$w/names.img"
mattrib -i "$w/hide.img" ::SUB | grep -E 'H.*SUB'
mcopy -i "$w/rename.img" ::RENAMED/DEEP/DOC.BIN "$w/renamed-bytes"
cmp "$w/deep" "$w/renamed-bytes"
# Full volume, one low cluster freed, last cluster holds a full directory.
truncate -s 1474560 "$w/grow.img"
mformat -i "$w/grow.img" -f 1440 ::
dd if=/dev/zero of="$w/low" bs=512 count=1 status=none
dd if=/dev/zero of="$w/fill" bs=512 count=2845 status=none
mcopy -i "$w/grow.img" "$w/low" ::LOW.BIN
mcopy -i "$w/grow.img" "$w/fill" ::FILL.BIN
mmd -i "$w/grow.img" ::TAIL
: > "$w/empty"
for n in 1 2 3 4 5 6 7 8 9 10 11 12 13 14; do mcopy -i "$w/grow.img" "$w/empty" ::TAIL/F$n; done
mdel -i "$w/grow.img" ::LOW.BIN
build/test_pending_fat grow "$w/grow.img"
mdir -i "$w/grow.img" ::TAIL/NEW.BIN >/dev/null
mcopy -i "$w/grow.img" ::TARGET.BIN "$w/target"
test ! -s "$w/target"
fsck.fat -n "$w/grow.img"
# Real windowed FAT16 backend, not only the FAT decoder.
truncate -s $((16384*512)) "$w/fat16.img"
mformat -i "$w/fat16.img" -T 16384 -h 16 -s 63 -c 1 ::
awk 'BEGIN { for(i=0;i<1600;i++) printf "FAT16 independent line %04d\r\n",i }' > "$w/fat16-input"
mcopy -i "$w/fat16.img" "$w/fat16-input" ::CHAIN.BIN
mcopy -i "$w/fat16.img" ::CHAIN.BIN "$w/fat16-reference"
build/test_pending_fat fat16 "$w/fat16.img" "$w/fat16-reference"
"$PYTHON_FAT_REFERENCE" kernel/tests/disk/check_conservation.py "$w/base.img" "$w/names.img" "$w/hide.img" "$w/rename.img" "$w/cwd.img" "$w/space.img" "$w/labels.img" "$w/grow.img"
