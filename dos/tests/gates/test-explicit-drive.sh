set -eu
check_suite=explicit-drive
. dos/tests/assertions.sh
truncate -s 1474560 build/drive.img
mformat -i build/drive.img -f 1440 ::
printf 'independent bytes\r\n' > build/file
mcopy -i build/drive.img build/file ::KEEP.TXT
# A is mounted; C is the default. Explicit A must use A's own volume/CWD.
rc=0
DOSUTIL_COMPLETION=1 DOSUTIL_DEFAULT=2 build/dosutil_host build/drive.img attrib '+R A:\KEEP.TXT' 2> build/attrib.log || rc=$?
grep -Fx "DOSUTIL-HOST-DONE rc=$rc" build/attrib.log >/dev/null || error 'ATTRIB host did not complete'
mattrib -i build/drive.img ::KEEP.TXT > build/attributes
check 'ATTRIB +R not set on explicit A' grep -q R build/attributes
rc=0
DOSUTIL_COMPLETION=1 DOSUTIL_DEFAULT=2 build/dosutil_host build/drive.img label 'A:CAPTURE' 2> build/label.log || rc=$?
grep -Fx "DOSUTIL-HOST-DONE rc=$rc" build/label.log >/dev/null || error 'LABEL host did not complete'
mlabel -i build/drive.img -s :: > build/label
check 'LABEL not set on explicit A' grep -q CAPTURE build/label
echo 'GREEN explicit-drive: ATTRIB and LABEL use mounted non-default A'
complete
