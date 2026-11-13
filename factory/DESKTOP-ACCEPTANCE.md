# Desktop journey acceptance

The factory owns boot, Finder disk/APPS navigation, input transport, serial
progress waits, fault checks, captures, DOS/FLAIR reentry and the live legacy
pixel comparison. desktop-journeys.list registers app-owned directories; its
first ten comment lines describe adding an app. Each directory supplies
journey.sh and verify.sh. All registered journeys share one product boot and
one writable disk per emulator; no test personality is linked into the OS.

The 123 journey preserves the original input and independent Lotus reader:
A1 integer 123, B1 number 1.5, C1 @SUM(A1..B1), cached 124.5, exact relative
range RPN, exactly three value records and a proper EOF. The compatibility
readback-worksheet.sh entry forwards to that app-owned verifier.

Both frontends use W marker waits. desktop-bochs.c adds occurrence-counted
serial waits to the unchanged Bochs RFB driver, via desktop-tools.mk; no shared
driver or factory/Makefile edit. Every awaited marker and its count is checked
afterward. QEMU additionally exits to a real A:\> and starts FLAIR again.
QEMU runs CPU 486, 8 MiB, acpi=off, -d int,guest_errors,cpu_reset; two power-on
resets and zero errors/exceptions are required. Bochs uses the installed
pentium CPU at 8 MiB, VBE mode 0101 and requires zero triple faults.

Original desktop: 29.94 s. Extracted 123-only journey: 31.07 s, both emulators
green, same four independent legacy pixel regions unchanged. The complete
original make smoke (DOS, probes, desktop, Turbo Initech) passed in 242.81 s;
this includes waiting for other engineers' serialized Bochs sessions.
Raw evidence: build/{baseline-desktop,journeys-123,smoke-baseline}*.{log,time},
build/journeys-123/{qemu,bochs}.{serial,report} and pixels.txt.

red-desktop.sh retains the original empty-worksheet and loader-cap controls.
An empty save completes the UI session but fails the host worksheet reader;
the loader cap stops QEMU input at the missing launch marker. Production
sources are restored even on interruption. Red runs are intentionally separate
from ordinary make smoke.
