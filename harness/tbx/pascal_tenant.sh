#!/bin/sh
# harness/tbx/pascal_tenant.sh -- build ONE Turbo Initech FLAIR tenant
# (.EXE) from a composed program, on one compiler rail (factory shell; beads
# initech-tdnl.22 R5.2, initech-tdnl.23 R5.3).
#
#   pascal_tenant.sh <rail> <TPSIN.PAS> <out.exe> <workdir>
#     rail = seed   the C seed cross-compiler on the host (seed/initechc)
#            tps    Turbo Initech (os/tps/tps.pas) built by fpc, run on the host
#            tpsasm <TPSIN.PAS> is ignored; <workdir>/TPSOUT.S is the
#                   assembly Turbo Initech already wrote (on InitechDOS)
#
# The composed program is os/tps/FLAIR.PAS followed by the application body
# (COPY /B FLAIR.PAS+APP.PAS TPSIN.PAS on InitechDOS; `cat` here). The
# compiler writes NASM assembly; it is assembled and linked with the binding
# stub object (seed/rt/flairstub.asm) by os/apps/tbx/tenant.ld TWICE -- at
# base 0 and at the probe base -- and mzlink --reloc-diff DERIVES every
# absolute dword into the InitechMZ relocation table, exactly the C tenants'
# tenant-exe-rules (Makefile): flair_entry at module byte 0 (DEC-08a.2), the
# .bss as e_minalloc, the V1 file cap TBX_TENANT_IMAGE_MAX (64 KiB).
#
# Environment: SEED_BIN TPS_BIN NASM LD OBJCOPY NM MZLINK TENANT_LD
#              FLAIRSTUB_OBJ TENANT_PROBE.
# Deterministic (Rule 11): no dates or paths reach the .EXE bytes.
set -eu
rail=$1; src=$2; out=$3; w=$4
: "${SEED_BIN:?}" "${TPS_BIN:?}" "${NASM:?}" "${LD:?}" "${OBJCOPY:?}" "${NM:?}" \
  "${MZLINK:?}" "${TENANT_LD:?}" "${FLAIRSTUB_OBJ:?}" "${TENANT_PROBE:?}"
mkdir -p "$w"
case "$rail" in
seed)
    "$SEED_BIN" --emit-asm -o "$w/p.s" "$src" \
        || { echo "!!! pascal_tenant: the seed rejected $src" >&2; exit 1; } ;;
tps)
    cp -f "$src" "$w/TPSIN.PAS"
    (cd "$w" && "$TPS_BIN" > tps.log) \
        || { echo "!!! pascal_tenant: Turbo Initech failed on $src" >&2; exit 1; }
    grep -q '^TPS-GEN-OK bytes=[0-9]* labels=[0-9]*$' "$w/tps.log" \
        && ! grep -q 'ERROR' "$w/tps.log" \
        || { grep -E 'ERROR|SKIP' "$w/tps.log" >&2; echo "!!! pascal_tenant: no clean TPS-GEN-OK" >&2; exit 1; }
    cp -f "$w/TPSOUT.S" "$w/p.s" ;;
tpsasm)
    test -s "$w/TPSOUT.S" || { echo "!!! pascal_tenant: $w/TPSOUT.S missing" >&2; exit 1; }
    cp -f "$w/TPSOUT.S" "$w/p.s" ;;
*)
    echo "!!! pascal_tenant: unknown rail $rail" >&2; exit 2 ;;
esac
(cd "$w" && "$NASM" -f elf32 p.s -o p.o)
for b in 0 "$TENANT_PROBE"; do
    "$LD" -m elf_i386 --no-warn-rwx-segments -z noexecstack -e flair_entry -T "$TENANT_LD" \
        --defsym=TBX_BASE="$b" -o "$w/l$b.elf" "$FLAIRSTUB_OBJ" "$w/p.o"
    "$OBJCOPY" -O binary "$w/l$b.elf" "$w/l$b.bin"
done
ent=$("$NM" "$w/l0.elf" | awk '$3=="flair_entry"{print $1}')
[ "$ent" = 00000000 ] || { echo "!!! pascal_tenant: flair_entry at $ent, not module byte 0 (DEC-08a.2)" >&2; exit 1; }
bss=$("$NM" "$w/l0.elf" | awk '$3=="__bss_len"{print $1}')
paras=$(( (0x$bss + 15) / 16 ))
"$MZLINK" "$w/l0.bin" "$out" --reloc-diff "$w/l$TENANT_PROBE.bin" "$TENANT_PROBE" \
    --entry 0 --minalloc "$paras" 2> "$w/mzlink.log" \
    || { cat "$w/mzlink.log" >&2; exit 1; }
sz=$(wc -c < "$out")
[ "$sz" -le 65536 ] || { echo "!!! pascal_tenant: $out is $sz bytes, over TBX_TENANT_IMAGE_MAX (64 KiB)" >&2; exit 1; }
