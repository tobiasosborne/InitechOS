# Locked Trash and lane acceptance - audit L118/B01

Context: lane-dosprot. Finder previously staged a read-only document through
move, recorded an origin, and purged it without a protection-specific refusal.
The file-layer UNLINK fix alone could preserve bytes but could not prevent
staging or explain the refusal to the desktop user.

What changed: finder_ops queries the live enumeration binding immediately
before Trash staging, refusing a locked source even if the window predates
the lock. Both the desktop Trash and its open window use this path. Missing
source/failed enumeration refuses before moving anything. finder_windows
checks each live purge entry, including descendants locked after staging;
protected survivors retain their origins and keep the Trash full. A typed
LOCKED status makes kmain display a one-button modal notice after refusal or
partial purge. The FAT/Finder read-only bit restatement has a static assertion.

Why: the local Apple_Macintosh_System_Software_Users_Guide_V6.0.pdf printed
pp. 83, 88 and 112 grounds the refusal/alert and unlock-before-discard rule.
The system7-decomp corpus supplies dialog geometry in
goldens/captures/s8_alert_modal.png and specs/sys8/INDEX-sys8.md; its available
Finder/window/alert briefs and resource catalogue do not settle locked-file
alert wording. Text is explicitly "authored, no local reference" in kmain:
"The item is locked. Unlock it before discarding it." No Macintosh wording
parity is claimed. The existing alert's missing caution icon is unchanged.

Deliberate spec act: add spec/flair_protection_traces.mk with the locked
README.TXT-to-Trash gesture, using the already grounded volume/window
geometry. No existing locked trace or diagnostic catalogue was weakened or
reworded; spec/dos_messages.json is unchanged. The authored text is ASCII.

Acceptance: host live-lock staging and late-lock mixed-purge checks were
observed RED before the Finder changes. The booted initial staging trace lost
the source (RED). Final mtools checks preserve source bytes/attributes without
staging or origins; mixed recursive purge retains protected TENANTFX.EXE and
the APPS origin, deletes writable siblings, reports purged=3/refused=2, and
passes fsck.fat. An independent IDB1 wire reader grades retained origins;
pixel checks require the message, OK ring, absence of Cancel, and correct
empty/full Trash. The staging clip was visually reviewed.

Mutants: LOCKED_STAGE permits the move (host live-attribute refusal RED;
guest source missing RED); LOCKED_PURGE bypasses Finder policy (host deletes
the protected mock item; real FAT still refuses, but GUI notice missing RED);
LOCKED_NOTICE suppresses the drawing while retaining the marker and protected
disk: the independent pixel check fails (notice pixels missing RED). Each is
checked for its named reason. Existing Finder, Trash,
DOS-safety and shell regression/mutation gates retain their assertions.

Frictions: overlapping headroom and GUI builds collided on bin2c (Text file
busy); builds were rerun sequentially. The new screenshot initially raced
before its completion wait: --quit-after occurs after screendump in this
harness. --screendump-after FINDER-LOCKED-ALERT now waits for the actual paint
and its settle, preserving all pixel assertions. The earlier failures remain
in the evidence logs. No inference from a serial marker alone certifies UI.

Before changes: 120 kernels, flagship 186792 loaded/9816 free; largest mutant
187496 loaded/9112 free. After the implementation: flagship 187816/8792;
largest mutant 188520/8088. Binding growth is 1024 bytes, within the 1536-byte
lane budget and above the unchanged 4096-byte floor. The final headroom gate
also enumerates the newly registered mutant kernels.

Pointers: build/dosprot-evidence/ logs and family-verdicts.tsv;
build/locked_trash*/ stopped images, serial, dumps, extracted bytes, attributes,
IDB1 records and fsck output; build/clips/locked_{trash,purge}.{gif,mp4}.
Final verification:

- make test-unit: ALL GREEN (389 host gates), including the new protection
  checks/mutants, existing Finder/window tests and inclusive headroom-floor
  mutant. The complete final-source retry passed; no gate was disabled.
- 55 affected QEMU gates pass (family-verdicts.tsv contains the per-gate final
  lines; g-test-*.log contains full evidence). The old DEL failures were rerun
  after correction and each mutant fails for its named reason.
- test-makefile-vars: green. The final headroom scan measures 128 kernels,
  largest 188520 loaded/8088 free, unchanged after final oracle hardening.
- record-flair-repro [locked_trash] and [locked_purge]: PASS -- two captures
  byte-identical. Both final frames were visually inspected. GIF/MP4 clips are
  in build/clips/. A comment/oracle-only edit left the normal flagship binary
  byte-identical under cmp.
- Bochs explicitly skipped per operator: test-boot-bochs,
  test-flair-file-ops-bochs and test-flair-trash-window-bochs. The concurrent
  Bochs gates were not attempted. No 86Box or live DOS 3.3 transcript claim.
- No reference corpus was opened for writing. All mutable emulator volumes
  are local fixture copies. No bd, push or all-of-make-test invocation.

Unrelated fixture issue observed, not repaired: one repeat of the full host
vector failed test-arena-disjoint (four shrink assertions and one distinct-
allocation assertion; 65613 checks/5 failures). Its immediate untouched rerun
passed (1321/0), as did the complete final-source retry. An instrumented local
diagnostic copy forcing PSP address 0x41000000 (segment 0000) reproduces exactly
those five failures: the MCB owner field's zero means free. The failed run's
original mapping was not logged, so its zero-segment cause remains an inference.
Follow-up belongs to the existing arena-disjoint fixture (initech-1q4u); no
allocator code or test assertion was weakened. The logs and diagnostic source
remain in build/dosprot-evidence/arena-*.
