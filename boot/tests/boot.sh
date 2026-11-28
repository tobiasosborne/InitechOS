#!/bin/sh
# Independent oracles: IBM DOS 3.30 TR 1-7,5-7..11, MS-DOS 3.3 UG 119;
# VBE Core 2.0 rev1.1 pp6,16-20; project hardware contract (8 MiB).
# Test only copies of the product disk; no test kernel or product hook.
set -eu
top=$1 artifacts=$2 cases=${3:-'normal indexed fallback ORDER ATTR_R CHAIN FRAGMENT RAM5 RAM7'}
[ -n "$cases" ] || exit 2
cd "$top"
test -s build/system/INITDOS.SYS && test -s build/image/data.img || {
    echo 'ERROR boot: run make image first' >&2; exit 2;
}
out=$(mktemp -d "$artifacts/smoke.XXXXXX")
echo "boot evidence: $out"
# The two firmware runs each mint and modify their own scenario images.
if [ "${INITECH_FULL_SMOKE:-0}" = 1 ] && [ "$#" -eq 2 ]; then
    . factory/smoke-emulators.sh
    run_emulator() { sh boot/tests/boot.sh "$top" "$artifacts" "$cases" "$1"; }
    smoke_emulators qemu bochs
    exit 0
fi
export LC_ALL=C INITECH_TEST_RAM_MB=8 INITECH_QMP_STDIO=1
export BOOT_REAL_QEMU BOOT_REAL_BOCHS BOOT_INDEXED_ROM
BOOT_REAL_QEMU=$(command -v qemu-system-i386)
BOOT_REAL_BOCHS=$(command -v bochs)
BOOT_INDEXED_ROM=$artifacts/indexed.rom
mkdir "$out/bin"
cp boot/tests/emulator-wrapper.sh "$out/bin/qemu-system-i386"
cp boot/tests/emulator-wrapper.sh "$out/bin/bochs"
chmod +x "$out/bin/"*
# Keep the input payloads used for minting: another root image build may
# replace its outputs while these emulator legs wait on the shared lock.
cp "$artifacts/IO.SYS" "$out/expected-IO.SYS"
cp build/system/INITDOS.SYS "$out/expected-INITDOS.SYS"
sh factory/image/mk_boot_system.sh "$out/boot.img" "$artifacts/mbr.bin" \
    "$artifacts/stage2.bin" "$out/expected-IO.SYS" "$out/expected-INITDOS.SYS" build/factory/boot_system
