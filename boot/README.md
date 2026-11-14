# InitechDOS bootstrap

Dependencies: kernel (generated DOS message catalogue); factory (public host
assertions, image builder and emulator drivers).

`make` builds the MBR, stage2 and IO.SYS into `build/boot/`. The MBR
loads 16 sectors at physical 0x8000. Stage2 requires IO.SYS and INITDOS.SYS
in the first two root slots with hidden/system/read-only attributes, follows
their FAT12 chains, and refuses inconsistent mirrors, premature/late EOC,
cycles, overlap and out-of-range clusters. It loads IO.SYS at 0x9000 and
INITDOS.SYS through INT 15h AH=87h at 0x500000, validating its K386 header,
length and BSS end against the reserved memory ceiling. IO.SYS captures
the font, probes RAM, enables A20, loads the GDT and enters protected mode.

Ground truth: the accepted loader spike (`24fcd8ba`) and legacy-final;
IBM DOS 3.30 Technical Reference printed 1-7 (root order), 5-4 (volume),
5-7..11 (FAT12 chains/HSR); MS-DOS 3.3 User's Guide printed 41 (refusal
text), 119 (SYS order, attributes, fragmentation permitted). Video:
VBE Core 2.0 revision 1.1, printed 6, 13-14, 16-20, 25, 30;
`docs/research/vesa8-ground-truth.md` retains the original acquisition.
The preference is 640x480 at 32, then 24, then indexed 0101h; VGA13 is
the floor. The hardware contract is an 8 MiB machine. QEMU requires
`-m 8 -machine acpi=off` because ACPI reserves memory in the fixed map.
Assembly comments retain the BIOS register contracts and source references.

`make test` builds all three target artifacts, runs an independent binary
layout reader (sizes, signature, empty partition-table area and fixed load/
entry addresses), and runs the shared `mk/import-check.mk` audit over the
actual build graph. No deliberately wrong or scenario-selected assembly
lives in product source. The audit also rejects the retired memory/VBE
scenario identifiers; this adds restrictions to the shared default.

After `make image` at the root, `make -C boot smoke` checks normal boot,
indexed VBE, Bochs VGA13, invalid order, missing read-only attribute,
premature kernel EOC and fragmented valid files. Both emulators must reach
the ordinary InitechDOS shell on valid disks. Invalid disks must print the
exact Non-System refusal before entering IO.SYS or protected mode. mtools
independently recovers both fragmented payloads and compares them to the
product artifacts. Each run snapshots its input payloads before minting so
a concurrent root rebuild cannot change the reference during a lock wait.
Every run has private image copies and retained
transcripts/reports under `build/boot/smoke.*/`. QEMU always uses
`-d int,guest_errors,cpu_reset`; faults and extra resets fail.

The test-only PC expansion ROM in `tests/` observes the actual BIOS
descriptor selected by the loader. Its indexed variant rejects non-0101h
mode queries, then delegates all supported operations to the real VGA BIOS.
It changes firmware offers without changing the product disk or loader.
It is injected using QEMU's option-ROM facility or Bochs' optromimage1.
The ROM format/call contract is independently defined by SeaBIOS
[`is_valid_rom`/`callrom`](https://github.com/coreboot/seabios/blob/master/src/optionroms.c)
and the Bochs BIOS
[`rom_scan`/`rom_checksum`](https://github.com/bochs-emu/Bochs/blob/master/bochs/bios/rombios.c).
No ROM, fixture program or altered loader ships in the product image.

`make -C boot mutants` applies each external patch in a private copy,
requires successful assembly and a clean baseline, then repeats the same
emulator observation twice. A kill requires a stable semantic RED
(prompt reached on invalid media, refusal of valid memory/media, or wrong
firmware display selection). Launch errors, crashes and timeouts without
such evidence are errors. All five former mutation hooks and the four
former memory/VBE diagnostic configurations are represented. The fixed
memory-report patch uses the former boundary value 7167 KiB. Per-patch
build and red logs survive in `build/boot/mutants.*/`.

`make -C boot bigkernel-check` is the existing tier-2 oversized-kernel
proof: a deterministic 128 KiB blob makes INITDOS.SYS exceed the old
196608-byte cap; QEMU and Bochs must print BIGKERNEL-OK with checksum
00FF0000 after clearing BSS. The target restores the production image.
Its initial clean removes previous build artifacts, so retain any earlier
evidence outside build first. The opt-in blob is absent from the product.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: startup
<!-- end-interface-declarations -->
