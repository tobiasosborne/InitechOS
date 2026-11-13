# Vertical slice acceptance

S1 f165d099: product composition (186010 bytes), standalone kernel, DOS smoke
on QEMU and Bochs, FLAIR reaches the weak not-installed hook. Top make test green.
Toolbox fix 6c1a628a removes the retired WINDOW_ENABLE_R1_OPS build guard;
the existing resize/zoom/collapse bodies are unchanged. Full make test green.
S2 5d43aba3: Finder desktop, cursor, Special > Exit to InitechDOS, prompt and
FLAIR again on QEMU. IRQ-masked 8042 polling preserves the legacy ordering.
S3 b749f26d: INITECH -> APPS -> exactly 123.EXE and 123.MNU; the product
AUTOEXEC.BAT ends with FLAIR. DOS smoke uses a private prompt-starting disk.
S4 ef6f004e: launch, entry, /File Save, quit, DOS and FLAIR again; independent
mtools/real-Lotus-corpus reader sees A1=123, B1=1.5 and C1 formula
@SUM(A1..B1), cache 124.5, relative range RPN and proper EOF, 1461 bytes.

S5: make smoke includes factory/smoke-desktop.sh in addition to the existing
DOS session. Desktop checks on QEMU and Bochs are startup, Finder navigation,
123 entry/save/quit, independent worksheet readback, video and CPU-fault checks.
The QEMU desktop session also exits to a live A:\> and starts FLAIR again.
Both run at 8 MiB. Bochs uses the installed pentium CPU and its LGPL VGA BIOS
with extension=vbe (mode 0101); QEMU uses CPU 486, acpi=off and 640x480x32.
Screenshots: build/smoke-desktop/{desktop,disk,apps,sum,dos,qemu,legacy}.ppm.
Serial, reports, QEMU fault logs, copied disks and reader listings remain there.

factory/red-desktop.sh applies and restores two patch-file mutants. The save
mutant writes a zero-length worksheet: the session completes but fails with
FAIL desktop: QEMU worksheet content. Restoring the old file cap gives
TENANT-LOAD-FAIL name=123.EXE rc=3 and FAIL desktop: QEMU session. The strict
factory W marker wait stops input on missing progress; it no longer continues
into a different UI state. Both mutations are killed and sources restored.
No mutant is compiled into the product by ordinary build flags.

S5 make repro: two clean builds identical, 56.42 seconds wall on this host:
boot e6a95046c0c5a961ad762c2ea6337a99912a8842f1b98746e1342ee3b7fb8482
data c8f046cba02b8f0a669e7a26c479896f78b2ed61ba96ce4995f60b5a5ee95b70
standalone kernel 6828c24d98ef745b4c7d83f9aae4c6fd05f9d2f40461c215385fc0b163686d12
Product INITDOS.SYS: 268894 bytes; standalone: 106116; boot disk 1507328;
working disk 1474560. S6 may change these bytes; final measurements follow.

Live legacy comparison is informational. The reference is rebuilt from the
read-only legacy-final tree, then booted and captured using the same pinned
RTC. The initial ready-marker comparison found 186 clock pixels because the
product first clock poll had not run. The final reference waits for its clock
paint. The disk icon, Trash icon and unobscured desktop pattern already had
zero pixel differences. Band 2 intentionally shows the Finder; legacy showed
the Photoshop reference tenant. HELLO and NOTES are absent by product design.

Deferred attic ownership: additional trace/checker imports belong to the
desktop; kernel/DOS tests and mutations belong to import/kernel-tests;
shared build conventions belong to conventions/hardening. No attic input is
read by any product build introduced here.

S6 extraction: scene.c 260 lines, input.c 78, tracking.c 716, finder_glue.c
589, launch.c 91, volumes.c 198, dialogs.c 607, start.c 296. Internal shared
state/prototypes are component-private. The heap descriptor retains static
lifetime. Exit restores the prior INT81 IDT gate. The initial scene now builds
zero windows, rather than creating and hiding two canon specimens. The generic
shell builder permits zero document windows; legacy nonempty scenes still pass
the retained shell/drag host oracles. kmain_legacy.c retains 335 unbuilt lines.

Final clean builds (53.55 s):
boot 3e3d92aa5e1eb8d37b4cccc6797e1fe89568f7c302260c7579ac5a640443126b
data c8f046cba02b8f0a669e7a26c479896f78b2ed61ba96ce4995f60b5a5ee95b70
product 74709462e8adb65539d49fad452c13865d4bb4741723e1a7a5a420040b1c0d4b
standalone 6828c24d98ef745b4c7d83f9aae4c6fd05f9d2f40461c215385fc0b163686d12
Final product 268670 bytes; standalone 106116; disk sizes unchanged. The repeat
red sweep after extraction killed both patches in 56.27 seconds and restored
all sources. Final live comparison: 0/12800 system-menu pixels, 0/3025 disk-icon,
0/4675 Trash and 0/8360 exposed-background pixels differ. It is informational,
not a claim that the entire new desktop equals a two-reference-app legacy frame.

Final make test: ALL GREEN, 65.35 seconds wall. Desktop: 17 host suites and
import/target audit; system: both links and standalone boundary/transport.
Final make smoke: 61.56 seconds wall, DOS 4 checks/0 failures and desktop
5 checks/0 failures. QEMU: triple_fault=0 cpu_reset=2 guest_errors=0, no
check_exception record. Bochs: triple_fault=0, mode 0101.
