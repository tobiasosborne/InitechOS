set -eu
check_suite=xcopy-drive
. dos/tests/assertions.sh
for disk in a c; do truncate -s 1474560 build/$disk.img; mformat -i build/$disk.img -f 1440 ::; done
printf 'on A\r\n' > build/a
printf 'on C\r\n' > build/c
mmd -i build/a.img ::SRC ::DST
mmd -i build/c.img ::SRC
mcopy -i build/a.img build/a ::SRC/ONE.TXT
mcopy -i build/c.img build/c ::SRC/ONE.TXT
XCOPY_DRIVE_C=build/c.img build/test_xcopy_tree --run build/a.img XCOPY 'C:\SRC\ONE.TXT A:\DST\' ''
read_product build/a.img DST/ONE.TXT build/got
check 'XCOPY copied C source bytes to A destination' cmp build/got build/c
echo 'GREEN XCOPY drive: distinct mounted A/C bytes, explicit source letter preserved'
complete
