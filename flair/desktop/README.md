# FLAIR desktop

make target builds build/flair/desktop/libdesktop.a. system/ links it with the
InitechDOS objects, resident COMMAND.COM, libflair.a and the region engine.
The product starts with the Finder alone, with zero document windows. FLAIR
returns to its caller after Special > Exit to InitechDOS; a second call works.
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

Still parked tests: finder_demo (reference tenants), film_scene (InitechCalc
face harness), egg_jtc (unclaimed authored-data packer/fixture), finder_icon_art
and finder_icon_oracles (additional art matrices), menubar_clock (extra NFNT
fixture), menu_handlers (legacy kernel composite). Their future owner is the
desktop import. make smoke delegates to the product's QEMU+Bochs smoke;
make mutants fails until the remaining mutation patches are imported.
