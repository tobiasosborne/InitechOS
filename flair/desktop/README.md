# FLAIR desktop

make target builds build/flair/desktop/libdesktop.a. system/ links it with the
InitechDOS objects, resident COMMAND.COM, libflair.a and the region engine.
The product starts with the Finder alone, with zero document windows. FLAIR
returns to its caller after Special > Exit to InitechDOS; a second call works.
There is one 20-pixel menu bar, at rows 0 through 19. The front program supplies
its titles and commands; the shell supplies the Initech glyph menu and clock.
The Finder supplies the default resource. InitechPaint's eight Photoshop titles
belong to that application's resource, not to a second shell bar. This follows
the operator ruling of 2026-11-13 in CLAUDE.md, Hazards that are still true,
and GOLDEN-MASTER item 3, superseding the legacy frame's two bands.

The usable desktop starts at row 20 rather than 40. The default volume icon
moves from y=48 to y=28; the bottom-anchored Trash stays in place. New Finder
windows start at y=40 rather than 60 and retain their 20-pixel cascade and size.
Drag limits and the overlay protect only rows 0 through 19. Window zoom and
the app gate's minimum window top are 20 (two minimal Toolbox changes).
Grow uses the available screen extent from the window's current position.
Saved icon/window positions and app-requested initial frames stay as requested.

The compositor keeps a shallow display view of the front owner's MenuBar,
reserving the glyph and attaching the clock without writing into app storage.
Mouse tracking uses this view at screen row zero; Finder/app dispatch receives
the original owner pointer so the Toolbox gate's ownership check still holds.
Activation, menu redraw, close, exit and preference repaint use the same view.
Menu dismissal restores desktop/window content directly below the sole bar.
The obsolete shell-owned About/Quit menu resource has been removed.
The legacy confirmation and application's opportunity to cancel are preserved.
No executable overlay, COMMAND transient, demo tenant or boot personality is
built. Default TARGET_CFLAGS and cdecl apply to every object. tbx_gate.asm
already pushed/cleaned stack arguments, so no ABI change was needed there.

Dependencies: kernel/include, flair/include and assets, flair/region/include,
sdk/include. Generated public font/palette headers are in build/flair/generated.
Host tests additionally use the Toolbox's render/reference test helpers and
factory/testlib; no product build reads attic/. Hardware headers were published
from kernel/ into kernel/include/ without changing their layouts.

Under CLAUDE.md "Two interfaces" (operator amendment c7398425), the resident
Finder temporarily calls the published kernel FAT interface directly. This
covers finder_fs.c and Desktop DB/Trash persistence in desktop_db.c, with the
mounted volume supplied through fileio_fat.h. Removal is tracked as
initech-02siz: route those file operations through DOS file services, preserving
GM2's FAT12 read-write/FAT16 read-only policy and failed-save guarantees.
That DOS migration is separate work; pbl0i enforces the current write policy
within the permitted resident binding. Disk applications still use INT 81h.

Directly linked pure libraries and data helpers (the link is composed by
system/Makefile; libdesktop.a itself does not perform the final link):

| Provider / target input | Pure code used by the desktop |
|---|---|
| flair/region, build/flair/region/target/region.o | Normalised region algebra and caller-owned region storage |
| flair, build/flair/libflair.a | Surface/blitter drawing, chrome and text drawing, flair_look colour/depth mapping and caller-owned heap allocation; public contracts in flair/include/{surface,blitter,chrome,text,flair_look,heap}.h |
| flair/include/asset_lz.h, flair/include/assets and build/flair/generated | Header-defined asset decompression, font metrics/strikes, icon assets, palettes and colour helpers compiled into the desktop's drawing paths |
| sdk/include | Shared ABI records and inline record helpers; no SDK runtime archive is linked into the resident desktop |

