#!/bin/sh
# Accepted GM8 layout and DOS 3.30 primary processor rules, tested on copies
# of the factory's staged C: system disk and optional ATA data volume.
set -eu
cd "$(dirname "$0")/../.."
export SOURCE_DATE_EPOCH=315532800 TZ=UTC LC_ALL=C MTOOLS_SKIP_CHECK=1
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
mkdir -p build/kernel/startup-native
out=$(mktemp -d "$PWD/build/kernel/startup-native/run.XXXXXX")
echo "C startup evidence: $out"
fail() { echo "FAIL C startup: $*" >&2; exit 1; }

# The staged media still carry factory's AUTOEXEC. Install the sentinels on
# private copies so the product masters remain pristine.
cp "${CSTART_BOOT_IMAGE:-build/image/c.img}" "$out/master.img"
cp "${CSTART_DATA_IMAGE:-build/image/a-data.img}" "$out/data.img"
nasm -f bin kernel/tests/programs/startup_drive.asm -o "$out/STARTCHK.COM"
build/turbo/seed/initechc --emit-asm -o "$out/cargs.s" turbo/tests/ide/runtime/arguments-all.pas
sh turbo/compiler/tests/package.sh "$out/cargs.s" "$out/cargs" > "$out/cargs-build.log" 2>&1
mcopy -o -i "$out/master.img@@32256" "$out/cargs/program.exe" ::APPS/CARGS.EXE

printf '%s\r\n' 'FILES=7' 'SHELL=COMMAND.COM /P' 'DEVICE=ANSI.SYS' > "$out/CONFIG.SYS"
mcopy -o -i "$out/master.img@@32256" "$out/STARTCHK.COM" "$out/CONFIG.SYS" ::
printf '%s\r\n' 'independent ATA data' > "$out/KEEP.TXT"
printf '%s\r\n' '@ECHO OFF' 'ECHO CSTART-WRONG-A' > "$out/a-autoexec"
printf '%s\r\n' 'FILES=16' 'SHELL=COMMAND.COM /P' > "$out/a-config"
mcopy -o -i "$out/data.img" "$out/KEEP.TXT" ::
mcopy -o -i "$out/data.img" "$out/a-autoexec" ::AUTOEXEC.BAT
mcopy -o -i "$out/data.img" "$out/a-config" ::CONFIG.SYS
printf '%s\r\n' 'C system persisted bytes' > "$out/expected-c"

assert_arguments() (
    drive=$1 run=$2
    # Reuse the MINT07 ParamStr(0) probe, with the literal path of this fixture.
    printf '%s\n' 'COUNT=4' 'ARG0=C:\APPS\CARGS.EXE' 'ARG1=alpha' 'ARG2="two' \
        'ARG3=words"' 'ARG4=omega' 'ARG5=' "DIR=$drive:"'\' > "$run/$drive.expected"
    sed -n "/^CSTART-ARGS-$drive-BEGIN\$/,/^CSTART-ARGS-$drive-END\$/p" "$run/serial.txt" |
        sed '1d;$d' > "$run/$drive.actual"
    diff -u "$run/$drive.expected" "$run/$drive.actual" ||
        fail "$run: executable environment path with $drive: selected"
)

make -s -f factory/desktop-tools.mk "$PWD/build/factory/desktop-bochs"
for emu in ${CSTART_EMULATORS:-qemu bochs}; do
    for data in alone with-data; do
        run=$out/$emu-$data
        mkdir "$run"
        cp "$out/master.img" "$run/boot.img"
        {
            printf '%s\r\n' '@ECHO OFF' 'PATH C:\;C:\APPS' 'ECHO CSTART-AUTOEXEC-C' \
                'ECHO %COMSPEC%' 'PATH' 'STARTCHK' 'ECHO CSTART-ARGS-C-BEGIN' \
                'CARGS alpha "two words" omega' 'ECHO CSTART-ARGS-C-END'
            if [ "$data" = with-data ]; then
                printf '%s\r\n' 'A:' 'ECHO CSTART-ARGS-A-BEGIN' \
                    'C:\APPS\CARGS.EXE alpha "two words" omega' 'ECHO CSTART-ARGS-A-END' \
                    'ECHO %COMSPEC%' 'PATH'
            fi
            printf '%s\r\n' 'ECHO CSTART-NATIVE-DONE'
        } > "$run/AUTOEXEC.BAT"
        mcopy -o -i "$run/boot.img@@32256" "$run/AUTOEXEC.BAT" ::
        set -- --disk "$run/boot.img"
        if [ "$data" = with-data ]; then
            cp "$out/data.img" "$run/data.img"
            set -- "$@" --disk2 "$run/data.img"
        fi
        case $emu in
            qemu) driver=build/factory/qemu_harness; set -- "$@" --boot-rw ;;
            bochs) driver=build/factory/desktop-bochs ;;
            *) echo "ERROR unknown emulator $emu" >&2; exit 2 ;;
        esac
        export DESKTOP_SERIAL="$run/$emu.serial" DESKTOP_WAIT_MS=30000
        "$driver" "$@" --expect CSTART-NATIVE-DONE --quit-after CSTART-NATIVE-DONE \
            --timeout-ms "${CSTART_TIMEOUT_MS:-90000}" --name "$emu" --out "$run" \
            > "$run/stdout" 2> "$run/report" || {
                cat "$run/report"
                fail "$emu $data did not complete"
            }

        tr -d '\r' < "$run/$emu.serial" > "$run/serial.txt"
        for marker in 'Directory of C:\' \
            'SYSINIT: source=CONFIG.SYS FILES=7 cap=7 SHELL=COMMAND.COM /P' \
            'CSTART-AUTOEXEC-C' 'CSTART-OK'; do
            grep -Fx "$marker" "$run/serial.txt" >/dev/null || fail "$emu $data missing [$marker]"
        done
        assert_arguments C "$run"
        marker=CSTART-A-ABSENT
        env_count=1
        if [ "$data" = with-data ]; then
            marker=CSTART-A-PRESENT
            env_count=2
            assert_arguments A "$run"
            cmp "$out/data.img" "$run/data.img" || fail "$emu changed independent A"
        fi
        grep -Fx "$marker" "$run/serial.txt" >/dev/null || fail "$emu $data wrong A identity"
        for marker in 'C:\COMMAND.COM' 'PATH=C:\;C:\APPS'; do
            count=$(grep -Fxc "$marker" "$run/serial.txt" || :)
            [ "$count" -eq "$env_count" ] || fail "$emu $data wrong [$marker]"
        done
        if grep -E 'CSTART-FAIL|CSTART-WRONG-A|PANIC|Bad command|KBD-RING-DROP' "$run/serial.txt"; then
            fail "$emu $data guest failure"
        fi
        grep 'triple_fault=0' "$run/report" >/dev/null || fail "$emu $data CPU fault"
        written=$run/boot.img
        if [ "$emu" = bochs ]; then
            written=$(sed -n 's/^\[bochs\] private_boot=//p' "$run/report")
        fi
        [ -f "$written" ] || { echo "ERROR missing written boot copy" >&2; exit 2; }
        mcopy -i "$written@@32256" ::CSTART.TXT "$run/persisted.txt"
        cmp "$out/expected-c" "$run/persisted.txt" || fail "$emu $data persisted C bytes"
        echo "ok $emu $data: C CONFIG, COMSPEC, PATH, EXEC, persisted bytes and A identity"
    done
done
echo 'C STARTUP: 0 failures'
