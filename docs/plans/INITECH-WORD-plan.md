<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# InitechWord -- Granular Implementation Plan (the word-processor leg of the canonical suite)

**Issuing Body:** Initech Systems Corporation -- Platform Engineering, Application Platform Section
**Document Class:** Implementation Plan (living; supersede in place)
**Programme / Milestone:** InitechOS (STAPLER) -- app-platform bar (PRD Sec 1.4, ADR-0012), the canonical
suite's word-processing leg. Sibling of `docs/plans/INITECH-123-plan.md` (Lotus 1-2-3, drafted
2026-09-26 -- this plan follows its template and its rulings wherever the same question recurs) and
`docs/plans/SAMIR-implementation-plan.md` (M6, ratified). Also touches `docs/plans/GUI-remediation-plan.md`
R4.2 (InitechText, the existing small SimpleText-alike).
**Status:** DRAFT for PM bead-tree filing. No architecture in this plan is self-ratifying; the two-face
shape (Sec 3) and the three operator rulings (Sec 8) need sign-off before Phase P1 code, exactly as
ADR-0008 gated SAMIR's Phase 0 and the INITECH-123-plan gated its own Phase P1.
**Last Reconciled:** 2026-09-26

> Read order for an implementing agent: `CLAUDE.md` (Laws 1-4; Rules 6, 8, 14) -> `InitechOS-PRD.md`
> Sec 1.4 (the app-platform bar) + Sec 5 (486-class floor, WordPerfect named as an era app) ->
> `docs/adr/ADR-0001-Hardware-Target-386-Origin-486-Floor.md` DEC-02/DEC-04 (486 floor; DEC-04's own
> forward note names dBASE III+/IV, Lotus 1-2-3, WordPerfect as the canonical-era trio) ->
> `docs/adr/ADR-0013-FLAIR-App-Contract.md` Sec 4 (the ALREADY-RATIFIED native-vs-text-host ruling --
> it names **InitechWord by name**, same as Initech 123; read this before designing any full-screen
> text-console host for it) -> `docs/plans/INITECH-123-plan.md` in full (the sibling plan; this plan
> mirrors its two-face reconciliation, golden-acquisition-first phasing, and bead-tree shape) ->
> `docs/plans/GUI-remediation-plan.md` R4.2 (InitechText, the existing light editor this plan must
> relate to, Sec 2 below) -> `os/flair/textedit.{h,c}`, `os/flair/stdfile.c`, `os/flair/scrap.c` (the
> existing Platform-Services building blocks and their CAPPED limits, Sec 2/Sec 9) ->
> `spec/win95ism_guardrails.md` (era ceiling) -> this plan -> the sister-corpus pattern in
> `../dbase3-decomp` (`README.md`, `GAPS.md`) which Sec 5 proposes cloning as `../wordperfect51-decomp`.

---

## 1. Executive summary

The operator's 2026-09-25 north-star restatement names three canonical apps the suite must ship at
"could have been purchased and judged competitive" depth: dBASE III+ (SAMIR, exists), Lotus 1-2-3
(Initech 123, planned in `docs/plans/INITECH-123-plan.md`), and **a word processor** (not yet planned
before this document). ADR-0001 DEC-02/DEC-04 already name WordPerfect explicitly as part of the
canonical-era trio that motivated the 486-class floor ratification (`docs/adr/ADR-0001-Hardware-Target-
386-Origin-486-Floor.md:39,89,107`) -- so, exactly as with Initech 123, the reference-target question is
pre-flagged by an already-ratified ADR, not invented here.

**Load-bearing finding (Sec 3, mirrors INITECH-123-plan Sec 3): ADR-0013 Sec 4 already rules on this app
by name.** `ADR-0013-FLAIR-App-Contract.md:164-166` -- "**RULING: Initech 123 and InitechWord are NATIVE
FLAIR Toolbox tenants** (C, ADR-0002) ... **NOT** character-mode tenants, and **NOT** run inside any
'windowed text-console host.'" A class-2 SAMIR-style full-screen text tenant for InitechWord is
therefore not on the table without reopening a ratified ADR (same stop condition INITECH-123-plan Sec
3.2/8 records for Initech 123). This plan follows the ruling and, like its sibling, reproduces the
"authentic full-screen editing surface" look through NATIVE pixel drawing rather than an `int 0x21`
console -- the same `chicago8x16` fixed-cell font InitechCalc and (per the sibling plan) Initech 123's
Face A already use.

Reference target: **WordPerfect 5.1 for DOS (1989)** (Sec 2). Ground truth: a new sister corpus
`../wordperfect51-decomp`, cloned from the `../dbase3-decomp` pattern (Sec 4). Relationship to the
existing `os/flair/textedit.{h,c}` engine and R4.2's planned InitechText: **InitechText stays the small,
always-resident, monostyled light editor** (Sec 2 -- it is not absorbed, superseded, or blocked by this
plan). InitechWord is a separate, larger, disk-launched app with its own document-model engine, because
`FlairTE`'s locked 4096-byte / 128-line reduction (`os/flair/textedit.h:44-45,67-68`) is architecturally
too small for a multi-page document and carries no code-as-first-class-run model, no undo, and no
styling -- exactly the four things a word processor needs most (Sec 9). Phasing: P0 (golden acquisition,
FIRST, Law 1 gate) through P6 (era-shortfall ledger), Sec 6. Proposed bead tree: Sec 7. Three rulings
the operator must make before Phase P1 ships code: Sec 8, repeated in the final report.

