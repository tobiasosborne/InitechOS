# DOS COPY/DEL data safety - audit K01-K04/K15

Context: branch lane-dos-dataloss, audit pass 5. Operator forbids bd and push;
issue ids in the commits identify the work without tracker writes.

## K01 - initech-vj28

COPY previously compared path text before destructive CREAT. COMMAND.COM now
opens the destination for a read-only identity probe and compares its resolved
parent-directory cluster and physical slot with the source handle. Distinct
empty files (both cluster zero) and identical names in different parents remain
distinct. Probe errors other than absence abort before CREAT.

Reference: Microsoft MS-DOS 3.3 User's Guide and User's Reference, July 1987,
User's Reference p. 50 (PDF p. 135): self refusal and zero-count footer. The
manual's general prohibition plus the resolved path model grounds alias handling;
no live real-DOS alias transcript was captured. Existing MSG-DOS-0020 is reused.

Acceptance: the new injected QEMU script first failed on SELF.BIN bytes changed
with SHELL-DONE and no triple fault. After repair mtools extracted all 8192 and
80928 bytes intact, with root/absolute/drive/dot/parent/CWD aliases refused;
the 114-byte control is intact too. fsck.fat -n is clean. Host integration checks
resolved identity including separate empty entries, with an unchanged disk.
The false-identity host mutant fails resolved entry identity. The existing
paired no-self-guard/no-read-CF shell mutant changes SELF.BIN, so the independent
byte oracle goes RED. Existing tests were not weakened.

Friction: this sandbox denies Unix bind(2); the unchanged test-shell failed
before boot. INITECH_QMP_STDIO=1 selects QEMU's documented -qmp stdio over an
inherited socketpair, using exactly the same key injection and grading. DOS
path/pattern keys add semicolon/slash QKeyCodes. The source default remains
Unix QMP; the environment mode uses no listener or shared port.

Before changes: test-kernel-headroom green, flagship 183848 loaded / 12760 free;
largest existing mutant 184552 / 12056 free (107 kernels).

Evidence: build/dosfix-evidence/ logs and build/dos_safety_*/ stopped images,
commands, serial, mcopy extraction and fsck output (build intermediates).

## K02 - initech-wdzq

fat12_create now rejects an existing directory/volume-label entry before any
chain release or write. COMMAND.COM resolves a directory destination read-only,
appends the source basename under a checked path bound, and applies K01's
identity guard to that final path. Root, dot, nested and trailing-separator
operands are covered; a copy into its own directory is refused.

Reference: Microsoft MS-DOS 3.3 User's Reference pp. 12 (file/directory namespace)
and 51 (animal.typ copied into c:\bigcats); IBM DOS 3.30 Technical Reference
pp. 6-122/6-123 (CREAT creates/truncates files). No catalogue/spec change.

Oracle first: host CREAT test had four nonregular-entry rejection failures;
booted shell audit sequence failed FILLED/SAVE.TXT missing. Green after repair:
whole image byte-identical after rejected CREAT, clean fsck.fat, mcopy sees both
SAVE.TXT and the new README.TXT in FILLED plus files in EMPTY and SUB/DEEP.
Mutants: removing the FAT guard makes CREAT succeed (rejection RED); omitting
directory basename assembly leaves FILLED/README.TXT missing (disk RED), with
the backend guard still protecting the original directory.

Commit restriction: root .git is read-only (index.lock rejected). Per-finding
commits are preserved on lane-dos-dataloss in build/dosfix-commit-store, a local
bare metadata copy with read-only access to the original objects. Original
checkout HEAD is not advanced. A portable bundle is produced at handoff.

## K03/K04 - initech-8uad / initech-p4h7

The common wildcard DEL traversal lost the parent when substituting DTA leaves,
and discarded matches after a 16-name roster. DEL now substitutes the matched
leaf under the original parent path and restarts FINDFIRST after each successful
UNLINK. No fixed roster/cursor survives directory mutation. A repeated first
leaf detects no progress. A failed delete/search prints the locked Access denied
message and enumerates remaining full paths without further mutation.

Reference: Microsoft MS-DOS 3.3 User's Reference pp. 13 (qualified DEL) and 56
(DEL/ERASE selects specified files, wildcard operations). CLAUDE.md Rule 2
requires the explicit failure/remainder report; its exact layout has no manual
transcript here and is a project safety policy, not a claimed DOS golden.