libflair.a also contains resident Window, Menu, Control, Dialog, List,
Standard File, Event, Cursor, Process, Scrap and TextEdit managers. Their
public flair/include contracts are the manager bindings used by the shell
and gate; the whole archive is not classified as a pure library. Kernel
hardware, DOS, loader and console calls belong to the resident integration
and the platform adapter below, rather than to the pure-library inventory.
platform.c supplies the freestanding memcpy runtime helper locally.

Published bindings implemented by this component:

| Contract | Desktop implementation / consumers |
|---|---|
| flair/include/flair_platform.h | Every flair_platform_* service in platform.c; the suffix table below lists the kernel/desktop service used for each binding |
| kernel/include/environment.h | kernel_graphical_environment in start.c, the resident DOS shell's returning graphical-environment entry; desktop_console_restore is supplied by the kernel |
| flair/include/tbxgate.h and sdk/include/toolbox_gate.h | tbx_gate_entry installs the INT 81h transport to the Toolbox dispatcher; tbx_tenant_call/tbx_tenant_abort implement its resident call/unwind support in tbx_gate.asm |
| kernel/include/{kbd,mouse,pit,panic}.h and flair/include/event.h hook contracts | The startup/pump registers keyboard/mouse raw-event callbacks, tick advancement, fault handling and cooperative yield, and removes/restores them on exit |

Finder/model/compositor headers within this directory describe internal
desktop calls, not additional OS service entries for disk applications.

Production extraction from legacy-final kmain.c (8207 lines): scene.c owns the
heap-backed scene, palette, splash and framebuffer present; input.c the raw IRQ
queue adapters and cursor; tracking.c drag/close/zoom/grow/collapse/scroll/menu
tracking; launch.c app launch, service ordering and reaping; volumes.c the live
DOS volume and Desktop DB bootstrap; finder_glue.c the Finder bindings/actions,
foreground synchronization and gestures; dialogs.c modal alerts, Standard File
and FILE COPY; start.c startup, binding, the cooperative pump and returning exit.
Function bodies preserve the legacy ordering. The mount reuses the volume DOS
already mounted. Controller replies are polled with interrupts masked. The heap
descriptor is static, because callbacks retain it after scene construction.
Exit removes IRQ hooks, restores the pre-FLAIR INT81 gate, clears pump references,
returns keyboard ownership to DOS and calls desktop_console_restore.

finder_ops_cold.c and finder_windows_cold.c are rejoined with their modules.
power.c extracts the old power/confirmation behavior; about_box.c and
about_format.c provide About This Computer without an overlay. The old power
and transient files are deleted. kmain_legacy.c is now a 335-line unbuilt quarry:
film modal tracking, film arrangement/pump, hidden-mat activation and text-app
handoff. Their integration needs the real InitechCalc/hidden-mat assets or the
text-application ownership work; they are not product entry points. egg_jtc.*,
film_scene.h, film_startup.h and rec_sync.h remain parked. Boot test scripts,
compiled mutants and retired overlay machinery were discarded. Importing the
quarry must obey current interfaces, not resurrect the old ABI or variants.

The platform adapter follows ../NOTES-FOR-KERNEL.md:

| Function suffix (flair_platform_) | Kernel or desktop service |
|---|---|
| files_init | psp_build with zero parameters, shadow PSP |
| bind_files | bind the mounted data-drive number for private file sessions |
| files_close | sft_close_process on shadow PSP |
| files_live | shadow JFT entries 5..19, JFT_CLOSED excluded |
| file_info_get | sft_from_handle, SFT_KIND_FILE only |
| path_is_device | int21_path_is_device |
| dos | int21_dispatch, save/restore PSP and CWD |
| swap_dta | int21_swap_dta |
| memory | int21_mcb_usage (paragraphs) |
| load | loader_load_tenant, explicit plan mapping, caller-owned carve |
| tenant_call | tbx_tenant_call (cdecl assembly) |
| tenant_abort | tbx_tenant_abort (non-returning unwind) |
| desktop_pattern | desktop_set_pattern |
| speaker | io.h inb/outb, PIT channel 2 / port 61h |
| serial | serial_puts |
| panic | panic_show_reason and cli/hlt, never returns |

