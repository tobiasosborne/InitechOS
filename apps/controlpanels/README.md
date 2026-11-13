# Control Panels

CPANELS.EXE is the chooser plus Date & Time, Desktop Patterns, Mouse, Sound
and Memory panels. Target dependency: SDK only, through typed gate bindings
with plain mk/tenant.mk flags and no semantic switches. The SDK supplies the
settings record and dialog transport. There are no dead commands.

Frozen legacy-final inventory: stage changes, OK to persist/apply, Cancel to
discard, Gregorian date/time adjustment, menu clock visibility, nine patterns
(teal and eight System 7 PAT# tiles), tracking/double-click speeds, three
speaker sounds/lengths and preview, and read-only memory labels. RTC is CMOS;
CPANELS.DAT stores the other settings in the claimed record.json format.
Native panel layouts are not recovered: existing Dialog/Control furniture is
an authored composition, not a pixel replica of a period panel.

make test runs two suites, 144 core/dialog checks and four shared-sdk/host
transport checks, in <1 second. The PAT# resource comparison is differential
against real System 7 bytes; routing/transactions/calendar/record assertions
are unit/regression. make mutants kills three patches; removing the hooks
leaves the 8,336-byte EXE identical. make smoke launches on QEMU, chooses
Stipple, saves, quits, then mtools/cmp checks a literal independent 16-byte
record and fsck checks the volume. Screendump:
build/smoke-desktop/patterns.ppm.

Temporary hosted access ends when FLAIR publishes a hosted Dialog library
and its flat-item adapter cp_dialog_items.h. Exact host-only sources are
listed in the Makefile: event, dialog, control, window, surface, blitter,
text, chrome, heap, flair_look, region, and tests/render/render.c. The adapter
is private today and needed only to preserve the existing real Dialog
Manager press/release assertions. No target recompiles these sources.
Remaining legacy panel pixel/bar-composition smokes belong to factory.
