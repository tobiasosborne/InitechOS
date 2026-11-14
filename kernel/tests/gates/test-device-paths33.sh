set -eu
capture=dos/command/tests/compat33/expected/DEVICES/files/TRACE.TXT
printf '%s  %s\n' b4434b53275a197d91ef76ce1f8d2e47b82b283e28e01f7d64f7100d2c31c050 "$capture" | sha256sum -c -
truncate -s 1474560 build/devices.img
mformat -i build/devices.img -f 1440 ::
mmd -i build/devices.img ::SUB
cp build/devices.img build/before.img
build/test_metadata_errors build/devices.img devices 0 "$capture"
cmp build/before.img build/devices.img
