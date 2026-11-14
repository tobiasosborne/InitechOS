# Desktop journey acceptance

Historical measurements below describe wave2/initechword before integration.
Current mechanics are described in factory/README.md.
Current whole-desktop enforcement and independent-reference limits are in
factory/PIXEL-REFERENCES.md; historical four-region equality below is only
a regression observation, not the current visual acceptance contract.
Integration retains 72 Word-owned patches; its shared short-write fault belongs to the SDK.
The shared Bochs loop now binds the RFB pump during frontend serial waits,
and factory tier 0 checks that transport progress. The top-level desktop-apps
prerequisite builds the full journey registry before assembling the image.

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
serial waits through the Bochs RFB driver, via desktop-tools.mk. The shared
loop supplies its RFB pump while those waits run. Every marker and count is checked
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

Word now joins the same boot: its F6 Bold pair encloses reports in the exact
text office reports are ready.; F10 writes WORD.WP5; F7/n/y exits. The real
WordPerfect corpus reader requires that text, both attribute tokens in order,
and END offset == filesize with zero trailer errors. The empty-save patch is
the third red control; it fails specifically at the independent host verifier.

launch_app uses the independent mtools directory listing to locate a named
EXE in the default four-column Finder grid. It does not infer any expected
saved data from the product. restore_pointer runs before each app exits, so
subsequent journeys start from the same anchor. Bochs's final W is supplied
by its completion-marker/count check, retaining its input-after-completion
failure. The guest RTC is pinned to 1992-03-01 09:00 UTC on both emulators.
The frontend regression test covers validation vs live replay, waiting for
a new occurrence of repeated progress, and invalid input tokens.

The top-level desktop-apps prerequisite calls desktop-build.sh before image
assembly. That mechanism builds every
registered app's actual artifacts, so a clean image/repro also includes Word
without adding another application dependency to the top-level Makefile.

Historical Word-branch acceptance (local branch wave2/initechword):

- make smoke: green, 131.48 s wall (original: 242.81 s). Both runs include
  the DOS session, separate probe disk, desktop and native Turbo Initech/FPC
  comparisons. Bochs lock waiting varies with other engineers' sessions;
  these raw wall measurements are not an isolated speedup claim.
- Desktop alone: original 29.94 s; extracted 123-only 31.07 s; both journeys
  42.95 s standalone, 40 s inside the final full smoke. Each app adds its own
  input/readback, not another boot. Final desktop: 9 checks, zero failures;
  the four live legacy pixel comparisons still have zero differing pixels.
- factory/red-desktop.sh: all three controls rejected, 71.16 s; empty 123
  save, restored 64 KiB loader cap, empty Word save. Each failed for its
  named reason, not an unrelated failure; all product patches were restored.
- make -C apps/initechword test: 13 programs, zero failures. Direct C suite
  summaries sum to 3147 checks; the three 32-file corpus legs add 19247:
  22394, unchanged. All face tests link the shared sdk/host fake.
- make -C apps/initechword mutants: 73 patches, 73 killed twice, zero
  survivors/errors, 395 s. All 75 original sites are represented; none is
  pending. The old catalogue's BLK_CUR_START and SENT_BARE catchers needed
  reveal/roundtrip selections, not weaker assertions. Deep scratch paths
  exposed a test-only path buffer overflow; checked 1024-byte paths fix it.
- WORD.EXE SHA-256 before and after removing every product mutation hook and its comments:
  9453efb0e90e0ff491cb9cf630c928a70294316f9652329a6432b0b891ea698e
- make status: apps/initechword: imported.
- make -j16 test: registered 11, run 11; ALL GREEN, 90.19 s wall.
- make repro: two clean builds identical, 42.84 s wall. Kernel/product/boot
  hashes remain the original values; working-volume SHA-256 with Word is
  d5110fbb7de3363b6ea25abfa2b9e60874821c50821acffacd03e7c49d5bbc62.
- Current Word captures: build/smoke-desktop/word{,-saved}.{ppm,png}.
  Retained serial/report/disks/readback: build/smoke-desktop/. Exact positive
  command logs: build/{smoke-final,all-test-final,word-test-final,word-repro,
  word-mutants-final,status-word}.log; red logs: build/word-red-final.log
  and build/red-desktop/. Baseline evidence survived the clean repro.

Present depth and 69 unavailable visible labels (49 command entries plus
20 advertised children of blocked Format groups) are frozen in
apps/initechword/README.md. Broader legacy pixel-heavy gates remain with the
InitechWord/factory importer in apps/initechword/tests/SMOKES.md; publication
artifacts and the kernel FAT differential retain their original owners.
