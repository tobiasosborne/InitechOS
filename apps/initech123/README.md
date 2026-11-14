# Initech 123

Worksheet store, formula compiler/evaluator, recalculation, range operations,
Lotus WK1 codec, slash menus and native Toolbox face. This component preserves
its present feature depth; import does not add spreadsheet commands.

## Build and public interface

`make` builds `123.EXE`, `123.MNU` and the deterministic target static library
`libi123.a` under `build/apps/initech123/`. The EXE uses mk/tenant.mk and the
plain shared target flags, with I123_MENU_DISK selecting APPS/123.MNU loading.
An independent factory/mz reader validates the executable and its relocations.
The resource generator is the C menu-tree suite's --resource mode. The full
captured RI tree remains unchanged research evidence. An independent AWK
subtraction of the reviewed omissions grades the compact embedded tree and a
separate C byte reader grades the actual resource, including original action
identities. The smoke verifier also extracts 123.MNU from the booted disk.
Missing or malformed resources terminate startup with EXIT(1), before a window
or menu bar appears, exactly as in legacy-final.

Public headers, consumed with `-Iapps/initech123/include`:

- `i123.h`: worksheet store, formatting, formulas, recalculation, ranges and WK1.
- `i123/wk1_format.h`: WK1 records, defaults and display-format contracts.
- `i123/wk1_formula.h`: formula bytecode, tokens and reference contracts.
- `menu.h`: separate menu model/resource API; not part of the core archive.

This component publishes `build/apps/initech123/libi123.a` as a declared pure
worksheet library. InitechCalc links it directly through the public worksheet
headers above. The archive contains sheet, fmt, wk1, formula, range, fill, eval
and recalc objects, including the latter two needed by the formula/range APIs.
It contains no menu, application face or Toolbox runtime. Use the same default
32-bit C ABI and x87 control word 0x037F as the target; no OS dependency exists
in the archive. This does not import InitechCalc.

`123.EXE` compiles its own worksheet engine and links no pure library from
another component. Its OS binding is the SDK Toolbox client runtime (INT 81h).
This component publishes no resident platform or manager binding.

The face depends only on sdk/include (`tbx.h`, `tbx_save.h` and their public
contracts). All OS calls use the Toolbox gate. Core/fs compile with only this
component's include path. The host placement test also depends on FLAIR's
public `chrome_metrics.h`. No build input comes from attic.

## Verification

`make test` runs 30 host suites: the original 18, protection, shipped-configuration
menu loading, EDIT, frame placement, file conformance, column ranges, time formats
and Data Fill, plus Global Zero, file browsing, Text import and every retained
menu route, using sdk/host's
shared fake Toolbox. No emulator runs in tier 0.
The original 1,023,560 counted assertions are retained; malformed Retrieve and
automatic numeric-entry recalculation add seven. Resource startup has 487
checks across 22 fresh launches; all 81 shipped leaves have 360 route checks.
Separate checks still cover eight real Copy/Move pairs, four tiling fixtures,
12 self-transfers and the complete 82-level/381-item reference. The shipped
25-level/105-item tree is graded by full path, order, descriptions and action
identity against the independent subtraction. The precision-negative
x87-027F invocation retains its two expected failures and three diagnostics.
The independent corpus comprises 59 real WK1 inputs. Formula checks retain
1090 formulas, 4000 random formulas, poisoned-cache files and manual examples;
Copy/Move retains 4000 random sheets and 1,020,041 assertions.

The new suite compiles both face and menu core with I123_MENU_DISK, supplies the
actual generated resource through sdk/host's memory volume, and checks slash
navigation and Retrieve dispatch. Fresh processes exercise normal/short reads,
truncated header/body, missing file, damaged magic/checksum and read/close errors.
Failure must EXIT(1), publish no UI and leave no open resource handle. Authority:
legacy-final os/i123/app/face_a.c tbx_app_main and os/i123/core/menu.c; this binary
resource framing and failure policy are authored, not Lotus file-format claims.

