<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# Initech 123 -- Granular Implementation Plan (the Lotus 1-2-3 Initech-version)

**Issuing Body:** Initech Systems Corporation -- Platform Engineering, Application Platform Section
**Document Class:** Implementation Plan (living; supersede in place)
**Programme / Milestone:** InitechOS (STAPLER) -- app-platform bar (PRD Sec 1.4, ADR-0012), the canonical
suite's spreadsheet leg. Sibling of `docs/plans/SAMIR-implementation-plan.md` (M6, ratified) and
`docs/plans/GUI-remediation-plan.md` R4.6/R4.7.
**Status:** DRAFT for PM bead-tree filing. No architecture in this plan is self-ratifying; Sec 3's
reconciliation and Sec 8's three open rulings need operator/ADR sign-off before Phase P1 code, exactly
as ADR-0008 gated SAMIR's Phase 0 (SAMIR-implementation-plan.md Sec 10).
**Last Reconciled:** 2026-09-26

> Read order for an implementing agent: `CLAUDE.md` (Laws/Rules) -> `InitechOS-PRD.md` Sec 1.4/6.5/6.6
> -> ADR-0001 (486-class floor, DEC-04's own forward pointer to "Initech 123") -> ADR-0008/ADR-0009
> (the SAMIR PAL + numeric pattern this plan borrows) -> ADR-0013 Sec 4 (the ALREADY-RATIFIED
> native-vs-text-tenant ruling that names Initech 123 explicitly -- read this before designing any
> "text tenant" for 123) -> `docs/design/GUI-remediation-D1-D2-D3-design.md` Part D1 (the disk-launched
> tenant / INT 81h gate, `initech-tdnl.14`, in flight) -> `docs/plans/GUI-remediation-plan.md` R4.6/R4.7
> -> `spec/win95ism_guardrails.md` (era ceiling) -> this plan -> the sister corpus pattern in
> `../dbase3-decomp` (README.md, GAPS.md) which Sec 5 proposes cloning as `../lotus123-decomp`.

---

## 1. Executive summary

The operator's 2026-09-25 north-star restatement (ADR-0001 DEC-02, the 486-class floor ratification)
names three canonical apps the suite must ship at "could have been purchased and judged competitive"
depth: dBASE III+ (SAMIR, exists), a word processor (not yet planned), and **Lotus 1-2-3**. This plan
covers the third. ADR-0001 DEC-04 already anticipates this app by name: "A hardware-float profile for
SAMIR and any future spreadsheet (Initech 123) ... a documented amendment to ADR-0009" -- so the numeric
question is pre-flagged, not new.

**Load-bearing finding (Sec 3): the task brief's suggested "class-2 text tenant first" design is IN
CONFLICT with an already-ratified ADR.** ADR-0013 Sec 4 ("The native-vs-text-host fork ruling") names
*Initech 123* explicitly and rules it MUST ship as a **native FLAIR windowed tenant**, never a
character-mode/text tenant, because co-residency and the single pixel/event spine (ADR-0006 D-2/E-D2)
forbid a full-screen text host from living inside the desktop. This plan follows the ratified ADR over
the brief's suggestion (Law 1: a local ratified source outranks an unratified proposal) and designs an
ADR-0013-compliant two-face split instead (Sec 3.3): both faces are native `FlairApp` tenants sharing
one recalc/.WK1 core; the "authentic slash-menu, full-screen text look" is reproduced by NATIVE pixel
drawing (the existing `chicago8x16` fixed-cell font, spec/assets/chicago8x16.h) rather than by an
INT-21h character console. This is period-defensible: it is exactly how a period *Mac* 1-2-3-alike
would have rendered a monospace worksheet, and it is exactly how the frame's InitechCalc window already
draws its grid.

Reference target: **Lotus 1-2-3 Release 2.2** for DOS (Sec 2). Ground truth: a new sister corpus
`../lotus123-decomp`, cloned from the `../dbase3-decomp` pattern (Sec 4). Phasing: P0 (golden
acquisition, FIRST, Law 1 gate) through P6 (era-shortfall ledger), Sec 6. Proposed bead tree: Sec 7.
Three rulings the operator must make before Phase P1 ships code: Sec 8, repeated in the final report.

---

## 2. Reference target + era ruling

**Recommendation: Lotus 1-2-3 Release 2.2 (1989) for DOS, text-mode, slash-menu, 256 cols (A..IV) x
8192 rows, `.WK1`.**

Argued against the alternatives:

- **Release 1A / 2.0 (1983/1985):** too early -- 2.0 introduced `.WK1` and macros but 2.2's expanded
  `@`-function set and `/Data Query`/`/Data Table` maturity are the ones later software (and users'
  muscle memory circa 1991-93) actually expects. Rejected as under-featured for "full-featured."
- **Release 2.2 / 2.3 (1989 / 1991):** **RECOMMENDED.** 2.x was still, by a wide margin, the dominant
  installed base on small-business 386/486 PCs through the early 90s -- it required only ~320-640K
  conventional RAM and no graphics coprocessor, so it kept shipping and kept being bought long after
  3.x existed. 2.3 (1991) added the Wysiwyg add-in and mouse support but is a strictly larger,
  less-well-documented target; 2.2 is the frozen, best-documented `.WK1` shape (Release 2.01 froze the
  `.WK1` record set; 2.2 is additive on top) and is the version most home/office archives and format
  documentation (the historical Lotus File Formats manual mirrored across format-preservation sites,
  and the WK1 support baked into Gnumeric/LibreOffice's `sc` filter, both **independent** of any
  InitechOS source) describe most completely. **This plan targets 2.2; a 2.3-only feature (Wysiwyg,
  mouse) is out of scope and flagged ERA-SHORTFALL, not silently dropped (Rule 3).**
- **Release 3.x (1989, DOS-protected-mode / EMS, `.WK3`, 3-D worksheets):** rejected as the primary
  target. `.WK3` is a different on-disk format (multi-sheet, BIFF-adjacent structure) with no local
  ground truth and a materially larger engine (3-D references, protected-mode overlay management);
  taking it on now would repeat the SAMIR IV-vs-III+ mistake ADR-0008 DEC-03 explicitly avoided
  (guessing a bigger format with no golden). 3-D worksheets are recorded as an accepted ERA-SHORTFALL
  (Sec 4), same posture as SAMIR's deferred `.mdx`.
- **1-2-3 for Windows (1991) / 1-2-3/G:** rejected. It is GUI-native Windows 3.x code, not a DOS
  text-mode app; adopting it as "the reference" would blur exactly the artifact this plan is chasing
  (the iconic slash-menu full-screen worksheet is the thing "Lotus 1-2-3" means to a period reviewer,
  PRD Sec 3's fidelity bar). Its existence is noted only as the historical justification for this
  plan's Face-B Mac-windowed rendering option (Sec 3.3) -- contemporary spreadsheets *did* eventually
  go windowed, so a windowed 1-2-3-alike is not anachronistic, just not the primary artifact.

**Era ruling:** Release 2.2 sits inside the `spec/win95ism_guardrails.md` 1991-93 era ceiling (2.2
shipped 1989, was still the majority installed base into 1993) and is consistent with ADR-0001 DEC-02's
486-class floor and DEC-04's era note (dBASE III+/IV, Lotus 1-2-3, WordPerfect "into the *Office
Space*-frame period"). No Win95-ism risk: 2.2 predates any Windows spreadsheet convention.

---

## 3. Coexistence with InitechCalc + the native-tenant ruling (Law 1 reconciliation)

### 3.1 What is already ratified (read before designing anything)

- **ADR-0013 Sec 4** ("The native-vs-text-host fork ruling (Initech 123 / InitechWord)"): *"Initech 123
  and InitechWord are NATIVE FLAIR Toolbox tenants ... NOT character-mode tenants, and NOT run inside
  any 'windowed text-console host.'"* Rationale given: (1) Law 4 -- the frame shows System-7 document
  windows with live pull-down menus, not a full-screen TUI; (2) only native tenants participate in the
  WindowMgr z-order and can co-reside; (3) a character-mode program inside a FLAIR window would need a
  **second pixel path and a second event path**, an ADR-0006 D-2/E-D2 violation. The same section
  explicitly reserves the character-mode/text-tenant lane for SAMIR and Turbo Initech ("the counter
  precedent") and explicitly REJECTS a "DOS box in a window."
- **R4.6** (`docs/plans/GUI-remediation-plan.md`): InitechCalc is the frame's own ledger-grid app (Geneva
  9, the 116% pie, the `570-` trailing-minus cell format) and must ship as a real tenant, unmodified in
  behavior. It is **not** Initech 123 -- it is a separate, smaller in-frame prop app.
- **Part D1** (`docs/design/GUI-remediation-D1-D2-D3-design.md`): a disk-launched native GUI tenant
  ships as an InitechMZ flat-32 `.EXE`, acquires the Toolbox API via a single `INT 81h` trap gate
  multiplexed by AX (vector, ABI, and oracle set fully specified D1.1-D1.7), and is loaded from the
  master FLAIR heap, not `PROGRAM_BASE` (which stays free for SAMIR's class-2 EXEC). This work is in
  flight now as `initech-tdnl.14`.

### 3.2 Consequence: the brief's suggested split cannot ship as literally worded

The task brief's "face A = class-2 text tenant with the authentic slash menu (SAMIR pattern), face B =
Mac-windowed frame app" is the exact shape ADR-0013 Sec 4 forbids for this app by name. Two honest
options exist: (a) ask the operator to open an ADR-0013 amendment carving an exception, or (b) find a
period-defensible design that delivers the same visible outcome (an authentic-looking slash-menu
full-screen worksheet) without violating the ratified single-pixel/single-event-spine law. This plan
takes (b) -- it costs nothing architecturally and is arguably *more* honest to how a Mac-ported 1-2-3
would actually have worked.

### 3.3 The reconciled design: one engine, two NATIVE renderer faces

**Shared core (`os/i123/core` + `os/i123/fs`, mirrors `os/samir/{core,fs}` exactly):** the recalc
engine (dependency graph, RPN evaluator, `@function` library) and the `.WK1` codec, touching the OS
*only* through a PAL vtable in the ADR-0008 DEC-02 style (`i123_pal.h`: byte file I/O, an injectable
clock, a fixed arena -- no `int 0x21`, no FLAIR calls in `core`/`fs`). This is what makes the engine
host-gradable from day one exactly as SAMIR's engine was (Phases P1-P4 below are host-only).

**Face A -- "Classic" full-screen worksheet (P1-P4, primary, ships first).** A single native `FlairApp`
tenant (`Initech123_procs`, ADR-0013 Sec 3.1 vtable) whose `open()` creates ONE maximized/full-screen
document window and whose `event()`/drawing path renders the authentic Release-2.2 chrome **natively**:
a reverse-video mode-indicator line, the cell grid, and the slash-menu overlay, all drawn through the
GrafPort using the existing fixed 8x16 `chicago8x16` glyph cell (`spec/assets/chicago8x16.h`) as the
monospace grid font -- the same mechanism InitechCalc already uses to draw its own grid, just at a
fixed cell pitch instead of proportional Geneva 9. This satisfies ADR-0013 Sec 4 to the letter (one
pixel path, one event path, real co-residency, real WindowMgr z-order) while looking, frame-for-frame,
like the real full-screen slash-menu product. **Packaging:** because a full-featured 1-2-3 engine is far
too large to compile into the kernel image (Sec 9 kernel-size-wall risk), Initech 123 ships as a
**disk-launched InitechMZ `.EXE`** via the Part-D1 `INT 81h` gate (`initech-tdnl.14`'s mechanism),
**not** compiled in alongside HELLO/NOTES/InitechCalc. This is a real dependency: P1 cannot land its
in-emulator gate until `initech-tdnl.14` (D1) is green.

**Face B -- Mac-windowed view (P5, secondary, later).** A second, smaller renderer over the SAME core
(same `.WK1` file, same recalc graph) that draws a proportional-font (Geneva 9) scrollable grid in an
ordinary resizable System-7-chrome window with the InitechCalc-style pie/chart furniture -- i.e. "what a
period Mac port of 1-2-3 would have looked like," not a second app. This is the natural place to fold
in the 116% pie chart requirement if the operator later wants Initech 123 and InitechCalc to converge on
one engine (see Sec 6 P5 and Sec 9); it does **not** touch InitechCalc's own existing canon test fixtures
(116%, `570-`), which stay exactly as R4.6 specifies regardless of whether InitechCalc is ever
re-plumbed onto the shared core.

**What is period-defensible and what is not:** a fixed-cell full-screen worksheet drawn by a GUI toolbox
(Face A) is exactly the "screenshot is the spec" move InitechCalc itself already makes; it is not an
anachronism. What would NOT be period-defensible is claiming the class-2 SAMIR pattern for 123 -- real
1-2-3 never ran "in a DOS box inside a window" on any period desktop (that convenience arrived with
Windows' own DOS-box, which InitechOS explicitly rejects per ADR-0001's no-v8086 rule, cited again in
ADR-0013 Sec 4's closing line).

---

## 4. Feature inventory -- "full-featured" checklist (MUST / SHOULD / ERA-SHORTFALL)

| Area | Item | Tier |
|---|---|---|
| Worksheet model | 256 cols (A..IV) x 8192 rows, sparse storage | MUST |
| | Named ranges, `/Range Name Create/Delete/Labels` | MUST |
| | Global vs cell-level format/width/protection | MUST |
| | Windows: `/Worksheet Window Horizontal/Vertical/Sync/Unsync/Clear` | SHOULD |
| Cell types | Value (number/formula), Label (`'` left, `^` center, `"` right, `\` repeating) | MUST |
| | Blank/erased cell distinct from zero | MUST |
| Cell formats | `/Range Format`: Fixed, Sci, Currency, `,` (Comma), General, `+/-`, Percent, Date (5 sub-formats), Time, Text, Hidden | MUST |
| | Column width `/Worksheet Column Set-Width/Reset-Width` | MUST |
| Slash-menu tree | `/Worksheet` (Global, Insert, Delete, Column, Erase, Titles, Window, Status, Page, Learn) | MUST |
| | `/Range` (Format, Label, Erase, Name, Justify, Protect, Unprotect, Input, Value, Trans, Search) | MUST |
| | `/Copy`, `/Move` | MUST |
| | `/File` (Retrieve, Save, Combine, Xtract, Erase, List, Import, Directory, Admin) | MUST |
| | `/Print` (Printer, File, Range, Line, Page, Options, Clear, Align, Go, Quit) | MUST |
| | `/Graph` (Type, X, A..F ranges, Reset, View, Save, Options, Name, Group, Quit) | SHOULD (text-console honesty, see below) |
| | `/Data` (Fill, Table, Sort, Query, Distribution, Matrix, Regression, Parse) | SHOULD, `/Data Matrix`/`Regression` = ERA-SHORTFALL-tagged if descoped |
| | `/System`, `/Add-In`, `/Quit` | MUST (`/System` shell-out is ERA-SHORTFALL under ADR-0001 no-v8086; stub with a "not supported" message, fail loud not silent) |
| `@functions` | Math: `@ABS @INT @MOD @ROUND @SQRT @EXP @LN @LOG @PI @RAND` | MUST |
| | Trig: `@SIN @COS @TAN @ASIN @ACOS @ATAN @ATAN2` | SHOULD |
| | Stat: `@SUM @AVG @COUNT @MAX @MIN @STD @VAR` | MUST |
| | Financial: `@NPV @IRR @PMT @PV @FV @RATE @TERM @CTERM @SLN @SYD @DDB` | MUST |
| | Logical: `@IF @TRUE @FALSE @AND @OR @NOT @ISERR @ISNA @NA @ERR` | MUST |
| | String: `@LEFT @RIGHT @MID @LENGTH @FIND @STRING @VALUE @UPPER @LOWER @PROPER @REPEAT @REPLACE @TRIM @EXACT @CHAR @CODE @N @S` | MUST |
| | Date/time: `@DATE @TIME @NOW @DAY @MONTH @YEAR @HOUR @MINUTE @SECOND @DATEVALUE @TIMEVALUE` | MUST |
| | Lookup: `@VLOOKUP @HLOOKUP @INDEX @CHOOSE @@ (cell-ref indirection)` | MUST |
| | Database `@D*`: `@DSUM @DAVG @DCOUNT @DMAX @DMIN @DSTD @DVAR` over a `/Data Query` input range | SHOULD |
| Recalc | Natural order (dependency-graph topological), `/Worksheet Global Recalc` Automatic/Manual, Columnwise/Rowwise order override, iterative recalc with iteration count, circular-reference detection + `CIRC` indicator | MUST |
| F-keys | `F2` Edit, `F3` Name, `F4` Abs-ref toggle, `F5` GoTo, `F6` Window pane, `F7` Query, `F8` Table, `F9` Calc, `F10` Graph | MUST (F2/F5/F9/F10 first; rest SHOULD) |
| Macros | `\a`..`\z` named key-macros; `{keystroke}` and `{command}` tokens (`{DOWN}`, `{EDIT}`, `{GOTO}`, `/`-menu selection strings); `/Range Name Labels` binding a macro to a cell | SHOULD |
| `/Print` | To PRN (line printer emulation) and To File; margins, page length, headers/footers, borders | MUST (PRN = redirected to a spooled text file under InitechDOS, no real printer path yet -- ERA-SHORTFALL note, not a silent no-op) |
| `/Graph` rendering | Text-console honesty: no period 1-2-3 text-mode graph renders bitmap charts on the CGA/EGA text console without a graphics adapter mode switch. FLAIR's LFB is graphics-capable, so `/Graph View` CAN render a real bar/line/pie/XY chart -- this is MORE capable than a bare text console, and is exactly what Face B's window is for. `/Graph` in Face A (full-screen classic mode) legitimately switches to a full-screen chart view, matching period behavior. | SHOULD, tracked as its own sub-phase (P4) |
| `.WK1` format | Record stream: `BOF 0x0000`, `EOF 0x0001`, `CALCMODE/CALCORDER 0x0002/0x0003`, `SPLIT/SYNC 0x0004/0x0005`, `RANGE 0x0006` (window ref), `WINDOW1 0x0007`, `COLW1 0x0008`, `WINDOW2 0x0009`, `COLW2 0x000A`, `NAME 0x000B`, `BLANK 0x000C`, `INTEGER 0x000D`, `NUMBER 0x000E`, `LABEL 0x000F`, `FORMULA 0x0010` (value + RPN token stream), `TABLE 0x0018`, print/graph range records `0x0019-0x0023`, `PROTEC 0x0021`, `FOOTER/HEADER 0x0022/0x0023`, `SETUP 0x0024`, `MARGINS 0x0025` | MUST -- **every record ID and byte layout above is a claim to be independently re-verified against a real-2.2-minted golden before it is locked spec-data (Sec 5); this table is a starting hypothesis from public format literature, not yet a Law-1 source** |

---

## 5. Ground truth + oracle strategy (Law 1 / Law 2)

**(a) `.WK1` codec ground truth.** Mirror ADR-0008 DEC-05/DEC-06 exactly: an INDEPENDENT Python
reader/writer (`wk1_ref.py`, the `dbf_ref.py`/`fat12_ref.py` pattern) authored from first principles
against the record-ID table (Sec 4), NOT sharing offset constants with the C engine's own
`spec/i123/wk1_format.h` (the DEC-05-revision "independence barrier" lesson -- pull `wk1_ref.py` forward
to run immediately after the format header is drafted, before Phase P1's codec is graded against it).
Cross-check `wk1_ref.py` against Gnumeric's/LibreOffice's independently-written `.wk1` import filters
(oracle-only, semantics cross-check, never an on-disk-byte authority -- same posture as
`oracles/harbour` in the dBASE corpus) and, where available, the archived Lotus File Formats reference
manual (`archive/` provenance ledger, same discipline as `dbase3-decomp/SOURCES.md`).

**(b) Recalc semantics + real goldens -- the golden-acquisition gate (Law 1, MUST run first).** Stand
up a sister repo `../lotus123-decomp`, structured identically to `../dbase3-decomp`
(`specs/`, `goldens/`, `re/`, `oracles/`, `SOURCES.md`, `GAPS.md`, gitignored binaries, licensed-in-place
copyright stance identical to the dBASE corpus's "abandonware, local reference material only" posture).
Mint goldens from a real Lotus 1-2-3 Release 2.2 `.EXE` under DOSBox-X/86Box: a formula corpus
(`.WK1` files with known-correct recalculated values, `@function` edge cases, circular-reference and
iteration behavior, `/Data Sort`/`/Data Query` transcripts) plus the raw `.WK1` byte fixtures the
independent reader validates against. **No formula/recalc value is locked as spec-data until it is
either minted from real 1-2-3 or independently cross-derivable (e.g. `@SUM` is not in dispute) --** the
exact `dbase3-decomp` GAPS.md discipline of tagging every unsettled cell `[oracle-resolves]` applies
here from day one, since (unlike SAMIR at plan time) **no `lotus123-decomp` corpus exists yet** -- this
plan's P0 bead is to create it.

**(c) Numeric model.** Real 1-2-3 stored values as 8-byte IEEE-754 doubles (same representation family
as dBASE's, ADR-0009 DEC-07/mint-004) and displayed with its own 15-significant-digit rule and its own
recalc rounding behavior (distinct from dBASE's rules -- do not assume the coercion table transfers).
**Numeric mechanism ruling needed (an explicit ADR-0009 amendment, exactly as ADR-0001 DEC-04
anticipated):** SAMIR's soft-float choice (ADR-0009 DEC-01) was driven by dBASE's *interactive*,
low-arithmetic-density workload where soft-float's speed cost is irrelevant. Initech 123's recalc graph
can touch thousands of cells per keystroke on a full sheet -- soft-float's per-op cost is no longer
free to ignore, and the 486-class floor (ADR-0001 DEC-02/DEC-04) makes an on-die x87 hardware-float
profile newly period-defensible (real 1-2-3 2.2 auto-detected and used an 80287/80387 when present,
falling back to its own software emulator otherwise -- so BOTH paths are period-authentic, unlike
SAMIR's DOS-8088 target where the FPU was rare). This plan does not pre-decide; Sec 8 Ruling 2 flags it
for the operator, and Phase P2 is the first phase that needs the answer.

**(d) Mutants per phase (Rule 6).** Every `test-i123-*` gate below ships with a `-mutant` sibling
exactly as `test-dbf-*`/`test-ndx-*` do (flip a `.WK1` record-ID constant, flip an `@function`
dispatch-table cell, flip the RPN operator-precedence table, corrupt a recalc dependency edge) and must
be shown RED for the right reason before being trusted (Rule 6's "the golden has caught a real
regression" bar, not "the golden was committed first").

**(e) Rule-14 clip for Face B.** `docs/HANDOFF.md`/CLAUDE.md Rule 14 ("window-system features accepted
on emu video clips, operator directive 2026-09-25") governs Face B's Mac-windowed grid/chart rendering
the same way it governs any other FLAIR window feature: graded by a `record-flair` clip + operator
eyeball (Law 4), never a hard SSIM gate (`harness/ssim.c` remains NOT YET BUILT per CLAUDE.md).

---

## 6. Phases

Each phase leaves `make test` green with its own gate, sized in agent-sessions (a session = one
focused implementation pass with its own oracle run, the SAMIR-plan convention).

- **P0 -- Golden acquisition (Law 1 gate, MUST run before any other phase).** ~2-3 sessions. Stand up
  `../lotus123-decomp` (README/GAPS/SOURCES scaffolding cloned from `../dbase3-decomp`); acquire a real
  Lotus 1-2-3 Release 2.2 distribution + license it in place (gitignored, same posture as the dBASE
  binaries); stand up the DOSBox-X/86Box mint harness (`re/harness-setup.md` clone); mint the first
  batch of `.WK1` fixtures (blank sheet, labels-only, numbers-only, one formula sheet) and confirm they
  parse under a throwaway hex dump. **Exit criterion:** `../lotus123-decomp/goldens/` exists with >=5
  real `.WK1` files + byte dumps; `SOURCES.md` records provenance. No Phase P1 oracle may reference a
  golden that does not exist yet (loud-skip, never guessed).

- **P1 -- Grid + labels + numbers + `.WK1` round-trip (demoable milestone).** ~4-5 sessions. `i123_pal.h`
  (ADR-0008-DEC-02-style contract) + `pal_host.c`; sparse cell store (Sec 9 sizing); label/number
  parsing incl. prefix characters; `wk1_ref.py` independent reader (pulled forward, Sec 5a); `.WK1`
  codec for `BOF/EOF/INTEGER/NUMBER/LABEL/COLW1/RANGE`; Face A's minimal window (grid draw via
  `chicago8x16`, no menu yet). **Oracle:** `test-i123-wk1-roundtrip` (+mutant) -- SAMIR-write ->
  normalize -> `cmp` vs P0 goldens, bidirectionally, mirroring `test-dbf-roundtrip`. **Exit:** a sheet
  built by hand, saved, and re-read losslessly; a P0 golden read and its values displayed correctly.

- **P2 -- Formulas + `@function` core + recalc.** ~6-8 sessions. RPN lexer/parser/evaluator; the
  `FORMULA 0x0010` codec (value cache + token stream); the MUST-tier `@function` groups (Sec 4: math,
  stat, financial, logical, string, date/time, lookup); natural-order dependency-graph recalc;
  circular-reference detection. **Needs Sec 5(c)'s numeric-mechanism ruling landed first** (soft-float
  vs hardware x87 changes the runtime linkage, not just performance). **Oracle:** `test-i123-formula`
  (+mutant) differential against the P0 mint corpus's formula sheets; `test-i123-recalc-order`
  (+mutant) against a natural-order dependency fixture. **Exit:** a 3-sheet chained-formula demo
  recalculates identically to the real-1-2-3 golden.

- **P3 -- Slash menu complete + `/Range` `/Copy` `/Move`.** ~5-6 sessions. The full menu tree (Sec 4);
  `/Copy`/`/Move` with relative/absolute (`$`) reference adjustment; `/Range Format/Name/Erase/Protect`;
  F2/F5/F9 keys; named ranges. **Oracle:** `test-i123-menu-tree` (menu structure golden, text-content
  diff) + `test-i123-copy-move` (+mutant, reference-adjustment property test: copying a formula N cells
  over shifts every relative ref by N, leaves every `$`-absolute ref fixed). **Exit:** the full slash
  menu is navigable and every MUST-tier leaf command works end-to-end in the emulator.

- **P4 -- `/Data`, `/Print`, macros, `/Graph`.** ~6-8 sessions. `/Data Fill/Sort/Query/Table`;
  `/Print` to a spooled text file (PRN redirection, ERA-SHORTFALL-noted for real hardware printing);
  `\a`..`\z` macros + `{command}` tokens; `/Graph` (Face A full-screen chart mode + Face B windowed
  chart, Sec 4). **Oracle:** `test-i123-sort` (+mutant, stability + multi-key), `test-i123-macro`
  (scripted keystroke-macro replay vs expected cell state), `test-i123-print` (spooled-file content
  diff). **Exit:** a payroll-style demo sheet sorts, prints to file, and replays a saved macro
  correctly.

- **P5 -- Face B: Mac-windowed view + shared-core option.** ~4-5 sessions. The Geneva-9 proportional
  windowed renderer (Sec 3.3); scrollable grid; optional chart palette. Investigate (do not force)
  whether InitechCalc's own renderer can be re-plumbed onto the shared `.WK1` core without touching its
  R4.6 canon fixtures (116% pie sums, `570-` format) -- if the investigation shows regression risk to
  the canon oracle, DEFER the convergence and ship Face B as its own renderer (Rule 3: no bandaid that
  risks a locked canon test). **Oracle:** Rule-14 `record-flair` clip (Sec 5e); `test-i123-facea-faceb`
  agreement test (both faces read the same `.WK1` and display the same values, structural not pixel
  diff).

- **P6 -- Era-shortfall ledger + competitive statement.** ~1-2 sessions, documentation + gate wiring
  only. Write the final ERA-SHORTFALL table (3-D worksheets/`.WK3`, `/System` shell-out, Wysiwyg,
  real-hardware `/Print`, any `@function` or `/Data` sub-feature descoped in P4) as a locked ledger
  (`spec/i123/era_shortfalls.md`, Rule 8 discipline) and a one-page "what a 1991-93 period reviewer
  would and would not find missing" statement, mirroring how ADR-0012 D-3 records SAMIR's re-
  implementation-parity shortfalls as accepted, not hidden.

---

## 7. Proposed bead tree (for the PM to `bd create`; IDs NOT assigned here)

**Epic: "Initech 123 -- Lotus 1-2-3 Initech-version" (parent, P1 priority, blocks nothing, blocked by
ADR-0012's Platform-Services epic for the App-Contract launch surface).**

1. **[FIRST -- Law 1 gate] `lotus123-decomp` golden acquisition + mint harness.**
   Type: research/infra. Priority: P0. Acceptance: `../lotus123-decomp` exists with README/GAPS/SOURCES
   cloned from the dbase3-decomp pattern; a real Lotus 1-2-3 Release 2.2 distribution is licensed-in-
   place and gitignored; DOSBox-X/86Box mint harness reproduces >=5 `.WK1` fixtures with byte dumps;
   `wk1_ref.py` independent reader agrees with the byte dumps on all fixtures. Blocks every phase below.

2. **P1 -- Grid, labels, numbers, `.WK1` round-trip, Face A skeleton.**
   Type: feature. Priority: P1. Depends on: bead 1. Acceptance: `test-i123-wk1-roundtrip` (+mutant)
   green; a hand-built sheet round-trips losslessly; a P0 golden reads back with correct displayed
   values; Face A window opens as a native FlairApp tenant (compiled-in demo build acceptable for this
   bead; disk-launch packaging is bead 2b).

2b. **P1b -- Disk-launch packaging via the D1 INT 81h gate.**
   Type: infra/integration. Priority: P1. Depends on: bead 2, `initech-tdnl.14` (D1 disk-tenant gate).
   Acceptance: Initech 123 ships as an InitechMZ flat-32 `.EXE`, launches via double-click through the
   `INT 81h` gate, registers, and runs the P1 grid demo under QEMU + Bochs with the D1.7 serial launch
   trace green.

3. **P2 -- Formula engine, `@function` core, recalc.**
   Type: feature. Priority: P1. Depends on: bead 2, the numeric-mechanism ADR-0009 amendment (Sec 8
   Ruling 2). Acceptance: `test-i123-formula` (+mutant) and `test-i123-recalc-order` (+mutant) green
   against the P0 mint corpus; circular-reference detection fires correctly on a known-circular fixture.

4. **P3 -- Slash menu tree + `/Range` `/Copy` `/Move`.**
   Type: feature. Priority: P1. Depends on: bead 3. Acceptance: full MUST-tier menu tree navigable
   end-to-end in-emulator; `test-i123-copy-move` (+mutant) reference-adjustment property test green.

5. **P4 -- `/Data`, `/Print`, macros, `/Graph`.**
   Type: feature. Priority: P2. Depends on: bead 4. Acceptance: `test-i123-sort`/`test-i123-macro`/
   `test-i123-print` (+mutants) green; Face A full-screen chart mode renders a bar/line/pie chart from
   a `/Graph`-configured range.

6. **P5 -- Face B Mac-windowed view + shared-core investigation.**
   Type: feature. Priority: P2. Depends on: bead 4. Acceptance: Face B renders the same `.WK1` file as
   Face A with structural agreement (`test-i123-facea-faceb`); `record-flair` clip filed for operator
   eyeball (Rule 14); InitechCalc-convergence investigation documented as a decision (converge or defer)
   with R4.6 canon fixtures re-run green regardless.

7. **P6 -- Era-shortfall ledger + competitive statement.**
   Type: docs/governance. Priority: P3. Depends on: bead 6. Acceptance: `spec/i123/era_shortfalls.md`
   locked (Rule 8) and worklog entry filed.

---

## 8. Risks

- **Memory: the 256x8192 sparse sheet cost vs the class-2 program window.** Naively, 256 x 8192 =
  2,097,152 possible cells. A period-plausible sparse store (populated cells only, e.g. a hashed/sorted
  sparse-row structure keyed by (row,col), each populated cell needing roughly: an 8-byte IEEE value +
  a small formula-token-stream pointer/length + a format byte + a recalc-graph edge list slot -- call it
  ~24-40 bytes per populated cell before token-stream storage) means a working sheet of even a few
  thousand populated cells (a real-world payroll/budget sheet, not a synthetic stress test) costs
  roughly 100-300 KiB, which **already exceeds** the class-2 text-tenant program window's ~187 KiB
  (`spec/memory_map.h`: `PROGRAM_BASE=0x40100` .. `ENV_BLOCK=0x6F000`). This is a second, independent
  argument (alongside Sec 3's ADR-0013 argument) against ever squeezing Initech 123 into the SAMIR-style
  program window: **it must live in the master FLAIR heap** (`[0x100000, 0x500000)`, 4 MiB per ADR-0004
  DEC-03, with ADR-0001 DEC-04 flagging a higher `FLAIR_HEAP_MIN` as period-defensible for 486-class
  machines) exactly as Part D1's disk-tenant carve already does for GUI tenants. Recommend the cell
  store be a column-major sparse structure (populated cells only, sorted or hashed per column) rather
  than a dense 256x8192 array (a dense array of even 16-byte cells would be 32 MiB, impossible).

- **Kernel size wall.** Per CLAUDE.md's build discipline and the WL-0090/initech-8z9j size-policy
  precedent (per-object `-Os` + measured mutant-aware headroom gate), a full-featured spreadsheet engine
  MUST NOT be compiled into the kernel image alongside HELLO/NOTES/InitechCalc. Bead 2b (disk-launch
  packaging via the D1 `INT 81h` gate) is not optional polish -- it is the only path that does not
  reopen the kernel-size gate the M7 corpus "batch stall" investigation (commit 888a5d2) was fixing.

- **Soft-float recalc speed.** A full-sheet recalc touching thousands of formula cells through libgcc
  soft-float helpers (the SAMIR pattern, ADR-0009 DEC-01) is materially slower than hardware x87 --
  unlike SAMIR's per-keystroke interactive load, a `/Worksheet Global Recalc` on a large sheet is a
  batch operation a period user would notice stall. This is exactly why Sec 5(c)/Sec 8 Ruling 2 asks the
  operator to decide the numeric mechanism explicitly rather than defaulting to the SAMIR precedent by
  inertia; ADR-0001 DEC-04 already names this as an open amendment, not a new question this plan
  invents.

- **TPS/no-pointers subset is irrelevant here.** Initech 123 is a bundled app (ADR-0002); it is **C**,
  compiled by the factory cross-toolchain like every other artifact app, never Pascal/TPS. The TPS
  no-pointer-subset constraints that shape `docs/plans/TPS-M7-subset-plan.md` and Part D2's Pascal
  Toolbox binding do not apply to this plan at all (Law 3 -- do not blur the two languages).

- **ADR-0013 amendment risk.** If the operator instead wants a literal character-mode "authentic
  slash-menu console" Initech 123 (rather than this plan's native-rendered lookalike), that requires
  formally reopening and amending ADR-0013 Sec 4 -- a Core-tier, ADR-by-committee act (CLAUDE.md Rule 7),
  not a decision this plan or its implementing agent may make unilaterally.

---

## 9. Sources

Local (Law 1): `CLAUDE.md` (Laws/Rules, tiered workflow); `InitechOS-PRD.md` Sec 1.4/6.5/6.6;
`docs/adr/ADR-0001-Hardware-Target-386-Origin-486-Floor.md` DEC-02/DEC-04 (486 floor, the explicit
"Initech 123" forward-pointer to a numeric-mechanism ADR-0009 amendment); `docs/adr/ADR-0008-SAMIR-
InitechBase-Architecture.md` (the PAL/storage-split/golden-tiering pattern this plan mirrors);
`docs/adr/ADR-0009-SAMIR-Milton-Integration-Numeric-Memory-Packaging.md` (soft-float precedent + its
explicit non-transfer caveat); `docs/adr/ADR-0013-FLAIR-App-Contract.md` Sec 3.1-3.2, Sec 4 (the
native-vs-text-host ruling naming Initech 123 by name -- the load-bearing citation for Sec 3 of this
plan); `docs/design/GUI-remediation-D1-D2-D3-design.md` Part D1 (disk-launched InitechMZ tenant, INT
81h gate, memory-tiering into the FLAIR heap); `docs/plans/GUI-remediation-plan.md` R4.6/R4.7;
`docs/plans/SAMIR-implementation-plan.md` (the phase/oracle/bead-tree shape this plan follows);
`spec/win95ism_guardrails.md` (era ceiling); `spec/memory_map.h` (`PROGRAM_BASE`/`ENV_BLOCK` window
sizing used in Sec 9); `spec/assets/chicago8x16.h` (the fixed-cell font reused for Face A's monospace
grid); `Makefile` (`DBASE3_DECOMP ?= ../dbase3-decomp` loud-skip idiom, mirrored as
`LOTUS123_DECOMP ?= ../lotus123-decomp`).

Sister-corpus pattern (not yet created for Lotus; cited as the template): `../dbase3-decomp/README.md`,
`../dbase3-decomp/GAPS.md`, `../dbase3-decomp/SOURCES.md` (the acquisition-and-licensing discipline
Sec 6 P0 clones for `../lotus123-decomp`).

Non-local, independent-oracle candidates named for Sec 5(a) only (semantics cross-check, never an
on-disk-byte authority, same posture as `oracles/harbour`): Gnumeric's and LibreOffice's independently
maintained `.wk1` import filters; archived Lotus File Formats reference documentation to be acquired
into `../lotus123-decomp/archive/` with full provenance per bead 1's acceptance criteria. **No `.WK1`
record-ID byte layout in Sec 4 of this plan is yet Law-1-sourced; all are hypotheses pending Phase P0
minting.**

---

*-- End of Plan (DRAFT; pending operator rulings, Sec 8, and PM bead-tree filing) --*

<!-- Tedium certified compliant with NFR-7. The 570- format is InitechCalc's, not native 1-2-3's; do not import it into the WK1 codec. -->
