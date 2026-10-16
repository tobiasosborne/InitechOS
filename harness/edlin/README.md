EDLIN oracle (initech-udr1)
===========================

EDLIN.COM is a freestanding flat C program. DEBUG is outside this lane.

Local sources inspected as page images at 110 dpi:

* `InitechOS-PRD.md` section 6.1, and
  `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md`, DEC-08 / Appendix A.
* `/home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf`:
  printed pp.162-193 = PDF pp.247-278; Appendix F p.313 = PDF p.398 (reserved .BAK filename refusal).
* The same directory's `80X0945_DOS_3.30_Technical_Reference_Apr87.pdf`:
  printed pp.6-11--6-12 = PDF pp.134-135 (00,scan extended-key input).

Manual behaviour map:

| Behaviour | Printed pages |
|---|---|
| New/existing banners, numbering, save/backup | 162-164, 179 |
| Template F1/F2/F3/Del/F4/Esc/Ins/F5/Backspace | 164-169, 177-178 |
| Command chaining, relative addresses, '.', '#', /B, Ctrl-V | 170-173 |
| A, C, D | 174-176 |
| I, L, M, P, Q | 180-186 |
| R (every occurrence, confirmation, remembered strings) | 187-189 |
| S, T, W | 190-193 |

The replacement rule on p.187 governs. The two truncated p.188 lines have
explicit PM-precedence comments in model.c. They are never truncated goldens.

The independent host model uses a heap vector of lines. The artifact uses a
packed bounded arena and streaming DOS handles; no implementation source or
expected-output routine is shared. The literal complete printed-file results
on pp.178 and 184 are independently checked and emitted as MANUAL.TXT. Every
script has a page citation in model.c. The staged case crosses the 3/4 memory
boundary and checks numbered lines after 125W/100A.

`test-edlin-host` replays the same raw key scripts against the native artifact
adapter. `test-edlin-emu` mints a FAT disk, copies it unchanged for the mtools
reference, boots the secondary shell, and injects the same keys through PS/2.
It compares the entire redirected console transcript and saved/backup bytes,
checks names against independent mcopy/mren/mdel operations, and runs fsck.fat.
DOS-qualified A:\SUB paths and a read-only BAK have separate controls.
Unknown/truncated injections, incomplete boots, and filesystem damage fail.

`test-edlin-host-mutant` and `test-edlin-emu-mutant` compile four artifact
variants: NUMBER (number + 1), BACKUP (discard original), FIRST (first R hit
only), QUIT (omit Abort prompt). Each must fail the corresponding transcript,
backup, output, or transcript comparison, after execution completed.
All four gates are members of TEST_UNIT_GATES / TEST_EMU_GATES.

RED before implementation: test-edlin-host reported
`RED EDLIN implementation missing` / `RED EDLIN-execution case=new`.
The emulator gate initially failed its missing edlin_start.asm prerequisite.
All QEMU runs export INITECH_QMP_STDIO=1. No Bochs or aggregate test run.
