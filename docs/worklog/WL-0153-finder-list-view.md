# WL-0146: Finder list views (initech-tdnl.44)

The third-launch capture unblocks the part-1 logic. Sources: PRD 3 and 6.2-6.5;
CLAUDE.md Laws 1-4 and Rule 14; system7-decomp re/mint-results-008.md Sec 6;
goldens/captures/s8_finder_list_view.png and s8_finder_list_view_bysize.png;
specs/desktop/finder-windows.md; the existing Finder window, menu, enumeration,
WindowScroll and kind=4 DESKTOP.DB paths. The old audit is
42-lower-view-menu.png in docs/audits/2026-10-03-flair-gui-codex/shots/.

The View command spine now enables Small Icon, Name, Size, Kind and Date with
exactly one check on the front window's current mode. Label, Buttons and Pop-up
stay disabled. Icon mode still uses the original grid/painter. List layout uses
Name / Date Modified / Size / Kind columns (234/178/70/123 px), 20px fixed
headers, 19px rows and native16 cells. The actual sort cue is a pressed, darker
header and E7E7E7 body column; there is no underline. Size sorts descending,
folders last, with case-folded 8.3 name ties and stable enumeration ties. Other
sorts retain the part-1 policy. Header clicks use the same mode transition as
menus; reselecting the current header is consumed without selecting a row.

FAT modification words, size and attributes are copied during enumeration beside
the existing icon array. The same array, selection and spatial-singleton model,
WindowScroll bars and owned-window update path serve every mode. The List Manager
was read: its 32-row fixed-cell store lacks scrolling and LDEF, so cannot hold
this 64-entry Finder model. No second List Manager, event pump, compositor or
persistence codec was introduced. Rows, scroll ranges and hit testing move
together; the header stays fixed. Returning to Icons restores the grid.

The clone does not contain the PM-named WL-0140-finder-icon-art.md or its minted
32px strikes; the existing 32px strikes are still the older three-tone art. A
32-to-16 reduction cannot reproduce the capture. Instead the small strikes are
minted DIRECTLY from the two native16 captures. The different E7E7E7/F3F3F3 Name
body backgrounds distinguish clear cells from opaque ones; all opaque cells must
agree across both captures. The factory transcriber rejects disagreement. The
emu oracle reads the PNGs independently and grades all 256 cells of each native
application, document and folder icon, including mask holes. The old 32px assets
are unchanged. Header-title masks are hand-transcribed title art from the capture,
packed as bitmap rows (the authored ASCII remains in comments), not a new font
engine. Body text reuses the Finder's transparent Geneva painter.

Rule-8 deliberate data extension, tracked by tdnl.44: seven captured icon colours
occupy canon slots 11..17, following the existing indexed-alert slots9/10
extension. This is necessary because the live compositor is indexed even when
the output is 32bpp. No second palette is installed. All existing named colours
retain their values; only seven unnamed dark-gray slots are replaced. The canon
header is regenerated with its existing C generator. Native small art references
canon indices. The live PNG pixel oracle grades the new colours, not the JSON
that the artifact renders from. The DAC generator and canon consistency gates
also pass. Explicit kmain dependencies reuse the existing vesa8.mk DAC generator.

The emulator oracle grades the four visible header cells pixel-for-pixel against
both captures, then shaded column/19px rule strips. APPS data text differs from
the HFS floppy, so text values are not confused with geometry probes. A root
Name-view leg grades the folder cell; APPS Name grades application/document
cells. Real Restart must produce a second FLAIR-LIVE-READY. mtools then extracts
DESKTOP.DB, and an independent byte oracle requires a single kind=4 APPS record
at origin8,60 with view_bits5. A fresh boot/reopen must restore bits5 and its
visible Size header. The existing kind=4 format retains origin/mode, not window
size; the reopen crop accounts for its default dimensions.