`make mutants` applies 57 strict patches (including the 41 former hook locations), builds
each and requires the selected host suite to kill it twice. Mutants run only
in tier 2. `make smoke` builds the product image and runs the shared desktop
journeys on QEMU and Bochs. It independently checks the worksheet saved from
the Finder and the actual menu resource extracted from both post-boot disks.
The broader legacy emulator scenarios still awaiting factory adoption are in
`tests/SMOKES.md`; the import does not claim those scenarios ran.

Tests require LOTUS123_DECOMP and its pinned corpus (missing input is an error).
Inherited independent Python readers and adapters remain under tests/oracles;
new factory work is C, make and POSIX shell. Scratch/logs stay under build.
Shared normal/x87 host core objects keep builds fast. The 30-second tier-0
execution budget covers the complete independent corpus selection; compilation
is measured separately. The shared runner preserves fresh scratch, file-reader
post-checks, timeouts and the precision-negative control. Import audit builds
all target artifacts and checks source/include/build boundaries in `make test`.

## Present depth

The GOLDEN-MASTER.md "present depth" inventory is frozen here. Worksheet
addresses span 256 columns by 8192 rows; capacity is 2048 populated cells, a
32 KiB byte pool per bank and 4096 bytes of peripheral/name records. Operations
prepare a candidate bank before publishing. Protection errors publish the
allowed prefix for Copy, Value, Trans, Erase and Name Table, as real Lotus
does. Move checks all destinations first; protected sources may be moved.
Name Labels rebinding, Range Label alignment and formats are permitted on
protected cells. Name Table checks both writes and both following erasures,
retains erasure formats and prints singleton addresses without `..`.
Authority: seven `goldens/dataloss/lotus` cases and 33 real R2.2 captures
in `tests/fixtures/protection` (byte/hash-pinned WK1 and VGA/PNG screens).
The protection suite adds 167 checks; it compares raw saved cell, name and
PROTEC records, plus READY/ERROR and diagnostics, to these independent files.
File links keep cached values;
three named formula exceptions remain documented by the formula oracle.

| Menu branch | Implemented today |
|---|---|
| Worksheet | Global formats/label prefix/width, recalculation order/mode/iteration, global protection and Zero No/Yes/Label; current-column and column-range Set-Width/Reset-Width; Erase No/Yes. |
| Range | Format (Fixed, Sci, Currency, comma, General, +/-, Percent, Date 1/2/3, fixed AM/PM Time 1/2, Text, Hidden, Reset), label alignment, Erase, Name Create/Delete/Labels/Reset/Table, Prot/Unprot, Value and Trans. |
| Copy, Move | Snapshot transfers, singleton-axis tiling for Copy rectangles, formula reference adjustment, named ranges; Copy retains the allowed prefix on protection refusal, Move checks all destinations first. Capacity failures discard the candidate bank. |
| File | Retrieve and Save WK1, Cancel/Replace on existing files; filtered Erase/List Worksheet/Print/Graph/Other, session Directory and Import Text. Backup is omitted. |
| Data | Fill, including expressions/names, remembered numeric defaults, column-first clear/fill and captured protection/memory prefixes. |
| Quit | No/Yes and unsaved-work guard: Save, Cancel, Don't Save. Native File > Quit and the window close box use the same guard. |

Entry, arrows, Tab/page navigation, F2 EDIT, F5 GOTO, F9 recalc, range POINT
selection and resized/scrolled cell hit testing work. F3/F4/F6/F7/F8/F10 and
macros remain deferred. International date/time display choices below refuse.

All saves, including Replace and the quit guard's Save, call the shared
`sdk/include/tbx_save.h` helper; this component has no separate file commit
path. The desktop-save ruling requires the old WK1 under its own name after
reported failure. That guarantee depends on the SDK owner's initech-vtl9 fix.
The explicit integration probe below fails on the pre-fix shared helper and
can be rerun when landing it; it is separate from the protection selection:

