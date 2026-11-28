set -eu
 . kernel/tests/assertions.sh
sha256sum -c kernel/tests/fixtures/config33/SHA256SUMS
reference=dos/command/tests/compat33/expected/CFG3
gzip -dc "$reference/data.img.gz" > build/reference.img
mcopy -o -i build/reference.img ::CFG3.CFG build/CONFIG.SYS
truncate -s 1474560 build/config.img
mformat -i build/config.img -f 1440 ::
mcopy -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
cp build/config.img build/before.img
build/test_config_apply build/config.img refused > build/console.raw
tr -d '\r' < build/console.raw > build/console.txt
sed -n '/Bad or missing Command Interpreter/{s/[[:space:]]*$//;p;}' "$reference/result.txt" > build/expected.txt
test -s build/expected.txt
assert diff -u build/expected.txt build/console.txt
assert cmp build/before.img build/config.img
printf 'SHELL=COMMAND.COM /P /E:junk\r\n' > build/CONFIG.SYS
mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
build/test_config_apply build/config.img refused > build/console.raw
tr -d '\r' < build/console.raw > build/console.txt
assert diff -u build/expected.txt build/console.txt
for shell in '' "$(printf '%064d' 0 | tr 0 x)"; do
 printf 'SHELL=%s\r\n' "$shell" > build/CONFIG.SYS
 mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
 assert build/test_config_apply build/config.img refused > build/console.raw
 tr -d '\r' < build/console.raw > build/console.txt
 assert diff -u build/expected.txt build/console.txt
done
# DOS 3.30 CONFIG.SYS audit row 4 measured MCB sizes 000A, 0020 and 0040.
for size in 160 512 1024; do
 printf 'SHELL=COMMAND.COM /P' > build/CONFIG.SYS
 if [ "$size" != 160 ]; then printf ' /E:%s' "$size" >> build/CONFIG.SYS; fi
 printf '\r\n' >> build/CONFIG.SYS
 mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
 assert build/test_config_apply build/config.img "env:$size" > build/console.raw
 assert test ! -s build/console.raw
done
finish
