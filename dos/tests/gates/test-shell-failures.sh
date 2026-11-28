set -eu
check_suite=shell-failures
. dos/tests/assertions.sh
truncate -s 1474560 build/fail.img
mformat -i build/fail.img -f 1440 ::
printf 'independent file bytes\r\n' > build/file
mcopy -i build/fail.img build/file ::KEEP.TXT
cp build/fail.img build/before.img
printf 'DEL *.TXT\n' > build/command
SHELL_DENY_UNLINK=1 build/shell_files build/fail.img build/command > build/delete-output
# An explicit backend refusal must identify the failed file and report it.
check 'DEL refusal names KEEP.TXT' grep -F 'KEEP.TXT' build/delete-output
check 'DEL refusal reports Access denied' grep -F 'Access denied' build/delete-output
check 'DEL refusal preserves volume' cmp build/fail.img build/before.img
printf 'TYPE KEEP.TXT\n' > build/command
SHELL_BREAK_READ=1 build/shell_files build/fail.img build/command > build/type-output
# A service interrupted before completion owns no bytes to emit.
check 'interrupted TYPE emits no bytes' test ! -s build/type-output
check 'TYPE interruption preserves volume' cmp build/fail.img build/before.img
echo 'GREEN shell-failures: failed unlink is visible; interrupted read emits nothing; volume unchanged'
ref=dos/command/tests/dos33/reference/batch
grep -F CAPTURE-DONE "$ref/result.txt" >/dev/null || error 'incomplete DOS capture'
for spaces in 122 123;do
 sed -n "/MASK-$spaces-BEGIN/,/MASK-$spaces-END/p" "$ref/result.txt" |
  sed '1d;$d;s/ *$//' | awk 'length {printf "%s\r\n",$0}' > build/captured-diagnostic
 check "DOS capture $spaces extraction agrees" cmp build/captured-diagnostic "$ref/MASK$spaces/DEL.OUT"
done
gzip -dc "$ref/before-data.img.gz" > build/masks.img
awk '/^CD |^DEL/ {print}' "$ref/AUTOEXEC.BAT" > build/masks.bat
build/shell_files build/masks.img --batch build/masks.bat > build/masks-output
cat "$ref/MASK122/DEL.OUT" "$ref/MASK123/DEL.OUT" > build/masks-expected
check 'DEL physical-line diagnostics match DOS 3.30' cmp build/masks-output build/masks-expected
# Also re-read the captured disk: a stale extracted inventory cannot certify it.
gzip -dc "$ref/data.img.gz" > build/masks-reference.img
for mask in MASK122 MASK123;do
 for image in masks masks-reference;do
  mdir -b -i build/$image.img "::$mask" > build/raw-names || error "list $image $mask"
  sed 's@.*/@@' build/raw-names > build/leaves
  LC_ALL=C sort build/leaves > build/$image-names
  check "$image $mask surviving names" cmp build/$image-names "$ref/$mask/names"
  while IFS= read -r leaf;do
   mtype -i build/$image.img "::$mask/$leaf" > build/mask-bytes
   check "$image $mask/$leaf surviving bytes" cmp build/mask-bytes "$ref/$mask/$leaf"
  done < "$ref/$mask/names"
 done
done
echo 'GREEN batch masks: 122/123 spaces match real DOS diagnostics, names and surviving bytes'
# DOS 3.30 deletes through a 127-character wildcard path (long-del/ORIGIN).
ref=dos/command/tests/dos33/reference/long-del
truncate -s 1474560 build/longdel.img
mformat -i build/longdel.img -f 1440 ::
mmd -i build/longdel.img ::A
printf 'report\r\n' > build/report
for leaf in REPORT01.TXT NOTES.TXT;do mcopy -i build/longdel.img build/report "::A/$leaf";done
mcopy -i build/longdel.img "$ref/KEEP.DAT" ::A/KEEP.DAT
build/shell_files build/longdel.img "$ref/batch.txt" > build/longdel-output
check 'long DEL path prints nothing' test ! -s build/longdel-output
mdir -b -i build/longdel.img ::A | sed 's@.*/@@' | LC_ALL=C sort > build/longdel-names
check 'long DEL path leaves the DOS 3.30 inventory' cmp build/longdel-names "$ref/names"
mtype -i build/longdel.img ::A/KEEP.DAT > build/longdel-keep
check 'long DEL path keeps KEEP.DAT bytes' cmp build/longdel-keep "$ref/KEEP.DAT"
echo 'GREEN long DEL path: 127-character wildcard deletes what DOS 3.30 deletes'
complete