```
mkdir -p ../../build/apps/initech123/save-probe
../../build/apps/initech123/host/test_i123_protection \
  ../../build/apps/initech123/save-probe ../../goldens/dataloss/lotus \
  tests/fixtures/protection --save-safety
```

The approved split is complete: **18 leaves implemented, 206 slash leaves and
File Save > Backup removed**. The following frozen table expands to individual
full paths by joining each parent with each named leaf. Every listed path is
absent; empty branches and navigation-only remnants are also removed below.
The independent shipping manifest is the full RI tree minus precisely these
omissions and structural removals. Paths and spelling come from the independently
checked full reference. Historical inventory sources: legacy-final
`docs/audits/2026-10-23-i123/{REPORT,TRIAGE}.md`, its menu evidence, and
`tests/oracles/menu/era_shortfalls.md`, verified against `app/face_a.c`.

| Parent menu | Every unavailable leaf |
|---|---|
| `/Worksheet/Global/Format/Date` | `4 (Long Intn'l)`, `5 (Short Intn'l)` |
| `/Worksheet/Global/Format/Date/Time` | `3 (Long Intn'l)`, `4 (Short Intn'l)` |
| `/Worksheet/Global/Default/Printer` | `Interface`, `Left`, `Right`, `Top`, `Bot`, `Pg-Length`, `Setup`, `Name` |
| `/Worksheet/Global/Default/Printer/AutoLF` | `Yes`, `No` |
| `/Worksheet/Global/Default/Printer/Wait` | `No`, `Yes` |
| `/Worksheet/Global/Default` | `Directory`, `Status`, `Update` |
| `/Worksheet/Global/Default/Other/International/Punctuation` | `A (.,,)`, `B (,..)`, `C (.;,)`, `D (,;.)`, `E (., )`, `F (,. )`, `G (.; )`, `H (,; )` |
| `/Worksheet/Global/Default/Other/International/Currency` | `Prefix`, `Suffix` |
| `/Worksheet/Global/Default/Other/International/Date` | `A (MM/DD/YY)`, `B (DD/MM/YY)`, `C (DD.MM.YY)`, `D (YY-MM-DD)` |
| `/Worksheet/Global/Default/Other/International/Time` | `A (HH:MM:SS)`, `B (HH.MM.SS)`, `C (HH,MM,SS)`, `D (HHhMMmSSs)` |
| `/Worksheet/Global/Default/Other/International/Negative` | `Parentheses`, `Sign` |
| `/Worksheet/Global/Default/Other/Help` | `Instant`, `Removable` |
| `/Worksheet/Global/Default/Other/Clock` | `Standard`, `International`, `None`, `Clock`, `Filename` |
| `/Worksheet/Global/Default/Other/Undo` | `Enable`, `Disable` |
| `/Worksheet/Global/Default/Other/Beep` | `Enable`, `Disable` |
| `/Worksheet/Global/Default/Other/Add-In` | `Set`, `Cancel` |
| `/Worksheet/Global/Default/Autoexec` | `Enable`, `Disable` |
| `/Worksheet/Insert` | `Column`, `Row` |
| `/Worksheet/Delete` | `Column`, `Row` |
| `/Worksheet/Column` | `Hide`, `Display` |
| `/Worksheet/Titles` | `Both`, `Horizontal`, `Vertical`, `Clear` |
| `/Worksheet/Window` | `Horizontal`, `Vertical`, `Sync`, `Unsync`, `Clear` |
| `/Worksheet` | `Status`, `Page` |
| `/Worksheet/Learn` | `Range`, `Cancel`, `Erase` |
| `/Range/Format/Date` | `4 (Long Intn'l)`, `5 (Short Intn'l)` |
| `/Range/Format/Date/Time` | `3 (Long Intn'l)`, `4 (Short Intn'l)` |
| `/Range` | `Justify`, `Input` |
| `/Range/Search/Formulas` | `Find`, `Replace` |
| `/Range/Search` | `Label`, `Both` |
| `/File/Combine/Copy` | `Entire-File`, `Named/Specified-Range` |
| `/File/Combine/Add` | `Entire-File`, `Named/Specified-Range` |
| `/File/Combine/Subtract` | `Entire-File`, `Named/Specified-Range` |
| `/File/Xtract` | `Formulas`, `Values` |
| `/File/List` | `Linked` |
| `/File/Import` | `Numbers` |
| `/File/Admin/Reservation` | `Get`, `Release` |
| `/File/Admin` | `Table`, `Link-Refresh` |
| `/Print/Printer` | `Range`, `Line`, `Page`, `Align`, `Go` |
| `/Print/Printer/Options` | `Header`, `Footer`, `Setup`, `Pg-Length` |
| `/Print/Printer/Options/Margins` | `Left`, `Right`, `Top`, `Bottom`, `None` |
| `/Print/Printer/Options/Borders` | `Columns`, `Rows` |
| `/Print/Printer/Options/Other` | `As-Displayed`, `Cell-Formulas`, `Formatted`, `Unformatted` |
| `/Print/Printer/Clear` | `All`, `Range`, `Borders`, `Format` |
| `/Print/File` | `Range`, `Line`, `Page`, `Align`, `Go` |
| `/Print/File/Options` | `Header`, `Footer`, `Setup`, `Pg-Length` |
| `/Print/File/Options/Margins` | `Left`, `Right`, `Top`, `Bottom`, `None` |
| `/Print/File/Options/Borders` | `Columns`, `Rows` |
| `/Print/File/Options/Other` | `As-Displayed`, `Cell-Formulas`, `Formatted`, `Unformatted` |
| `/Print/File/Clear` | `All`, `Range`, `Borders`, `Format` |
| `/Graph/Type` | `Line`, `Bar`, `XY`, `Stack-Bar`, `Pie` |
| `/Graph` | `X`, `A`, `B`, `C`, `D`, `E`, `F`, `Reset`, `View`, `Save`, `Options` |
| `/Graph/Name` | `Use`, `Create`, `Delete`, `Reset`, `Table` |
| `/Graph/Group` | `Columnwise`, `Rowwise` |
| `/Data` | `Distribution` |
| `/Data/Table` | `1`, `2`, `Reset` |
| `/Data/Sort` | `Data-Range`, `Primary-Key`, `Secondary-Key`, `Reset`, `Go` |
| `/Data/Query` | `Input`, `Criteria`, `Output`, `Find`, `Extract`, `Unique`, `Delete`, `Reset` |
| `/Data/Matrix` | `Invert`, `Multiply` |
| `/Data/Regression` | `X-Range`, `Y-Range`, `Output-Range`, `Reset`, `Go` |
| `/Data/Regression/Intercept` | `Compute`, `Zero` |
| `/Data/Parse/Format-Line` | `Create`, `Edit` |
| `/Data/Parse` | `Input-Column`, `Output-Range`, `Reset`, `Go` |
| `/` | `System` |
| `/Add-In` | `Attach`, `Detach`, `Invoke`, `Clear` |