make test runs 19 host suites plus the shared target/import audit: Finder
commands/model/windows/operations/menu/input/list view/replace/limits/notices/
restore, label properties, captured desktop icons, Apple menu, shell, drag, power notice and heap
RAM contract. Logs and generated host screens stay in build/flair/desktop/host.
The pure manager objects are shared between tests. Test sources and their
literal/capture oracles are retained. tests/MUTANTS-TODO.md lists removed knobs
only where an existing test/harness still names them; patch conversion remains
unclaimed for those rows. The remaining power-notice hook is now the external
mutants/power-silent-cancel.patch, killed by test_power_notice. make mutants
uses the shared isolated-copy runner, builds each patch, and requires two
matching runtime failures. The slice's two independent red checks are
factory/red-desktop.sh. The full shared import audit is part of make test;
there is no desktop-specific substitute or mutation-pattern exception.

Finder icon names remain complete on one line, centered beneath the sprite.
Primary evidence is the genuine Mac OS 8.1 Finder session in the phase-2
review's review-fix-desktop-evidence/: mac-long-names.png (SHA-256
e466a143ca781d2624249783f71c0fefda4f70c1c8d8c51897d8c03ce7d3c5e3),
mac-view-menu.png (8cb41cfee371b12af627a7301cbcb5f7ba8de4df1fc5b97a73a487b91965a87f,
as Icons checked), and mac-list-names.png
(b83420ea178f56976a2b65b9b597bae1b5fbc750378972d96f808500f57069cb).
The fixture includes CALCULAT.EXE, CPANELS.EXE, INITBASE.EXE,
"A Long Name for Testing.txt" and "WWWWWWWWWWWWWWWWWWWWWWWWWWW.TXT".
The first capture shows all five names in full; only list view shortens the
wide W-name. reference-notes.md records the disposable reference disk, pinned
system7-decomp HEAD, Quadra ROM, Basilisk II configuration and capture recipe.

Ordinary columns are 128 pixels apart, matching system7-decomp's mint-008
capture s8_icons_floppy_window.png (SHA-256
53079a514994e0bb61d2fabf115e4ee883b7022f3dd4c7f6fba1b3125a730ffd).
The former provisional 68-pixel grid is retired. Full labels reserve enough
whole columns for their measured width plus a gutter; an item wraps before it
would overlap the next row's allocation. This is deterministic allocation for
FLAIR's authored font, not a claim of exact native Finder pixel coordinates.
Population, Clean Up, Arrange by Name and return from list view share this
allocator. Band, glyphs, selection, hit and damage geometry use the full name.
Scrolling translates this geometry without pinning hidden names at an edge;
the viewport clips paint. Arbitrarily dragged overlapping icons remain possible.
The display record holds 31 characters for the captured stress case. DOS
operands, actual FAT filenames and DESKTOP.DB records retain their 8.3 limits;
this does not add long-filename filesystem support or change persistence.

List Name-column overflow follows Apple's Macintosh Human Interface Guidelines
(1992), p.221: replace the middle, preserving the beginning and ending.
Primary document: https://raw.githubusercontent.com/gingerbeardman/apple-human-interface-guidelines/main/1992%20Macintosh%20Human%20Interface%20Guidelines.pdf
(SHA-256 df13d3ceb76d235b1475578e959d9936442ca52b9a29b8267bec37f7a92ba7fb).
That paragraph describes scrolling lists and is applied only there. Geneva 9's
ASCII strike has no MacRoman ellipsis, so list names use three periods. If even
those do not fit, the list spelling is empty. The label suite checks the literal
shipped and captured names, disjoint automatic extents across viewport widths,
complete painted text including the last glyph, scrolling and list-only middle
omission. It does not bless a screenshot generated by this renderer.

