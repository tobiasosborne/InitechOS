set -eu
truncate -s $((2943*512)) build/tracer_secondary.img
printf '\200\000\001\000\001\377\377\377\077\000\000\000\100\013\000\000' | dd of=build/tracer_secondary.img bs=1 seek=446 conv=notrunc status=none
printf '\125\252' | dd of=build/tracer_secondary.img bs=1 seek=510 conv=notrunc status=none
mformat -i build/tracer_secondary.img@@32256 -f 1440 -H 63 -N 0x494e4954 ::
printf 'IO system fixture' > build/io.fixture
printf 'Kernel system fixture' > build/kernel.fixture
printf 'Command system fixture' > build/transient_flairtenants.bin
mcopy -i build/tracer_secondary.img@@32256 build/io.fixture ::IO.SYS
mcopy -i build/tracer_secondary.img@@32256 build/kernel.fixture ::INITDOS.SYS
mcopy -i build/tracer_secondary.img@@32256 build/transient_flairtenants.bin ::COMMAND.COM
mattrib -i build/tracer_secondary.img@@32256 +r +s +h ::IO.SYS ::INITDOS.SYS

sh kernel/tests/disk/dos_volumes.sh host build/tracer_secondary.img