## Remaining legacy material

Factory owns adoption of the menu/numeric serial checker programs in
attic/harness/diff/wk1_diff, emulator trace fragments and copy smoke keys in
attic/spec, and the remaining emulator scenarios in tests/SMOKES.md. Fixture
minting utilities are factory support, not additional host tests. Cross-app
Calculator, Finder and InitechWord tests belong to their respective importers.
The retired factory bookkeeping and size-policy gates are not app behavior.
This component does not consume any of those files while building or testing.

EDIT's Left/Right/Home/End, five-character Tab/Shift-Tab and Ctrl arrows,
Delete, Backspace and insert/overwrite follow R2.2 Reference pp. 1-22/1-23
and the independent `tests/fixtures/edit` capture. The saved LABEL records
are compared to real R2.2 by a separate Python WK1 reader in tier 0;
`edit_left_ignored.patch` proves that leaving F2 working while dropping its
Left route fails. Inline cursor movement leaves the worksheet pointer fixed.

The initial document structure region starts immediately below the single
menu bar (CLAUDE.md, Hazards that are still true). The frame test reads
`FLAIR_CHROME_MENUBAR_H` from `flair/include/chrome_metrics.h`; this is a
test-only dependency on FLAIR's public metrics. Product calls still cross
only the Toolbox gate. The taller window can show 26 whole lines. The
app-owned smoke uses keys after launch, so no factory click needs moving.

