#!/bin/sh
# DOS 3.30 primary and secondary shell behavior on the installed system disk.
set -eu
root=${1:?root} out=${2:?output}
cd "$root"
mkdir -p "$out"
out=$(realpath "$out")
. factory/image/test-media.sh
export INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
export LC_ALL=C TZ=UTC
ref=dos/command/tests/dos33/reference/c-command-startup
fail() { echo "FAIL COMMAND startup: $*" >&2; exit 1; }

nasm -f bin "$ref/probe.asm" -o "$out/STARTCHK.COM"
nasm -f bin dos/tests/runtime/done.asm -o "$out/DONE.COM"
printf '%s\r\n' '@echo off' 'PATH C:\;C:\PROBES' 'ECHO C-COMMAND-READY' > "$out/AUTOEXEC.BAT"
printf '%s\r\n' 'FILES=7' 'SHELL=COMMAND.COM /P' > "$out/CONFIG.SYS"
printf '%s\r\n' '@echo off' 'ECHO WRONG-AUTOEXEC' > "$out/a-autoexec"
printf '%s\r\n' 'FILES=16' 'SHELL=COMMAND.COM /P' > "$out/a-config"
printf '%s\r\n' '@echo off' 'A:' > "$out/SELECT.BAT"
printf '%s\r\n' '@echo off' 'STARTCHK>C:\S.OUT' > "$out/SAVE.BAT"
sed 's/A:/C:/g' "$ref/console.txt" | sort > "$out/expected-console"
printf 'EXTERNAL-PATH\r\n' > "$out/expected-file"

make -s -f factory/desktop-tools.mk "$PWD/build/factory/desktop-bochs"
for emulator in ${DOS_CSTART_EMULATORS:-qemu bochs}; do
    for attachment in alone with-data; do
        run=$out/$emulator-$attachment
        mkdir -p "$run"
        media_prepare "${DOS_CSTART_MEDIA:-staged}" "$attachment" "$run"
        [ "$media_system" = C ] || fail 'expected an installed C: system volume'
        mmd -i "$media_work" ::PROBES
        mcopy -i "$media_work" "$out/STARTCHK.COM" ::PROBES
        mcopy -o -i "$media_work" "$out/AUTOEXEC.BAT" "$out/CONFIG.SYS" "$out/DONE.COM" ::
        mcopy -i "$media_work" "$out/SELECT.BAT" "$out/SAVE.BAT" ::PROBES
        if [ "$attachment" = with-data ]; then
            mcopy -o -i "$media_data" "$out/a-autoexec" ::AUTOEXEC.BAT
            mcopy -o -i "$media_data" "$out/a-config" ::CONFIG.SYS
        fi

        keys=$(
            {
                printf '%s\n' path
                if [ "$attachment" = with-data ]; then printf '%s\n' select; fi
                printf '%s\n' 'command /c set' startchk save done
            } | build/factory/keys
        )
        case $emulator in
            qemu) driver=build/factory/qemu_harness ;;
            bochs)
                driver=build/factory/desktop-bochs
                keys=$(printf '%s\n' "$keys" | sed 's/^/k/; s/,/,k/g')
                export DESKTOP_SERIAL="$run/shell.serial"
                ;;
            *) fail "unknown emulator $emulator" ;;
        esac
        media_run "$emulator" "$driver" --keys-after C-COMMAND-READY --keys "$keys" \
            --expect TRANSPORT-DONE --quit-after TRANSPORT-DONE --timeout-ms 55000 \
            --out "$run" --name shell > "$run/stdout" 2> "$run/report" ||
            fail "$emulator $attachment transport"
        grep -Fq 'triple_fault=0' "$run/report" || fail "$emulator CPU fault"
        tr -d '\r' < "$run/shell.serial" > "$run/console"
        [ "$(grep -Fxc C-COMMAND-READY "$run/console")" -eq 1 ] ||
            fail "$emulator $attachment did not run the primary C: batch once"
        grep -Fq 'SYSINIT: source=CONFIG.SYS FILES=7 cap=7 SHELL=COMMAND.COM /P' "$run/console" ||
            fail "$emulator $attachment did not select C: CONFIG.SYS"
        if grep -Eq 'WRONG-AUTOEXEC|KBD-RING-DROP|PANIC' "$run/console"; then
            fail "$emulator $attachment selected the wrong startup or lost input"
        fi
        selected=C
        [ "$attachment" != with-data ] || selected=A
        grep -Fq "$selected:\>command /c set" "$run/console" ||
            fail "$emulator $attachment did not run secondary COMMAND at the selected prompt"
        awk '/^(COMSPEC=|PATH=|PROMPT=|EXTERNAL-PATH$)/ { print }' "$run/console" | sort > "$run/actual"
        cmp "$out/expected-console" "$run/actual" || fail "$emulator $attachment secondary environment and PATH"
        media_readback "$emulator" "$run/report"
        mtype -i "$media_volume" ::S.OUT > "$run/written"
        cmp "$out/expected-file" "$run/written" || fail "$emulator $attachment saved bytes"
        echo "ok $emulator $attachment primary C: and secondary COMMAND"
    done
done
