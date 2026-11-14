set -eu
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
diff -u build/expected.txt build/console.txt
cmp build/before.img build/config.img
printf 'SHELL=COMMAND.COM /P /E:1024\r\n' > build/CONFIG.SYS
mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
build/test_config_apply build/config.img refused > build/console.raw
tr -d '\r' < build/console.raw > build/console.txt
diff -u build/expected.txt build/console.txt
printf 'SHELL=COMMAND.COM /P /E:512\r\n' > build/CONFIG.SYS
mcopy -o -i build/config.img build/CONFIG.SYS ::CONFIG.SYS
build/test_config_apply build/config.img ready > build/console.raw
test ! -s build/console.raw
