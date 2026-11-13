set -eu
truncate -s 1474560 build/not-ready.img
mformat -i build/not-ready.img -f 1440 ::
build/test_dos_not_ready build/not-ready.img