cp build/image/data.img "$out/data.img"
mdel -i "$out/data.img" ::AUTOEXEC.BAT
failed=0
for scenario in $cases; do
    image=$out/$scenario.img
    cp "$out/boot.img" "$image"
    wanted=prompt
    ram=8
    case $scenario in
        # Real firmware RAM profiles below the 8 MiB hardware contract
        # (memory_map.h: usable RAM must reach EXT_APP_CEIL 0x800000, i.e.
        # 7168 KiB above the 1 MiB line). Stage2 must refuse before the
        # high kernel load; no altered loader or forced report is used.
        RAM5) ram=5; wanted=memrefusal ;;
        RAM7) ram=7; wanted=memrefusal ;;
        ORDER|ATTR_R|CHAIN) build/factory/boot_system "$image" "$scenario"; wanted=refusal ;;
        FRAGMENT)
            build/factory/boot_system "$image" FRAGMENT
            # A second reader proves relocation preserved BOTH file payloads.
            mcopy -i "$image@@32256" ::IO.SYS - > "$out/IO.SYS"
            mcopy -i "$image@@32256" ::INITDOS.SYS - > "$out/INITDOS.SYS"
            cmp "$out/expected-IO.SYS" "$out/IO.SYS"
            cmp "$out/expected-INITDOS.SYS" "$out/INITDOS.SYS" ;;
        normal|indexed|fallback) ;;
        *) echo "ERROR boot: unknown scenario $scenario"; exit 2 ;;
    esac
    for emu in ${4:-qemu bochs}; do
        # QEMU's stock VGA has the preferred 32-bit tier. Bochs' stock VBE
        # advertises 0101h. The ROM independently limits either to 0101h.
        extra=''
        [ "$emu" != bochs ] || extra='--vga-ext vbe'
        [ "$scenario" != fallback ] || [ "$emu" != bochs ] || extra='--vga-ext none'
        # VGA13 is exercised on Bochs, which can disable its VBE extension.
        [ "$scenario:$emu" != fallback:qemu ] || continue
        name=$scenario-$emu
        # Each Bochs termination can leave its data image's lock file. Give
        # every leg its own writable disk rather than deleting source locks.
        data=$out/$name-data.img
        cp "$out/data.img" "$data"
        marker=SHELL-READY
        [ "$wanted" != refusal ] || marker='Replace and strike any key when ready'
        [ "$wanted" != memrefusal ] || marker='Insufficient extended memory: 8 MB required'
        rc=0
        BOOT_INDEXED_ROM=$artifacts/monitor.rom
        [ "$scenario" != indexed ] || BOOT_INDEXED_ROM=$artifacts/indexed.rom
        if [ "$scenario" = indexed ] || [ "$scenario" = normal ]; then
            INITECH_TEST_RAM_MB=$ram PATH="$out/bin:$PATH" "build/factory/${emu}_harness" \
                --disk "$image" --disk2 "$data" $extra \
                --expect "$marker" --quit-after "$marker" --timeout-ms 8000 \
                --name "$name" --out "$out" > "$out/$name.stdout" 2> "$out/$name.report" || rc=$?
        else
            INITECH_TEST_RAM_MB=$ram "build/factory/${emu}_harness" --disk "$image" --disk2 "$data" $extra \
                --expect "$marker" --quit-after "$marker" --timeout-ms 8000 \
                --name "$name" --out "$out" > "$out/$name.stdout" 2> "$out/$name.report" || rc=$?
        fi
        report=$out/$name.report
        serial=$out/$name.txt
        if [ ! -f "$out/$name.serial" ]; then
            cat "$report"; echo "ERROR boot $name: no serial capture"; exit 2
        fi
        tr -d '\r' < "$out/$name.serial" > "$serial"
        # Launch/lock/port errors and crashes are errors, never mutation kills.
        if ! grep -q 'launched=1' "$report" || ! grep -q 'triple_fault=0' "$report" ||
           { [ "$emu" = qemu ] && ! grep -q 'cpu_reset=2 guest_errors=0' "$report"; }; then
            cat "$report"; echo "ERROR boot $name: emulator fault/launch"; exit 2
        fi
        refused=0; prompt=0
        if grep -Fx 'Non-System disk or disk error' "$serial" >/dev/null &&
           grep -Fx 'Replace and strike any key when ready' "$serial" >/dev/null; then refused=1; fi
        # The IO memory refusal includes HALTED; stage2 has a separate message.
        if grep -Fx HALTED "$serial" >/dev/null; then refused=1; fi
        memrefused=0
        if grep -Fx 'Insufficient extended memory: 8 MB required' "$serial" >/dev/null; then
            refused=1; memrefused=1
        fi
        if grep -Fx SHELL-READY "$serial" >/dev/null; then prompt=1; fi
        if [ "$wanted" = refusal ] && [ "$prompt" -eq 1 ]; then
            echo "RED boot $name: reached InitechDOS prompt; corrupt $scenario must be refused"; failed=1
        elif [ "$wanted" = prompt ] && [ "$refused" -eq 1 ]; then
            echo "RED boot $name: refused a valid 8 MiB disk/memory configuration"; failed=1
        elif [ "$wanted" = memrefusal ] && grep -Fx IOSYS-FOUND "$serial" >/dev/null; then
            # Stage2 found IO.SYS, refused on the firmware RAM report, and
            # never loaded INITDOS or entered IO/PM/kernel. Judged from the
            # serial alone: a harness timeout here only means the stage2
            # refusal marker never came, which is itself the RED evidence
            # when INITDOS-FOUND shows the high load proceeded.
            if [ "$memrefused" -ne 1 ] || [ "$prompt" -eq 1 ] ||
               grep -Eq '^(INITDOS-FOUND|S2|PM|KERNEL|SHELL-READY)$' "$serial"; then
                echo "RED boot $name: missing stage2 memory refusal or loaded INITDOS/entered IO"; failed=1
            else echo "GREEN boot $name: stage2 refused ${ram} MiB firmware profile before INITDOS load"; fi
        elif [ "$rc" -ne 0 ]; then
            cat "$report"; echo "ERROR boot $name: no completed semantic observation"; exit 2
        elif [ "$wanted" = refusal ]; then
            if [ "$refused" -ne 1 ] || grep -Eq '^(S2|PM|KERNEL|SHELL-READY)$' "$serial"; then
                echo "RED boot $name: missing refusal or entered IO/kernel"; failed=1
            else echo "GREEN boot $name: exact Non-System refusal before IO/PM/kernel"; fi
        else
            # Completion must include the ordinary loader and PM handoff.
            for mark in S1 JMP2 IOSYS-FOUND INITDOS-FOUND S2 KLOAD A20 GDT PM LFB OK KHI KERNEL SHELL-READY; do
                grep -Fx "$mark" "$serial" >/dev/null || {
                    echo "ERROR boot $name: incomplete handoff ($mark)"; exit 2;
                }
            done
            mode=''
            case $scenario:$emu in
                indexed:*) mode='VBE-MODE=0101' ;;
                normal:qemu) mode='BPP=32' ;;
                fallback:bochs) mode=VGA13 ;;
            esac
            if [ -n "$mode" ] && ! grep -Fx "$mode" "$serial" >/dev/null; then
                echo "RED boot $name: wrong firmware display selection (want $mode)"
                sed -n '/^VBE-/p; /^VGA13/p' "$serial"; failed=1
            else echo "GREEN boot $name: FAT -> IO -> protected mode -> InitechDOS prompt ${mode:+($mode)}"; fi
        fi
    done
done
exit "$failed"
