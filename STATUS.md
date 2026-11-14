# Status

Rewritten 2026-11-14 after landing wave 2 on main. Setup: `docs/SETUP.md`.

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
`K2 == K3 == D2`: 163,439 bytes, SHA-256
`08499523b5b74b394a05775008481162948b81054adf989785cee18902d55b3f`.
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
with a red-first regression. The first cert on main is next in this landing.

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
The six Turbo bugs need red-first FPC execution differentials for both compilers.

Two local branches are next to land, neither pushed:

- `fixes/initechdos` at `18750e4a`: 17 reviewed INT 21h and DOS commits.
  Landing must update FLAIR's CREATE/read EOF test with seek/read and
  write-only controls, align SDK fake FIND with the real DOS attribute
  matrix and extension matching, and require `WRITE-READBACK-OK` in factory.
- `goldens/dataloss` at `773d45da`: real DOS 3.30, Lotus R2.2 and
  WordPerfect 5.1 captures for the data-loss repairs.

Operator decisions pending: real DOS 3.3 COPY deletes the target on
failure; WordPerfect 5.1 can leave only `NAME.BK!` after a failed save.
Should InitechOS reproduce those outcomes or protect the user's file?
InitechBase `@GET/READ` was filed as Release 2 although GOLDEN-MASTER
lists it for this release; the operator must resolve that conflict.
Dead commands in the frozen app inventories also need implement/remove decisions.

## Next, in order

1. Land the two local branches above with the three shared-place follow-ups.
2. Repair the reviewed acceptance gates, then the data-loss bugs under the
   agreed recovery policy; add the six compiler execution regressions.
3. Finish kernel/FLAIR desktop imports and boot's audit; prove pending mutants.
4. Initech's menu mark, app windows up 20 pixels, Finder long-label overlap.
5. The film frame: safe FILE COPY and honest gaps in the ledger; do not
   invent missing `TODO_FRAME` cells. Resolve the frozen dead commands.
6. Turbo Initech IDE, runtime encoders and fixed point on Bochs.
7. Install path and a single product disk.

## Notes for this week

- Shared places (`mk/`, `sdk/`, `factory/`, top Makefile) owner:
  the orchestrator session of 2026-11-14, for landing fixes/initechdos; released when it lands.
- Remaining Word/small-app attic journeys and pixel harnesses belong to
  their component/factory importers; publication inventories and manuals
  to the publication importer; composed chrome and menu/panel coverage
  to FLAIR/desktop. Separate hosted TextEdit work belongs to FLAIR.
  No build reads attic. FIND operands and DOS overlay behavior remain open.
- The old deep-path Word List Files failure did not reproduce in integration
  or its deeper mutation paths; it remains tracked without a path waiver.
- QEMU: 486, 8 MiB, ACPI off. Bochs: installed Pentium model, 8 MiB.
  IDE master boots; the slave is working volume A:. All checks run locally.
- This session ran with a Fable advisor setting direction, an Opus orchestrator running the fleet, Sol 6.1 xhigh doing implementation and review, Astra hands-on, Muse Wonder Full for the outside view, and a cap of 1 to 2 percent ahead of pace on every account.