Rule-6 named mutants: SORT_UNSTABLE (V2), SIZE_COLUMN_OFFBYONE (V3),
HEADER_IGNORED (V4), SAVE_ICONS (V5), LIST_HEADER_FACE (pressed Size header pixel),
and SMALL_ICON_SHIFT (native16 application cell). The existing ABS_COORDS mutant
needed a dead-helper reference to remain warning-clean; its intended failure is
unchanged, and all twelve Finder-ops host mutants pass their rejection gates.

Rule-14 registration: finder_list_view, BEFORE record-flair, uses the locked
APPS -> Name -> Size header trace and the existing recordlongdbl image. The
capture-required preflight is removed. The PM records the clip on main after
merge. No clip is recorded in this lane, and no bead, CLAUDE.md, HANDOFF.md or
.git state is edited. No commit or push is performed. No Bochs or full make test
is run, per the lane directive.

Changed files:

- Makefile
- os/milton/kmain.c
- os/flair/finder_windows.c, finder_windows.h
- os/flair/finder_desktop.c, finder_desktop.h
- os/flair/finder_cmd.c, finder_cmd.h, finder_menu.c
- os/flair/finder_icon.c, finder_icon.h
- os/flair/flair_look.c, flair_look.h
- spec/assets/color_canon.json, color_canon.h
- spec/assets/finder_list_titles.h, finder_small_icons.h
- spec/flair_list_view_traces.mk
- tools/ppm_flair_list_view_check.c, mint_finder_small_icons.c,
  color_canon_extract.c
- harness/proptest/test_finder_list_view.c, test_finder_menu.c,
  test_finder_cmd.c, test_menu_handlers.c, test_finder_windows.c,
  test_finder_ops.c
- docs/worklog/WL-0153-finder-list-view.md

The only added gate target is test-finder-small-icons-gen (a dependency of the
host list gate). The pre-existing test-finder-list-view and -mutant targets are
extended; the pre-existing RED test-flair-list-view and -mutant targets are now
real, registered emulator gates. Mutant images use the existing Finder-window
mutant Makefile template. The two existing Makefile vectors retain these gates.

Acceptance (INITECH_QMP_STDIO=1 for every emulator run):

- test-finder-small-icons-gen: green (paired-background native16 mask recovery).
- test-finder-list-view: 407 checks, 0 failures; freestanding compilation green.
- test-finder-list-view-mutant: all four named host mutants RED for their rules.
- test-finder-menu, test-finder-cmd, test-menu-handlers, test-finder-windows,
  test-finder-ops and their mutant families: green.
- test-desk-icons and its mutants, test-palette-seafoam, test-makefile-vars:
  green. The emitted source scan finds no non-ASCII, host paths or getenv seam.
- test-color-canon-gen, test-color-canon and both colour mutant targets: green;
  39 independently graded existing canon roles, no missing goldens.
- test-flair-dac and -mutant: green; all256 generated entries agree with the
  canon, and the two generated-table omission mutants go RED.

Exact list-gate acceptance tails:

```text
>>> ppm_flair_list_view_check: green (independent native16 folder cell)
>>> ppm_flair_list_view_check: green (name; independent header cells + shaded row rules)
>>> ppm_flair_list_view_check: green (size; independent header cells + shaded row rules)
>>> list-view DESKTOP.DB: green (independent kind=4 APPS origin8,60 bits5)
>>> ppm_flair_list_view_check: green (reopen; independent header cells + shaded row rules)
>>> test-flair-list-view: green (Name/Size header cells, sort shading, Restart kind=4)
>>> test-flair-list-view-mutant: green (LIST_HEADER_FACE and SMALL_ICON_SHIFT correctly RED)
```

Named RED witnesses:

```text
SORT_UNSTABLE: V2 stable rank matches independent 8.3 ordering
SIZE_COLUMN_OFFBYONE: V3 column boundaries match independent half-open arithmetic
HEADER_IGNORED: V4 Date header click changes sort and preserves unrelated bits
SAVE_ICONS: V5 Restart kind=4 bytes match independent records
LIST_HEADER_FACE: FAIL list-view header pixel428,83 RGB dadada != capture427,70 a5a5a5
SMALL_ICON_SHIFT: FAIL list-view small application pixel55,103 RGB e7e7e7 != capture54,90 000000
```

