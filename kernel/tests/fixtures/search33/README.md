# DOS 3.30 handle-search capture

`matrix.bin` was captured by `probe.asm` under genuine Packard Bell OEM
MS-DOS 3.30 on QEMU, 486, 8 MiB. `version.txt` is that boot's VER output.
No InitechDOS code produces these expected results.

Media: Sys and Util disk 1, SHA-256
`3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30`.
Provenance: legacy `docs/research/sidekick/PROVENANCE.md`, DOS33 row;
https://archive.org/download/packard-bell-ms-dos-3.3/Packard%20Bell%20-%20MS-DOS%203.3%20-%20Sys%20and%20Util.img
Media is external and is never committed or altered.

Reproduce from the repository root:
`sh kernel/tests/fixtures/search33/mint.sh /path/to/media.img /tmp/search33`.
The script builds only our probe, boots disposable media copies and extracts
our output with mtools. No captured binary contains DOS code.

Matrix: all 64 masks 00h..3Fh, each over seven
root entries A0..A6 with attributes 20,22,24,26,10,08,16; each entry has
two bytes, exact name then wildcard extension. Each byte is the returned CF
(0 found, 1 absent), giving 896 comparisons. Masks 08h, 09h, 28h and 29h
search only labels: read-only/archive request bits do not change eligibility.
Label masks combined with H/S/directory bits also include eligible ordinary
entries; all H/S/directory bits on an entry must be requested.

The independent reviewer expanded the original 11-mask probe to all 64
masks and captured this matrix. Re-running the committed mint script on
the pinned genuine media reproduced their bytes exactly before the fix.

SHA-256 matrix.bin:
`dda6fc6ea7e70c6c33bc4a7ce9466aed4ac28e3a4ac525eba7fb6e960a36a0a2`.
SHA-256 version.txt:
`6bc60fc69c431ca4f7a8aee9e3e705c9712c09c876f3877b0c9729b495d0c9d7`.
