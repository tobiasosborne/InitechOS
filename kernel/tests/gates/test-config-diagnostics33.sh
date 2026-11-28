set -eu
 . kernel/tests/assertions.sh
sha256sum -c kernel/tests/fixtures/config33/SHA256SUMS
for config_case in CFGA CFGB CFGC CFGD CFGE CFGF; do
 reference=dos/command/tests/compat33/expected/$config_case
 gzip -dc "$reference/data.img.gz" > build/reference.img
 mcopy -o -i build/reference.img "::$config_case.CFG" build/CONFIG.SYS
 truncate -s 1474560 build/config.img
 mformat -i build/config.img -f 1440 ::
 mcopy -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
 cp build/config.img build/before.img
 build/test_config_apply build/config.img > build/console.raw
 tr -d '\r' < build/console.raw > build/console.txt
 sed -n '/Unrecognized command in CONFIG.SYS\|Bad or missing MISSING.SYS/{s/[[:space:]]*$//;p;}' "$reference/result.txt" > build/expected.txt
 test -s build/expected.txt
 printf '%s diagnostics\n' "$config_case"
 assert diff -u build/expected.txt build/console.txt
 assert cmp build/before.img build/config.img
done

# GM2: the configurable cache and a larger SFT are unsupported here. Refuse
# visibly using the captured period diagnostic; do not claim DOS implements
# these omissions. The valid supported control must remain silent.
sed -n '/Unrecognized command in CONFIG.SYS/{s/[[:space:]]*$//;p;}' dos/command/tests/compat33/expected/CFGA/result.txt > build/expected.txt
for request in 'BUFFERS=20' 'FILES=255'; do
 printf '%s\r\n' "$request" > build/CONFIG.SYS
 mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
 build/test_config_apply build/config.img > build/console.raw
 tr -d '\r' < build/console.raw > build/console.txt
 assert diff -u build/expected.txt build/console.txt
done
printf 'FILES=20\r\nBREAK=ON junk\r\nLASTDRIVE=ZZ\r\nDEVICE=ANSI.SYS\r\n' > build/CONFIG.SYS
printf 'ANSI driver fixture\r\n' > build/ANSI.SYS
mcopy -o -i build/config.img build/CONFIG.SYS build/ANSI.SYS ::
build/test_config_apply build/config.img > build/console.raw
assert test ! -s build/console.raw
for files in 5 6 7 8; do
 fixture=kernel/tests/fixtures/config33/files$files
 mcopy -o -i build/config.img "$fixture/CONFIG.SYS" ::CONFIG.SYS
 : > build/COMMAND.COM
 mcopy -o -i build/config.img build/COMMAND.COM ::COMMAND.COM
 count_hex=$(sed -n 's/.*CFG-STATE=[0-9A-F]\{4\}\([0-9A-F]\{2\}\).*/\1/p' "$fixture/result.txt")
 count=$(printf '%d' "0x$count_hex")
 cp build/config.img build/before.img
 build/test_config_apply build/config.img "$count" > build/console.raw
 assert test ! -s build/console.raw
 assert cmp build/before.img build/config.img
done
for config_case in files4 filesjunk buffersjunk fileszero files256 filesspace lastjunk breakjunk breaktight; do
 fixture=kernel/tests/fixtures/config33/$config_case
 mcopy -o -i build/config.img "$fixture/CONFIG.SYS" ::CONFIG.SYS
 build/test_config_apply build/config.img > build/console.raw
 tr -d '\r' < build/console.raw > build/console.txt
 sed -n '/Unrecognized command in CONFIG.SYS/{s/[[:space:]]*$//;p;}' "$fixture/result.txt" > build/expected.txt
 assert diff -u build/expected.txt build/console.txt
done
# A bad numeric token crossing byte 1024 still refuses.
printf 'FILES=8\r\n' > build/CONFIG.SYS
awk 'BEGIN {for(i=0;i<1008;i++)printf " "; printf "FILES=9junk\r\n"}' >> build/CONFIG.SYS
mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
build/test_config_apply build/config.img 5 > build/console.raw
tr -d '\r' < build/console.raw > build/console.txt
sed -n '/Unrecognized command in CONFIG.SYS/{s/[[:space:]]*$//;p;}' dos/command/tests/compat33/expected/CFGA/result.txt > build/expected.txt
assert diff -u build/expected.txt build/console.txt
# A malformed first line crossing byte 1024 leaves the default capacity.
awk 'BEGIN {for(i=0;i<1017;i++)printf " "; printf "FILES=9junk\r\n"}' > build/CONFIG.SYS
mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
build/test_config_apply build/config.img 5 > build/console.raw
tr -d '\r' < build/console.raw > build/console.txt
assert diff -u build/expected.txt build/console.txt
# The driver exists, but the built-in interpreter has no driver arguments.
printf 'DEVICE=ANSI.SYS /Z\r\n' > build/CONFIG.SYS
mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
cp build/config.img build/before.img
build/test_config_apply build/config.img state:000505:0 > build/console.raw
tr -d '\r' < build/console.raw > build/console.txt
assert diff -u build/expected.txt build/console.txt
assert cmp build/before.img build/config.img

# BREAK, logical drives and the handle limit are sampled through INT 21h.
while read -r letter state; do
 printf 'BREAK=OFF\r\nLASTDRIVE=%s\r\nFILES=8\r\nDEVICE=ANSI.SYS\r\n' "$letter" > build/CONFIG.SYS
 mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
 assert build/test_config_apply build/config.img "state:$state:1" > build/console.raw
 assert test ! -s build/console.raw
done < kernel/tests/fixtures/config33/lastdrive-state.txt
finish
