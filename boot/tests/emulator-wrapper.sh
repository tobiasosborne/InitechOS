#!/bin/sh
# Only used by boot's external firmware fixture. Preserve harness arguments.
set -eu
case $(basename "$0") in
 qemu-system-i386) exec "$BOOT_REAL_QEMU" "$@" -option-rom "$BOOT_INDEXED_ROM" ;;
 bochs)
    # The harness creates a per-run config; add the standard expansion ROM.
    previous=''
    for arg do
        if [ "$previous" = -f ]; then
            printf '\noptromimage1: file="%s", address=0xd0000\n' "$BOOT_INDEXED_ROM" >> "$arg"
            break
        fi
        previous=$arg
    done
    exec "$BOOT_REAL_BOCHS" "$@" ;;
 *) exit 2 ;;
esac