Oracle first: K03 boot RED, root KEEP.TXT missing; K04 boot RED, D017/Z*.TXT still
present. Green: complete paths including drive/absolute/nested CWD/dot/parent;
0/1/16/17/20/129 file directories drained, each KEEP.TXT intact, mtools plus
fsck.fat on stopped images. Parent-loss and cap16 mutants independently RED on
the wrong deletion and seventeenth remainder. The combined audit gate replays
K01/K02/K03 in ONE boot; all three mutants independently fail its disk assertions.
An injected unlink refusal leaves the entire disk unchanged and reports the
first through last of 21 remaining paths. A silent-error mutant fails the
failure diagnostic assertion. The refusal is factory-injected, not real-device
write-protection evidence. No existing tests or locked messages changed.

## K15 - initech-jzhh

DEL/ERASE now asks the existing locked MSG-DOS-0012 before any *.* enumeration,
including drive/path-qualified operands. A single Y/y plus Enter authorizes
removal; N, other keys, an empty line, and a longer response leave the whole disk
unchanged. Y removes all regular matches and preserves child directories.

Reference: Microsoft MS-DOS 3.3 User's Reference pp. 13, 56 (the exact question
and qualified pattern); IBM DOS 3.10 Reference p. 7-78 explicitly says lowercase
y/n and Enter. No message was added or reworded, no deliberate spec change.

Oracle first: K15 negative response RED on whole-disk change. Green: five
negative responses/prompts, raw image cmp unchanged, plus approved qualified
and CWD deletions verified by mtools/fsck.fat with child directories surviving.
Three mutants RED independently: omit prompt and accept all responses each
change the disk on non-Y; refuse Y leaves DELTEST/*.TXT. test-spec remains green.

## Final acceptance and handoff

Observed: make test-unit completed ALL GREEN (374 host gates). Its first run
stopped at the untouched test-dbf-read: external TAX.DBF could not open RW under
the sandbox. Rerun used DBASE3_DECOMP=build/dosfix-evidence/dbase3-ref, with every
copied golden cmp-proven identical before and after; no assertion changed. The
vector retains its existing loud skips (absent copyrighted frame and gated
live-dBASE / unresolved non-DOS-shell checks), not a claim that those ran.

Observed: 51 shell/FAT/COPY/DEL QEMU gates all passed serially, including the
combined audit and all new mutants. Exact verdict lines: build/dosfix-evidence/
emu-verdicts.tsv; individual logs there. Initial parallel emulator attempts hit
QEMU write locks on a shared boot image; final runs are serial. No Bochs process
was started, no foreign process was touched, and no all-of-make-test run occurred.
Existing Bochs/86Box legs for these shell gates are deferred, not proven here.

Headroom: before 107 kernels, flagship 183848 loaded / 12760 free, binding largest
184552 / 12056 free. After 115 kernels, flagship 184072 / 12536, largest 184776 /
11832; test-kernel-headroom green and its boundary mutants green. The actual DOS
shell was also linked against the original command/int21/fat12 sources with the
SAME recipe flags (-Os where the Makefile specifies it) and object order:
99654 -> 100998 loaded, +1344 bytes (below 2 KB), 96954 -> 95610 bytes free.
The earlier ad-hoc measurement without the two -Os flags is explicitly labelled
shell-size-invalid-flags.log and is not acceptance evidence.

Manual grounding: all five requested behaviors were located in the manuals;
from memory: none. Exact historical footer spacing/casing and partial-failure
layout have no real-DOS execution golden here. Existing footer contract is
retained; volume-label CREAT refusal and failure/remainder layout follow the
project's nonregular-entry invariant and fail-loud policy, rather than being
claimed as verbatim DOS transcripts. No spec/dos_messages.json change. Added
source is ASCII, git diff --check clean, final spec and Makefile-vars green.

Source-only observation (not emulator-confirmed or changed): fat12_unlink lacks
read-only and nonregular-entry rejection, while IBM DOS 3.10 Reference p. 7-78
says DEL cannot remove read-only files or subdirectories. This merits separate
tracking; operator forbids bd here. Wildcard COPY/REN, DIR operands and Ctrl-C/Z
were not implemented. No tracker writes and no push.

Original checkout HEAD remains 228c07e because .git/index.lock is read-only.
Four commits on lane-dos-dataloss are in the isolated build/dosfix-commit-store;
a verified portable bundle and format-patch series accompany the working-tree
changes. Import into the original repository remains for a writable session.
