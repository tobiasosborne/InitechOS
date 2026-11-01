# WL-0190 -- Merge-velocity fan-out, InitechBase ship (2026-11-01)

## Context

Operator directive: orchestrate work and prioritise merge velocity, with a time box
that was tightened and ended by the operator ("no new work", then recover and stop).
Resumed from WL-0188. Operator rulings taken this session are listed below.

## What changed (main)

- 6af19619 train: F7b key pacing + QEMU slots + one-producer intermediates; SET FIXED,
  HEADINGS, UNIQUE, MARGIN, PRINT, MEMOWIDTH, HISTORY, DOHISTORY (+ LIST/DISPLAY HISTORY,
  EJECT); numeric display conversion (initech-1x6x2.7).
- 52b6fff3 F4b: verb hooks driven by union-merged per-verb .def rows (initech-1x6x2.4).
- c0359143 shrink 2: five cold functions into overlays, resident -1,824 B.
- 3bc3bd80 W3 forms slice 1: @ TO [DOUBLE], @ CLEAR [TO], bare @, CLEAR + subcommands,
  FORMS overlay, ANSI.SYS ESC[=7l/h (DOS 3.30 TR p. 3-16).
- 579bbe89 recorder lockstep capture (initech-x8gsi.1), three Finder clip fixture values
  (initech-4sny), QEMU drives cache=unsafe.

SAMIR resident 87,616 B, headroom about 6,160 B against the 4,096 B floor.

## Operator rulings

- Kernel cap 169,984 B stays (ADR-0014 formula): buy room with overlays, never edit the number.
- Both DEC-19.3 deviation notes ratified; DEC-19 merge deferred until an overlay lane frees room.
- W2 omissions: implement SET ECHO, SET DEBUG (initech-1x6x2.8), SET STEP after W4.1
  (initech-1x6x2.9); omit CALL, LOAD, IMPORT, EXPORT (initech-1x6x2.10).
- Small lanes: PM reruns gates per merge train; core lanes also per lane.
- Lane clones are archived then deleted (../initech-lane-archive); 93 merged origin branches
  deleted by the operator.

## Why

The merge vector's cold failures were not the make races the bead blamed. Three causes
were found: `rm -rf build` deleting tracked build/README.md; keys typed faster than a
starved guest reads them (KBD-RING-DROP); and the guest's FLUSH CACHE after each write
becoming a host fdatasync, which timed out line-by-line output legs under load (timing
evidence; the fdatasync calls were not traced directly). Before this main itself failed
the vector cold. After: 3 cold -j64 greens with zero timed_out, and vector reruns on main
at 114-119 s.

## Frictions

- Lanes ran test-merge-samir at -j32 simultaneously and each reported "load flake";
  several reds were real starvation. Stagger the vector, or fan out fewer lanes.
- bd does not work in clones (no database): lanes silently lost bead notes. Use BEADS_DIR
  and --append-notes; --notes overwrites. Numfmt's clone-filed beads were never in the
  tracker; refiled as initech-1x6x2.16 and .17, and its beads commit was left out of main.
- Closing an epic's only open child auto-closes the epic (initech-x8gsi); reopened.
- The auto-mode classifier refuses agent remote-branch deletion and refused a subagent
  deleting sibling clones.
- A top-level `make -j` over several kernel gates races on build/transient_imports.rsp.
- One lane agent re-sent the same report many times while a background job drained.

## Stopped mid-flight

- Kernel overlays Lane 2 (origin/lane/kov2-size ee2cd1f7, on origin/lane/kov2-train): see
  Acceptance. Stub pruning gets the largest kernel to 169,892 B, 92 B under the cap.
- lane/f2b 71fd1d7c (initech-1x6x2.2): UNVERIFIED, do not merge as is; it removes a lock
  that was hiding the input-emu pair's failure. Re-judge now that cache=unsafe is in.
- lane/phaseA-dec19: ratified, deferred; +1,152 B will not fit under the cap yet.
- wip/f7b-testpar-stack 3f80dd4b: UNVERIFIED test-par commits; test-par has not been run.
- W3 slices 2-4 (GET, READ, PICTURE/FUNCTION/RANGE, CLEAR GETS) not started.

## Acceptance

Each merge reran test-merge-samir on merged main, plus targeted gates named in each merge
commit: green, except c0359143, where dml-memory-emu timed out once in the vector and then
passed 3/3 serially, before cache=unsafe landed. The vector has had zero timeouts since.
record-flair-repro: 37/37 on 579bbe89 with the clips script's three fixture overrides, now
committed values. Not run this session: full `make test`, 86Box, full pubs typeset, test-par.
No gate pins cache=unsafe; the extreme CPU-starvation repro still goes red.

## Pointers

- Beads: initech-1x6x2 (ship), initech-x8gsi (overlays), initech-p3nn0 (W3); follow-ups
  initech-1x6x2.1, .8-.18, initech-fzsvo (P1), initech-fiwr9, initech-z1gxb.
- Remaining PURE SET: CATALOG, DEFAULT, FIELDS, PATH, PROCEDURE, VIEW. W2b verbs are unblocked.
