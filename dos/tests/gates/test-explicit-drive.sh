set -eu
failed=0
trap 'rc=$?; if [ $rc -ne 0 ] && [ $failed -eq 0 ]; then failed=1; echo "FAIL explicit-drive: check failed (exit $rc)"; echo "INITECH_CHECKS 1 failures=1"; exit 1; fi' EXIT
truncate -s 1474560 build/drive.img
mformat -i build/drive.img -f 1440 ::
printf 'independent bytes\r\n' > build/file
mcopy -i build/drive.img build/file ::KEEP.TXT
# A is mounted; C is the default. Explicit A must use A's own volume/CWD.
DOSUTIL_DEFAULT=2 build/dosutil_host build/drive.img attrib '+R A:\KEEP.TXT'
mattrib -i build/drive.img ::KEEP.TXT | grep -q R
DOSUTIL_DEFAULT=2 build/dosutil_host build/drive.img label 'A:CAPTURE'
mlabel -i build/drive.img -s :: | grep -q CAPTURE
echo 'GREEN explicit-drive: ATTRIB and LABEL use mounted non-default A'
