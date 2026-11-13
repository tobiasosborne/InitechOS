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
Its VBE extension is disabled explicitly in smoke to require VGA13 fallback.
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
