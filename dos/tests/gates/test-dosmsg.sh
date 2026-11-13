set -eu
header=${SOURCE_ROOT}/build/kernel/dos_messages.h
for pair in common/dosutil.c:ATTRIB common/dosutil.c:LABEL chkdsk/chkdsk.c:CHKDSK xcopy/xcopy_tree.c:XCOPY xcopy/xcopy_tree.c:TREE edlin/edlin.c:EDLIN find/find_program.asm:FIND sort/sort_program.asm:SORT tpsrep/tpsrep_program.asm:TPSREP; do
 build/dosmsg_scan --image "$header" "dos/${pair%:*}" "${HOST_TEST_OUTPUT}/../${pair#*:}.COM"
done
build/dosmsg_scan --known "$header" dos/common/*.c dos/chkdsk/*.c dos/diskutil/*.c dos/edlin/*.c dos/fdisk/*.c dos/xcopy/*.c
