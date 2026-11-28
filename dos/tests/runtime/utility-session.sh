# Utilities exercise the program and fixture on a private A: disk. When the
# master is installed C:, its AUTOEXEC switches to A: and calls the fixture.
# Before the installed-image cutover, the legacy master starts that batch
# itself. Remove the legacy path when the boot.img/data.img pair becomes C+A.
utility_session() (
    boot=${DOS_UTILITY_BOOT:-$1} data=$2 output=$3
    shift 3
    cpu=${DOS_UTILITY_EMULATOR:-qemu}
    case $cpu in qemu|bochs) ;; *) exit 2;; esac
    driver=$root/build/factory/${cpu}_harness
    if [ "$cpu" = bochs ] && [ "${DOS_UTILITY_ORDERED:-0}" = 1 ]; then
        driver=$root/build/factory/desktop-bochs
    fi
    mdir -a -b -i "$boot@@32256" :: > "$output/master-files" || exit 2
    if grep -Fxiq '::/COMMAND.COM' "$output/master-files"; then
        cp "$boot" "$output/master.img" || exit 2
        boot=$output/master.img
        printf '%s\r\n' '@ECHO OFF' 'A:' 'CALL A:\AUTOEXEC.BAT' > "$output/c-start.bat"
        mcopy -o -i "$boot@@32256" "$output/c-start.bat" ::AUTOEXEC.BAT || exit 2
        if [ "$cpu" = qemu ]; then set -- --boot-rw "$@"; fi
    fi
    exec timeout -k 5 120 "$driver" \
        --disk "$boot" --disk2 "$data" --out "$output" "$@"
)

utility_transport() (
    out=${2:-$out}
    if [ "${DOS_UTILITY_EMULATOR:-qemu}" = qemu ]; then
        transport_pass "$1"
    else
        grep -Fx '[bochs] OK=1' "$1" >/dev/null || error 'Bochs transport did not complete'
        grep -F 'timed_out=0' "$1" >/dev/null || error 'Bochs transport timed out'
        grep -Fx '[bochs] triple_fault=0' "$1" >/dev/null || error 'Bochs CPU fault'
        realpath "$1" > "$out/transport.ok"
    fi
)
