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

make test runs 17 retained host suites plus a target/import audit: Finder
commands/model/windows/operations/menu/input/list view/replace/limits/notices/
restore, captured desktop icons, Apple menu, shell, drag, power notice and heap
RAM contract. Logs and generated host screens stay in build/flair/desktop/host.
The pure manager objects are shared between tests. Test sources and their
literal/capture oracles are retained. tests/MUTANTS-TODO.md lists removed knobs
only where an existing test/harness still names them; patch conversion remains
unclaimed. The slice's two independent red checks are factory/red-desktop.sh.

test_shell additionally checks literal rows 20 through 39 for absence of menu
chrome with the Finder alone and an application in front, compares the top bar
against the owner's titles plus glyph/clock, tracks both owners' pull-downs,
returns to the Finder, and repaints a window starting at row 20. Its new band
assertion was red on the two-bar compositor before the implementation changed.
Default-window/list-view persistence expectations move up 20 pixels under the
same ruling. The operations suite places its existing fixtures explicitly so
its file-operation probes remain independent of default launch geometry.

Still parked tests: finder_demo (reference tenants), film_scene (InitechCalc
face harness), egg_jtc (unclaimed authored-data packer/fixture), finder_icon_art
and finder_icon_oracles (additional art matrices), menubar_clock (extra NFNT
fixture), menu_handlers (legacy kernel composite). Their future owner is the
desktop import. make smoke delegates to the product's QEMU+Bochs smoke;
make mutants fails until the remaining mutation patches are imported.
