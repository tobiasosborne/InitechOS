# DOS file protection - audit L005/L006

Context: lane-dosprot, initech-1vcl and initech-1nsj. The operator forbids bd,
push, Bochs, and writes to the original Git metadata. Commits use the metadata
copy in build/git-dosprot; the final branch is delivered as build/lane.bundle.

What changed: fat12_create refuses an existing read-only entry before freeing
its chain or changing the directory entry. fat12_unlink refuses read-only,
directory, and volume-label entries before any mutation. The same attribute
protects whole-file and positioned writes, and INT21 OPEN refuses output access
to protected files. Positioned writes reread the attribute, so a handle opened
before a lock cannot bypass it. COMMAND.COM maps access failures to the existing
MSG-DOS-0009 for COPY and plain DEL. Wildcard DEL skips protected matches without
mutation, restarts FINDFIRST after each successful deletion, and reports the
protected remainder once. The backend's indices count surviving entries.

Why: local MS-DOS_3.3_Users_Guide_198707.pdf, User's Reference printed p. 33
(PDF p. 118) says read-only prevents deletion or modification; printed p. 308
(PDF p. 393) supplies Access denied. IBM
80X0945_DOS_3.30_Technical_Reference_Apr87.pdf pp. 5-10/5-11 and 6-141 ground
attributes, refusal of output OPEN, and protected UNLINK; pp. 6-122/6-123 ground
CREAT. spec/dos_messages.json is reused without modification.

Acceptance: new host INT21 tests first failed four protected CREAT checks,
four protected UNLINK checks, four nonregular UNLINK checks, and the write/OPEN
checks. Booted COPY and DEL first failed whole-image comparison. After repair,
the root/nested, empty/multicluster refusals leave the entire disk byte-identical
(including metadata and both FATs). Explicit mtools -R controls permit COPY;
mixed wildcard DEL removes writable matches while preserving protected bytes
and attributes. mtools and fsck.fat judge the stopped guest disks. A late-lock
AH=40h check extends the positioned-write coverage.

Mutants: CREATE_READONLY and UNLINK_READONLY remove their guards (host refusals
fail; guest whole-image comparison fails); UNLINK_NONREGULAR permits directories
and labels (host nonregular refusal fails); WRITE_READONLY permits positioned
and whole-file writes; OPEN_READONLY permits write-mode opens. Every mutant has
a named RED assertion in the registered host/emulator gates. No oracle was
weakened. All new source is ASCII. No spec-data change belongs to this commit.
The STOP_PROTECTED shell mutant additionally fails the writable LOCK0.TXT
control, proving that a protected match cannot hide writable ones.

Frictions: changing DEL to resume FINDNEXT after mutation skipped alternating
matches. Existing K04 (16/17/20/129 matches), audit replay and K15 caught it.
Restoring the restart after deletion fixes the actual surviving-entry index
semantics while retaining the repeated-first success-without-removal detector.
The failed runs are preserved as initial-g-* logs and final replays pass.

Pointers: harness/diff/fat_diff/dos_safety.sh; os/milton/test_dos_safety.c;
build/dosprot-evidence/{protection-final,unit}.log; build/dos_safety_*/ disks,
commands, serial, reference bytes and fsck output. Final lane-wide acceptance
and size measurements are recorded in WL-0100.
