# Status

Rewritten 2026-11-14 after landing the reviewed InitechDOS fixes, data-loss
captures and Turbo Initech compiler repairs on main. Setup: `docs/SETUP.md`.

## Where we are

The three wave-2 branches are merged: InitechWord, small apps, factory.
Every shipped GUI tenant is built before the disk. Nine GUI executables
launch and exit in app-owned journeys on QEMU and Bochs at 8 MiB.
Independent readers check worksheet, WP5, text, Note Pad/Scrapbook,
settings and ledger saves. Calculator matches its real Mac capture;
Paint has an independent canvas check. InitechBase and Turbo Initech
retain separate sessions on both emulators.

Turbo Initech's wrong-write runtime entry points are removed; both
runtime tables regenerate exactly. Self-host and diverse bootstrap prove
`K2 == K3 == D2`: 163,087 bytes, SHA-256
`7bf6424c74a46106b01f5d75de2c422005a026d2624f1e044e626fc1ae801ff6`.
Corruption, contamination and changed-source controls go red.

This machine's baseline exceeded Word and Turbo host time budgets and
opened `123.MNU` instead of `123.EXE`. Hosted Word checks and the FPC
bootstrap are now faster; no entry timeout or suite budget was raised.
FAT directory insertion is lexical at every depth, independent of host
creation order. `LEGACY_TREE` defaults to `$HOME/Projects/InitechOS-legacy`
and accepts an override, replacing another user's hard-coded host path.
Control Panels uses public FLAIR interfaces and an owner-built Dialog
archive. Fresh compiler outputs build their seed in its declared directory.

## Verification and limits

The reviewed integration passed a complete local `make cert` at
`2281fa64` in 41m21s: clean host tests, both-emulator smoke, reproducible
builds, mutations, selfhost/DDC, oversized kernel and status audit.
All 960 active mutation entries were killed, zero survivors or errors.
Later commits correct documentation and reject an empty mutation inventory
with a red-first regression. The first cert on main passed at `049b6b4a`
in 41m7.69s: all eight stages green, 960 active kills, zero survivors/errors.
Host tests passed in 63.66s and full smoke in 212.12s before the push.
Logs: `build/cert/2026-11-14T005336Z-049b6b4a/`; an external copy is
retained with the landing report. No limit was relaxed and no fix was needed.

The independently reviewed Turbo repairs landed as `9228ccac` and `e717e105`,
rebased onto `d4a0cfad`. Fresh self-host/DDC passed in 146.80s with the unchanged
fixed point above. Worktree `make -j12 test` passed in 62.90s and full
`make smoke` in 333.70s; the fast-forwarded main checkout's host gate passed
in 50.11s. Default Turbo smoke now compares 280 generated executions on QEMU
and Bochs: all 168 extended comparisons plus 112 real flat COM executions,
with independent FPC stdout/files and no budget increase. The new flat-only
runtime fault completes with wrong digits and is caught by execution.

After fast-forwarding the main checkout from `62830873`, `make -j12 test`
passes all 19 registered components in 55.96s, including default factory
verification of the captures and configured reference readers. Full
`make smoke` passes in 314.98s: DOS, programs, nine GUI executables,
Turbo Initech and InitechBase on QEMU and Bochs at 8 MiB. Before landing,
the full host gate also passed with relocated corpora in 84.89s. Fresh
Lotus replay from a nonexistent output directory passes all seven cases;
all 14 WK1 files match the retained captures. A delayed-monitor control
proves the capture driver waits for command completion.

The reviewed candidate's additional console-pair smoke passes four cases,
and `make repro` passes in 84.01s across two clean builds. All four rebuilt
main artifact hashes match those reproducibility hashes; the final review
fixes affect capture tooling only. No budget was relaxed or needed a retry.
Logs survive at `/tmp/integration-next-logs/` and
`/tmp/integration-next-landing-logs/`. This landing has not run a new full cert.

