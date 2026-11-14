set -eu
truncate -s 1474560 build/fail.img
mformat -i build/fail.img -f 1440 ::
printf 'independent file bytes\r\n' > build/file
mcopy -i build/fail.img build/file ::KEEP.TXT
cp build/fail.img build/before.img
printf 'DEL *.TXT\n' > build/command
SHELL_DENY_UNLINK=1 build/shell_files build/fail.img build/command > build/delete-output
# An explicit backend refusal must identify the failed file and report it.
grep -F 'KEEP.TXT' build/delete-output
grep -F 'Access denied' build/delete-output
cmp build/fail.img build/before.img
printf 'TYPE KEEP.TXT\n' > build/command
SHELL_BREAK_READ=1 build/shell_files build/fail.img build/command > build/type-output
# A service interrupted before completion owns no bytes to emit.
test ! -s build/type-output
cmp build/fail.img build/before.img
echo 'GREEN shell-failures: failed unlink is visible; interrupted read emits nothing; volume unchanged'
# The original reported physical-mask boundary, captured from real DOS 3.30.
# Real DOS retains KEEP.DAT and JUNK.TMP; verify actual command + FAT behavior.
printf 'temp\r\n' > build/temp
mcopy -i build/fail.img build/temp ::JUNK.TMP
awk '/^DEL/ {print}' dos/command/tests/dos33/reference/batch/AUTOEXEC.BAT > build/long.bat
cp build/fail.img build/before.img
build/shell_files build/fail.img --batch build/long.bat > build/long-output
cmp build/fail.img build/before.img
tr -d '\r' < build/long-output | grep -Fx 'File not found' >/dev/null
