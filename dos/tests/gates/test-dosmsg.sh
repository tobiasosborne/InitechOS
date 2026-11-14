set -eu
header=${SOURCE_ROOT}/build/kernel/dos_messages.h
# ATTRIB's target core omits LABEL. Process conditionals while retaining macro
# references, then discard preprocessing directives (including header defines).
# LABEL is still scanned against its original complete source below.
"${TARGET_CC:-cc}" -m32 -E -fdirectives-only -DDOSUTIL_ATTRIB \
 -I"${SOURCE_ROOT}/kernel/include" -I"${SOURCE_ROOT}/build/kernel" -Idos/common \
 dos/common/dosutil.c > "${HOST_TEST_OUTPUT}/dosutil-attrib.i"
sed '/^[[:space:]]*#/d' "${HOST_TEST_OUTPUT}/dosutil-attrib.i" > "${HOST_TEST_OUTPUT}/dosutil-attrib.c"
build/dosmsg_scan --image "$header" "${HOST_TEST_OUTPUT}/dosutil-attrib.c" "${HOST_TEST_OUTPUT}/../ATTRIB.COM"
for pair in common/dosutil.c:LABEL chkdsk/chkdsk.c:CHKDSK xcopy/xcopy_tree.c:XCOPY xcopy/xcopy_tree.c:TREE edlin/edlin.c:EDLIN find/find_program.asm:FIND sort/sort_program.asm:SORT tpsrep/tpsrep_program.asm:TPSREP; do
 build/dosmsg_scan --image "$header" "dos/${pair%:*}" "${HOST_TEST_OUTPUT}/../${pair#*:}.COM"
done
build/dosmsg_scan --known "$header" dos/common/*.c dos/chkdsk/*.c dos/diskutil/*.c dos/edlin/*.c dos/fdisk/*.c dos/xcopy/*.c