There are still 129 unproved pending patches: kernel 39, DOS 46,
FLAIR 16, InitechBase 28. Default cert reports PASS_PENDING; it is not
full mutation coverage. The audit reports 17 imported and two host-green:
kernel includes a private DOS header; FLAIR desktop retains a product
mutation hook and needs its import check. Boot's optional mutation branches
are outside the registered audit. The legacy whole-desktop pixel comparison
is informational; it does not certify the whole screen.

## Review work and decisions

Today's targeted reviews filed 37 new issues plus six Turbo compiler bugs.
The fix plan starts with trustworthy readback, timeout, nonempty-run and
smoke-coverage gates; then FAT/write protection, stage ownership and handles;
then recovery policy, file conservation and independent execution/ABI oracles.
The six Turbo bugs now have red-first FPC execution differentials for both
compilers: record/var layout, byte-safe character access, for-loop bounds and
endpoints, and single-evaluation seed record copies. Extended and flat native
paths are both exercised; the three restored execution patches are active.

`integration/next` was independently reviewed and fast-forwarded to main
after the two capture-harness findings and the timing nit were fixed:

- `fixes/initechdos` at `18750e4a`, all 17 commits, merged as `1fa2ebc5`.
  FLAIR now checks CREATE EOF, seek/literal read and explicit write-only
  OPEN refusal. SDK fake FIND matches all 896 genuine-DOS attribute/name
  cases; the matrix was independently re-minted here. Factory requires
  `WRITE-READBACK-OK` as well as independent disk bytes. Each follow-up
  has a semantic red-first regression (`95a43cd0`).
- `goldens/dataloss` at `773d45da`, merged as `6593a4df`: real DOS 3.30,
  Lotus R2.2 and WordPerfect 5.1 captures. All 422 pinned files,
  mtools disk readback and independent reader dumps verify; corrupted
  evidence is rejected. Default factory verification is `3ce50293`.
  `abdeb56c` makes that verifier use the configured, preflight-checked
  readers, creates fresh Lotus output directories before compilation,
  and waits for HMP completion. Retained capture bytes are unchanged;
  only helper/documentation hashes were updated. No product data-loss
  policy or behavior was changed.

The sole merge conflict was the research branch's stale STATUS.md; the
current main status was retained before the candidate rewrite. This landing
closes initech-oecdm with guest and factory proof. The ten fully fixed
kernel/DOS issues already closed by the implementer retain that status,
with landing/commit comments. Partial issues and recovery-policy decisions
remain open; the current tracker snapshot is committed with this rewrite.

Operator decisions pending: real DOS 3.3 COPY deletes the target on
failure; WordPerfect 5.1 can leave only `NAME.BK!` after a failed save.
Should InitechOS reproduce those outcomes or protect the user's file?
InitechBase `@GET/READ` was filed as Release 2 although GOLDEN-MASTER
lists it for this release; the operator must resolve that conflict.
Dead commands in the frozen app inventories also need implement/remove decisions.

## Next, in order

1. Repair the reviewed acceptance gates, then the data-loss bugs under the
   agreed recovery policy.
2. Finish kernel/FLAIR desktop imports and boot's audit; prove pending mutants.
3. Initech's menu mark, app windows up 20 pixels, Finder long-label overlap.
4. The film frame: safe FILE COPY and honest gaps in the ledger; do not
   invent missing `TODO_FRAME` cells. Resolve the frozen dead commands.
5. Turbo Initech IDE, runtime encoders and fixed point on Bochs.
6. Install path and a single product disk.

## Notes for this week

- Shared places (`mk/`, `sdk/`, `factory/`, top Makefile) owner:
  released after this landing; currently unassigned.
- Remaining Word/small-app attic journeys and pixel harnesses belong to
  their component/factory importers; publication inventories and manuals
  to the publication importer; composed chrome and menu/panel coverage
  to FLAIR/desktop. Separate hosted TextEdit work belongs to FLAIR.
  No build reads attic. FIND operands and DOS overlay behavior remain open.
- The old deep-path Word List Files failure did not reproduce in integration
  or its deeper mutation paths; it remains tracked without a path waiver.
- QEMU: 486, 8 MiB, ACPI off. Bochs: installed Pentium model, 8 MiB.
  IDE master boots; the slave is working volume A:. All checks run locally.