---

## 2. Relationship to R4.2 InitechText (ruling, read before designing anything)

`docs/plans/GUI-remediation-plan.md:157-158` -- "**R4.2** InitechText (SimpleText-alike: TextEdit +
Standard File + Scrap; wire `ww9c` live clipboard as part of this)." InitechText is scoped as a *small*
app built directly on the existing `FlairTE`/`FlairScrap`/`FlairSF` platform services (Sec 9), the
natural role being "double-click a `.TXT` file, or write a quick note, inside the 4 KB / 128-line
ceiling those services already lock."

**Ruling: KEEP, do not absorb or supersede.** InitechText remains the light, always-in-image, single-
style editor -- the honest InitechOS analogue of a SimpleText/Notepad, and the fastest possible "open a
README" experience on a machine that has not disk-launched anything. InitechWord is a **separate,
disk-launched app** (Sec 9 kernel-size-wall risk) with its own document-model engine (a piece table, not
`FlairTE`'s flat buffer, Sec 9) because full-featured word processing needs undo, hidden formatting
codes as first-class data, multi-page documents, and eventually styled/proportional rendering -- none of
which the reduced `FlairTE` cut carries or was ever meant to (`os/flair/textedit.h:22-33`, the committee
reduction note). The two apps DO share platform services at the edges: both route cut/copy/paste through
the one shell-owned `FlairScrap` (`os/flair/scrap.c`), and InitechWord's dialogs (filename entry, search
string, header/footer text) are exactly the kind of short, capped input `FlairTE` instances are good at,
so InitechWord embeds small `FlairTE` fields for those, never for the document body itself. If a future
operator wants convergence (InitechText re-plumbed onto InitechWord's engine), that is the same kind of
deferred investigation INITECH-123-plan Sec 6 P5 poses for InitechCalc vs Initech 123's shared core --
this plan does not force it, and Sec 6 P5 below states the same "investigate, do not force" posture.

---

## 3. Reference target + era ruling

**Recommendation: WordPerfect 5.1 for DOS (1989), text-mode, blue screen, F-key template, Reveal Codes,
`.WP5` (aka "WordPerfect 5.x") format, as Face A's model; a Mac-windowed WYSIWYG-ish Face B (Sec 5) grown
from the R4.2 InitechText lineage as the second native renderer over the same document core.**

Argued against the alternatives (mirrors INITECH-123-plan Sec 2's structure):

- **Microsoft Word 5.x for DOS (1989-91):** rejected as the PRIMARY target. Word 5.x is a strong,
  well-documented competitor, but through 1991-93 (`spec/win95ism_guardrails.md`'s era ceiling)
  WordPerfect was the dominant business word processor by a wide margin -- the same kind of
  majority-installed-base argument INITECH-123-plan Sec 2 used for Lotus 1-2-3 Release 2.2 over 3.x.
  WordPerfect's signature **Reveal Codes** (Alt-F3, a split-screen view of every hidden formatting code)
  is also the most pedagogically useful mechanism to reimplement -- it forces the document model to
  treat formatting as first-class data from day one (Sec 9). Word 5.x is cited, not golden: it sets the
  SHOULD-tier bar for "what a competitive alternative also offered" (Sec 5).
- **WordPerfect 5.1 (1989) vs 5.0 (1988) vs 6.0 (1993):** 5.0's WYSIWYG-ish elements were incomplete;
  5.1 added **Tables** (a headline new feature over 5.0) and is the best-documented frozen target,
  mirroring the "frozen, best-documented" argument INITECH-123-plan Sec 2 made for Release 2.2 over 2.0.
  6.0 moved toward a more graphical interface right at the era boundary and would repeat the SAMIR
  III+-vs-IV mistake ADR-0008 DEC-03 avoided (bigger, less-documented, no local ground truth). **This
  plan targets 5.1; a 6.0-only feature is ERA-SHORTFALL, not silently dropped (Rule 3).**
- **Word for Windows 2.0 (1991) / MacWrite II:** rejected as PRIMARY for the same reason
  INITECH-123-plan Sec 2 rejected 1-2-3 for Windows -- GUI-native products, not the DOS full-screen
  application a period reviewer typed at with a function-key template taped above the keyboard (PRD
  Sec 3's fidelity bar). Their existence justifies Face B (Sec 5): a windowed word processor is not
  anachronistic for FLAIR's Mac-heritage shell, just not the primary artifact. MacWrite II is Face B's
  closer visual-language reference (Geneva/Chicago, a ruler, a toolless single window), cited as mood
  reference, not a format source.

**Era ruling:** WordPerfect 5.1 (1989) sits inside the 1991-93 era ceiling (still the majority DOS
word processor through that window) and is named directly by ADR-0001 DEC-04's own forward note. No
Win95-ism risk: 5.1 predates any Windows word-processing convention (ribbons, WYSIWYG toolbars as
default) that `spec/win95ism_guardrails.md` forbids.

---

## 4. The two-face shape (Law 1 reconciliation, mirrors INITECH-123-plan Sec 3)

**Shared core (`os/word/core` + `os/word/fs`, mirrors `os/i123/core+fs` and `os/samir/{core,fs}`
exactly):** the document model (Sec 9: a piece table of paragraphs/runs/codes) and the `.WP5` codec,
touching the OS *only* through a PAL vtable in the ADR-0008 DEC-02 style (`word_pal.h`: byte file I/O,
an injectable clock for print/save timestamps if any, a fixed arena -- no `int 0x21`, no FLAIR calls in
`core`/`fs`). Host-gradable from day one exactly as SAMIR's and Initech 123's engines were.

**Face A -- "Classic" full-screen editing surface (P1-P4, primary, ships first).** A native `FlairApp`
tenant (`InitechWord_procs`, ADR-0013 Sec 3.1 vtable) whose `open()` creates ONE maximized/full-screen
document window and whose `event()`/drawing path renders the authentic WordPerfect-5.1-alike chrome
**natively**: the blue editing field, the reverse-video status line ("Doc 1 Pg 1 Ln 1" Pos 10"), the
Reveal-Codes split screen (Alt-F3), and the F-key template band -- all through the GrafPort using the
existing fixed 8x16 `chicago8x16` glyph cell (`spec/assets/chicago8x16.h`), the same monospace mechanism
InitechCalc and Initech 123's Face A already use. This satisfies ADR-0013 Sec 4 to the letter (one pixel
path, one event path, real co-residency, real WindowMgr z-order) while looking, frame-for-frame, like the
real product. **Packaging:** the engine plus dictionary/thesaurus data (Sec 9) is far too large for the
kernel image, so InitechWord ships as a **disk-launched InitechMZ `.EXE`** via the Part-D1 `INT 81h` gate
(`docs/design/GUI-remediation-D1-D2-D3-design.md` Part D1, `initech-tdnl.14`), exactly the Initech 123
packaging decision -- P1 cannot land its in-emulator gate until D1 is green (INITECH-123-plan Sec 3.3/7).

**Face B -- Mac-windowed WYSIWYG-ish view (P5, secondary, later; grows from the R4.2 InitechText
lineage per Sec 2).** A second, resizable-window renderer over the SAME core (same `.WP5` file, same
piece-table document) drawing a proportional-font (Geneva 9 body / Chicago 12 headings) paginated view
with a ruler and a minimal styles list, in ordinary System-7 chrome -- the natural evolution point Sec 2
names for InitechText's lineage without touching InitechText itself. True WYSIWYG needs proportional
metrics and distinct bold/italic glyphs, which the Font Manager (not yet built,
`docs/plans/FLAIR-implementation-plan.md:96`) does not yet provide -- Face B is gated on that landing
(Sec 9 Ruling 3) unless it ships first with a labeled fixed-font placeholder.

**What is period-defensible and what is not:** a fixed-cell full-screen editor drawn by a GUI toolbox
(Face A) is exactly the "screenshot is the spec" move InitechCalc and Initech 123 Face A already make;
it is not an anachronism. What would NOT be period-defensible is a class-2 SAMIR-style text tenant for
InitechWord -- ADR-0013 Sec 4's closing line rejects "a DOS box in a window" outright, and real
WordPerfect never ran that way on any period desktop either.

---

## 5. Feature inventory -- "full-featured" checklist (MUST / SHOULD / ERA-SHORTFALL)

| Area | Item | Tier |
|---|---|---|
| Document model | Paragraphs, runs, hidden formatting codes as first-class data (Reveal Codes is the heart of WP -- Sec 9) | MUST |
| | Multi-page documents (tested to >= 50 pages without a performance cliff) | MUST |
| | Multiple open documents (Doc 1 / Doc 2 window-split, `Shift-F3` Switch) | SHOULD |
| Editing | Insert / typeover (Ins key toggle) | MUST |
| | Block (Alt-F4): select, bold, underline, center, indent, delete, move, copy | MUST |
| | Search / Replace (F2 / Alt-F2), including a code-aware search (find a `[BOLD]` code) | MUST |
| | Undo (WP 5.1 shipped only single-level "Cancel/Undelete" (F1) restoring the last 3 deletions -- period-authentic undo is SHALLOW, not a modern multi-level stack; this plan's engine supports deeper undo internally (Sec 9 piece-table snapshots) but Face A's exposed UI matches the period-authentic shallow model) | MUST |
| Formatting | Bold / underline / italic (double-underline, strikeout as SHOULD) as codes | MUST |
| | Fonts / point sizes as codes (Ctrl-F8 Font) -- Face A shows an attribute indicator, not a distinct glyph (Sec 9 Font Manager note); Face B renders true distinct fonts once unblocked | MUST |
| | Margins, tabs (incl. tab align `Ctrl-F6`), justification (left/center/right/full), line spacing | MUST |
| | Page breaks (soft + hard), page numbering, suppress-on-page-1 | MUST |
| | Headers / footers | MUST |
| | Footnotes / endnotes (`Ctrl-F7`) with page-break-aware renumbering | MUST |
| | Newspaper / parallel columns (`Alt-F7`) | MUST |
| | Tables (`Alt-F7` in 5.1 -- rows/cols, cell borders; cell formulas/math ARE out of scope) | MUST (cell math = ERA-SHORTFALL) |
| | Merge (`Shift-F9` codes, `F9` primary/secondary merge, mail-merge-style) | SHOULD |
| | Outline / paragraph numbering (`Shift-F5`) | SHOULD |
| | Column math (`Alt-F7` Math, simple +/-/*/totals, NOT a recalc graph) | SHOULD, ERA-SHORTFALL if descoped |
| Spell / thesaurus | Spell check (`Ctrl-F2`) against a word list; thesaurus (`Alt-F1`) | MUST, dictionary DATA SIZE is an operator ruling (Sec 8 Ruling 2) |
| Macros | Keystroke macros (`Ctrl-F10` Define, `Alt-F10` Execute); the full WP 5.1 macro COMMAND language (`IF`/`CASE`/variables) is materially more complex than 1-2-3's `{command}` tokens | SHOULD, full command language = ERA-SHORTFALL if descoped to keystroke-replay only |
| List Files | `F5` List Files: navigate, retrieve, delete, rename, look, copy, print, word-search -- built on `os/flair/stdfile.c`'s existing navigate/select/return model + `FlairList` | MUST |
| Setup | `Shift-F1` Setup: display, keyboard, location of files, initial codes | SHOULD |
| F-key template | The full `F1`-`F12` + `Shift`/`Alt`/`Ctrl` template (28+ bindings) -- **hypothesis pending P0 golden verification, not yet Law-1-sourced (same posture as the `.WP5` table below)**: `F1` Cancel/Undelete, `F2`/`Alt-F2` Search fwd/Replace, `F3`/`Ctrl-F3` Help/Screen, `F4`/`Shift-F4` -->Indent<--, `F5` List Files, `F6`/`Shift-F6` Bold/Center, `F7`/`Shift-F7` Exit/Print, `F8`/`Shift-F8` Underline/Format, `F9`/`Shift-F9` Merge R/Merge Codes, `F10`/`Shift-F10` Save/Retrieve, `Alt-F3` Reveal Codes, `Alt-F4` Block, `Alt-F6` Flush Right, `Alt-F7` Columns/Table, `Alt-F8` Style, `Alt-F10` Macro, `Ctrl-F2` Spell, `Ctrl-F4` Move, `Ctrl-F7` Footnote, `Ctrl-F8` Font, `Ctrl-F10` Macro Define | MUST |
| Print | Draft to a spooled `PRN` text file (era printer definitions applied as a formatting pass, not a real device path -- `PC LOAD LETTER` canon preserved on failure) | MUST |
| | Real per-printer driver files (WP 5.1's `.PRS`/`.ALL` printer-definition binaries) | ERA-SHORTFALL (blocked on the Print Manager, Sec 9) |
| File formats | `.WP5` native (round-trip) | MUST |
| | Plain ASCII text import/export (`Ctrl-F5` Text In/Out) | MUST |
| | `.DOC`/RTF import/export | ERA-SHORTFALL -- RTF existed by 1987 but was a Word-ecosystem interchange format, not commonly read/written by WordPerfect 5.1 itself; no local ground truth exists for it and adding it would blur the reference target (same posture INITECH-123-plan Sec 4 took on `.WK3`) |

---

## 6. Ground truth + oracles (Law 1 / Law 2)

Distilled spec files now exist in the corpus: `../wordperfect51-decomp/specs/` (`file-formats`;
bead initech-mbn1, closed).

**(a) `.WP5` codec ground truth.** Mirror ADR-0008 DEC-05/DEC-06 and INITECH-123-plan Sec 5(a) exactly:
an INDEPENDENT Python reader/writer (`wp5_ref.py`, the `dbf_ref.py`/`wk1_ref.py` pattern), NOT sharing
offset constants with the C engine's own `spec/word/wp5_format.h` (the "independence barrier" lesson).
The starting hypothesis (from public format-preservation literature: a fixed leading area starting with
a `0xFF` sentinel and a `"WPC"` product/type/version identifier, a document-area offset, and a formatting
"packet" area holding per-code records) is **explicitly NOT yet Law-1-sourced** -- every byte offset
must be independently re-verified against a real-5.1-minted golden (same discipline INITECH-123-plan
Sec 4's closing line applies to its `.WK1` table). **No `.WP5` byte layout is locked until P0 mints it.**

**(b) Reveal Codes + real goldens -- the golden-acquisition gate (Law 1, MUST run first).** Stand up a
sister repo `../wordperfect51-decomp`, structured identically to `../dbase3-decomp` (`specs/`,
`goldens/`, `re/`, `oracles/`, `SOURCES.md`, `GAPS.md`, gitignored binaries, "abandonware, local
reference material only" posture -- **operator-flagged, copyrighted media**, same flag INITECH-123-plan
Sec 7 bead 1 carries). Mint goldens under DOSBox-X/86Box: a document corpus (plain text, one doc per
Sec 5 formatting feature, a footnoted doc, a tabled doc, a multi-column doc) plus Reveal-Codes
screen-capture dumps (raw `.WP5` bytes AND the on-screen code-glyph vocabulary -- `[BOLD]`, `[UND]`,
`[HRt]`, `[SRt]`) plus the F-key template's real bindings (a verified reference, not memory) plus one
dictionary/thesaurus sample for Ruling 2's sizing. **No formatting code or F-key binding is locked until
minted from real 5.1 or independently cross-derivable** -- the `dbase3-decomp` GAPS.md
`[oracle-resolves]` discipline applies from day one, since **no `wordperfect51-decomp` corpus exists
yet** -- P0's job is to create it.

**(c) Document-model property tests.** The piece-table engine (Sec 9) gets the region-engine/SAMIR-style
property suite: code-insertion invariants (a code's insertion/removal never corrupts a neighboring run
or code scope), reflow idempotence (re-laying-out an unmodified document twice is byte-identical), and
undo/redo round-trip (apply N edits, undo N, redo N -> byte-identical to the original).

**(d) Reveal Codes golden dumps.** A diff oracle comparing a serialized Reveal-Codes-mode screen dump
(the code-glyph stream, not pixels) against the P0 mint corpus's captures.

**(e) Print-to-PRN byte goldens.** Mirrors INITECH-123-plan Sec 6 P4: the formatted stream written to
the `PRN` spool file is diffed byte-for-byte against a golden from real WordPerfect 5.1's "Print to File".

**(f) Font Manager / Print Manager sequencing.** Face A needs neither; Face B and real printer output
are gated on services not yet built (Sec 9's two dedicated risk items give the full argument and cite).

**(g) Mutants per phase (Rule 6) / Rule-14 clip.** Every `test-word-*` gate ships with a `-mutant`
sibling (flip a `.WP5` prefix constant, a code-scope boundary, a piece-table link, a footnote renumber)
shown RED for the right reason before being trusted; Face B is graded by a `record-flair` clip + operator
eyeball (Law 4), never a hard SSIM gate (`harness/ssim.c` NOT YET BUILT) -- the posture
INITECH-123-plan Sec 5(d)/(e) states.

---

## 7. Phases

Each phase leaves `make test` green with its own gate, sized in agent-sessions (the SAMIR-plan/
INITECH-123-plan convention: a session = one focused implementation pass with its own oracle run).

- **P0 -- Golden acquisition (Law 1 gate, MUST run before any other phase).** ~3-4 sessions. Stand up
  `../wordperfect51-decomp` (README/GAPS/SOURCES scaffolding cloned from `../dbase3-decomp`); acquire a
  real WordPerfect 5.1 distribution + license it in place (gitignored, same posture as the dBASE/Lotus
  binaries -- **operator-flagged, copyrighted media**); stand up the DOSBox-X/86Box mint harness; mint
  the first batch of `.WP5` fixtures (blank doc, plain text, bold/underline/center, one footnote, one
  table) + Reveal-Codes dumps + the F-key template's real bindings + one dictionary/thesaurus file
  sample. **Exit:** `../wordperfect51-decomp/goldens/` exists with >= 5 real `.WP5` files + Reveal-Codes
  dumps; `SOURCES.md` records provenance; no Phase P1 oracle may reference a golden that does not exist
  yet (loud-skip, never guessed).

- **P1 -- Document model core + `.WP5` round-trip (plain text) + Face A skeleton.** ~5-6 sessions.
  `word_pal.h` (ADR-0008-DEC-02-style contract) + `pal_host.c`; the piece-table document model (Sec 9)
  for plain paragraphs (no codes yet); `wp5_ref.py` independent reader (pulled forward, Sec 6a); `.WP5`
  codec for the document-header prefix + plain-paragraph text; Face A's minimal full-screen window
  (blue field, native `chicago8x16` typing/cursor/CR-wrap, no menu/status line/codes yet); save/retrieve
  to a fixed path (List Files integration is P3). **Oracle:** `test-word-wp5-roundtrip` (+mutant).
  **Exit:** a hand-typed plain document round-trips losslessly through `.WP5`; a P0 golden plain-text
  doc reads back with correct displayed text.

- **P1b -- Disk-launch packaging via the D1 `INT 81h` gate.** ~1-2 sessions. Type: infra/integration.
  Depends on: P1, `initech-tdnl.14` (D1 disk-tenant gate). Acceptance: InitechWord ships as an
  InitechMZ flat-32 `.EXE`, launches via double-click through the `INT 81h` gate, registers, and runs the
  P1 plain-text demo under QEMU + Bochs with the D1.7 serial launch trace green.

- **P2 -- Codes + formatting + Reveal Codes + block ops.** ~7-9 sessions. The hidden-code-as-run model
  (bold/underline/italic/center/indent/margin/tab codes as pieces, Sec 9); Reveal Codes (`Alt-F3`)
  split-screen native rendering; Block (`Alt-F4`) select/bold/underline/center/indent/delete/move/copy;
  the reverse-video status line; the F-key template's core bindings (Sec 5) wired to real actions.
  **Oracle:** `test-word-codes` (+mutant, code-insertion invariants); `test-word-reveal-codes` (golden
  dump diff vs the P0 mint corpus). **Exit:** bold/underline/center/margins/tabs round-trip through
  `.WP5` and display correctly in both plain and Reveal-Codes views.

- **P3 -- Search/replace, undo, F-key template complete, List Files.** ~5-6 sessions. Search/replace
  including code-aware search; period-authentic shallow undo/undelete (`F1`, Sec 5); the remaining
  F1-F12 (+Shift/Alt/Ctrl) bindings; List Files (`F5`) built on `os/flair/stdfile.c`'s existing
  navigate/select/return model + `FlairList`. **Oracle:** `test-word-undo` (+mutant, reflow idempotence +
  undo/redo round-trip); `test-word-fkeys` (template-to-action golden). **Exit:** every MUST-tier F-key
  combination performs its documented action; List Files retrieves a P0 golden doc into Face A correctly.

- **P4 -- Print + headers/footers/footnotes/columns/tables.** ~8-10 sessions (the largest phase: tables,
  columns, and footnotes are each nontrivial layout work). Headers/footers with suppress-on-page-1;
  footnote/endnote numbering with page-break-aware renumbering; newspaper/parallel columns; WP 5.1
  tables (rows/cols, cell borders; cell math out of scope, ERA-SHORTFALL); print-to-PRN spooled text
  file with the era printer definition applied as a text-formatting pass (`PC LOAD LETTER` canon
  preserved on a print-path failure). **Oracle:** `test-word-print` (spooled-file byte diff);
  `test-word-tables` / `test-word-columns` (+mutants); `test-word-footnote` (page-break-aware fixture).
  **Exit:** a business-letter-plus-table-plus-footnote demo doc prints to a spooled file matching the
  golden and displays identically in-emulator.

- **P5 -- Face B window (InitechText-lineage convergence investigation), spell-check data strategy.**
  ~5-6 sessions. The Geneva-9/Chicago-12 proportional windowed renderer (Sec 4); ruler; a minimal
  styles list. **Investigate (do not force)** whether R4.2 InitechText's own engine can share Face B's
  piece-table core without breaking InitechText's own light-editor acceptance bar (Sec 2's default
  ruling is KEEP-separate; only converge if the investigation shows a clean win, mirroring
  INITECH-123-plan Sec 6 P5's identical posture for InitechCalc). Spell check: land the dictionary-
  loading PAL + a REDUCED starter word list sized to the 1.44 MB data-volume budget (Sec 9 Ruling 2);
  the FULL WP5.1-scale dictionary is an explicit operator ruling, not decided here. **Oracle:** Rule-14
  `record-flair` clip; `test-word-facea-faceb` structural agreement test; `test-word-spell-reduced`
  (known-misspelling fixture against the reduced list).

- **P6 -- Era-shortfall ledger + competitive statement.** ~1-2 sessions, documentation + gate wiring
  only. Write the final ERA-SHORTFALL table (`.DOC`/RTF, the full macro command language, column math,
  real per-printer `.PRS` drivers, full-scale dictionary/thesaurus if reduced, WP 6.0-only features) as
  a locked ledger (`spec/word/era_shortfalls.md`, Rule 8 discipline) and a one-page "what a 1991-93
  period reviewer would and would not find missing" statement, mirroring INITECH-123-plan Sec 6 P6 and
  ADR-0012 D-3's SAMIR precedent.

**Total estimate: ~35-45 agent-sessions**, somewhat larger than Initech 123's ~28-37 given tables,
columns, and footnotes are each their own nontrivial layout subsystem and a word processor's document
model (Sec 9) is a bigger lift than a cell-grid sparse store.

---

## 8. Proposed bead tree (for the PM to `bd create`; IDs NOT assigned here)

**Epic: "InitechWord -- WordPerfect 5.1 Initech-version" (parent, P1 priority, blocks nothing, blocked
by ADR-0012's Platform-Services epic for the App-Contract launch surface, same as the Initech 123 epic).**

1. **[FIRST -- Law 1 gate, operator-flagged: copyrighted media] `wordperfect51-decomp` golden
   acquisition + mint harness.** Type: research/infra. Priority: P0. Acceptance: `../wordperfect51-
   decomp` exists with README/GAPS/SOURCES cloned from the dbase3-decomp pattern; a real WordPerfect 5.1
   distribution is licensed-in-place and gitignored; DOSBox-X/86Box mint harness reproduces >= 5
   `.WP5` fixtures + Reveal-Codes dumps + the real F-key template; `wp5_ref.py` independent reader
   agrees with the byte dumps on all fixtures. Blocks every phase below.

2. **P1 -- Document model core, `.WP5` plain-text round-trip, Face A skeleton.** Type: feature.
   Priority: P1. Depends on: bead 1. Acceptance: `test-word-wp5-roundtrip` (+mutant) green; a
   hand-typed plain document round-trips losslessly; a P0 golden plain-text doc reads back correctly;
   Face A window opens as a native FlairApp tenant (compiled-in demo build acceptable for this bead;
   disk-launch packaging is bead 2b).

2b. **P1b -- Disk-launch packaging via the D1 INT 81h gate.** Type: infra/integration. Priority: P1.
   Depends on: bead 2, `initech-tdnl.14` (D1 disk-tenant gate). Acceptance: InitechWord ships as an
   InitechMZ flat-32 `.EXE`, launches via double-click through the `INT 81h` gate, and runs the P1 demo
   under QEMU + Bochs with the D1.7 serial launch trace green.

3. **P2 -- Codes, formatting, Reveal Codes, block ops.** Type: feature. Priority: P1. Depends on: bead
   2. Acceptance: `test-word-codes` (+mutant) and `test-word-reveal-codes` green against the P0 mint
   corpus; bold/underline/center/margins/tabs round-trip through `.WP5`.

4. **P3 -- Search/replace, undo, F-key template complete, List Files.** Type: feature. Priority: P1.
   Depends on: bead 3. Acceptance: `test-word-undo` (+mutant) and `test-word-fkeys` green; every
   MUST-tier F-key combination performs its documented action; List Files retrieves a golden doc.

5. **P4 -- Print, headers/footers, footnotes, columns, tables.** Type: feature. Priority: P2. Depends
   on: bead 4. Acceptance: `test-word-print`/`test-word-tables`/`test-word-columns`/`test-word-footnote`
   (+mutants) green; the business-letter demo prints to a spooled file matching the golden.

6. **P5 -- Face B Mac-windowed view + InitechText-lineage investigation + spell-check data strategy.**
   Type: feature. Priority: P2. Depends on: bead 5, Sec 8 Ruling 3 (Font Manager sequencing decided).
   Acceptance: Face B renders the same `.WP5` file as Face A with structural agreement
   (`test-word-facea-faceb`); `record-flair` clip filed for operator eyeball; the reduced-dictionary
   spell check passes `test-word-spell-reduced`; the InitechText-convergence investigation is documented
   as a decision (converge or defer) with InitechText's own R4.2 acceptance re-run green regardless.

7. **P6 -- Era-shortfall ledger + competitive statement.** Type: docs/governance. Priority: P3. Depends
   on: bead 6. Acceptance: `spec/word/era_shortfalls.md` locked (Rule 8) and worklog entry filed.

---

## 9. Risks

- **Memory model: piece table vs. gap buffer, with numbers -- RECOMMEND piece table.** A gap buffer
  is cheap when edits cluster at the cursor but pays an O(distance) memmove whenever an edit lands
  elsewhere -- and Reveal Codes / code-aware search-replace / reformatting are exactly that "edit far
  from the last cursor" pattern, on top of needing codes as first-class structural data (Sec 5) which a
  gap buffer has no natural slot for. A **piece table** (immutable original buffer + append-only add
  buffer + a list of `{buffer_id, offset, length, kind}` records, `kind` = TEXT or CODE) makes Reveal
  Codes a direct rendering of the piece list and makes undo a cheap snapshot/inverse-op operation
  (period-authentic shallow undo, Sec 5). Worked numbers: a piece record is ~16 bytes; a heavily-edited
  50 KB document typically fragments to ~1,000-3,000 pieces in real piece-table editors (~16-48 KiB of
  piece list) plus a ~20-60 KiB add buffer, so call it **~100-200 KiB per open document** -- comfortably
  inside the 4 MiB master FLAIR heap (`spec/hardware.json` FLAIR_HEAP `[0x100000,0x500000)`, the heap
  INITECH-123-plan Sec 8 sizes its own sparse cell store against) and nowhere near the ~187 KiB class-2
  program window (`spec/memory_map.h` PROGRAM_BASE/ENV_BLOCK) -- a second, independent argument
  (alongside Sec 3's ADR-0013 argument) against ever squeezing InitechWord into that window.

- **Kernel size wall.** Per the WL-0090/initech-8z9j size-policy precedent, a full word-processing
  engine plus dictionary data MUST NOT compile into the kernel image alongside HELLO/NOTES/InitechCalc.
  Bead 2b (disk-launch via the D1 `INT 81h` gate) is the only path that avoids reopening that gate --
  the identical argument INITECH-123-plan Sec 8 makes for its own bead 2b.

- **Dictionary/thesaurus size vs. the 1.44 MB data volume (Sec 8 Ruling 2).** `flair_data.img` is a
  primary-slave **FAT12 floppy** (`spec/hardware.json` `"formats": ["FAT12 floppy image", ...]`) --
  1.44 MB shared with every other app's data. Real WordPerfect 5.1's compressed common dictionary
  (`WP{WP}US.LEX`) ran to a few hundred KB, plus thesaurus data on top; **no Law-1-verified figure exists
  yet (P0's job, Sec 6b)**, but even a conservative estimate can eat a large fraction of one floppy.
  Three options, none pre-decided: (a) a REDUCED starter word list (P5's default, ERA-SHORTFALL-tagged);
  (b) a SEPARATE companion volume (second floppy, or the existing FAT16 HDD format) mounted on demand;
  (c) accept the full dictionary and grow `flair_data.img` (its own FAT12/boot-cost ripple). **Ruling 2**
  -- the operator must pick before P5 locks spell-check scope.
  **RULED 2026-10-03 (operator): start with the reduced starter word list (a), target a
  separate companion volume (b) for the full-scale dictionary; do NOT grow `flair_data.img`
  (c rejected).** Measured from the installed WordPerfect 5.1 tree
  (`/home/tobias/Projects/wordperfect51-decomp/mint/work/WP51`): `WP{WP}US.LEX` 363,165 bytes,
  `WP{WP}US.THS` 358,472 bytes, `WP{WP}US.HYC` 9,676 bytes; total 731,313 bytes against the
  1,474,560-byte data volume (about 50 percent of one floppy before any app data).

- **Font Manager absence blocks true Face B WYSIWYG (Sec 8 Ruling 3).** `docs/plans/FLAIR-
  implementation-plan.md:96` still lists "Font Manager + proportional Chicago + txFace styles" as
  un-built. Face A needs none of it (fixed 8x16 cells, attribute-only bold/underline -- period-authentic
  for text-mode, not a shortfall). Face B cannot render true proportional/styled text until the Font
  Manager lands. **Ruling 3:** ship Face B early with a labeled fixed-font placeholder (explicit
  ERA-SHORTFALL), or gate P5 entirely on the Font Manager -- the same sequencing dependency
  INITECH-123-plan Sec 5(c) flags for its own numeric-mechanism ruling.
  **RULED 2026-10-03 (operator): Face B is gated on the Font Manager; no fixed-font
  placeholder.** Face A and phases P1-P4 proceed without it.

- **Printing without a Print Manager.** `initech-o5vm` is BLOCKED until the GrafPort verb layer lands
  (`re30` P3-pre; `grafProcs` NULL today, `docs/HANDOFF.md:426`, `docs/worklog/WL-0065-...md`). P4's
  print oracle therefore targets the same interim path INITECH-123-plan Sec 6 P4 uses: draft text to a
  spooled `PRN` file with the era printer definition as a formatting pass, `PC LOAD LETTER` canon on
  failure. Real per-printer `.PRS`/`.ALL` driver support is ERA-SHORTFALL pending the Print Manager,
  mirroring `ADR-0013-FLAIR-App-Contract.md:193`'s note that print canon lives in app content, not the
  service -- InitechWord's own print canon (P0-minted) lives in `spec/word/`.

- **`os/flair/textedit.{h,c}`'s reduced-TERec limits -- Face B does NOT reuse it for the document body
  (decided, Sec 2/4).** `FlairTE`'s locked ceilings (`FLAIR_TE_TEXT_MAX = 4096`, `FLAIR_TE_MAX_LINES =
  128`, `textedit.h:67-68`, `_Static_assert`-pinned Rule-8 constants) are architecturally too small for
  a multi-page document, and even a large raise would not give code-as-run modeling, undo, or styling
  (Sec 9's piece table exists to provide those). **Ruling:** the document body is a NEW engine
  (`os/word/core`), not a scaled-up `FlairTE`. `FlairTE` instances ARE reused, unmodified, for
  InitechWord's own short capped fields (filename, search string, header/footer entry) and for
  InitechText itself (Sec 2, unchanged) -- the reduced TERec's actual sweet spot.

- **TPS/no-pointers subset is irrelevant here.** InitechWord is a bundled C app (ADR-0002), never
  Pascal/TPS (Law 3) -- the identical note INITECH-123-plan Sec 8 makes for Initech 123.

- **ADR-0013 amendment risk.** A literal character-mode "authentic full-screen console" InitechWord
  requires formally reopening ADR-0013 Sec 4 -- a Core-tier, ADR-by-committee act (Rule 7), not a
  decision this plan or its implementing agent may make unilaterally, identical to the risk
  INITECH-123-plan Sec 8 records for Initech 123.

---

## 10. Sources

Local (Law 1): `CLAUDE.md` (Laws 1-4, Rules 6/8/14); `InitechOS-PRD.md` Sec 1.4 (the app-platform bar,
quoted Sec 1) + Sec 5; `docs/adr/ADR-0001-Hardware-Target-386-Origin-486-Floor.md:39,89,107` (486 floor,
the explicit dBASE III+/IV + Lotus 1-2-3 + WordPerfect era note); `docs/adr/ADR-0008-SAMIR-InitechBase-
Architecture.md` (the PAL/storage-split/golden-tiering pattern this plan mirrors); `docs/adr/ADR-0013-
FLAIR-App-Contract.md:164-166,193,228` (the native-vs-text-host ruling naming InitechWord by name; the
Print Manager's future scope note); `docs/design/GUI-remediation-D1-D2-D3-design.md` Part D1 (disk-
launched InitechMZ tenant, `INT 81h` gate); `docs/plans/GUI-remediation-plan.md:157-158` (R4.2
InitechText); `docs/plans/INITECH-123-plan.md` (the sibling plan this document mirrors throughout --
Sec 2 reference-target argument shape, Sec 3 two-face reconciliation, Sec 5 ground-truth strategy, Sec 6
phasing, Sec 7 bead tree, Sec 8 risks); `docs/plans/FLAIR-implementation-plan.md:92,96` (Print Manager
scope, Font Manager not-yet-built note); `docs/HANDOFF.md:426` and `docs/worklog/WL-0065-flair-phase45-
platform-services-first-wave.md` (Print Manager BLOCKED status, Standard File/TextEdit/Scrap CLOSED
status); `os/flair/textedit.h`/`os/flair/textedit.c` (180 + 401 LOC, the reduced monostyled TERec and
its locked capacity ceilings); `os/flair/stdfile.c` (462 LOC, the Standard File navigate/select/return
model List Files reuses); `os/flair/scrap.c` (209 LOC, the shell-owned cross-tenant Scrap InitechWord
and InitechText both route through); `spec/win95ism_guardrails.md` (era ceiling); `spec/memory_map.h`
(`PROGRAM_BASE`/`ENV_BLOCK` window sizing used in Sec 9); `spec/hardware.json` (the 4 MiB FLAIR heap
window, the FAT12-floppy/FAT16-HDD data-volume formats used in Sec 9's dictionary-size risk);
`spec/assets/chicago8x16.h` (the fixed-cell font reused for Face A); `Makefile` (`DBASE3_DECOMP` loud-
skip idiom, mirrored as `WORDPERFECT51_DECOMP ?= ../wordperfect51-decomp`).

Sister-corpus pattern (not yet created for WordPerfect; cited as the template): `../dbase3-decomp/
README.md`, `../dbase3-decomp/GAPS.md`, `../dbase3-decomp/SOURCES.md` (the acquisition-and-licensing
discipline Sec 7 P0 clones for `../wordperfect51-decomp`).

Non-local, independent-oracle candidates (semantics cross-check only, never an on-disk-byte authority,
same posture as `oracles/harbour` and the Gnumeric/LibreOffice `.wk1` filters cited by INITECH-123-plan
Sec 5a): publicly archived WordPerfect file-format reference documentation and format-preservation-site
write-ups, to be acquired into `../wordperfect51-decomp/archive/` with full provenance per bead 1's
acceptance criteria. **No `.WP5` byte layout and no F-key template binding in Sec 5/6 of this plan is
yet Law-1-sourced; all are hypotheses pending Phase P0 minting.**

---

*-- End of Plan (DRAFT; pending operator rulings, Sec 8, and PM bead-tree filing) --*

<!-- Tedium certified compliant with NFR-7. Reveal Codes' [BOLD]/[UND]/[HRt]/[SRt] glyph vocabulary is
     WordPerfect's own, not InitechCalc's 570-/116% canon; do not conflate the two apps' locked fixtures. -->
