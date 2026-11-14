# Prompt's final cell

Captured from genuine dBASE III PLUS 1.1 on DOS 3.30 before adding the
oracle. Runtime/media hashes are in runtime.sha256; SHA256SUMS pins inputs
and observation bytes. The independent goldens/dataloss/drive.c ran a 486,
8 MiB, fixed RTC 1992-03-02T09:30:00 and fresh boot of the reference media.
The disposable capture directory is recorded in the Phase 2 report.

SETUP/TAIL.PRG was installed as MINT.PRG. It clears the page, writes Z at
(24,79), positions the cursor at row 5 with PROMPT-READY, then returns to the
dot prompt. STEPS observes the prompt before typing QUIT. `prompt.vga` is
the actual 4000-byte text plane. `cursor.bin` is page zero's BIOS cursor
at physical 450h (column, row), captured after the settled prompt. The
screen_tool encoder combines those unchanged cell/attribute bytes with the
cursor in row,column order to produce tail.screen. No InitechBase output
supplies an expected value.

The real prompt cursor is (24,2). Row 24 contains `. `, 77 spaces and Z in
column 79: prompt clearing touches 79 columns, preserving the last cell.
The host test executes the actual source, screen policy and prompt writer
and compares all 4002 bytes. Its clear-80 mutation must fail at that Z.

For replay, use the fresh-media setup in ../conformance/capture.sh, install
SETUP/TAIL.PRG as MINT.PRG, use STEPS with @OUT@ replaced by the absolute
capture directory, then run goldens/dataloss/drive. Encode the 4000 VGA bytes
followed by cursor.bin byte 1 then byte 0 with `screen_tool encode`.
