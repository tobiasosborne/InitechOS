# Overlay review fixes (2026-10-25, initech-i15)

Historical: the PM subsequently rejected cb3c7910's module-sized NOBITS
staging. WL-0184 (initech-f9t) supersedes that memory/atomicity design with
streamed validation and an explicit partial-device-read exception.

Context: review of 91192754 found partial destination writes on failure and a
missing backend binding on the flagship GUI text-tenant path. The operator
requested fixes on this branch, no push, and no change to DEC-19's relocation
decision or the existing kernel size floor.

What changed: fe1ebf72 commits the RED tests and transcripts before artifact
changes. The loader now reads into private kernel staging, reads each fixup
chunk once, validates/relocates staging, rechecks ownership, then commits the
exact footprint. The FLAIR mount binds the overlay backend. The flagship gate
uses the existing fixture as SAMIR.COM through Ctrl-Alt-backslash, and an
unbound-backend mutant reproduces the failure. A direct-write host mutant
breaks the whole-destination canary tests. No MCB or locked ABI layout changes.

Why: a failed replacement must preserve the entire old overlay. The original
A9 test only failed the first read; the new tests fault every read after copying
its bytes, reject bad fixups in both chunks, and exercise a real short FAT
chain. NOBITS staging holds PROGRAM_IMAGE_MAX bytes within the existing kernel
runway, avoiding any DOS allocation or loaded buffer bytes. Resource exhaustion
returns 0008h before a destination write; ownership failures remain 0009h.

Period evidence: harness/fixtures/dos33_overlay retains the independent genuine
MS-DOS 3.30 probe, captures, provenance and reproducible remint. The captures
show factor-based fixup addresses, including a different-segment control.
DEC-19's load-based rule remains unchanged, with the extra deviation clearly
marked PENDING OPERATOR RATIFICATION. Pointer-validation wording now says
NULL/wrap, without claiming readability detection.

Frictions: this worktree's beads database lacked issue_prefix; setting the local
prefix to initech allowed tracking initech-i15. Git commits require access to
the parent repository's worktree index outside the filesystem sandbox. No main
or remote branch is changed.

Acceptance: overlay host/emulator gates and their mutants, EXEC/MZEXEC, nested
and subdirectory EXEC, loader/MZ/tenant/MCB gates and ADR citation gates all
returned exit 0. DOS33 remint exit 0, with both saved captures byte-identical.
Loaded flagship bytes: 181040 -> 180944 (-96); NOBITS BSS +192256;
_kernel_end=0x572850 < 0x580000. Detailed commands/exit codes are in the local
ignored -o report CODEX-OUT.md and build/overlay-fix/.
The known test-kernel-size-r4 failure remains outside this task; its floor and
oracle are unchanged. QEMU is the EXEC family's existing emulator gate;
Bochs/86Box coverage remains the original initech-x0i obligation.
