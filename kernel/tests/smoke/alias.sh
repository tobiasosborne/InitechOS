#!/bin/sh
# Only the ordinary disk client executes in the product kernel. The faulted
# host operation prepares a real retained-alias image; mtools grades the bytes.
set -eu
root=$(cd "$1" && pwd)
mkdir -p "$2"
parent=$(cd "$2" && pwd)
out=$(mktemp -d "$parent/run.XXXXXX")
echo "Alias smoke evidence: $out"
cd "$root"
export LC_ALL=C TZ=UTC SOURCE_DATE_EPOCH=315532800 INITECH_QMP_STDIO=1 INITECH_TEST_RAM_MB=8
nasm -f bin kernel/tests/programs/alias_program.asm -o "$out/ALIAS.COM"
printf 'independently minted replacement bytes\n' > "$out/new"
printf 'previous named bytes\n' > "$out/old"
printf '@ECHO OFF\r\nALIAS\r\nECHO ALIAS-END\r\n' > "$out/AUTOEXEC.BAT"
truncate -s 1474560 "$out/base.img"
mformat -i "$out/base.img" -f 1440 ::
mcopy -i "$out/base.img" "$out/ALIAS.COM" ::ALIAS.COM
mcopy -i "$out/base.img" "$out/AUTOEXEC.BAT" ::AUTOEXEC.BAT
mcopy -i "$out/base.img" "$out/new" ::STAGE.BIN
mcopy -i "$out/base.img" "$out/old" ::DEST.BIN
build/kernel/host/tool_test_commit_outcome "$out/base.img" replace root 1 2 0
mcopy -i "$out/base.img" ::DEST.BIN "$out/committed"
cmp "$out/new" "$out/committed"
for emu in qemu bochs; do
    dir=$out/$emu
    mkdir -p "$dir"
    cp "$out/base.img" "$dir/data.img"
    "build/factory/${emu}_harness" --disk "${ALIAS_SMOKE_BOOT:-build/image/boot.img}" \
      --disk2 "$dir/data.img" --out "$dir" --name result \
      --expect ALIAS-END --quit-after ALIAS-END --timeout-ms 60000 > "$dir/driver.log" 2>&1
    tr -d '\r' < "$dir/result.serial" > "$dir/transcript"
    grep -Fx ALIAS-GUARD-OK "$dir/transcript"
    if grep -E 'ALIAS-GUARD-FAIL|PANIC' "$dir/transcript"; then exit 1; fi
    grep -F triple_fault=0 "$dir/driver.log"
    cmp "$out/base.img" "$dir/data.img"
    mcopy -i "$dir/data.img" ::DEST.BIN "$dir/destination"
    cmp "$out/new" "$dir/destination"
    echo "ok $emu retained alias: CREATE, truncate, overwrite, extend refused; whole-image identity"
done
