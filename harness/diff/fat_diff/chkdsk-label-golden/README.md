# CHKDSK label classification: genuine DOS 3.30

Bead initech-ug8l is a suspected defect, refuted by this independent mint.
MS-DOS 3.3 User's Reference p.43 (PDF 128) defines the report categories.
IBM DOS 3.10 Reference p.7-50 explicitly includes the volume label among
hidden files. The 3.30 executable confirms that this convention persists:

| Fixture | Hidden bytes | Hidden count |
|---|---:|---:|
| none | 0 | 0 (category omitted by DOS) |
| label only | 0 | 1 |
| hidden data file only | 512 | 1 |
| both | 512 | 2 |

The four `.txt` files are the actual redirected reports, with CR removed
and every other byte retained. They were produced by genuine DOS, before
any publication capture was refreshed. `VER` returned `MS-DOS Version 3.30`.
The data disk is FAT12, 1.44 MB, 512 bytes per cluster. HIDDEN.TXT contains
`fixture` followed by CR LF (9 bytes), and has the hidden attribute.
The label is CHKTEST, a root-directory entry with no data cluster.
Initech's report always prints the empty hidden category, so the focused
gate compares the count/allocation while the existing oracle checks exact
Initech report formatting and every other report category.

Reference media: local library
`initech-manuals-ref/sidekick/dosboot/PB_MSDOS33_Sys_and_Util.img`, described
by the adjacent `PB_MSDOS33_Sys_and_Util_ImagingLog.txt` (360K DSDD).
SHA-256 of media:
`3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30`.
SHA-256 of its CHKDSK.COM (9,819 bytes):
`e792fc0125287710d635cc372914aaf1040f94521aab627d221e9ef0b2ed8c0b`.
The original media is never written or added to version control; disposable
boot copies stay under the ignored build directory.

Reproduce with QEMU, NASM and mtools:

```sh
sh harness/diff/fat_diff/chkdsk_label_mint.sh \
  /path/to/PB_MSDOS33_Sys_and_Util.img build/chkdsk-label-mint
```

`chkdsk_label.sh` compares the artifact with these fixed reference reports.
CHKDSK_MUT_LABEL drops the volume-label increment: the host and emulator
mutant families must reject it. Do not regenerate these goldens from
InitechDOS output.
