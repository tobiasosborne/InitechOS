# Physical system files (initech-kod)

The scope is the physical boot split. `IO.SYS` is the former stage2 BIOS
setup and protected-mode handoff; `INITDOS.SYS` is the unchanged padded flat
kernel. Resident C drivers and SYSINIT remain linked into that flat kernel;
this does not reorganize the kernel's resident C subsystems.

Local ground truth, rendered at 110 dpi and visually inspected:

| Source | Printed page | PDF page | Contract used |
|---|---|---|---|
| IBM DOS 3.30 Technical Reference, 80X0945 | 1-7 | 24 | IBMBIO.COM then IBMDOS.COM in the first two root entries; boot loads the first, then DOS |
| Same | 5-4 | 111 | Boot record, two FATs, root, data area; nonsystem boot reports an error |
| Same | 5-7, 5-8 | 114, 115 | FAT12 next-cluster decoding, cluster-to-sector arithmetic, FF8..FFF EOC |
| Same | 5-11 | 118 | System files marked read-only, hidden, system; bits 01/02/04 |
| Same | 9-6, 9-7 | 393, 394 | Active partition byte 80h; four 16-byte entries at 1BEh; relative start and length |
| MS-DOS 3.3 User's Guide, July 1987 | 119 (SYS) | 204 | IO.SYS then MSDOS.SYS, hidden; DOS 3.3 permits noncontiguous system files |
| Same | 41 | 54 | Exact two-line Non-System / Replace and strike any key when ready text |

The IBM scan and Microsoft scan are in the sibling `initech-manuals-ref/dos33`
reference corpus. These pages were read from the text layer and viewed as scans.
ADR-0003 DEC-01 supplies the Initech names `IO.SYS` and `INITDOS.SYS` and HSR
root slots 0/1; DEC-07 and Appendix B.1 supply the directory/FAT contract;
DEC-08 requires the kernel to stay flat. Appendix A's INT21 interface is
unchanged. Appendix C MSG-DOS-0015 supplies the error's first line. ADR-0001
DEC-01/03 and `spec/memory_map.h` retain the protected flat model and the
384-sector bounce ceiling. PRD 6.1 and 8 specify the serial/emulator oracle.

Contiguous *build output* is the lane's policy, not a claim that DOS 3.3
requires it. The positive fragmented-image test moves BOTH files to remote,
nonadjacent clusters and judges the same bytes with mcopy and fsck.fat before
booting. This distinguishes a real FAT walk from a fixed-LBA or contiguous-run
implementation.

The emitted boot disk is 2944 sectors: 46 complete 2-head/32-sector cylinders.
The active FAT12 BOOT partition occupies sectors [63,2943). The MBR still
loads exactly 16 sectors [1,17) at physical 8000h. Stage2 has an assembly-time
4096-byte bootstrap budget at [8000h,9000h); IO.SYS has a 4096-byte budget at
[9000h,A000h). The total is padded to exactly 8192 bytes. The embedded copy of
IO.SYS serves only old raw diagnostic images with an empty partition table.
A nonempty table selects the FAT contract; missing IO.SYS never selects raw
LBA17. The loader accepts the shipped first-active-FAT12-partition profile;
it is not a general FAT16/multipartition boot manager.

Before executing IO.SYS, stage2 validates both root slots, HSR, byte budgets,
BPB bounds/hidden-sector origin, identical FAT mirrors, complete chains,
exact EOC, cycles and cross-links. INITDOS.SYS must fill the unchanged
[10000h,40000h) bounce. IO.SYS then performs the existing VBE/font/memory
probe, A20/GDT/PE/far-jump and high-kernel copy/handoff. Serial milestones are
preserved; IOSYS-FOUND / INITDOS-FOUND and four-digit hexadecimal cluster
counts precede S2. No protected-mode transition is attempted on failure.
The period error is emitted once to serial and BIOS text mode, then CLI/HLT.

The host and injected-QEMU gates were authored and run before changing the
loader: RED `FAT boot partition missing`, RED `normal missing IOSYS-FOUND`.
The host oracle links no artifact loader code. It checks raw slots/attributes/
chains, then mdir/mattrib/mcopy/cmp and fsck.fat independently judge the very
same image, for both secondary-shell and shipped desktop kernels.

Rule-6 mutants ORDER, ATTR_R and CHAIN perturb the images and go RED in the
host oracle. NASM BOOT_MUT_ORDER, BOOT_MUT_ATTR_R and BOOT_MUT_CHAIN separately
remove the matching real-loader invariant. Each boots its fault to KERNEL;
the unchanged negative oracle goes RED for missing Non-System text. The
production loader halts on all three faults and on a real mtools deletion of
IO.SYS. All new host and emulator gates are in the Makefile TEST_* vectors.
Bochs/86Box validation belongs to the PM's post-merge emulator legs.
