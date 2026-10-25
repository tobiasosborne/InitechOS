# Overlay streamed preflight (2026-10-25, initech-f9t)

Context: the PM rejected cb3c7910's 192,256-byte NOBITS staging allocation;
the kernel runway is reserved for resident code and ADR-0014 overlay tables.
The replacement must use at most 1 KiB of fixed byte buffers. All input
validation precedes destination writes. Genuine device I/O errors after copy
starts may leave the destination partly written or partly relocated.

What changed: a bounded, O(1)-space FAT chain walk proves the whole declared
file is backed through EOC, without data-sector reads. It rejects short chains,
cycles, free/bad/reserved/out-of-range links, and overflow-sized declarations.
The overlay core validates header/tag, whole-footprint ownership, the backing
chain and every fixup offset, then rechecks ownership before streaming the
module and re-streaming the validated fixups. It reuses the existing two
512-byte FAT buffers. The rejected array and its staging-capacity error path
are gone. The flagship GUI binding, launch trace and unbound mutant stay.

Why: this retains whole-destination canaries for every validation refusal,
including the 1024-byte-declared/one-cluster chain and late invalid fixups,
while reclaiming all 192,256 B of staging BSS. A real second-sector device
failure test asserts READ, unpublished out_len, exactly 512 changed bytes,
and an intact destination tail. Copy-before-validation and skipped-chain
mutants prove the two validation boundaries; the linked-buffer budget gate
is RED on the rejected implementation (193,280 B) and GREEN at 1024 B.

Source consistency: the entry/file/ownership/cached FAT remain stable during
the non-reentrant DOS call. An impossible repeated out-of-range fixup or FAT
structural failure traps as an invariant breach, rather than being returned
after writes as an input refusal or mislabeled as a genuine device error.
Genuine DOS 3.30 mid-copy device-read-error behavior is UNVERIFIED; the
retained period captures prove only the fixup-address difference. DEC-19
documents this exception next to the other pending-ratification notes.

Acceptance: overlay host/emulator gates (105 host checks), five named host
mutants, flagship GUI/unbound-mutant, EXEC/MZEXEC/nested/subdirectory gates
and mutants, selected FAT regressions and ADR/makefile gates all exit 0.
Against 91192754, loaded bytes are 181040 -> 181360 (+320 B), while BSS stays
95856 B (delta 0); cb3c7910's 192256-byte staging allocation is entirely gone.
The linked loader buffers total 1024 B and _kernel_end is 0x543AF0, leaving
247056 B below KERNEL_IMAGE_CEIL. Commands and exit codes are in the local
-o report CODEX-OUT.md and build/overlay-stream/. The original
CODEX-OUT.r1.md was preserved. The kernel size-r4 floor remains outside scope.
No amend, push, merge/rebase or changes to main are authorized for this lane.
