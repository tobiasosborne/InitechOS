# Kernel overlays Lane 2 -- current-main replay

Date: 2026-10-25. Authority: PM replay brief; ADR-0014 DEC-14-5A/-6/-8/-9.

The requested fetch selected origin/main at 3c194c50, including ff5dd607 and
the later NDX merge. Commits 6c29f0cd and 460487d3 cherry-picked as 33fc15d3
and adf0734d on lane/kov2-r without textual conflicts. No main branch was
checked out or modified; no push or bd command was run.

Lane 1's lazy-load coverage/stub mutants, stricter absolute-code-relocation
check, and directory underflow hardening are preserved. The stricter static
pointer and moved-symbol absence gates pass on the Finder section objects.
The inherited absence gate includes a retained-Finder-body mutant.

The lazy gate now also drives the locked README.TXT-to-APPS Finder move.
It requires exactly one OVL-OK after READY and the warm destination
highlight, followed by the committed move; mtools checks destination bytes
and source absence. Both nosave and noload mutants must fail this Finder
leg as well as the existing JTC and About legs. No recorder code is used.

The unchanged resident cap exposed one four-byte overrun after the replay:
the DB-write-skip production variant loaded 169,988 B against 169,984 B.
finder_fat_map and finder_lerp16 are COLD-only utilities, not callbacks or
resident entries. Giving them internal linkage removes their unnecessary
generated stubs. The census returns GREEN for all 77 production kernels:
flagship 168,788 B; largest production variant 169,956 B. COLD's existing
32 KiB budget stays inside the unchanged 128 KiB window.

Per PM's ruling, recorder determinism belongs to initech-x8gsi.1 and this
lane merges only after all clips reproduce. The three recorder metadata
edits carried by the old cherry-pick were restored to main's values. No
capture logic, encoder, input trace or recorder setting is changed here.

The validation run uses BUILD=build/kov2r and serial image/emulator gates.
Full gate outcomes, per-kernel loaded sizes, and command/exit records are
written to build/lane-kov2-r-report.txt and build/lane-kov2-r-evidence/.

The 149-target matrix completed with one legacy artifact-location mismatch:
locked_trash.sh writes into build/ even under BUILD=build/kov2r, so the icon
oracle mutant looked for a nonexistent custom-directory screenshot. Its
unchanged default-directory rerun passed. Every other matrix target passed,
including all overlay gates/mutants, app-switch Bochs with OVL-OK, launchkey,
and SAMIR suspend with byte-identical pre/post desktop screenshots.

The complete census measured 336 kernels; the largest mutant loads 170,124 B
and retains 26,484 B headroom. All 77 production kernels meet 169,984 B.
The strict static checker, global/local code-address mutants and Finder
lazy nosave/noload probes all pass. No recorder work was performed.
