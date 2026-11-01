# WL-0183 -- Finder operation bodies in the kernel overlay

Date: 2026-10-25. Authority: Lane 2 brief; ADR-0014, ratified 2026-10-25.

## Context

Lane 1 (61fa5720) supplied the fixed-window overlay mechanism and left the
169,984 B resident-size gate RED. The local unmodified census was 11 of 77
kernels over, rather than the brief's 12 of 78. The flagship loaded 179,648 B;
the largest enumerated kernel, the DB-write-skip variant, loaded 181,008 B.

## What changed

Split Finder move/Replace/Put Away/Trash-suffix operations into
`finder_ops_cold.c`, destructive operations and alert wording into
`finder_windows_cold.c`, and the kernel's cold operation adapters, progress
dialog, notices, outcome reporting and changed-position commit into
`finder_actions.c`. Host suites include the same cold bodies. The resident
files retain the model, mutable state, enumeration callbacks, drag feedback,
pending-request/outcome polls, disk drivers and shared alert manager.

Resident bindings refer to generated stubs. Public cold entry points used
only by mutants also get generated stubs; shared DOS utilities remain
eligible for section GC. Mutants rebuild the corresponding native overlay
body and retain per-kernel identity/CRC binding. The absence oracle checks
all cold families and now proves that a retained Finder body is rejected.

One section remains: COLD. Its deliberate budget increase, 4,096 to 32,768 B,
fits inside the unchanged 128 KiB window. The 64 KiB stack guard, resident
cap, named refusals and 5A preload policy are unchanged. The size gate now
joins the default test vector on its first GREEN result.

## Why

ADR-0014 DEC-14-8/-10 selects cold user-initiated operations; DEC-14-6 keeps
callbacks and stored code pointers resident or behind stubs. The alert text
buffers also stay resident, including copied locked-notice text published
through DialogItem.text. Existing file semantics and independent FAT/pixel
oracles are retained (PRD 6.3/6.5).

## Frictions

The shared ccache is read-only in this sandbox. Builds source lane-env.sh,
use its shared cache read-only and put scratch files under /tmp. Images and
emulator gates run serially or with modest parallelism.

Two existing recorder markers omitted serial suffixes despite exact-line
matching. Their values now name the complete unchanged lines. The disk-full
clip uses the declared valid full-volume fixture; the independent host and
QEMU disk-full gates retain their original saturation probe.

The icon-mutant image template still supplied an unpatched kernel to the
boot-image builder. It now installs each mutant's matching patched kernel,
overlay and transient; the builder's refusal check remains intact.

The shutdown-cancel MKDIR_CUT mutant also needs the moved mkdir adapter.
Its fault-injection branch imports additional FAT services, so that one
kernel/overlay pair gets object-derived import, stub and want tables of its
own. The independent overlay reader checks this pair as well. The normal
kernel's tables and resident bytes do not change for this fault injection.

The pristine Lane 1 tree failed record-flair-repro for Empty Trash: one frame
differed at 89 cursor pixels while OK was held. Increasing settle time did
not cure the race, and those experimental settings were removed. A decision
about recorder/presentation synchronization was escalated under CLAUDE.md's
stop condition; no pixel tolerance, event class or frame was discarded.
The pristine limits clip also failed its own reproducibility check while
opening many windows. Across 37 clips, 30 GIF/MP4 pairs were byte-identical;
the seven intermediate-frame mismatches are listed in the lane report.

Bochs RFB sockets initially failed under sandbox permissions before guest
execution. Reruns with local socket permission pass, including OVL-OK and
the 36-line tenant CPU transcript compared byte-for-byte against QEMU.

## Acceptance and pointers

The resident-size gate changed RED to GREEN for all 77 enumerated kernels.
Flagship: 179,648 -> 168,788 B, saving 10,860 B. Largest: 181,008 -> 169,956 B,
saving 11,052 B. The complete final verification results, per-kernel table,
moved symbols, capture comparisons and command/exit ledger are recorded in
`build/lane-kov2-report.txt` and `build/lane-kov2-evidence/`.

This lane was instructed to commit on lane/kov2 without pushing or using bd.
Unresolved capture acceptance is explicitly reported, rather than certified
by selecting convenient frames or weakening an oracle.
