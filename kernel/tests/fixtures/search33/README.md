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

Matrix: 11 masks (00,01,02,04,06,08,0A,0E,16,1E,3F), each over seven
root entries A0..A6 with attributes 20,22,24,26,10,08,16; each entry has
two bytes, exact name then wildcard extension. Each byte is the returned CF
(0 found, 1 absent). In particular 08 searches only labels, while mixed
label masks include eligible ordinary entries; all H/S/directory bits on an
entry must be requested. Read-only/archive search bits do not filter.

SHA-256 matrix.bin:
`20f734132fe0cb9a900c7267106545a97ce5b7c24fed14751b3ae79c0738d33a`.
SHA-256 version.txt:
`6bc60fc69c431ca4f7a8aee9e3e705c9712c09c876f3877b0c9729b495d0c9d7`.
