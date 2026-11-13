set -eu
truncate -s 1474560 build/data.img
mformat -i build/data.img -f 1440 -v INITECH ::
LC_ALL=C TZ=UTC mdir -i build/data.img :: > build/dir_space.mdir
build/test_dir_disk build/data.img build/dir_space.mdir build/dir_space.cells
