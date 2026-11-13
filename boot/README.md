# InitechDOS bootstrap

`make` builds the MBR, stage2 and IO.SYS into `build/boot/`.
The sole loader is the accepted `reboot/loader-spike` implementation
(24fcd8ba), relocated from os/boot. Stage2 follows the two FAT12 system-file
chains, loads INITDOS.SYS a sector at a time via INT 15h AH=87h to 0x500000,
and validates its K386 header, loaded length and BSS end before entering it.
The limit is physical placement below the kernel stack, not a padded sector
count. IO.SYS retains the VBE selection and VGA13 fallback, font capture,
A20, GDT and protected-mode handoff. Insufficient extended memory now prints
`Insufficient extended memory: 8 MB required` on serial and the screen.

Ground truth: the loader spike and legacy-final bootstrap; their IBM DOS 3.30
TR pp.1-7, 5-4, 5-7..11 and MS-DOS 3.3 User's Guide pp.41,119 references
remain in assembly comments. The memory floor includes the existing native
application window through 8 MiB. QEMU needs `-m 8 -machine acpi=off`;
ACPI reserves part of an 8 MiB machine and cannot satisfy this fixed map.

`make test` checks fixed bootstrap sizes. `make smoke` composes the product
smoke. `make -C boot bigkernel-check` from the root is the tier-2 oversized
kernel proof: a deterministic 128 KiB blob makes INITDOS.SYS exceed the old
196608-byte cap, then QEMU and Bochs must print BIGKERNEL-OK with checksum
00FF0000 after clearing BSS. The target restores the production image at the
end. Its emulator evidence is in build/bigkernel (the initial clean removes
previous build artifacts). The blob and checker exist only in this opt-in
entry object, never in kernel/main.c or the production image.
