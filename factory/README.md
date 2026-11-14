# Local product factory

emu/ owns the C QEMU and Bochs drivers claimed from attic/harness/emu.
image/ owns the legacy FAT boot-volume builder and its independent C layout
checker, plus product assembly. `make` builds host tools into build/factory;
`make image` assembles already-built components; `make smoke` drives the actual
product disk set, copies the writable disk per run, checks serial lines and
uses mtools to read written bytes independently. All factory programs are C,
make or POSIX shell. No target test personalities or remote automation.

Tier 0 rejects host-specific home paths in factory shell scripts. parity.sh
uses smoke-desktop.sh's LEGACY_TREE default ($HOME/Projects/InitechOS-legacy),
with INITECH_LEGACY retained as its first-choice override.

QEMU smoke uses 8 MiB, CPU 486, acpi=off and -d int,guest_errors,cpu_reset.
Two power-on CPU Reset records are required; any additional reset, triple
fault cascade or guest error fails. Full logs and stderr survive success.
Bochs retains the legacy BIOS/RFB driver and key pacing, 8 MiB and its installed
pentium CPU model (this distro's Bochs build does not offer a 486 model).
The DOS session disables its VBE extension to require VGA13 fallback; the
desktop session uses the normal LGPL BIOS with extension=vbe and mode 0101.
Run logs/config/transcripts are in build/smoke. The Bochs driver serializes
host access to its fixed RFB port. run-bochs is a serial-emitting harness run;
interactive graphical operation is available with the generated bochsrc.

session.txt is the shared keyboard session. keys.c maps text to QMP tokens.
Bochs's shorter VER/DIR session uses its RFB key vocabulary. Source behavior is
the frozen legacy shell and the period references in each imported driver.
The smoke checks are literal independent expectations, not computed from the
shell implementation. The CHKDSK and file checks fail if the program never
returns, even if keyboard echo includes the final command. repro.sh builds
from clean twice and compares boot/data/kernel SHA-256. bigkernel-check.sh
checks the accepted loader spike's independent 128 KiB checksum on both CPUs.

The legacy harnesses retain optional GUI input, capture, pacing and fault-test
support for future desktop smoke imports. Their old factory self-test gates
were not imported. Add focused driver tests when changing these features.

The completion-only QEMU path now opens QMP for --quit-after even when there
are no injected keys or screenshots. This is exercised by the oversized and
low-memory checks; the legacy wrapper otherwise timed out despite receiving
the expected serial line.

smoke-programs.sh is the second session, invoked by `make smoke-programs` and
by `make smoke`. dos/tests-programs.mk assembles normal INT21 probes into
build/dos/test-programs. A separate mtools-minted data disk carries them; the
product disks contain none. Both emulators compare literal service results,
the real-DOS FCB captures and persisted files. Bochs uses its existing wait
steps between commands so disk-heavy probes do not overflow the keyboard ring.
The exact WRITE-READBACK-OK line is required in addition to failure-marker
rejection and independent OUT.TXT byte comparison. `make smoke-write-readback`
rejects a disposable WRITE probe that writes/closes correctly and exits before
READ, while independently proving the persisted literal bytes remain correct.
It requires an existing product image; `make smoke-programs` covers the normal
probe on both emulators. PROGRAM_SMOKE_PROGRAMS selects the disposable probe
directory for this control; its default remains build/dos/test-programs.

Tier 0 also verifies goldens/dataloss: all pinned capture bytes, mtools disk
readback and independent Lotus/WordPerfect reader dumps. These are reference
observations only; DOS failed-COPY and WordPerfect recovery policy still need
the operator's decision before any product behavior changes.

smoke-desktop.sh is the product Finder/application journey on both emulators.
It checks a worksheet through mtools and the independent Lotus corpus reader,
retains key PPMs and compares unchanged regions to a live legacy capture.
red-desktop.sh kills worksheet empty-save, restored-file-cap and Word empty-save patch mutants in isolated
source/build copies, after a passing baseline and two matching semantic kills.
`make -C factory mutants` runs it; no production source is patched in place. Missing
QEMU W progress markers now stop replay rather than continuing input into an
unexpected UI. trace-input-test.c pins the slash/chord RFB transport values.
The image list is system/working-volume.list; AUTOEXEC.BAT ends with FLAIR.

Bochs journey serial waits keep draining RFB framebuffer updates. A blocked
display client can otherwise stop guest drawing before a serial marker is
emitted. desktop-primitives-test.c proves progress requires transport pumping;
ordinary factory tier 0 runs it. Marker and session timeout limits are unchanged.

`make cert` composes clean/test, image/smoke, repro, patch mutation sweeps,
Turbo Initech selfhost/DDC, oversized-kernel boot, status and clean git status.
Every stage runs even after a failure; the final exit is nonzero if any failed.
Mutation workers must exit successfully and produce fresh completion records;
every summary is validated against the discovered patch inventory, with zero
survivors/errors and no omitted entries. Reused cross-component patches are
declared in mutants/extra.list. A killed worker or missing log/result is red.
Logs and per-stage wall times survive clean builds in
build/cert/<UTC date/time>-<short sha>/{summary,full.log}. Every certificate requires a complete, unique imported status row for every
registered component, including at the default CERT_STRICT=0. Standalone
make status remains informational. CERT_JOBS (default 16) and
CERT_MUTANT_JOBS (default 4) bound parallel work. Mutation summaries include
pending patches without counting them as kills. Targets are invoked even for
pending-only owners: unimplemented targets fail. Standalone active sweeps with pending work report incomplete coverage.
Certificates reject pending or missing mutation coverage receipts with
INCOMPLETE and a nonzero result at either CERT_STRICT setting.
The seed's
execution catchers use seed-guest.sh: real product DOS, separate test volume,
FPC Delphi-mode 32-bit integer reference, stdout and persisted-file comparison.

mk/dependencies.mk is the authoritative build dependency graph, displayed by
`make dependencies`. README dependency prose describes the interfaces behind
those edges. The top-level preflight audits actual make graph paths, including
generated headers, against those declarations. The kernel/resident shell and
SDK/FLAIR source-audit cycles have explicit test-order exceptions there; the
component recipes still build their required artifacts.

Gate regressions run in `make test` (`gate-test` alone while iterating).
The worksheet driver checks complete independent Lotus reader records and
separate byte framing, including formula lengths and EOF consumption. Its
test writer uses Initech 123's public worksheet interface and real serializer;
four serializer patches prove wrong values and malformed output are rejected.
Bochs's final verdict rejects deadline expiry even if final serial has all
markers; the deterministic verdict fixture also preserves timely SIGTERM.
Both shared host runners refuse empty selections. Top-level test additionally
requires a positive named execution summary (both children for Turbo, explicit
composition checks for system), besides the child exit status.

Top-level smoke requires the complete component, journey and emulator sets;
Turbo's source/path selectors are checked too. `smoke-selection` checks this
without building or booting. Component-only red runs may select a nonempty
subset, which is labelled SUBSET SMOKE and lists actual emulators. Zero work,
unknown entries and duplicate entries cannot certify a full run. Gate patches
in mutants/gates are applied to disposable source trees by gate-mutants.sh;
each must fail twice for its named semantic reason. These controls need no
emulator. Full make smoke still boots both emulators.

Ready, acknowledgement and completion waits in QEMU use the caller's absolute
wall deadline, without a second 120-second cap. Expiry explicitly fails the
session. The virtual-clock/socketpair QMP test accepts completion at125s with
an180s deadline and rejects90s expiry. The default remains5000ms. Bochs also
rechecks its session deadline after frontend callbacks. DESKTOP_WAIT_MS permits
an invocation-only override of its10000ms marker wait; session and app-driver
pacing overrides likewise retain their defaults. These controls support busy
shared-machine verification without weakening the oracle or test selection.
# Concurrent smoke runs

Smokes run concurrently, without a host smoke lock, Bochs lock or QEMU slot
queue. Each DOS, program, desktop, Base and Turbo execution invocation creates
a private `build/.../run.XXXXXX` directory and prints its location. Explicit
output overrides remain available; concurrent callers must choose distinct
paths. Serial, logs, QMP endpoints, writable data and screenshots stay private.

Bochs 2.7 binds an available RFB port starting at 5900. The harness reads the
successful listener publication from that child's fresh log and reports the
actual port. It never assumes that a failed attempt at 5900 was a bind, and
rejects partial/stale log input. Each guest gets a private boot clone; source
image locks are never removed. Ready/handshake waits share the caller deadline.
Normal QEMU smoke has no TCP listener: QMP uses an inherited socketpair and
serial uses a private file. Only optional GDB's specific fixed endpoint 1234
has a resource lock; it cannot block ordinary smoke.

Whole-desktop comparison is strict by default over every RGB pixel (or every
pixel of an explicit binary reference mask). pixels-check.sh enforces 1578
opaque icon pixels recovered from hash-pinned genuine Mac OS 8.1 captures on
four composed QEMU screens, in smoke and therefore cert. The live legacy
whole-screen comparison explicitly requests informational mode. It shares
ancestral rendering code and is not independent fidelity evidence. Exact
screen-by-screen coverage and the absent-reference reasons are recorded in
PIXEL-REFERENCES.md. No current-renderer output is used to mint references.

The DOS session retains both a copied file and a renamed file for mtools byte
comparison, establishes the rename source is gone via a successful directory
listing, and checks persisted MD separately from RD. DEL/RD operate on files
and directories independently created and verified before the guest starts.
An unsuccessful mtools listing is a failure, never proof of absence. FIND
has a dedicated exact-output file so TYPE cannot supply its passing evidence.
smoke-mutants.sh exercises four operation faults and a drawing-only icon
fault on isolated product images, requiring two named semantic failures each.

Shared-machine smoke limits can be overridden per invocation without changing
defaults: DOS_SMOKE_TIMEOUT_MS (QEMU 150000, Bochs 45000),
PROGRAM_SMOKE_TIMEOUT_MS (55000), DESKTOP_SMOKE_TIMEOUT_MS (90000), and
IBASE_SMOKE_TIMEOUT_MS (90000). Red-control desktop capture has a 60000 ms
budget. Gate tests remain host-only; product mutation controls are tier 2.
PROGRAM_SMOKE_WAIT_MS and IBASE_SMOKE_WAIT_MS (both default 1000) set Bochs's
inter-command wait for a busy host; they do not alter the oracle or test set.
DESKTOP_WAIT_MS (default 10000) bounds Bochs's frontend marker waits. The driver
rechecks its overall deadline after a frontend wait/capture, so increasing
that invocation limit never permits late completion to pass the session.
LEGACY_SMOKE_TIMEOUT_MS controls the diagnostic legacy boot (default 15000);
when only DESKTOP_SMOKE_TIMEOUT_MS is supplied it applies to that boot too.

QEMU's ready/ack/completion waits use the absolute wall deadline computed from
its caller's timeout_ms. There is no separate 120-second ceiling. Existing
idle budgets still detect a stalled guest; progress cannot renew the caller's
wall deadline. Missing completion returns a failed QMP session with an explicit
expiry diagnostic. No default was raised (QEMU_DEFAULT_TIMEOUT_MS remains 5000).
The deterministic qemu-deadline-test drives the actual QMP session with a
socketpair and a virtual clock: completion at 125s passes a requested 180s run,
while 90s expiry (including a marker exactly at that deadline) fails. A patch
restoring the old 120s cap is part of gate-mutants.sh.

QEMU validates every --keys/--mouse token before launch, using its QMP key
vocabulary and strict complete numeric tokens. Invalid input fails before any
prefix is delivered. Input-sized storage preserves complete scripts, including
absolute capture paths in long checkouts; allocation failure fails the run.
There is no total 4095-byte script limit. Chords, paths and waits retain their
individual protocol bounds and reject excess or trailing data. The actual QMP
regression checks 4095/4096/6001 bytes, an impossible mandatory suffix after
4100 separators, and invalid suffixes with no prefix delivery. The live bisect
control is `sh factory/tests/trace-suffix.sh` against the ordinary product image.
Finder journeys open APPS by name and launch the exact full EXE name after
clearing the timed type-ahead prefix with Up. Finder reveals selected offscreen
rows; the pointer stays at its reviewed control anchor, independent of directory
insertion order. All app journeys use this helper; SDK's two test-tenant journeys
also use Finder name selection. Volume, menu and app-control clicks retain their
independent position contracts. navigation-mutants.sh shifts one real product
icon slot: the positional control fails, name-based first/offscreen launches
still work on both emulators, and the independent layout oracle rejects the
wrong product layout twice.

The complete host and smoke registries include the imported boot component.
Its host receipt is required by the aggregate, and full smoke runs its ordinary
boot/firmware/disk observations on both emulators. The boot-registration control
rejects missing host registration and a full-smoke selection omitting boot;
its external omission patch is included in the factory gate sweep.
