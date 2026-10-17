# WL-0161 -- ADR-0001 / ADR-0002 reconstruction and the citation gate

Lane: `lane/adr0002` (Claude Opus), based on main `4755d030`. Bead
`initech-lk29` (ADR governance: ADR-0001 and ADR-0002 cited as ratified
architecture with no file in `docs/adr/`). No CLAUDE.md, HANDOFF or beads
edits; nothing pushed.

## Context

CLAUDE.md names ADR-0001, ADR-0002 and ADR-0003 as the ratified
architecture. ADR-0001 was filed on 2026-09-25 (WL-0091, commit `99320c45`:
DEC-01 reconstructed, DEC-02..DEC-05 ratified), after which WL-0114 narrowed
lk29 to ADR-0002. ADR-0002 had never existed: `git log --all --
'docs/adr/ADR-0002*'` is empty, yet the tree mentions it 85 times on 81
lines (CLAUDE.md, the PRD, HANDOFF Sec 3, nine ADR/CDR files, worklogs,
plans, and 17 artifact source headers), and CDR-0001 deviates from it by id.

## What changed

1. **`docs/adr/ADR-0002-Toolchain-Implementation-Language-Executable-Format.md`**
   (new). House format (Document Control, Context, Decision, Consequences,
   Compliance, Related, Appendices). Status: "RECONSTRUCTED 2026-10-17 from
   the citations; ratification status: as cited". Five blocks, each marked
   AS CITED and each with its citation sites in Appendix A:
   - ADR-0002 DEC-01 -- the OS is C, freestanding (assembly as the
     toolchain's companion for boot/entry/ISR code).
   - ADR-0002 DEC-02 -- `i686-elf` cross-compilation; under deviation
     (CDR-0001, host `gcc -m32 -ffreestanding -nostdlib` + nasm + ld,
     `KERNEL_CC`; bead `initech-6pm` open).
   - ADR-0002 DEC-03 -- the language partition (PNC-1, operator 2026-06-08):
     Pascal is Turbo Initech's; two compilers never conflated; the self-host
     fixed point concerns Turbo Initech only; bundled apps are C.
   - ADR-0002 DEC-04 -- the factory is C, no second factory language (as
     ADR-0005 AM-1, ADR-0007 Sec 8 and `tools/ppm_flair_cursor_check.c`
     attribute it).
   - ADR-0002 DEC-05 -- executable format: delegated to ADR-0003 DEC-08 /
     DEC-08a; the toolchain's ELF32 products are intermediates only, every
     shipped image is flat (kernel via `kernel.ld` + `objcopy -O binary`;
     `.COM`-equivalent; InitechMZ).
   No surviving citation names an ADR-0002 DEC number, so the numbering is
   the reconstruction's (stated in its Sec 1.3). Appendix R records six
   conflicts (below).
2. **ADR-0001 Rev 2.1** (citation reconciliation, no decision changed):
   Appendix A (every site citing ADR-0001 DEC-01..DEC-05) and Appendix R
   (five conflicts, below); Document Control, revision history and sign-off
   rows for 2.1.
3. **`harness/check_adr_citations.c`** + **`harness/adr_citations.mk`**
   (included at the end of the Makefile): host gates `test-adr-citations`
   and `test-adr-citations-mutant`, both appended to `TEST_UNIT_GATES`. The
   checker walks the tree (sorted; skips dot-dirs and `build/`; `.md .c .h
   .json .mk .asm .inc .pas .ld .sh` and `Makefile`), and for the scoped
   Records ADR-0001, ADR-0002, CDR-0001 asserts (F) cited => filed and (D)
   every `<id> DEC-nn` citation (also `<id>-DEC-nn`, `<id> (... DEC-a ...
   DEC-b)`, `DEC-a/DEC-b`, `DEC-01/03`, `+`, `and`) names a heading whose
   text begins (after a section number, "Sub-Decision ", "<ID> Amendment ")
   with that DEC token. A DEC merely mentioned inside another heading does
   not count. Writes `build/adr-citations.inventory`.

## Conflicts found (recorded, not silently fixed)

ADR-0002 Appendix R:
- R-1 executable format: the title and ADR-0003 Sec 2 give it to ADR-0002;
  HANDOFF Sec 3, PRD Sec 5/15, CDR-0001 Sec 3 give it to ADR-0003 DEC-08.
  Shipped `kernel.ld`, the Makefile kernel section and `mz.c`/`loader.c`
  cite ADR-0003 -> DEC-05 delegates.
- R-2 PRD Sec 14 says the bootloader is C; the shipped boot chain is nasm
  (`os/boot/*.asm`), CLAUDE.md's file map says "C + asm".
- R-3 CDR-0001 / bead 6pm say `CC_KERNEL`; the Makefile is `KERNEL_CC`.
- R-4 HANDOFF Sec 3 still says ADR-0007 *pending*; it was ratified 2026-07-11.
- R-5 title variants; R-6 the historical "artifact is Pascal" (PNC-1).

ADR-0001 Appendix R:
- R-1 ADR-0003 cites "ADR-0001 Sec 3/4/6/8" for content the reconstructed
  ADR-0001 has under other numbers (or not at all); resolved to PRD Sec 3,
  Appendix A, Sec 2 Non-Goals, Sec 8 -- not by renumbering ADR-0001.
- R-2 "386+" wording (seven ADR Related-Documents rows, PRD Sec 2/4/15,
  `spec/memory_map.h`, Makefile 663) vs DEC-02's 486-class target: true for
  codegen (`-march=i386` stays, DEC-03), superseded as a target claim.
- R-3 "no isolation" / "no real iron" attributed to ADR-0001 (ADR-0013,
  ADR-0012): the decisions are PRD Sec 2 Non-Goals; DEC-01's single flat
  segment is the mechanism.
- R-4 ADR-0013 line 28 still says ADR-0001 has "no standalone file".
- R-5 erratum: ADR-0001 Sec 1.2 claimed `docs/adr/` held "ADR-0002 through
  ADR-0013" (ADR-0002 absent; ADR-0011 pending).

## Why

Law 1: a governance document cited by id must exist, and its decisions must
be numbered blocks a citation can resolve to (Rule 8). The oracle is the
citing corpus -- written independently of the ADR files -- so a renamed,
renumbered or dropped block goes RED (Law 2: no by-construction agreement).

## Frictions

- Other Records are not uniformly gradeable by the same grammar. A survey
  run with scope ADR-0003..ADR-0013 found 13 unresolved (rec, DEC) pairs
  (111 sites): ADR-0004 DEC-03/04 and ADR-0008 DEC-02/03/05/06/07 are not
  headings in those Records (prose/table form); ADR-0003 DEC-08a.1/.2/.5 and
  DEC-14.2 are cited sub-items with no heading; ADR-0003 DEC-08b
  (`docs/design/kernel-runway-relocation.md:180`) and ADR-0007 DEC-09
  (`docs/design/GUI-remediation-ADR-reconciliation.md:9`) cite blocks that
  do not exist. Out of this lane's scope; follow-up bead recommended
  (widen `ADR_CITATIONS_SCOPE` after those Records gain DEC headings or the
  citations are corrected).
- The first ADR-0002 draft cited ADR-0007 and ADR-0005 AM-1 "Sec 9" for
  their references sections (both are Sec 8), and ADR-0008 "Sec 2" for its
  Context (Sec 1); caught by checking every section number against the files.
- The checker does not parse purely prose mentions ("ADR-0001 authored ...
  DEC-02 ..."); those are listed by hand in ADR-0001 Appendix A as
  *(prose)*. All resolve.

## Acceptance

- RED first: on the base tree the checker went RED for the lk29 reason --
  `!!! test-adr-citations FAIL: ADR-0002 is cited at 85 site(s) (first:
  CLAUDE.md:24) but no docs/adr/ADR-0002-*.md is filed`.
- `make test-adr-citations`: green with ADR-0002 filed.
- `make test-adr-citations-mutant`: three named mutants, each on a copy of
  `docs/adr/`, each RED for the named reason: ADR_UNFILED (ADR-0002
  removed), ADR1_DEC02_RENAMED (ADR-0001 `### DEC-02` -> `DEC-92`),
  ADR2_DEC03_RENAMED (ADR-0002 `### DEC-03` -> `DEC-93`). A no-op mutant
  is itself a FAIL.
- `make test-makefile-vars`: green (the include reads no variable before
  its definition).
- ASCII-clean: `harness/check_adr_citations.c`, `harness/adr_citations.mk`.

## Pointers

- `docs/adr/ADR-0002-Toolchain-Implementation-Language-Executable-Format.md`
- `docs/adr/ADR-0001-Hardware-Target-386-Origin-486-Floor.md` (Rev 2.1, Appendices A, R)
- `harness/check_adr_citations.c`, `harness/adr_citations.mk`
- Beads: `initech-lk29` (closable by the PM on merge), `initech-6pm`
  (CDR-0001 remediation, still open).