Retrieve distinguishes a missing worksheet EOF from invalid framing using
real R2.2's `Part of file is missing` (study fixture L05). The original
reader study's component fixtures/captures are imported under
`tests/conformance`; their historical observations are not passing checks.
The registered conformance suite preserves the current sheet on refusal and
independently compares a 240-character label's no-op F2 save with real R2.2.
That shortening is reference behavior (initech-pmw9), retained deliberately.

Retrieved non-string expressions follow the independently captured
250-character rendering boundary (`tests/conformance/boundaries`), with the
real `Formula too long` refusal before the candidate worksheet is published.
The entry-marker '+' is excluded; the 240-character typing limit is separate.
Integer, floating, signed and binary neighboring cases are retained. Literal
strings follow a different reference path and keep the existing behavior;
the accepted 249-character string fixture prevents imposing a guessed global
bytecode or nesting cap. The decompiler can print the accepted deep neighbors.

Frozen omission: **File Save > Backup**. The existing-file row is exactly
`Cancel  Replace`; B has no action, no gray item and no refusal prompt.
This is the first removal from the approved 18-implement/207-remove split.
Cancel/Replace and save-failure preservation remain checked independently.

Range and Global Format Date Time 1/2 now provide fixed AM/PM display,
including captured rounding/carry behavior, leading hour zeros, formula
results and explicit-cell overrides. `tests/fixtures/time-formats` retains
independent VGA/WK1 captures. The time suite compares glyphs directly to VGA
character bytes and saved cell/global-format records with a separate reader.
International Time 3/4 remain in the approved removal inventory.

Data Fill follows the Reference 4-5/4-6 and `tests/fixtures/fill`: it clears
the range before filling, preserves formats, clears the tail beyond Stop,
and retains the captured erasure or numeric prefix on protection/memory
failure. Defaults are numeric and reset on Retrieve/Worksheet Erase;
Cancel preserves the worksheet. The independent reader compares every saved
cell/name/protection/settings record, including the smaller native model's
memory-limited prefix. No new WK1 parameter record is introduced.

Global Zero provides No/Yes/Label, including editable label prefixes and the
240-character input limit. It substitutes only actual numeric zero, clips
replacement labels to the column and leaves Hidden/formula Text/errors alone.
`tests/fixtures/zero` compares whole rendered rows directly against real VGA;
paired complete WK1 saves must be byte-identical. The option never dirties
or enters worksheet records, and resets on Retrieve/Worksheet Erase. The
same captures correct Date1 serial-zero display to `00-Jan-00`.

File Erase and List provide the captured *.WK? / *.PRN / *.PIC / all-file
choosers, sorted files before directories, selected FAT date/time/size, full
paging and No/Yes confirmation for deletion. Directory is a validated session
prefix used by Retrieve, Save, List, Erase and Import; no kernel chdir or global
default is changed. Save's default retains the retrieved file's complete path.
Listing refuses at the 512th SDK result rather than show an incomplete list;
511 entries are supported. Only confirmed deletion changes disk data; these
commands preserve the worksheet and dirty state.

Import Text consumes at most 64 KiB per file (reported native input capacity),
CR-delimited lines, ignored LF/NUL, eight-column tabs and Ctrl-Z EOF. It stores
literal left labels, leaves cells under empty lines alone, accepts 240 expanded
characters per line and silently stops at row 8192. Completed lines survive
protection/length/memory failures; disk read/close failures preserve the old
worksheet. This follows the saved R2.2 bytes even when its ERROR screen is
stale. Existing formats and global alignment records are preserved.
`tests/fixtures/files` retains all reference inputs, VGA, saved WK1 and mtools
FAT file sets/hashes. Independent tests compare complete pages, metadata,
whole saved worksheets, all fifteen import outputs and all four deletion
results. Same basenames in two directories exercise every file route.

