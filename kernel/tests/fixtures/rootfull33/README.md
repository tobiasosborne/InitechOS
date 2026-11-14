# DOS 3.30 full-root errors

`result.bin` was captured by the phase-2 kernel reviewer on 2026-11-14
using the adjacent `probe.asm` on genuine Packard Bell OEM MS-DOS 3.30
360 KB System and Utilities disk 1. The reference disk SHA-256 is
3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30.
No system binary is distributed here.

Source: phase2/review-kernel.md and phase2/kernel-repro/capture-rootfull.sh
in the orchestrator's retained reports. The probe fills all 224 root slots
on a 1.44 MB data disk, leaving data clusters and handles available, then
calls CREAT (3Ch) and CREATE_NEW (5Bh). QEMU exits 33 via its DOS exit helper.
The four little-endian words are AX, FLAGS for each call: 0005 0247 0005 0247.
The test compares AX and carry, not undefined/irrelevant FLAGS bits.

SHA-256 of result.bin:
df02323a04f9094c40e4ae4b5f5310ea0d3432b5633bb4e78c3c0defe4057c1c