Targeted regression tails (each make exit status0):


`make test-flair-disk-windows`:

```text
VERDICT   : PASS -- the booted Finder opens the REAL volume in a REAL window,
```

`make test-flair-disk-windows-mutant`:

```text
VERDICT   : PASS -- all three mutants drive their legs RED (the gate bites; Rule 6)
```

`make test-flair-finder-service`:

```text
>>> test-flair-finder-service: green
```

`make test-flair-finder-service-mutant`:

```text
>>> test-flair-finder-service-mutant [2/2]: with the filter removed, the H03 move is refused by the identity guard (reason=service; mtools: ::/TRASH stays at the root)
```

`make test-flair-trash-window`:

```text
>>> test-flair-trash-window: green
```

`make test-flair-trash-window-mutant`:

```text
>>> test-flair-trash-window-mutant: TRASH_ROOT correctly RED -- serial "FINDER-OPEN-TRASH win=0 n=3 singleton=1"; grader: no Trash window at (40,80)
```

`make test-flair-empty-trash`:

```text
>>> test-flair-empty-trash: green
```

`make test-flair-empty-trash-mutant`:

```text
>>> test-flair-empty-trash-mutant: EMPTY_NO_CONFIRM correctly RED (ET-CANCEL: README.TXT is gone from ::/TRASH after Cancel -- purged without confirmation)
```

`make test-flair-trash-purge-views`:

```text
>>> test-flair-trash-purge-views: green
```

`make test-flair-trash-purge-views-mutant`:

```text
>>> test-flair-trash-purge-views-mutant: L002 RESTORED correctly RED (Ctrl-N wrote into the purged folder's freed cluster; fsck.fat finds the orphan)
```

`make test-flair-locked-purge`:

```text
VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
```

`make test-flair-app-launch-mutant`:

```text
>>> test-flair-app-launch: green
>>> test-flair-app-launch-mutant: green (all six mutants RED for their named reasons)
```

`make test-flair-desktop-icons`:

```text
VERDICT   : PASS -- the booted Finder draws, selects, marquees, opens, drags and
```

`make test-flair-desktop-icons-mutant`:

```text
VERDICT   : PASS -- both R3.2 mutants drive their legs RED (the gate bites; Rule 6)
```

`make image`:

```text
>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
```


Headroom correction during final validation:

The first full headroom build failed: kernel_flairtenants_db_write_skip was
197160 loaded bytes, exceeding the196608-byte loader window. The loader window
and KERNEL_HEADROOM_FLOOR4096 remain unchanged. The list header was compacted
without changing its pixels: packed title masks and a20-run rectangular bevel
replace string masks and a per-pixel classifier. The metadata moved to the end
of finder_window_t to retain existing member offsets. The runtime now calls the
already-tested pure rank function directly, rather than its metadata-adapting
private loop. Explicit noinline on list helpers bounds caller duplication.

A measured Finder-only GCC size policy inhibits inlining of its shared
operations; no-tree-loop-distribute-patterns keeps the freestanding string loops
from turning into a hosted strlen call under LTO. This is a factory code-size
policy, not a runtime dependency. After compaction, the measured persistence
mutant was195560 loaded bytes:1048 headroom, still3048 short of the required
4096 reserve. Further shared-kernel reduction or architectural work exceeds the
one-shot Finder lane's scope. The required headroom oracle stays RED; this bead
is not accepted or closed. The compact implementation is revalidated below.

Final compact-build regression tails (all completed targets below exit0):

`make test-flair-list-view-mutant`:

```text
>>> test-flair-list-view: green (Name/Size header cells, sort shading, Restart kind=4)
>>> test-flair-list-view-mutant: green (LIST_HEADER_FACE and SMALL_ICON_SHIFT correctly RED)
```