test_shell additionally checks literal rows 20 through 39 for absence of menu
chrome with the Finder alone and an application in front, compares the top bar
against the owner's titles plus glyph/clock, tracks both owners' pull-downs,
returns to the Finder, and repaints a window starting at row 20. Its new band
assertion was red on the two-bar compositor before the implementation changed.
Default-window/list-view persistence expectations move up 20 pixels under the
same ruling. The operations suite places its existing fixtures explicitly so
its file-operation probes remain independent of default launch geometry.
It also independently requires File-title ink during owner switches and checks
that clock redraws preserve every title pixel. The reviewed screenshots alleged
to omit File actually retain identical File ink masks; no missing-title product
fault has been reproduced. mutants/file-title-erased.patch proves the ink
assertion detects a lost title even if renderer-to-renderer comparisons agree.

Still parked tests: finder_demo (reference tenants), film_scene (InitechCalc
face harness), egg_jtc (unclaimed authored-data packer/fixture), finder_icon_art
and finder_icon_oracles (additional art matrices), menubar_clock (extra NFNT
fixture), menu_handlers (legacy kernel composite). Their future owner is the
desktop import. make smoke delegates to the product's QEMU+Bochs smoke.
The unconverted rows in tests/MUTANTS-TODO.md remain coverage limits; the
registered mutation set covers the power notice, full icon names, wide-name
allocation, list-column fitting and File title.

Finder storage uses one write-capability decision in finder_storage.h, shared
by the filesystem binding (finder_fs.c) and Desktop DB/Trash persistence.
The operator's 2026-11-14 ruling is FAT12 read/write and FAT16 read-only.
Every mutating binding call checks before invoking the FAT writer, including
rename/move, create, positioned write, unlink, rmdir and mkdir. Desktop DB
staging/bootstrap and Trash creation/attribute updates use the same check.
Read/enumeration/identity checks retain their separate geometry predicate;
FAT16 still opens and reads through the kernel's two-sector FAT window.
The mount adapter passes that window's buffer when the DOS cache getter
returns NULL, retaining a zero whole-FAT length; the FAT API requires a
non-NULL argument even though windowed link decoding uses the volume.
The write-protect status survives New Folder, drag/move, Put Away and purge
error mapping and uses the ordinary one-button Finder alert.

The alert's "Write protect error." is the Finder form of MSG-DOS-0008 in
kernel/include/dos_messages.json ("Write protect error writing drive %c:").
Primary source: IBM DOS 3.30 User's Guide, Appendix F, printed p.365 / PDF450;
the local transcription is the legacy tree's
`docs/audits/2026-10-15-look-pass6/evidence/dos33-manual.txt` (line 16045).
The catalogue wording is reused, not claimed as a captured native Finder
locked-disk sentence. FAT layout/type classification is documented in
`docs/research/fat16-ground-truth.md`, sections 0 and 2; the refusal policy
comes from the operator, rather than from FAT16's physical capabilities.

