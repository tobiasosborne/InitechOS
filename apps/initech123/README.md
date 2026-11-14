# Initech 123

Worksheet store, formula compiler/evaluator, recalculation, range operations,
Lotus WK1 codec, slash menus and native Toolbox face. This component preserves
its present feature depth; import does not add spreadsheet commands.

## Build and public interface

`make` builds `123.EXE`, `123.MNU` and the deterministic target static library
`libi123.a` under `build/apps/initech123/`. The EXE uses mk/tenant.mk and the
plain shared target flags, with I123_MENU_DISK selecting APPS/123.MNU loading.
An independent factory/mz reader validates the executable and its relocations.
The resource generator is the C menu-tree suite's --resource mode; that suite
compares its entire tree against installed real 123.RI and the captured tree.
Missing or malformed resources terminate startup with EXIT(1), before a window
or menu bar appears, exactly as in legacy-final.

Public headers, consumed with `-Iapps/initech123/include`:

- `i123.h`: worksheet store, formatting, formulas, recalculation, ranges and WK1.
- `i123/wk1_format.h`: WK1 records, defaults and display-format contracts.
- `i123/wk1_formula.h`: formula bytecode, tokens and reference contracts.
- `menu.h`: separate menu model/resource API; not part of the core archive.

InitechCalc can link `build/apps/initech123/libi123.a` instead of compiling this
component's sources. The archive contains sheet, fmt, wk1, formula, range, eval
and recalc objects, including the latter two needed by the formula/range APIs.
It contains no menu, application face or Toolbox runtime. Use the same default
32-bit C ABI and x87 control word 0x037F as the target; no OS dependency exists
in the archive. This does not import InitechCalc.

The face depends only on sdk/include (`tbx.h`, `tbx_save.h` and their public
contracts). All OS calls use the Toolbox gate. Core/fs compile with only this
component's include path. No build input comes from attic.

## Verification

`make test` runs the original 18 host programs plus the protection and shipped-configuration
menu suite, using sdk/host's shared fake Toolbox. No emulator runs in tier 0.
The original 1,023,560 counted assertions are retained; malformed Retrieve and
automatic numeric-entry recalculation add seven, and menu_disk adds 354.
Separate checks still cover eight real Copy/Move pairs, four tiling fixtures,
12 self-transfers and all 82 menu levels/381 items. The precision-negative
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

`make mutants` applies 43 strict patches (including the 41 former hook locations), builds
each and requires the selected host suite to kill it twice. Mutants run only
in tier 2. `make smoke` uses the top-level product-image journey on QEMU and
Bochs, including an independent check of the worksheet saved from the Finder.
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
| Worksheet | Global formats/label prefix/width, recalculation order/mode/iteration, global protection; current-column Set-Width/Reset-Width; Erase No/Yes. |
| Range | Format (Fixed, Sci, Currency, comma, General, +/-, Percent, Date 1/2/3, Text, Hidden, Reset), label alignment, Erase, Name Create/Delete/Labels/Reset/Table, Prot/Unprot, Value and Trans. |
| Copy, Move | Snapshot transfers, singleton-axis tiling for Copy rectangles, formula reference adjustment, named ranges; Copy retains the allowed prefix on protection refusal, Move checks all destinations first. Capacity failures discard the candidate bank. |
| File | Retrieve and Save WK1, Cancel/Replace on existing files. Backup refuses. |
| Print, Data | Menu navigation and Quit/escape; execution leaves below refuse. |
| Graph | Type/Name/Group navigation and Quit; execution leaves below refuse. |
| System, Add-In | System and Add-In execution refuse. |
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

There are **224 refused slash-menu leaves**, plus **File Save > Backup** in
the replacement prompt (225 visible unavailable choices in total). Every leaf
below shows `Command not implemented`; Esc returns to READY. Quit leaves that
return to READY are navigation, not unavailable operations. No listed command
is implemented by this import. Paths and spelling come from the independently
checked tree; current menu_select dispatch was exercised for all 300 leaves
and agrees with the 224-leaf October audit. Sources: legacy-final
`docs/audits/2026-10-23-i123/{REPORT,TRIAGE}.md`, its menu evidence, and
`tests/oracles/menu/era_shortfalls.md`, verified against `app/face_a.c`.

| Parent menu | Every unavailable leaf |
|---|---|
| `/Worksheet/Global/Format/Date` | `4 (Long Intn'l)`, `5 (Short Intn'l)` |
| `/Worksheet/Global/Format/Date/Time` | `1 (HH:MM:SS AM/PM)`, `2 (HH:MM AM/PM)`, `3 (Long Intn'l)`, `4 (Short Intn'l)` |
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
| `/Worksheet/Global` | `Zero` |
| `/Worksheet/Insert` | `Column`, `Row` |
| `/Worksheet/Delete` | `Column`, `Row` |
| `/Worksheet/Column` | `Hide`, `Display` |
| `/Worksheet/Column/Column-Range` | `Set-Width`, `Reset-Width` |
| `/Worksheet/Titles` | `Both`, `Horizontal`, `Vertical`, `Clear` |
| `/Worksheet/Window` | `Horizontal`, `Vertical`, `Sync`, `Unsync`, `Clear` |
| `/Worksheet` | `Status`, `Page` |
| `/Worksheet/Learn` | `Range`, `Cancel`, `Erase` |
| `/Range/Format/Date` | `4 (Long Intn'l)`, `5 (Short Intn'l)` |
| `/Range/Format/Date/Time` | `1 (HH:MM:SS AM/PM)`, `2 (HH:MM AM/PM)`, `3 (Long Intn'l)`, `4 (Short Intn'l)` |
| `/Range` | `Justify`, `Input` |
| `/Range/Search/Formulas` | `Find`, `Replace` |
| `/Range/Search` | `Label`, `Both` |
| `/File/Combine/Copy` | `Entire-File`, `Named/Specified-Range` |
| `/File/Combine/Add` | `Entire-File`, `Named/Specified-Range` |
| `/File/Combine/Subtract` | `Entire-File`, `Named/Specified-Range` |
| `/File/Xtract` | `Formulas`, `Values` |
| `/File/Erase` | `Worksheet`, `Print`, `Graph`, `Other` |
| `/File/List` | `Worksheet`, `Print`, `Graph`, `Other`, `Linked` |
| `/File/Import` | `Text`, `Numbers` |
| `/File` | `Directory` |
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
| `/Data` | `Fill`, `Distribution` |
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