Frozen structural removals: these 57 menu ancestors have no surviving operative
child; their navigation-only Quit leaves are not retained as empty menus.

```text
/Worksheet/Insert
/Worksheet/Delete
/Worksheet/Titles
/Worksheet/Window
/Worksheet/Learn
/Worksheet/Global/Default
/Worksheet/Global/Default/Printer
/Worksheet/Global/Default/Other
/Worksheet/Global/Default/Other/International
/Worksheet/Global/Default/Other/International/Punctuation
/Worksheet/Global/Default/Other/International/Currency
/Worksheet/Global/Default/Other/International/Date
/Worksheet/Global/Default/Other/International/Time
/Worksheet/Global/Default/Other/International/Negative
/Worksheet/Global/Default/Other/Help
/Worksheet/Global/Default/Other/Clock
/Worksheet/Global/Default/Other/Undo
/Worksheet/Global/Default/Other/Beep
/Worksheet/Global/Default/Other/Add-In
/Worksheet/Global/Default/Autoexec
/Worksheet/Global/Default/Printer/AutoLF
/Worksheet/Global/Default/Printer/Wait
/Range/Search
/Range/Search/Formulas
/File/Combine
/File/Combine/Copy
/File/Combine/Add
/File/Combine/Subtract
/File/Xtract
/File/Admin
/File/Admin/Reservation
/Print
/Print/Printer
/Print/Printer/Options
/Print/Printer/Options/Margins
/Print/Printer/Options/Borders
/Print/Printer/Options/Other
/Print/Printer/Clear
/Print/File
/Print/File/Options
/Print/File/Options/Margins
/Print/File/Options/Borders
/Print/File/Options/Other
/Print/File/Clear
/Graph
/Graph/Type
/Graph/Name
/Graph/Group
/Data/Table
/Data/Sort
/Data/Query
/Data/Matrix
/Data/Regression
/Data/Regression/Intercept
/Data/Parse
/Data/Parse/Format-Line
/Add-In
```

The 13 additional navigation-only leaves removed with those branches are:

```text
/Worksheet/Global/Default/Quit
/Worksheet/Global/Default/Printer/Quit
/Worksheet/Global/Default/Other/International/Quit
/Print/Printer/Quit
/Print/Printer/Options/Quit
/Print/File/Quit
/Print/File/Options/Quit
/Graph/Quit
/Data/Sort/Quit
/Data/Query/Quit
/Data/Regression/Quit
/Data/Parse/Quit
/Add-In/Quit
```

Six descriptions enumerate children. Their shortened wording is an authored
projection of RI tokens and double-space delimiters, checked against these
reviewed literal expectations; it is not claimed as an unchanged RI capture.

```text
/	Worksheet	Global  Column  Erase
/	Range	Format  Label  Erase  Name  Prot  Unprot  Value  Trans
/	File	Retrieve  Save  Erase  List  Import  Directory
/	Data	Fill
/Worksheet	Global	Format  Label-Prefix  Column-Width  Recalculation  Protection  Zero
/Worksheet	Column	Set-Width  Reset-Width  Column-Range
```

The complete subtraction leaves 81 functional/navigation leaves in 25 levels
and 105 total items. Original sibling IDs survive in the version-2 resource;
visible positions and the Escape highlight stack are separate. Startup rejects
old versions, short/trailing bytes, unreferenced strings, duplicate names/actions,
dangling/cyclic/shared children, overlapping item ranges and unreachable levels
before publishing a window. Removing menus does not remove unknown WK1 records
or formula/file compatibility. READY F2/F5/F9 remain; deferred READY F3/F4/F6/F7/
F8/F10 remain unmapped without command-name prompts. F3 in the file chooser is
the captured full-list toggle, a working context-specific file command.
