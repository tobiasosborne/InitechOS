#!/bin/sh
# GM item 8 and the ready-made disk ruling in STATUS.md. Both launchers use
# the same working copy. The factory only builds and checks the masters.
set -eu

# The recursive make recipe is entered even in these GNU make modes.
case ${MAKEFLAGS%% *} in *[nqt]*) exit 0 ;; esac
make_cmd=$1
launcher=$2
disks=user-disks

# Hold the same lock across first-time copying and emulation under either
# backend. Lock the checkout directory so the disk set can be renamed away.
exec 9<.
flock -n 9 || { echo 'InitechOS: a session is already running in this directory.' >&2; exit 1; }

if [ ! -e "$disks" ] && [ ! -L "$disks" ]; then
    "$make_cmd" image
    stage=$(mktemp -d .user-disks.XXXXXX)
    trap 'rm -rf "$stage"' EXIT
    trap 'exit 1' HUP INT TERM
    cp build/image/boot.img build/image/data.img "$stage/"
    # Publish the pair in one rename on the same filesystem.
    if ! mv -nT "$stage" "$disks" || [ -d "$stage" ]; then
        if [ -e "$disks" ] || [ -L "$disks" ]; then
            echo "InitechOS: $disks appeared during preparation. Start again to use it." >&2
        fi
        exit 1
    fi
    trap - EXIT HUP INT TERM
fi

boot=$disks/boot.img
data=$disks/data.img
if [ -L "$disks" ] || [ -L "$boot" ] || [ -L "$data" ] ||
   [ ! -f "$boot" ] || [ ! -f "$data" ]; then
    echo "InitechOS: $disks must contain a working pair of boot.img and data.img." >&2
    exit 1
fi

case "$launcher" in
    run)
        mkdir -p build
        qemu-system-i386 -m 8 -machine acpi=off -cpu 486 \
            -drive "file=$boot,format=raw,index=0" -drive "file=$data,format=raw,index=1" \
            -serial stdio -d int,guest_errors,cpu_reset -D build/qemu.log -no-reboot
        ;;
    run-bochs)
        "$make_cmd" -C factory "$PWD/build/factory/bochs_harness"
        INITECH_TEST_RAM_MB=8 build/factory/bochs_harness --disk "$boot" --disk2 "$data" \
            --write-boot-disk --serial-stdout --timeout-ms 3600000 --out build/factory
        ;;
esac