The storage host suite mounts mtools FAT12/FAT16 images with a writable backend.
It checks zero write callbacks and every image byte after each FAT16 mutation,
including the real New Folder/drag/Trash/Put Away/Empty Trash/Delete Now routes.
FAT12 writes a folder, a copied stream and Desktop DB; mtools reads them back.
Nested multi-cluster streams are read against a literal pattern on both
formats, and bytes read from FAT16 are copied onto a separate FAT12 image
and checked by mtools.
The external patch fat16-write-check-dropped removes the capability decision
and is killed by these invariants. `make smoke-storage` drives the Finder in
both emulators and compares the FAT16 volume byte-for-byte, with FAT12 as the
positive control. No test scenario is compiled into the product.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: resident
Interface temporary: finder-fat initech-02siz
Published FAT calls in finder_fs.c and desktop_db.c expire when Finder file
operations use DOS. The pure contracts and resident manager/platform bindings
are the inventory described above; this section adds no private app service.
Interface use-pure: flair/include/assets/chicago12.h
Interface use-pure: flair/include/assets/color_canon.h
Interface use-pure: flair/include/assets/desk_icons.h
Interface use-pure: flair/include/assets/font_linkage.h
Interface use-pure: flair/include/assets/geneva12.h
Interface use-pure: flair/include/assets/geneva9.h
Interface use-pure: flair/include/assets/palette.h
Interface use-binding: flair/include/chrome_metrics.h
Interface use-binding: flair/include/control.h
Interface use-binding: flair/include/cursor.h
Interface use-binding: flair/include/dialog.h
Interface use-binding: flair/include/event.h
Interface use-binding: flair/include/filecopy_cancel.h
Interface use-binding: flair/include/flair_platform.h
Interface use-binding: flair/include/grafport.h
Interface use-binding: flair/include/imaging.h
Interface use-binding: flair/include/list.h
Interface use-binding: flair/include/menu.h
Interface use-binding: flair/include/ostype.h
Interface use-binding: flair/include/process.h
Interface use-binding: flair/include/stdfile.h
Interface use-binding: flair/include/tbxgate.h
Interface use-binding: flair/include/window.h
Interface use-binding: flair/include/window_record.h
Interface use-binding: flair/include/winscroll.h
Interface use-binding: kernel/include/blockdev.h
Interface use-binding: kernel/include/boot_info.h
Interface use-binding: kernel/include/console.h
Interface use-binding: kernel/include/devices.h
Interface use-binding: kernel/include/dos_structs.h
Interface use-binding: kernel/include/environment.h
Interface use-binding: kernel/include/fat12.h
Interface use-binding: kernel/include/fileio_fat.h
Interface use-binding: kernel/include/find_data.h
Interface use-binding: kernel/include/idt.h
Interface use-binding: kernel/include/int21.h
Interface use-binding: kernel/include/io.h
Interface use-binding: kernel/include/kbd.h
Interface use-binding: kernel/include/loader.h
Interface use-binding: kernel/include/memory_map.h
Interface use-binding: kernel/include/mouse.h
Interface use-binding: kernel/include/mouse_pack.h
Interface use-binding: kernel/include/panic.h
Interface use-binding: kernel/include/panic_screen.h
Interface use-binding: kernel/include/pic.h
Interface use-binding: kernel/include/pit.h
Interface use-binding: kernel/include/power.h
Interface use-binding: kernel/include/psp.h
Interface use-binding: kernel/include/rtc.h
Interface use-binding: kernel/include/sft.h
Interface use-gate: sdk/include/tbx.h
Interface use-pure: flair/include/asset_lz.h
Interface use-pure: flair/include/blitter.h
Interface use-pure: flair/include/chrome.h
Interface use-pure: flair/include/flair_look.h
Interface use-pure: flair/include/heap.h
Interface use-pure: flair/include/scrap.h
Interface use-pure: flair/include/surface.h
Interface use-pure: flair/include/text.h
Interface use-pure: flair/region/include/region.h
Interface use-pure: flair/region/include/region_algebra.h
Interface use-records: build/flair/generated/asset_lz_data.h
Interface use-records: build/flair/generated/boot_logo.h
Interface use-records: build/flair/generated/chicago12.h
Interface use-records: build/flair/generated/chicago_packed_data.h
Interface use-records: build/flair/generated/color_canon.h
Interface use-records: build/flair/generated/flair_dac.h
Interface use-records: build/flair/generated/geneva12.h
Interface use-records: build/flair/generated/geneva12_packed_data.h
Interface use-records: build/flair/generated/palette.h
Interface use-records: flair/include/assets/boot_logo.h
Interface use-records: flair/include/assets/finder_icons.h
Interface use-records: flair/include/assets/finder_list_titles.h
Interface use-records: flair/include/assets/finder_small_icons.h
Interface use-records: flair/include/assets/menu_canon.h
Interface use-binding: kernel/startup.c
Interface use-binding: kernel/isr.asm
Interface use-binding: flair/event.c
<!-- end-interface-declarations -->
