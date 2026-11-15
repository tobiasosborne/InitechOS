# Status

Rewritten 2026-11-15 at the close of phase 2. Setup: `docs/SETUP.md`.
Phase 2's briefing (what landed, reviews, open work, next phase) is kept
outside the tree by the orchestrator; this file is the in-tree truth.

## State of main

- Certificate: `make cert CERT_STRICT=1` on 09230b74 (clean, load 4-5) is
  RED at its mutation stage only: clean-test, the full two-emulator smoke
  and repro are green; under the strict runner kernel has 2 surviving
  MKDIR-rollback mutants (initech-xn1ev) and many components report runner
  errors rather than kills (initech-vocfm, fl7ce); self-host was still
  running at writing. Later commits (to 1ec9e363) are not covered.
  CERT_STRICT stays 0 until the strict run is green.
- Last full `make smoke` seen green on BOTH emulators from a clean
  checkout: `aba1918d` (2026-11-14 22:38, default limits, 905 s). The
  QEMU desktop session then outgrew its fixed 90 s deadline after
  8fab6e40 lengthened the Word journey; `09230b74` sizes it from
  measurement (180 s; factory/README.md).
- `make status`: all 20 components imported. The import check now also
  rejects product conditionals gated by macros no build defines; 26 such
  branches existing before the rule are listed as issue-linked exceptions
  in `factory/testlib/fault-branches.known` (initech-yuumo kernel,
  jk1yb dos, ao6o0 flair, a8bhu InitechBase) and must be removed.
- Mutation coverage: no pending patches remain (the 129 pending at the
  start of phase 2 are all proven: DOS 46, kernel 39, FLAIR 16, InitechBase
  28). The shared runner counts a kill only on a semantic assertion seen
  twice; timeouts and startup failures are errors. Under that rule the
  strict cert still reports errors instead of kills in kernel, dos,
  Initech 123, Note Pad, SDK, turbo and factory (initech-vocfm, fl7ce),
  and two kernel MKDIR-rollback mutants survive (initech-xn1ev).
- Self-host: K2 == K3 == D2 proven on QEMU and Bochs at 37c815ad,
  170,607 bytes, SHA-256
  a4ddd89971dfe232ccf0f43602fa5fba33ba4966d0bf7aeb7371aea2701ffe47 (the
  IDE work moved it from 164,263 bytes).
- Host suites at their time budgets under load (defects for their owners,
  not for re-budgeting): InitechWord (29-33 s of 30), Initech 123 (27-32
  s), kernel (under budget after eab97a51 but grows). Bochs drops typed
  characters under heavy load in program smoke (initech-ix9by).

## Rulings in force

Operator (2026-11-13/14), also in CLAUDE.md and GOLDEN-MASTER.md:
- GM2: FAT12 read-write; FAT16 read-only, a FAT16 volume mounts, lists
  and reads, and every write to it is refused before the first sector
  changes (DOS file layer, Finder, INT 13h/26h and the utilities all
  refuse; FAT16 writes are Release 2). FAT12 and the FCB calls TPSREP
  uses stay real: the FAT item expresses the product's accreted
  corporate legacy.
- "Two interfaces" (CLAUDE.md) is an explicit allowed list with two
  temporary exceptions: the resident command processor (until a loaded
  COMMAND.COM, Release 2, initech-1frx2) and the Finder's direct FAT use
  (until its file operations go through DOS, initech-02siz). Four
  crossings outside the list are being retired (initech-q9p80).
- InitechDOS COPY follows real DOS 3.30 as captured, including removing
  the target after a disk-full failure. The graphical FILE COPY never
  destroys its destination on failure or cancel.
- The menu mark is Initech's three-block mark; no Apple glyph remains.
- Delivery (GM8) is by start: a ready-made reproducible fixed-disk image;
  no SETUP and no install diskettes.
- Frame: unrecovered ledger cells are empty ruled cells with the line
  "Blank entries were not recovered from the surviving material.";
  InitechCalc's own menus are in the bar during the copy; InitechPaint is
  shown separately in front with the Photoshop menus; two disk apps are
  never resident at once (Release 2).
- Landing (CLAUDE.md): the agent that made a change lands it on main;
  reviews read what has landed; `make cert` is the backstop.

Advisor, from the golden master: desktop saves keep the previous file
openable under its own name after any reported failure (not crash
atomicity); since the kernel replace primitive (f4cac360, 7mb66) the
Toolbox commits a save in one directory-sector write, and InitechBase,
a DOS program, still reports the backup name in the double-failure case.
Initech 123 protection follows the Lotus captures. InitechBase
`@...SAY/GET` and `READ` are in this master (initech-p3nn0, in
progress). Refused visible commands are implemented when small or
removed (approved split: 26 implement, 325 remove; landed for 123, the
small apps, Word and the Finder). One product image. Turbo keeps 32-bit
Integer.

## Product today

Boots on QEMU and Bochs at 8 MiB to InitechDOS and FLAIR. Nine GUI apps
plus InitechBase and the DOS utilities on the working volume, with
Initech's mark, full single-line Finder labels, app windows under the
single menu bar. The product disk carries TURBO.COM with its source and
HELLO.PAS (`\TURBO`), InitechBase samples and TPS.DBF/TPSPARM.DAT
(`\SAMPLES`); TPSREP, USE/LIST and the Y2K program, and compiling HELLO
work from the shipped disk. FILE COPY stages and commits, shows the
hourglass and honours Cancel/Escape without touching the destination.
InitechCalc's ledger shows honest gaps (treatment A). Fresh screenshots
were taken by hand on 2026-11-14 (outside the tree).

Not yet built: the frame as a live arrangement (GM3; its reference
package and the InitechCalc ledger content are on main, 1f7e16b0,
ee66540c), the rest of the Turbo Initech IDE (GM7; editor, save and error
display are on main; the loader now runs a flat child under a resident
extended parent, da8f453e), InitechBase forms (GM5; work in progress is on branch
`base-forms`, cut off when its agent's session ended), the C:
consolidation and start route (GM8), the manuals.

## Who holds what (one writer per directory)

- Phase 3's orchestrator assigns all directories from 2026-11-15 03:00.
  No phase-2 agent holds a directory. `apps/initechbase/` forms work sits
  unlanded on branch `base-forms`. Shared places (`factory/`, `mk/`, top
  Makefile, `sdk/`) had no holder at the close of phase 2.

## Notes for this week

- QEMU: 486, 8 MiB, ACPI off. Bochs: installed Pentium model, 8 MiB
  (GM1 says a 486; open question for the operator).
- Smoke journeys find Finder targets by name; per-run emulator ports; a
  disk-content change runs a clean `make image` and `make smoke` before
  pushing (both caught regressions on 2026-11-14).