`make test-flair-disk-windows`:

```text
VERDICT   : PASS -- the booted Finder opens the REAL volume in a REAL window,
```

`make test-flair-disk-windows-mutant`:

```text
VERDICT   : PASS -- all three mutants drive their legs RED (the gate bites; Rule 6)
```

`make test-flair-finder-service`:

```text
>>> test-flair-finder-service: green
```

`make test-flair-finder-service-mutant`:

```text
>>> test-flair-finder-service-mutant [2/2]: with the filter removed, the H03 move is refused by the identity guard (reason=service; mtools: ::/TRASH stays at the root)
```

`make test-flair-trash-window`:

```text
>>> test-flair-trash-window: green
```

`make test-flair-trash-window-mutant`:

```text
>>> test-flair-trash-window-mutant: TRASH_ROOT correctly RED -- serial "FINDER-OPEN-TRASH win=0 n=3 singleton=1"; grader: no Trash window at (40,80)
```

`make test-flair-empty-trash`:

```text
>>> test-flair-empty-trash: green
```

`make test-flair-empty-trash-mutant`:

```text
>>> test-flair-empty-trash-mutant: EMPTY_NO_CONFIRM correctly RED (ET-CANCEL: README.TXT is gone from ::/TRASH after Cancel -- purged without confirmation)
```

`make test-flair-trash-purge-views`:

```text
>>> test-flair-trash-purge-views: green
```

`make test-flair-trash-purge-views-mutant`:

```text
>>> test-flair-trash-purge-views-mutant: L002 RESTORED correctly RED (Ctrl-N wrote into the purged folder's freed cluster; fsck.fat finds the orphan)
```

`make test-flair-locked-purge`:

```text
VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
```

`make test-flair-app-launch-mutant`:

```text
>>> test-flair-app-launch: green
>>> test-flair-app-launch-mutant: green (all six mutants RED for their named reasons)
```

`make test-flair-desktop-icons`:

```text
VERDICT   : PASS -- the booted Finder draws, selects, marquees, opens, drags and
```

`make test-flair-desktop-icons-mutant`:

```text
VERDICT   : PASS -- both R3.2 mutants drive their legs RED (the gate bites; Rule 6)
```

`make image`:

```text
>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
```


Final headroom result (make exited2; all221 kernels built and measured):

```text
>>> test-kernel-headroom: 221 kernels measured; window 196608 B (384 sectors); floor 4096 B
    flagship kernel_flairtenants.elf                              194728 B loaded, headroom   1880 B
    LARGEST  kernel_flairtenants_db_write_skip.elf                195560 B loaded, headroom   1048 B (binding)
!!! test-kernel-headroom FAIL: largest kernel kernel_flairtenants_db_write_skip.elf leaves 1048 B < KERNEL_HEADROOM_FLOOR 4096 B -- a size-policy decision is due (bead initech-8z9j) before any slice grows the kernel
make: *** [Makefile:435: test-kernel-headroom] Error 1
```

Flagship loaded194728 bytes (1880 headroom); binding
kernel_flairtenants_db_write_skip loaded195560 (1048 headroom). The reserve
shortfall is3048 bytes. Neither the loader window nor reserve floor was changed.
All requested functional/emulator families and make image are GREEN on the
compact implementation. The final flagship binary hash matches the one verified
by those runs. The source scan is ASCII-clean. No current functional gate is
left RED; the acceptance blocker is the kernel reserve, plus the deliberately
PM-owned post-merge video. Buttons/Pop-up and Label remain period-grayed.

Raw final gate statuses: build/list_view_compact_runs.tsv. Raw compact host run:
build/list_view_compact_host.log. Raw pixel/mutant/Restart evidence:
build/list_compact_test-flair-list-view-mutant.log, build/list_view_restart.serial,
build/list_view_desktop.db and build/list_view_{name,size,reopen,folder}.ppm.
The working tree is left uncommitted for PM review; no .git or beads edits.
