# Local product factory

emu/ owns the C QEMU and Bochs drivers claimed from attic/harness/emu.
image/ owns the legacy FAT boot-volume builder and its independent C layout
checker, plus product assembly. `make` builds host tools into build/factory;
`make image` assembles already-built components; `make smoke` drives the actual
product disk set, copies the writable disk per run, checks serial lines and
uses mtools to read written bytes independently. All factory programs are C,
make or POSIX shell. No target test personalities or remote automation.

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
build/cert/<UTC date/time>-<short sha>/{summary,full.log}. CERT_STRICT=1 also
requires every registered component to be imported. CERT_JOBS (default 16) and
CERT_MUTANT_JOBS (default 4) bound parallel work. Mutation summaries include
pending patches without counting them as kills. Targets are invoked even for
pending-only owners: unimplemented targets fail. Successful active sweeps with
pending work are marked PASS_PENDING, with coverage explicitly incomplete;
this does not certify the deferred patches or unfinished component imports.
The seed's
execution catchers use seed-guest.sh: real product DOS, separate test volume,
FPC Delphi-mode 32-bit integer reference, stdout and persisted-file comparison.

mk/dependencies.mk is the authoritative build dependency graph, displayed by
`make dependencies`. README dependency prose describes the interfaces behind
those edges. The top-level preflight audits actual make graph paths, including
generated headers, against those declarations. The kernel/resident shell and
SDK/FLAIR source-audit cycles have explicit test-order exceptions there; the
component recipes still build their required artifacts.
