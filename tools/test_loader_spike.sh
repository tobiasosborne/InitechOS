#!/bin/bash
# Reproduce the chunked-loader spike's boot, bounds, checksum and rebuild proofs.
# All emulator artifacts stay under the ignored build/ directory. No bd or git.
set -euo pipefail
cd "$(dirname "$0")/.."
. tools/lane-env.sh
for corpus in SYSTEM7 WIN31 DBASE3 LOTUS123 WORDPERFECT51; do
    var=${corpus}_DECOMP
    name=${corpus,,}
    export "$var=${!var:-$HOME/Projects/$name-decomp}"
done
make -j32 BIG_KERNEL=0 BUILD=build factory image build/qemu_harness_mut_keep_log
make -j32 BIG_KERNEL=1 factory image
big=build/loader-big
# Same fault oracle, with only successful-log deletion disabled, so the
# -d int,guest_errors,cpu_reset evidence remains available for inspection.
qemu=build/qemu_harness_mut_keep_log
bochs=build/bochs_harness

# Use pristine secondary volumes for every desktop run. Both harnesses log
# faults; QEMU enables -no-reboot -d int,guest_errors,cpu_reset itself.
for variant in normal big; do
    dir=build
    [ "$variant" != big ] || dir=$big
    for emu in qemu bochs; do
        label=loader-$variant-$emu
        cp -f "$dir/flair_data.img" "$dir/$label-data.img"
        extra=()
        [ "$emu" != qemu ] || extra=(--screendump)
        "${!emu}" --disk "$dir/initech.img" --disk2 "$dir/$label-data.img" \
            --expect FLAIR-LIVE-OK --quit-after FLAIR-LIVE-OK "${extra[@]}" \
            --name "$label" --out "$dir" --timeout-ms 45000 \
            > "$dir/$label.stdout" 2> "$dir/$label.report"
        grep -F 'OK=1' "$dir/$label.report"
        grep -x 'FLAIR-LIVE-OK' "$dir/$label.serial"
        if [ "$variant" = big ]; then
            size=$(wc -c < "$big/INITDOS.SYS")
            [ "$size" -gt 196608 ]
            grep -x "BIGKERNEL-OK size=$size sum=00FF0000" "$dir/$label.serial"
        fi
    done
done

# No AUTOEXEC on the shell disk: accept the DOS date/time prompts. Bochs's
# line-exact oracle needs a third Return to terminate the first A:\> line.
for emu in qemu bochs; do
    keys=ret,ret
    [ "$emu" != bochs ] || keys=ret,ret,ret
    label=loader-shell-$emu
    "${!emu}" --disk build/tracer_boot.img --expect 'A:\>' --quit-after 'A:\>' \
        --keys-after SHELL-READY --keys "$keys" --name "$label" --out build \
        --timeout-ms 45000 > "build/$label.stdout" 2> "build/$label.report"
    grep -F 'OK=1' "build/$label.report"
    grep -F 'A:\>' "build/$label.serial"
done

# Deliberately bypass the link bound with an oversized FILE, then a valid-size
# file whose header claims out-of-window BSS. Neither may reach IO or the kernel.
cp -f build/INITDOS.SYS build/loader-oversize.sys
truncate -s 524289 build/loader-oversize.sys
python3 - <<'PY'
from pathlib import Path
import struct
p = bytearray(Path('build/INITDOS.SYS').read_bytes())
struct.pack_into('<I', p, 10, 0x580001)
Path('build/loader-bss-overflow.sys').write_bytes(p)
PY
for negative in oversize bss-overflow; do
    sh tools/mk_boot_system.sh "build/loader-$negative.img" build/mbr.bin \
        build/stage2.bin build/IO.SYS "build/loader-$negative.sys" \
        build/boot_system build/transient_flairtenants.bin \
        build/flairovl_kernel_flairtenants.ovl
    "$qemu" --disk "build/loader-$negative.img" --expect ERR-KERNEL-SIZE \
        --quit-after ERR-KERNEL-SIZE --screendump \
        --screendump-after 'Replace and strike any key when ready' \
        --name "loader-$negative" --out build --timeout-ms 10000 \
        > "build/loader-$negative.stdout" 2> "build/loader-$negative.report"
    grep -F 'OK=1' "build/loader-$negative.report"
    grep -x ERR-KERNEL-SIZE "build/loader-$negative.serial"
    grep -F 'Non-System disk or disk error' "build/loader-$negative.serial"
    ! grep -qx KERNEL "build/loader-$negative.serial"
done

# Make sure the proof's failure marker is live: alter one byte of its actual
# high-address blob, retaining the original file length and boot header.
python3 - <<'PY'
from pathlib import Path
import subprocess
p = bytearray(Path('build/loader-big/INITDOS.SYS').read_bytes())
syms = subprocess.check_output(['nm', 'build/loader-big/kernel_flairtenants.elf'], text=True)
addr = next(int(s.split()[0], 16) for s in syms.splitlines() if s.endswith(' bigkernel_blob'))
p[addr - 0x500000 + 65537] ^= 1
Path('build/loader-bad-blob.sys').write_bytes(p)
PY
sh tools/mk_boot_system.sh build/loader-bad-blob.img "$big/mbr.bin" \
    "$big/stage2.bin" "$big/IO.SYS" build/loader-bad-blob.sys \
    build/boot_system "$big/transient_flairtenants.bin" \
    "$big/flairovl_kernel_flairtenants.ovl"
"$qemu" --disk build/loader-bad-blob.img --expect BIGKERNEL-BAD \
    --quit-after BIGKERNEL-BAD --screendump --name loader-bad-blob --out build \
    --timeout-ms 10000 > build/loader-bad-blob.stdout 2> build/loader-bad-blob.report
grep -x BIGKERNEL-BAD build/loader-bad-blob.serial
! grep -x FLAIR-LIVE-OK build/loader-bad-blob.serial

# SeaBIOS/QEMU emit two startup CPU Reset records before executing the MBR.
# Reject any additional reset, plus faults and guest errors, on all QEMU boots.
for label in build/loader-normal-qemu build/loader-shell-qemu "$big/loader-big-qemu"; do
    grep -F 'triple_fault=0 cpu_reset=2 guest_errors=0' "$label.report"
    [ "$(grep -c 'CPU Reset' "$label.qemu.log")" -eq 2 ]
    ! grep -Ei 'triple fault|check_exception old: 0x8' "$label.qemu.log"
done

# Rebuild the complete normal and big factory products from source. Fixed
# timestamps and byte-identical kernels/boot/data volumes are required.
sha256sum build/{initech.img,flair_data.img,INITDOS.SYS} \
    "$big"/{initech.img,flair_data.img,INITDOS.SYS} > build/loader-repro.sha256
make -B -j32 BIG_KERNEL=0 BUILD=build factory image > build/loader-rebuild.log 2>&1
make -B -j32 BIG_KERNEL=1 factory image > "$big/loader-rebuild.log" 2>&1
sha256sum -c build/loader-repro.sha256
printf 'LOADER-SPIKE-PASS\n'
