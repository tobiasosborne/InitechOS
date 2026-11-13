# flair-desktop

Finder, system menus, desktop shell, resident demo tenants and production GUI adapters from kmain. Desktop slices of kmain are shared-source evidence rather than duplicate files/LOC in SUMMARY.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

46 production/source-support files; 12,569 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/ref_demo_menu.h:1`|42|tdnl.79: the actual HELLO/NOTES menu resources. Ref: PRD 6.3,|
|`os/apps/ref_tenant.c:1`|418|- HELLO + NOTES, the two reference co-resident FLAIR|
|`os/apps/ref_tenant.h:1`|37|- the two reference co-resident FLAIR tenants (THE ARTIFACT).|
|`os/flair/apple_menu.c:1`|65|- the system Apple menu (THE ARTIFACT; beads|
|`os/flair/apple_menu.h:1`|48|- the system Apple menu (THE ARTIFACT; beads|
|`os/flair/desktop.c:1`|350|- the FLAIR desktop compositor / repaint glue (THE ARTIFACT).|
|`os/flair/desktop.h:1`|173|- the FLAIR desktop compositor / repaint glue (THE ARTIFACT).|
|`os/flair/egg_jtc.c:1`|133|os/apps/ref_tenant.c (AC-2 records and updateEvt clipping);|
|`os/flair/egg_jtc.h:1`|13|Resident vtable (kmain.c): stub addresses only, never overlay addresses|
|`os/flair/egg_jtc_model.c:1`|71|Half-open pads, row-major; gaps and outside points never select a pad. */|
|`os/flair/egg_jtc_model.h:1`|35|The factory packs twelve little-endian words, then the title, the heading|
|`os/flair/film_scene.h:1`|23|Film-scene arrangement policy: existing kmain fd_win_bounds, PRD 6.5.|
|`os/flair/film_startup.h:1`|22|Audit DAS L001/B08; CLAUDE Rule 2. Required media/asset refusal remains|
|`os/flair/finder_cmd.c:1`|410|- the R3 Finder COMMAND-TABLE SPINE (THE ARTIFACT).|
|`os/flair/finder_cmd.h:1`|280|- the R3 Finder COMMAND-TABLE SPINE (THE ARTIFACT).|
|`os/flair/finder_desktop.c:1`|976|- the R3 Finder DESKTOP MANAGER (THE ARTIFACT).|
|`os/flair/finder_desktop.h:1`|621|- the R3 Finder DESKTOP MANAGER (THE ARTIFACT).|
|`os/flair/finder_icon.c:1`|177|- the Finder icon blitter (THE ARTIFACT).|
|`os/flair/finder_icon.h:1`|114|- the Finder desktop-icon blitter (THE ARTIFACT).|
|`os/flair/finder_menu.c:1`|352|- THE FINDER MENU BAR RESOURCE (THE ARTIFACT).|
|`os/flair/finder_menu.h:1`|179|- THE FINDER MENU BAR RESOURCE (THE ARTIFACT).|
|`os/flair/finder_ops.c:1`|192|- the R3.4a Finder FILE OPERATIONS (THE ARTIFACT).|
|`os/flair/finder_ops.h:1`|213|- the R3.4a Finder FILE OPERATIONS: drag-move and|
|`os/flair/finder_ops_cold.c:1`|487|User-initiated move, Replace, Put Away and Trash rename bodies, section COLD. **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/flair/finder_ops_private.h:1`|13|filesystem receives its resident address, never a raw overlay address. */ **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/flair/finder_windows.c:1`|2019|- the R3 Finder DISK WINDOWS (THE ARTIFACT).|
|`os/flair/finder_windows.h:1`|1097|- the R3 Finder DISK WINDOWS (THE ARTIFACT).|
|`os/flair/finder_windows_cold.c:1`|441|Destructive operations and their alert text, section COLD. Window painting, **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/flair/finder_windows_private.h:1`|20|The enumeration callback and traversal model remain resident. */ **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/flair/flair_look.c:1`|369|- the FLAIR policy seam resolver (THE ARTIFACT).|
|`os/flair/flair_look.h:1`|206|- the FLAIR policy seam: the PART->pixel resolver|
|`os/flair/power_notice.h:1`|21|initech-0xs6, audit DAS L022: a refused quit must explain the cancellation|
|`os/flair/shell.c:1`|421|- the FLAIR DESKTOP SHELL (THE ARTIFACT, FLAIR Layer 5).|
|`os/flair/shell.h:1`|303|- the FLAIR DESKTOP SHELL (THE ARTIFACT, FLAIR Layer 5).|
|`os/flair/shell_menus.h:1`|48|- the ITEM LIST of the shell-owned band-1 bar|
|`os/milton/about_box.c:1`|40|About This Computer, section COLD of C:\FLAIR.OVL (ADR-0014|
|`os/milton/about_box.h:1`|28|Apple > About This Computer, OVERLAID (ADR-0014 DEC-14-10:|
|`os/milton/desktop_db.c:1`|365|Boot-time DESKTOP.DB/TRASH persistence skeleton (beads initech-tdnl.8).|
|`os/milton/desktop_db.h:1`|105|DESKTOP.DB/TRASH persistence skeleton for the flagship FLAIR volume.|
|`os/milton/finder_actions.c:1`|825|Cold, user-initiated Finder operations; state and hot polling stay resident.|
|`os/milton/finder_actions.h:1`|129|run in section COLD. State, event polls, disk drivers and alert machinery|
|`os/milton/kmain_cold.c:1`|35|About This Computer's cold text path, mechanically moved from kmain.c. **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/milton/kmain_cold.h:1`|9|Cold shell formatting, split from kmain. Ref: PRD Sec 6.5; About dialog **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/milton/power.h:1`|97|the kernel RESET and HALT primitives (Special > Restart and|
|`os/milton/power_cold.c:1`|504|DEC-14-10 ("Then: ... shutdown and exit"); bead initech-qshpn. **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|
|`os/milton/power_cold.h:1`|43|The resident side of section COLD's Restart / Shut Down / Exit to **SPLIT-BY-SIZE: merge back with parent implementation after removing cap/overlay ABI**|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/apple_menu_items.h:1`|35|- the Apple menu's rows (LOCKED spec-data; beads|
|`spec/assets/alert_icons.h:1`|133|InitechOS FLAIR alert icon strike (LOCKED spec-data).|
|`spec/assets/boot_logo.h:1`|194|- GENERATED by tools/boot_logo_gen.c|
|`spec/assets/desk_icons.h:1`|589|InitechOS FLAIR desktop icon strikes v2 (LOCKED spec-data).|
|`spec/assets/finder_icons.h:1`|323|InitechOS FLAIR disk-window icon strikes v2 (LOCKED|
|`spec/assets/finder_list_titles.h:1`|87|tdnl.44 LOCKED title masks, hand-transcribed from mint008 Sec6 captures:|
|`spec/assets/finder_small_icons.h:1`|66|tdnl.44 LOCKED 16x16 art, minted by tools/mint_finder_small_icons.c.|
|`spec/easter/jump_to_conclusions.json:1`|30|"source": "Authored from operator brief initech-2ter part (a); PRD 6.3/6.4 and Appendix B. No corpus precedent for this hidden trigger was found.",|
|`spec/flair_app_launch_trace.golden:1`|80|- the LOCKED R3.7 launch trace (bead|
|`spec/flair_app_launch_traces.mk:1`|191|- the LOCKED input traces for the R3.7|
|`spec/flair_appswitch_trace.mk:1`|72|- the LOCKED O-5 app-switch input trace + the|
|`spec/flair_desktop_icons_traces.mk:1`|202|- the LOCKED input traces for the R3.2|
|`spec/flair_disk_windows_traces.mk:1`|505|- the LOCKED input traces for the R3.3|
|`spec/flair_egg_jtc_traces.mk:1`|9|LOCKED, operator-authored initech-2ter (a). Start (320,240).|
|`spec/flair_exit_dos_traces.mk:1`|81|- the LOCKED input traces for Special > Exit|
|`spec/flair_fg_close_traces.mk:1`|89|- the LOCKED input traces for the|
|`spec/flair_file_ops_traces.mk:1`|258|- the LOCKED input traces for the R3.4a Finder|
|`spec/flair_file_traces.mk:1`|11|initech-tdnl.91: deliberate locked input addition. Authored fixture actions;|
|`spec/flair_film_scene_traces.mk:1`|9|LOCKED M024-M026 traces. Scene bounds retain kmain's authored geometry:|
|`spec/flair_finder_alerts_trace.mk:1`|20|LOCKED injected traces: tdnl.65/.88. PRD 6.3; s8_alert_modal.png.|
|`spec/flair_finder_cmds_traces.mk:1`|77|- the LOCKED input traces for the "no|
|`spec/flair_finder_follow_traces.mk:1`|85|- the LOCKED input traces for the|
|`spec/flair_finder_icons_traces.mk:1`|46|tdnl.65 re-key: consume root activation before selecting its document.|
|`spec/flair_finder_open_traces.mk:1`|66|- the LOCKED input traces of|
|`spec/flair_fullsave_traces.mk:1`|50|- the LOCKED input traces of|
|`spec/flair_grow_box_trace.mk:1`|12|h6y1 locked mouse trace. Ref: finder_windows.h default cascade/grid;|
|`spec/flair_input_traces.mk:1`|123|- the LOCKED input traces for the input-handling|
|`spec/flair_iword_traces.mk:1`|151|- the LOCKED input traces of test-flair-iword|
|`spec/flair_launchkey_traces.mk:1`|11|initech-rex3: deliberate Rule-8 addition, PRD 6.3/6.5; DEC-LAUNCH-1.|
|`spec/flair_list_view_traces.mk:1`|17|tdnl.44 LOCKED mouse traces. Sources: finder_windows.h default frame/grid;|
|`spec/flair_menu_look_traces.mk:1`|9|LOCKED 2026-10-15 tdnl.93/.94/.95/.70. Open root at600,64; hold File70,21;|
|`spec/flair_menubar_clock_traces.mk:1`|5|LOCKED tdnl.13 R3.6: real mouse packets wake the existing cooperative pump;|
|`spec/flair_power_traces.mk:1`|87|- the LOCKED input traces for the Special >|
|`spec/flair_protection_traces.mk:1`|13|LOCKED trace for audit L118. Same volume/window geometry as|
|`spec/flair_scroll_look_traces.mk:1`|27|Locked trace, 2026-10-15, initech-tdnl.82/.86/.96/.97/.101.|
|`spec/flair_scroll_traces.mk:1`|76|- the LOCKED input traces for the WORKING|
|`spec/flair_skins.h:1`|320|- the FLAIR era/heritage skin registry: a DATA-ONLY VIEW|
|`spec/flair_solid_traces.mk:1`|140|- the LOCKED input traces for the FLAIR|
|`spec/flair_tenants_demo.h:1`|156|- FLAIR App Contract tenants DEMO layout contract|
|`spec/flair_toolbox_hardening_trace.mk:1`|5|Toolbox hardening valid-input regression and Rule-14 clip.|
|`spec/flair_trash_edges_traces.mk:1`|74|- the LOCKED input traces for the Finder|
|`spec/flair_trash_traces.mk:1`|157|- the LOCKED input traces for the TRASH emulator|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/ref_demo_menu.h:5`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`os/apps/ref_demo_menu.h:6`|`flair_tenants_demo.h` → `spec/flair_tenants_demo.h`|PUBLIC INTERFACE|
|`os/apps/ref_tenant.c:77`|`process.h` → `os/flair/process.h`|INTERNAL LEAK|
|`os/apps/ref_tenant.c:78`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/apps/ref_tenant.c:79`|`window_record.h` → `spec/window_record.h`|PUBLIC INTERFACE|
|`os/apps/ref_tenant.c:80`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/apps/ref_tenant.c:81`|`blitter.h` → `os/flair/blitter.h`|INTERNAL LEAK|
|`os/apps/ref_tenant.c:82`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/apps/ref_tenant.c:83`|`heap.h` → `os/flair/heap.h`|INTERNAL LEAK|
|`os/apps/ref_tenant.c:85`|`event_model.h` → `spec/event_model.h`|PUBLIC INTERFACE|
|`os/apps/ref_tenant.c:86`|`flair_tenants_demo.h` → `spec/flair_tenants_demo.h`|PUBLIC INTERFACE|
|`os/apps/ref_tenant.h:29`|`process.h` → `os/flair/process.h`|INTERNAL LEAK|
|`os/flair/apple_menu.c:8`|`apple_menu_items.h` → `spec/apple_menu_items.h`|PUBLIC INTERFACE|
|`os/flair/apple_menu.h:19`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`os/flair/desktop.c:68`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/desktop.c:69`|`blitter.h` → `os/flair/blitter.h`|INTERNAL LEAK|
|`os/flair/desktop.c:70`|`chrome.h` → `os/flair/chrome.h`|INTERNAL LEAK|
|`os/flair/desktop.c:71`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/desktop.c:72`|`grafport.h` → `spec/grafport.h`|PUBLIC INTERFACE|
|`os/flair/desktop.c:73`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/desktop.c:104`|`cpanels_record.h` → `spec/cpanels_record.h`|PUBLIC INTERFACE|
|`os/flair/desktop.h:83`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/desktop.h:84`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/desktop.h:85`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/egg_jtc.c:16`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/egg_jtc.c:17`|`blitter.h` → `os/flair/blitter.h`|INTERNAL LEAK|
|`os/flair/egg_jtc.c:19`|`text.h` → `os/flair/text.h`|INTERNAL LEAK|
|`os/flair/egg_jtc.h:4`|`process.h` → `os/flair/process.h`|INTERNAL LEAK|
|`os/flair/egg_jtc_model.h:10`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/egg_jtc_model.h:11`|`event_model.h` → `spec/event_model.h`|PUBLIC INTERFACE|
|`os/flair/film_scene.h:7`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/film_scene.h:8`|`cursor.h` → `os/flair/cursor.h`|INTERNAL LEAK|
|`os/flair/film_scene.h:9`|`event_model.h` → `spec/event_model.h`|PUBLIC INTERFACE|
|`os/flair/film_startup.h:7`|`../milton/panic_screen.h` → `os/milton/panic_screen.h`|INTERNAL LEAK|
|`os/flair/film_startup.h:8`|`../milton/fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|
|`os/flair/film_startup.h:9`|`../../spec/toolbox_gate.h` → `spec/toolbox_gate.h`|PUBLIC INTERFACE|
|`os/flair/finder_desktop.c:19`|`event.h` → `os/flair/event.h`|INTERNAL LEAK|
|`os/flair/finder_desktop.c:22`|`desk_icons.h` → `spec/assets/desk_icons.h`|PUBLIC INTERFACE|
|`os/flair/finder_desktop.c:23`|`finder_icons.h` → `spec/assets/finder_icons.h`|PUBLIC INTERFACE|
|`os/flair/finder_desktop.c:33`|`asset_lz.h` → `os/flair/asset_lz.h`|INTERNAL LEAK|
|`os/flair/finder_desktop.c:34`|`asset_lz_data.h` → `os/flair/asset_lz_data.h`|INTERNAL LEAK|
|`os/flair/finder_desktop.c:62`|`blitter.h` → `os/flair/blitter.h`|INTERNAL LEAK|
|`os/flair/finder_desktop.c:63`|`geneva9.h` → `spec/assets/geneva9.h`|PUBLIC INTERFACE|
|`os/flair/finder_desktop.h:106`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/finder_desktop.h:107`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/finder_desktop.h:108`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/finder_desktop.h:109`|`process.h` → `os/flair/process.h`|INTERNAL LEAK|
|`os/flair/finder_icon.c:19`|`finder_small_icons.h` → `spec/assets/finder_small_icons.h`|PUBLIC INTERFACE|
|`os/flair/finder_icon.c:30`|`asset_lz.h` → `os/flair/asset_lz.h`|INTERNAL LEAK|
|`os/flair/finder_icon.c:31`|`asset_lz_data.h` → `os/flair/asset_lz_data.h`|INTERNAL LEAK|
|`os/flair/finder_icon.h:53`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/finder_icon.h:54`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/finder_icon.h:55`|`desk_icons.h` → `spec/assets/desk_icons.h`|PUBLIC INTERFACE|
|`os/flair/finder_menu.h:95`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`os/flair/finder_windows.c:48`|`blitter.h` → `os/flair/blitter.h`|INTERNAL LEAK|
|`os/flair/finder_windows.c:49`|`heap.h` → `os/flair/heap.h`|INTERNAL LEAK|
|`os/flair/finder_windows.c:50`|`finder_list_titles.h` → `spec/assets/finder_list_titles.h`|PUBLIC INTERFACE|
|`os/flair/finder_windows.c:51`|`event_model.h` → `spec/event_model.h`|PUBLIC INTERFACE|
|`os/flair/finder_windows.c:52`|`window_record.h` → `spec/window_record.h`|PUBLIC INTERFACE|
|`os/flair/finder_windows.c:53`|`chicago12.h` → `spec/assets/chicago12.h`|PUBLIC INTERFACE|
|`os/flair/finder_windows.h:135`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/finder_windows.h:136`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/finder_windows.h:137`|`region.h` → `os/flair/atkinson/region.h`|PUBLIC INTERFACE|
|`os/flair/finder_windows.h:138`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/finder_windows.h:139`|`process.h` → `os/flair/process.h`|INTERNAL LEAK|
|`os/flair/finder_windows_cold.c:5`|`chicago12.h` → `spec/assets/chicago12.h`|PUBLIC INTERFACE|
|`os/flair/flair_look.c:45`|`finder_small_icons.h` → `spec/assets/finder_small_icons.h`|PUBLIC INTERFACE|
|`os/flair/flair_look.c:46`|`flair_skins.h` → `spec/flair_skins.h`|PUBLIC INTERFACE|
|`os/flair/flair_look.c:47`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/flair_look.c:48`|`color_canon.h` → `spec/assets/color_canon.h`|PUBLIC INTERFACE|
|`os/flair/flair_look.h:43`|`grafport.h` → `spec/grafport.h`|PUBLIC INTERFACE|
|`os/flair/shell.c:64`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`os/flair/shell.c:65`|`dialog.h` → `os/flair/dialog.h`|INTERNAL LEAK|
|`os/flair/shell.c:66`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/shell.c:67`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/shell.c:68`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/shell.c:69`|`menu_canon.h` → `spec/assets/menu_canon.h`|PUBLIC INTERFACE|
|`os/flair/shell.h:87`|`surface.h` → `os/flair/surface.h`|INTERNAL LEAK|
|`os/flair/shell.h:88`|`region_algebra.h` → `spec/region_algebra.h`|PUBLIC INTERFACE|
|`os/flair/shell.h:89`|`window.h` → `os/flair/window.h`|INTERNAL LEAK|
|`os/flair/shell.h:90`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`os/flair/shell.h:91`|`dialog.h` → `os/flair/dialog.h`|INTERNAL LEAK|
|`os/flair/shell.h:92`|`control.h` → `os/flair/control.h`|INTERNAL LEAK|
|`os/flair/shell_menus.h:32`|`menu.h` → `os/flair/menu.h`|INTERNAL LEAK|
|`os/milton/about_box.c:14`|`int21.h` → `os/milton/int21.h`|INTERNAL LEAK|
|`os/milton/about_box.h:12`|`boot_info.h` → `os/milton/boot_info.h`|PUBLIC INTERFACE|
|`os/milton/desktop_db.h:11`|`fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|
|`os/milton/finder_actions.c:5`|`fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|
|`os/milton/finder_actions.c:6`|`tbxgate.h` → `os/flair/tbxgate.h`|INTERNAL LEAK|
|`os/milton/finder_actions.c:7`|`toolbox_gate.h` → `spec/toolbox_gate.h`|PUBLIC INTERFACE|
|`os/milton/finder_actions.c:8`|`event.h` → `os/flair/event.h`|INTERNAL LEAK|
|`os/milton/finder_actions.c:9`|`blockdev.h` → `os/milton/blockdev.h`|PUBLIC INTERFACE|
|`os/milton/finder_actions.h:6`|`boot_info.h` → `os/milton/boot_info.h`|PUBLIC INTERFACE|
|`os/milton/finder_actions.h:9`|`dialog.h` → `os/flair/dialog.h`|INTERNAL LEAK|
|`os/milton/finder_actions.h:11`|`fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|
|`os/milton/finder_actions.h:12`|`event.h` → `os/flair/event.h`|INTERNAL LEAK|
|`os/milton/power.h:43`|`io.h` → `os/milton/io.h`|INTERNAL LEAK|
|`os/milton/power_cold.c:20`|`tbxgate.h` → `os/flair/tbxgate.h`|INTERNAL LEAK|
|`os/milton/power_cold.c:23`|`console.h` → `os/milton/console.h`|INTERNAL LEAK|
|`os/milton/power_cold.c:24`|`int21.h` → `os/milton/int21.h`|INTERNAL LEAK|
|`os/milton/power_cold.c:25`|`kbd.h` → `os/milton/kbd.h`|INTERNAL LEAK|
|`os/milton/power_cold.c:26`|`transient.h` → `os/milton/transient.h`|INTERNAL LEAK|
|`os/milton/power_cold.c:27`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/power_cold.c:28`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/power_cold.c:29`|`color_canon.h` → `spec/assets/color_canon.h`|PUBLIC INTERFACE|
|`os/milton/power_cold.h:9`|`boot_info.h` → `os/milton/boot_info.h`|PUBLIC INTERFACE|
|`os/milton/power_cold.h:10`|`console.h` → `os/milton/console.h`|INTERNAL LEAK|
|`os/milton/power_cold.h:11`|`psp.h` → `os/milton/psp.h`|INTERNAL LEAK|
|`os/milton/power_cold.h:12`|`fat12.h` → `os/milton/fat12.h`|INTERNAL LEAK|

### Generated inputs

|Header|Generator inputs / recipe origin|
|---|---|
|`build/flair_dac.h`|`build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json`; #  recipe to execute (from 'harness/emu/vesa8.mk', line 24):|
|`build/geneva12_packed_data.h`|`build/geneva12_pack spec/assets/geneva12.h`; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 51):|
|`build/dos_messages.inc`|`build/dos_messages.h | build`; #  recipe to execute (from 'Makefile', line 28357):|
|`spec/assets/palette.h`|`spec/assets/palette.json build/palette_extract`; #  recipe to execute (from 'Makefile', line 12063):|
|`build/chicago_packed_data.h`|`build/chicago_pack spec/assets/chicago12.h`; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 122):|
|`build/dos_messages.h`|`spec/dos_messages.json | build`; #  recipe to execute (from 'Makefile', line 28325):|

### Checked-in generated header provenance

These generation facts supplement section 2; checked-in output is an input during the current build, not necessarily regenerated by make.

|Header|Generator|Generator inputs|
|---|---|---|
|`os/flair/asset_lz_data.h:1`|`tools/asset_lz.c:1`|spec/assets/{desk_icons,finder_icons,alert_icons,finder_small_icons,boot_logo}.h|
|`spec/assets/boot_logo.h:1`|`tools/boot_logo_gen.c:1`|compiled-in geometry; generator:4–22 names pubs/common/initech-logo.tex and docs/pubs/DESIGN-REFERENCE.md|
|`spec/assets/chicago12.h:1`|`tools/font_art2h.c:1`|spec/assets/chicago12.art|
|`spec/assets/finder_small_icons.h:1`|`tools/mint_finder_small_icons.c:1`|system7 captures s8_finder_list_view{,_bysize}.png + spec/assets/color_canon.h|
|`spec/assets/color_canon.h:1`|`tools/color_canon_extract.c:1`|spec/assets/color_canon.json|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-flair-desktop.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/flair_tenants.img build/flair_tenants_interactive.img
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:8801: update target 'build/mbr.bin' due to: os/boot/mbr.asm
nasm -f bin os/boot/mbr.asm -o build/mbr.bin
Makefile:28325: update target 'build/dos_messages.h' due to: spec/dos_messages.json
printf '>>> codegen: %s -> %s (63 messages, deterministic)\n' "spec/dos_messages.json" "build/dos_messages.h"
python3 -c "import json; \
d=json.load(open('spec/dos_messages.json')); \
m=d['messages'] if isinstance(d,dict) and 'messages' in d else d; \
assert isinstance(m,dict), 'messages is not an object'; \
esc=lambda s: s.replace(chr(92),chr(92)*2).replace(chr(34),chr(92)+chr(34)).replace(chr(13),chr(92)+'r').replace(chr(10),chr(92)+'n'); \
ids=['MSG-DOS-%04d'%i for i in range(1,64)]; \
missing=[k for k in ids if k not in m]; \
assert not missing, 'spec missing message id(s) (DEC-13 controlled scope): %r'%missing; \
[ (_ for _ in ()).throw(AssertionError('non-string/empty text for %s'%k)) for k in ids if not (isinstance(m[k],str) and m[k]) ]; \
[ (_ for _ in ()).throw(AssertionError('non-ASCII text for %s (Rule 12)'%k)) for k in ids if not all(ord(c)<128 for c in m[k]) ]; \
L=[]; \
L.append('/*'); \
L.append(' * dos_messages.h -- InitechDOS Approved Diagnostic Message Catalogue.'); \
L.append(' *'); \
L.append(' * GENERATED -- DO NOT EDIT; edit the spec and rebuild.'); \
L.append(' *'); \
L.append(' * Source:  spec/dos_messages.json'); \
L.append(' * Ref:     ADR-0003 Appendix C / DEC-13 (controlled vocabulary).'); \
L.append(' *          %c denotes a drive-letter substitution; spacing and'); \
L.append(' *          punctuation are part of the controlled vocabulary.'); \
L.append(' */'); \
L.append('#ifndef INITECH_DOS_MESSAGES_H'); \
L.append('#define INITECH_DOS_MESSAGES_H'); \
L.append(''); \
L+=['#define %s \"%s\"'%(k.replace('-','_'),esc(m[k])) for k in ids]; \
L.append(''); \
L.append('#endif /* INITECH_DOS_MESSAGES_H */'); \
open('build/dos_messages.h','w').write(chr(10).join(L)+chr(10))" \
	|| { printf '!!! codegen FAIL: %s invalid (missing id / bad shape / non-ASCII)\n' "spec/dos_messages.json"; exit 1; }
Makefile:28357: update target 'build/dos_messages.inc' due to: build/dos_messages.h
sed -n 's/^#define \(MSG_DOS_[0-9]*\) "\(.*\)"$/%define \1 `\2`/p' build/dos_messages.h > build/dos_messages.inc
Makefile:8813: update target 'build/IO.SYS' due to: os/boot/io_sys.asm Makefile build/dos_messages.inc
nasm -f bin -Ibuild/ -DMK_KERNEL_SECTORS=384 os/boot/io_sys.asm -o build/IO.SYS
test $(wc -c < build/IO.SYS) -le 4096
Makefile:8817: update target 'build/stage2.bin' due to: os/boot/stage2.asm build/IO.SYS Makefile
nasm -f bin -DMK_KERNEL_SECTORS=384 -DIO_SYS_PATH='"build/IO.SYS"' os/boot/stage2.asm -o build/stage2.bin
Makefile:8857: update target 'build/kstart.o' due to: os/milton/kstart.asm Makefile spec/memory_map.h
nasm -f elf32 -DMK_KERNEL_SECTORS=384 -DMK_KERNEL_STACK_BOT=0x005F0000 -DMK_KERNEL_STACK_TOP=0x005FFFFC os/milton/kstart.asm -o build/kstart.o
Makefile:8949: update target 'build/console.o' due to: os/milton/console.c os/milton/console.h os/milton/boot_info.h os/flair/surface.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/milton -Ios/flair -c os/milton/console.c -o build/console.o
harness/proptest/kernel_size_r4.mk:165: update target 'build/kmain_cold.o' due to: os/milton/kmain_cold.c os/milton/kmain_cold.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/milton -c os/milton/kmain_cold.c -o build/kmain_cold.o
Makefile:8866: update target 'build/surface.o' due to: os/flair/surface.c os/flair/surface.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -c os/flair/surface.c -o build/surface.o
Makefile:8953: update target 'build/idt.o' due to: os/milton/idt.c os/milton/idt.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing -fomit-frame-pointer -Ios/milton -c os/milton/idt.c -o build/idt.o
Makefile:8956: update target 'build/pic.o' due to: os/milton/pic.c os/milton/pic.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/pic.c -o build/pic.o
Makefile:8959: update target 'build/panic.o' due to: os/milton/panic.c os/milton/idt.h os/milton/io.h os/milton/console.h os/milton/panic_screen.h os/milton/boot_info.h os/milton/panic.h os/flair/film_startup.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi --param=min-pagesize=0 -Ios/milton -c os/milton/panic.c -o build/panic.o
Makefile:8967: update target 'build/int21.o' due to: os/milton/int21.c os/milton/int21.h os/milton/conedit.h os/milton/idt.h os/milton/sft.h os/milton/psp.h os/milton/mcb.h os/milton/ata.h spec/dos_structs.h build/dos_messages.h os/milton/ansi.c os/milton/ansi.h os/milton/native.c spec/initechmz_extended.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -Ibuild -c os/milton/int21.c -o build/int21.o
Makefile:8973: update target 'build/devices.o' due to: os/milton/devices.c os/milton/devices.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/devices.c -o build/devices.o
Makefile:8979: update target 'build/mcb.o' due to: os/milton/mcb.c os/milton/mcb.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/milton -c os/milton/mcb.c -o build/mcb.o
Makefile:8985: update target 'build/psp.o' due to: os/milton/psp.c os/milton/psp.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/psp.c -o build/psp.o
Makefile:8991: update target 'build/sft.o' due to: os/milton/sft.c os/milton/sft.h os/milton/psp.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/sft.c -o build/sft.o
Makefile:8996: update target 'build/config_sys.o' due to: os/milton/config_sys.c os/milton/config_sys.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/config_sys.c -o build/config_sys.o
Makefile:9005: update target 'build/sysinit.o' due to: os/milton/sysinit.c os/milton/sysinit.h os/milton/fpu.h spec/fpu_contract.h os/milton/config_sys.h os/milton/sft.h os/milton/psp.h os/milton/int21.h os/milton/pic.h os/milton/idt.h os/milton/pit.h os/milton/kbd.h os/milton/fat12.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/sysinit.c -o build/sysinit.o
Makefile:10832: update target 'build/loader_tenant.o' due to: os/milton/loader.c os/milton/loader_cold.h os/milton/loader.h os/milton/mz.c spec/initechmz_extended.h spec/memory_map.h os/milton/loader_cold.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DLOADER_COLD_EXTERNAL -Ispec -Ios/milton -c os/milton/loader.c -o build/loader_tenant.o
Makefile:9019: update target 'build/ata.o' due to: os/milton/ata.c os/milton/ata.h os/milton/blockdev.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/ata.c -o build/ata.o
harness/diff/fat_diff/block_device.mk:24: update target 'build/block_device.o' due to: os/milton/block_device.c os/milton/block_device.h spec/block_device_contract.h spec/drive_letters.h os/milton/diskutil.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/block_device.c -o build/block_device.o
Makefile:9025: update target 'build/fat12.o' due to: os/milton/fat12.c os/milton/fat12.h os/milton/blockdev.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/fat12.c -o build/fat12.o
Makefile:9034: update target 'build/fileio_fat.o' due to: os/milton/fileio_fat.c os/milton/fileio_fat.h os/milton/fat12.h os/milton/int21.h os/milton/blockdev.h spec/dos_structs.h spec/memory_map.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/fileio_fat.c -o build/fileio_fat.o
Makefile:9058: update target 'build/kbd.o' due to: os/milton/kbd.c os/milton/kbd.h os/milton/io.h spec/tsr_contract.h os/flair/ring_report.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing -fomit-frame-pointer -DKERNEL_PURE_DECODE_OZ -Ios/milton -c os/milton/kbd.c -o build/kbd.o
Makefile:9064: update target 'build/mouse.o' due to: os/milton/mouse.c os/milton/mouse.h os/milton/mouse_pack.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/mouse.c -o build/mouse.o
Makefile:9074: update target 'build/pit.o' due to: os/milton/pit.c os/milton/pit.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/pit.c -o build/pit.o
Makefile:9084: update target 'build/rtc.o' due to: os/milton/rtc.c os/milton/rtc.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -Ios/milton -c os/milton/rtc.c -o build/rtc.o
Makefile:9090: update target 'build/irq.o' due to: os/milton/irq.c os/milton/irq.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/irq.c -o build/irq.o
Makefile:9121: update target 'build/test_program.bin' due to: os/milton/test_program.asm
nasm -f bin os/milton/test_program.asm -o build/test_program.bin
Makefile:9116: update target 'build/bin2c' due to: tools/bin2c.c
cc -std=c11 -Wall -Wextra -Werror -o build/bin2c tools/bin2c.c
Makefile:9126: update target 'build/test_prog_blob.c' due to: build/test_program.bin build/bin2c
build/bin2c build/test_program.bin g_test_prog_image > build/test_prog_blob.c
Makefile:9129: update target 'build/test_prog_blob.o' due to: build/test_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/test_prog_blob.c -o build/test_prog_blob.o
Makefile:9134: update target 'build/type_program.bin' due to: os/milton/type_program.asm
nasm -f bin os/milton/type_program.asm -o build/type_program.bin
Makefile:9137: update target 'build/type_prog_blob.c' due to: build/type_program.bin build/bin2c
build/bin2c build/type_program.bin g_type_prog_image > build/type_prog_blob.c
Makefile:9140: update target 'build/type_prog_blob.o' due to: build/type_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/type_prog_blob.c -o build/type_prog_blob.o
Makefile:9143: update target 'build/dir_program.bin' due to: os/milton/dir_program.asm
nasm -f bin os/milton/dir_program.asm -o build/dir_program.bin
Makefile:9146: update target 'build/dir_prog_blob.c' due to: build/dir_program.bin build/bin2c
build/bin2c build/dir_program.bin g_dir_prog_image > build/dir_prog_blob.c
Makefile:9149: update target 'build/dir_prog_blob.o' due to: build/dir_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/dir_prog_blob.c -o build/dir_prog_blob.o
Makefile:9228: update target 'build/isr.o' due to: os/milton/isr.asm
nasm -f elf32 os/milton/isr.asm -o build/isr.o
Makefile:8887: update target 'build/region.o' due to: os/flair/atkinson/region.c os/flair/atkinson/region.h spec/region_algebra.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -Ispec -Ios/flair/atkinson -Iseed -c os/flair/atkinson/region.c -o build/region.o
Makefile:8890: update target 'build/heap.o' due to: os/flair/heap.c os/flair/heap.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Iseed -c os/flair/heap.c -o build/heap.o
Makefile:8893: update target 'build/event.o' due to: os/flair/event.c os/flair/event.h spec/event_model.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ios/flair -Ispec -Iseed -c os/flair/event.c -o build/event.o
Makefile:8896: update target 'build/cursor.o' due to: os/flair/cursor.c os/flair/cursor.h os/flair/surface.h spec/assets/cursors.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ispec/assets -c os/flair/cursor.c -o build/cursor.o
Makefile:8907: update target 'build/window_live.o' due to: os/flair/window.c os/flair/window.h os/flair/winscroll.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/chrome_metrics.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DWINDOW_ENABLE_R1_OPS -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -c os/flair/window.c -o build/window_live.o
Makefile:8910: update target 'build/blitter.o' due to: os/flair/blitter.c os/flair/blitter.h os/flair/atkinson/region.h os/flair/surface.h spec/region_algebra.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -c os/flair/blitter.c -o build/blitter.o
Makefile:12043: update target 'build/palette_extract' due to: tools/palette_extract.c
cc -std=c11 -Wall -Wextra -Werror -o build/palette_extract tools/palette_extract.c
Makefile:12063: update target 'spec/assets/palette.h' due to: spec/assets/palette.json build/palette_extract
build/palette_extract --header spec/assets/palette.json > spec/assets/palette.h.tmp.$$ \
	&& { if cmp -s spec/assets/palette.h.tmp.$$ spec/assets/palette.h; then rm -f spec/assets/palette.h.tmp.$$; \
	     else mv -f spec/assets/palette.h.tmp.$$ spec/assets/palette.h && printf ">>> assets: regenerated %s from %s\n" "spec/assets/palette.h" "spec/assets/palette.json"; fi; } \
	|| { rm -f spec/assets/palette.h.tmp.$$; exit 1; }
Makefile:8918: update target 'build/chrome.o' due to: os/flair/chrome.c os/flair/chrome.h os/flair/winscroll.h os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h os/flair/rdoc.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/chrome.c -o build/chrome.o
harness/proptest/kernel_size_r4.mk:120: update target 'build/chicago_pack' due to: tools/chicago_pack.c spec/assets/chicago12.h
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -o build/chicago_pack tools/chicago_pack.c
harness/proptest/kernel_size_r4.mk:122: update target 'build/chicago_packed_data.h' due to: build/chicago_pack spec/assets/chicago12.h
build/chicago_pack >build/chicago_packed_data.h.tmp && mv -f build/chicago_packed_data.h.tmp build/chicago_packed_data.h
harness/proptest/geneva12.mk:49: update target 'build/geneva12_pack' due to: tools/chicago_pack.c spec/assets/geneva12.h spec/assets/font_linkage.h
cc -std=c11 -Wall -Wextra -Werror -DPACK_GENEVA12 -Ispec/assets -o build/geneva12_pack tools/chicago_pack.c
harness/proptest/geneva12.mk:51: update target 'build/geneva12_packed_data.h' due to: build/geneva12_pack spec/assets/geneva12.h
build/geneva12_pack >build/geneva12_packed_data.h.tmp && mv -f build/geneva12_packed_data.h.tmp build/geneva12_packed_data.h
Makefile:8921: update target 'build/text.o' due to: os/flair/text.c os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/surface.h os/flair/chicago_packed.h build/chicago_packed_data.h os/flair/geneva12_packed.h build/geneva12_packed_data.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/flair -Ispec/assets -Ispec -Iseed -c os/flair/text.c -o build/text.o
Makefile:8927: update target 'build/menu.o' due to: os/flair/menu.c os/flair/menu.h os/flair/flair_look.h spec/assets/menu_canon.h spec/assets/apple_glyph.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h spec/assets/geneva9.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/menu.c -o build/menu.o
Makefile:8930: update target 'build/control.o' due to: os/flair/control.c os/flair/control.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h spec/progress_platinum.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/control.c -o build/control.o
Makefile:8933: update target 'build/dialog.o' due to: os/flair/dialog.c os/flair/dialog.h spec/assets/alert_icons.h spec/assets/desk_icons.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/event_model.h spec/window_record.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/dialog.c -o build/dialog.o
Makefile:8936: update target 'build/desktop.o' due to: os/flair/desktop.c os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/event.h os/flair/blitter.h os/flair/chrome.h os/flair/surface.h os/flair/heap.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/event_model.h spec/grafport.h spec/imaging.h spec/chrome_metrics.h spec/assets/palette.h os/flair/flair_look.h os/flair/rdoc.h spec/cpanels_record.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/desktop.c -o build/desktop.o
Makefile:8940: update target 'build/flair_look.o' due to: os/flair/flair_look.c os/flair/flair_look.h spec/flair_skins.h spec/assets/color_canon.h os/flair/surface.h spec/grafport.h spec/imaging.h spec/assets/finder_small_icons.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/flair_look.c -o build/flair_look.o
Makefile:8943: update target 'build/flair_shell.o' due to: os/flair/shell.c os/flair/shell.h os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/chrome.h os/flair/blitter.h os/flair/surface.h os/flair/heap.h os/flair/event.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/imaging.h spec/chrome_metrics.h spec/assets/menu_canon.h spec/assets/palette.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/shell.c -o build/flair_shell.o
harness/proptest/kernel_size_r4.mk:86: update target 'build/serial_trace_pack' due to: tools/serial_trace_pack.c
cc -std=c11 -Wall -Wextra -Werror -O2 -o build/serial_trace_pack tools/serial_trace_pack.c
harness/emu/vesa8.mk:22: update target 'build/flair_dac_gen' due to: tools/flair_dac_gen.c
cc -std=c11 -Wall -Wextra -Werror -O2 tools/flair_dac_gen.c -o build/flair_dac_gen
harness/emu/vesa8.mk:24: update target 'build/flair_dac.h' due to: build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json
build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json > build/flair_dac.h.tmp
mv -f build/flair_dac.h.tmp build/flair_dac.h
Makefile:10814: update target 'build/kmain_flairtenants.o' due to: os/milton/kmain.c os/milton/ansi_cursor.h os/milton/finder_actions.h os/milton/power_cold.h build/serial_trace_pack os/milton/kmain_cold.h os/apps/ref_demo_menu.h os/milton/boot_info.h os/milton/io.h os/milton/console.h os/milton/idt.h os/milton/pic.h os/milton/int21.h os/milton/loader.h os/milton/test_prog.h os/milton/psp.h os/milton/sft.h os/milton/ata.h os/milton/fat12.h os/milton/fileio_fat.h os/milton/desktop_db.h os/milton/blockdev.h os/milton/kbd.h os/milton/mouse.h os/milton/mouse_pack.h os/milton/pit.h os/milton/sysinit.h os/milton/command.h os/flair/heap.h os/flair/surface.h os/flair/shell.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/event.h os/flair/system_key.h os/flair/power_notice.h os/flair/process.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_windows.h os/flair/finder_ops.h os/flair/finder_menu.h os/apps/ref_tenant.h spec/event_model.h spec/memory_map.h spec/dos_structs.h spec/region_algebra.h spec/flair_tenants_demo.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/menu_canon.h spec/assets/palette.h spec/assets/color_canon.h os/flair/apple_menu.h os/flair/tbxgate.h spec/toolbox_gate.h os/flair/scrap.h os/flair/stdfile.h os/flair/list.h os/flair/text.h os/milton/transient.h build/dos_messages.h os/flair/cursor.h os/milton/boot_screen.h spec/assets/boot_logo.h spec/assets/chicago12.h build/flair_dac.h os/milton/about_box.h os/flair/egg_jtc.h os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -E os/milton/kmain.c -o build/kmain_flairtenants.o.i
build/serial_trace_pack build/kmain_flairtenants.o.i build/kmain_flairtenants.o.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -c build/kmain_flairtenants.o.c -o build/kmain_flairtenants.o
Makefile:10443: update target 'build/process.o' due to: os/flair/process.c os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/heap.h os/flair/surface.h os/flair/event.h spec/event_model.h spec/region_algebra.h spec/window_record.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/process.c -o build/process.o
Makefile:10449: update target 'build/ref_tenant.o' due to: os/apps/ref_tenant.c os/apps/ref_tenant.h os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/blitter.h os/flair/surface.h os/flair/heap.h os/flair/flair_look.h spec/window_record.h spec/region_algebra.h spec/event_model.h spec/chrome_metrics.h spec/flair_tenants_demo.h spec/assets/color_canon.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/apps -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/apps/ref_tenant.c -o build/ref_tenant.o
Makefile:9040: update target 'build/desktop_db.o' due to: os/milton/desktop_db.c os/milton/desktop_db.h os/milton/fat12.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/desktop_db.c -o build/desktop_db.o
Makefile:10458: update target 'build/finder_icon.o' due to: os/flair/finder_icon.c os/flair/finder_icon.h os/flair/flair_look.h os/flair/surface.h spec/assets/desk_icons.h spec/region_algebra.h spec/assets/finder_small_icons.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_icon.c -o build/finder_icon.o
Makefile:10462: update target 'build/finder_desktop.o' due to: os/flair/finder_desktop.c os/flair/finder_desktop.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/text.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/geneva9.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_desktop.c -o build/finder_desktop.o
Makefile:10470: update target 'build/finder_windows.o' due to: os/flair/finder_windows.c os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/chicago12.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h spec/assets/finder_list_titles.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_windows.c -o build/finder_windows.o
Makefile:10479: update target 'build/finder_ops.o' due to: os/flair/finder_ops.c os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/surface.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_ops.c -o build/finder_ops.o
Makefile:10486: update target 'build/finder_cmd.o' due to: os/flair/finder_cmd.c os/flair/finder_cmd.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -c os/flair/finder_cmd.c -o build/finder_cmd.o
Makefile:10495: update target 'build/finder_menu.o' due to: os/flair/finder_menu.c os/flair/finder_menu.h os/flair/finder_cmd.h os/flair/menu.h os/flair/text.h os/flair/blitter.h os/flair/surface.h spec/chrome_metrics.h spec/grafport.h spec/region_algebra.h spec/assets/chicago12.h os/flair/finder_windows.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_menu.c -o build/finder_menu.o
Makefile:10424: update target 'build/tbxgate.o' due to: build/serial_trace_pack spec/find_data.h os/flair/cp_service.h spec/cp_dialog.h spec/cpanels_record.h os/flair/scrap.h os/flair/ostype.h os/flair/menu.h os/flair/tbxmenu.h spec/grafport.h os/flair/text.h os/flair/tbxgate.c os/flair/tbxgate.h os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h os/flair/event.h os/milton/loader.h os/milton/psp.h spec/toolbox_gate.h spec/event_model.h spec/region_algebra.h spec/window_record.h spec/chrome_metrics.h spec/assets/chicago12.h spec/assets/geneva12.h os/flair/tbxfile.h os/milton/int21.h os/milton/sft.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ios/milton -Ispec -Ispec/assets -Dtputs=serial_puts -Dtputu=serial_putu -Dtputi=serial_puti -E os/flair/tbxgate.c -o build/tbxgate.o.i
build/serial_trace_pack build/tbxgate.o.i build/tbxgate.o.c tbx
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ios/milton -Ispec -Ispec/assets -c build/tbxgate.o.c -o build/tbxgate.o
Makefile:10426: update target 'build/tbx_gate.o' due to: os/milton/tbx_gate.asm
nasm -f elf32 os/milton/tbx_gate.asm -o build/tbx_gate.o
Makefile:10435: update target 'build/scrap.o' due to: os/flair/scrap.c os/flair/scrap.h os/flair/ostype.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ispec -c os/flair/scrap.c -o build/scrap.o
Makefile:10437: update target 'build/stdfile.o' due to: os/flair/stdfile.c os/flair/stdfile.h os/flair/list.h spec/region_algebra.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -c os/flair/stdfile.c -o build/stdfile.o
Makefile:10439: update target 'build/list.o' due to: os/flair/list.c os/flair/list.h spec/region_algebra.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -c os/flair/list.c -o build/list.o
Makefile:10822: update target 'build/apple_menu.o' due to: os/flair/apple_menu.c os/flair/apple_menu.h os/flair/menu.h spec/apple_menu_items.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/apple_menu.c -o build/apple_menu.o
harness/proptest/egg_jtc.mk:53: update target 'build/egg_jtc_model.o' due to: os/flair/egg_jtc_model.c os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -c os/flair/egg_jtc_model.c -o build/egg_jtc_model.o
harness/proptest/egg_jtc.mk:6: update target 'build/egg_jtc_pack' due to: tools/egg_jtc_pack.c os/flair/egg_jtc_model.h
cc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -o build/egg_jtc_pack tools/egg_jtc_pack.c
harness/proptest/egg_jtc.mk:8: update target 'build/egg_jtc_data.bin' due to: spec/easter/jump_to_conclusions.json build/egg_jtc_pack
build/egg_jtc_pack spec/easter/jump_to_conclusions.json > build/egg_jtc_data.bin
harness/proptest/egg_jtc.mk:10: update target 'build/egg_jtc_data.c' due to: build/egg_jtc_data.bin build/bin2c
build/bin2c build/egg_jtc_data.bin egg_jtc_data > build/egg_jtc_data.c
harness/proptest/egg_jtc.mk:57: update target 'build/egg_jtc_data.o' due to: build/egg_jtc_data.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -c build/egg_jtc_data.c -o build/egg_jtc_data.o
Makefile:9100: update target 'build/command.o' due to: os/milton/command.c os/milton/command.h os/milton/env.h os/milton/int21.h os/milton/batch.h spec/find_data.h spec/dos_structs.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -DCOMMAND_KERNEL_REPL -Ispec -Ios/milton -Ibuild -c os/milton/command.c -o build/command.o
Makefile:9105: update target 'build/env.o' due to: os/milton/env.c os/milton/env.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c os/milton/env.c -o build/env.o
Makefile:9111: update target 'build/batch.o' due to: os/milton/batch.c os/milton/batch.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c os/milton/batch.c -o build/batch.o
harness/proptest/egg_jtc.mk:45: update target 'build/egg_jtc.o' due to: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -c os/flair/egg_jtc.c -o build/egg_jtc.o
harness/proptest/egg_jtc.mk:48: update target 'build/about_box_ovl.o' due to: os/milton/about_box.c os/milton/about_box.h os/milton/kmain_cold.h os/milton/int21.h os/milton/boot_info.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/milton -Ispec -c os/milton/about_box.c -o build/about_box_ovl.o
harness/proptest/egg_jtc.mk:50: update target 'build/kmain_cold_ovl.o' due to: os/milton/kmain_cold.c os/milton/kmain_cold.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/milton -c os/milton/kmain_cold.c -o build/kmain_cold_ovl.o
Makefile:10854: update target 'build/finder_actions_ovl.o' due to: os/milton/finder_actions.c os/milton/finder_actions.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -c os/milton/finder_actions.c -o build/finder_actions_ovl.o
Makefile:10856: update target 'build/finder_ops_ovl.o' due to: os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_ops_cold.c -o build/finder_ops_ovl.o
Makefile:10858: update target 'build/finder_windows_ovl.o' due to: os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_windows_cold.c -o build/finder_windows_ovl.o
Makefile:10849: update target 'build/power_cold_ovl.o' due to: os/milton/power_cold.c os/milton/power_cold.h os/milton/finder_actions.h os/milton/power.h os/flair/power_notice.h os/milton/transient.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -c os/milton/power_cold.c -o build/power_cold_ovl.o
Makefile:10851: update target 'build/loader_cold_ovl.o' due to: os/milton/loader_cold.c os/milton/loader_cold.h os/milton/mz.c os/milton/mz.h spec/initechmz_extended.h spec/memory_map.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DLOADER_COLD_EXTERNAL -Ispec -Ios/milton -c os/milton/loader_cold.c -o build/loader_cold_ovl.o
Makefile:10985: update target 'build/transient_imports.rsp' due to: build/command.o build/env.o build/batch.o build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o tools/mk_ovl_tables.sh
export LC_ALL=C; \
for o in build/command.o build/env.o build/batch.o; do gcc-nm -u $o 2>/dev/null | awk '{print $NF}'; done | grep -v ':$' | sort -u > build/transient_imports.rsp.und; \
for o in build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o; do gcc-nm --defined-only $o 2>/dev/null | awk 'NF==3{print $3}'; done | sort -u > build/transient_imports.rsp.kdef; \
for o in build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o; do gcc-nm -u $o 2>/dev/null | awk '{print $NF}'; done | grep -v ':$' | sort -u > build/transient_imports.rsp.kund; \
grep -xF -f build/transient_imports.rsp.kdef build/transient_imports.rsp.und > build/transient_imports.rsp.keep || true; \
n=$(wc -l < build/transient_imports.rsp.keep); [ "$n" -gt 0 ] || { printf '!!! transient imports: no kernel symbol kept -- the derivation is broken\n'; exit 1; }; \
sed 's/^/-Wl,--undefined=/' build/transient_imports.rsp.keep > build/transient_imports.rsp; \
{ sed 's/^/extern /' build/transient_imports.rsp.keep; printf '%%define TRANSIENT_N_IMPORTS %s\n' "$n"; } > build/transient_imports_ext.inc; \
{ sed '$d' build/transient_imports.rsp.keep | sed 's/^/    dd /'; printf '    dd %s + TRANSIENT_LAST_IMPORT_SKEW\n' "$(tail -n 1 build/transient_imports.rsp.keep)"; } > build/transient_imports_dd.inc; \
{ printf 'bits 32\n'; sed 's/^/extern /' build/transient_imports.rsp.keep; \
  printf 'section .rodata progbits alloc noexec nowrite align=4\nglobal transient_import_want\nglobal transient_import_count\ntransient_import_want:\n'; \
  sed 's/^/    dd /' build/transient_imports.rsp.keep; printf 'transient_import_count:\n    dd %s\nsection .note.GNU-stack noalloc noexec nowrite progbits\n' "$n"; } > build/transient_want.asm; \
nasm -f elf32 build/transient_want.asm -o build/transient_want.o || exit 1; \
printf '%s\n' 'build/transient_want.o' >> build/transient_imports.rsp; \
printf '>>> transient imports: %s kernel symbols kept (%s undefined in the transient); header table + kernel want table generated\n' "$n" "$(wc -l < build/transient_imports.rsp.und)"; \
nm --defined-only build/transient_want.o | awk 'NF==3{print $3}' | sort -u - build/transient_imports.rsp.kdef -o build/transient_imports.rsp.kdef; \
sh tools/mk_ovl_tables.sh build nasm build/transient_imports.rsp.kdef build/transient_imports.rsp.kund build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o || exit 1; \
cat build/ovl_undefined.rsp >> build/transient_imports.rsp; \
printf '%s\n' 'build/ovl_kernel.o' >> build/transient_imports.rsp
Makefile:10903: update target 'build/kernel_flairtenants.elf' due to: build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o os/milton/kernel.ld build/transient_imports.rsp
gcc -m32 -march=i386 -nostdlib -no-pie -flto -Oz -flto-partition=one -fno-jump-tables -ffunction-sections -fdata-sections -mregparm=3 -mrtd -freg-struct-return -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Wl,--build-id=none -Xlinker --gc-sections -T os/milton/kernel.ld -o build/kernel_flairtenants.elf build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o @build/transient_imports.rsp
Makefile:11042: update target 'build/overlay_hdr.o' due to: os/milton/overlay_hdr.asm build/transient_imports.rsp
nasm -f elf32 -Ibuild/ os/milton/overlay_hdr.asm -o build/overlay_hdr.o
Makefile:11056: update target 'build/flairovl_kernel_flairtenants.elf' due to: build/kernel_flairtenants.elf build/overlay_hdr.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o os/milton/overlay.ld build/transient_imports.rsp
objcopy $(sed 's/^/-N /' build/ovl_exports.txt) build/kernel_flairtenants.elf build/flairovl_kernel_flairtenants.elf.syms
ld -m elf_i386 -T os/milton/overlay.ld --just-symbols=build/flairovl_kernel_flairtenants.elf.syms -o build/flairovl_kernel_flairtenants.elf build/overlay_hdr.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o
Makefile:10906: update target 'build/kernel_flairtenants.bin' due to: build/kernel_flairtenants.elf
objcopy -O binary build/kernel_flairtenants.elf build/kernel_flairtenants.bin
sz=$(wc -c < build/kernel_flairtenants.bin); max=$(( 384 * 512 )); \
if [ "$sz" -gt "$max" ]; then \
	printf '!!! kernel_flairtenants.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$sz" "$max"; \
	exit 1; \
fi; \
dd if=/dev/zero of=build/kernel_flairtenants.bin bs=1 seek="$sz" count="$(( max - sz ))" conv=notrunc status=none; \
printf ">>> kernel(flairtenants): %s (flat binary @0x500000, padded to %d sectors)\n" "build/kernel_flairtenants.bin" "384"
end=$(nm build/kernel_flairtenants.elf | awk '$3=="_kernel_end"{print "0x"$1}'); lowest=$(nm -n build/kernel_flairtenants.elf | awk '$1 ~ /^[0-9A-Fa-f]+$/ && $2 ~ /^[TtRrDdBb]$/ {print "0x"$1; exit}'); if [ -z "$end" ]; then printf '!!! kernel-end guard (flairtenants): _kernel_end symbol not found in %s\n' "build/kernel_flairtenants.elf"; exit 1; fi; if [ -z "$lowest" ]; then printf '!!! kernel-floor guard (flairtenants): no defined symbol address found in %s\n' "build/kernel_flairtenants.elf"; exit 1; fi; endv=$(( end )); lowv=$(( lowest )); base=$(( 0x00500000 )); ceil=$(( 0x00580000 )); if [ "$lowv" -lt "$base" ]; then printf '!!! kernel-floor guard (flairtenants): lowest symbol=0x%x below KERNEL_BASE=0x%x -- LOAD_LOW relapse\n' "$lowv" "$base"; exit 1; fi; if [ "$endv" -gt "$ceil" ]; then printf '!!! kernel-end guard (flairtenants): _kernel_end=0x%x passes KERNEL_IMAGE_CEIL=0x%x (transient base; kernel stack above) -- shrink the kernel or re-ratify the runway sub-windows; bump KERNEL_SECTORS too if loaded bytes grew\n' "$endv" "$ceil"; exit 1; fi; printf '>>> kernel placement guard (flairtenants): lowest=0x%x >= KERNEL_BASE=0x%x; _kernel_end=0x%x <= KERNEL_IMAGE_CEIL=0x%x OK\n' "$lowv" "$base" "$endv" "$ceil"
Makefile:11040: update target 'build/flairovl' due to: tools/flairovl.c spec/overlay_format.h
cc -std=c11 -O2 -Wall -Wextra -Werror -Ispec tools/flairovl.c -o build/flairovl
Makefile:11056: update target 'build/flairovl_kernel_flairtenants.sys' due to: build/flairovl_kernel_flairtenants.elf build/kernel_flairtenants.bin build/flairovl
objcopy -O binary build/flairovl_kernel_flairtenants.elf build/flairovl_kernel_flairtenants.raw
build/flairovl pack build/flairovl_kernel_flairtenants.raw build/kernel_flairtenants.bin build/flairovl_kernel_flairtenants.ovl build/flairovl_kernel_flairtenants.sys
Makefile:10919: update target 'build/INITDOS.SYS' due to: build/flairovl_kernel_flairtenants.sys build/flairovl_kernel_flairtenants.ovl
cp -f build/flairovl_kernel_flairtenants.sys build/INITDOS.SYS
Makefile:10957: update target 'build/transient_hdr.o' due to: os/milton/transient_hdr.asm build/transient_imports.rsp
nasm -f elf32 -Ibuild/ os/milton/transient_hdr.asm -o build/transient_hdr.o
Makefile:11071: update target 'build/transient_flairtenants.elf' due to: build/kernel_flairtenants.elf build/transient_hdr.o build/command.o build/env.o build/batch.o os/milton/transient.ld
end=$(nm build/kernel_flairtenants.elf | awk '$3=="_kernel_end"{print "0x"$1}'); [ -n "$end" ] && [ $(( end )) -le $(( 0x00580000 )) ] || { printf '!!! transient(%s): kernel _kernel_end=%s reaches TRANSIENT_BASE=%s\n' 'flairtenants' "$end" '0x00580000'; exit 1; }
ld -m elf_i386 -T os/milton/transient.ld --just-symbols=build/kernel_flairtenants.elf -o build/transient_flairtenants.elf build/transient_hdr.o build/command.o build/env.o build/batch.o
Makefile:11071: update target 'build/transient_flairtenants.bin' due to: build/transient_flairtenants.elf
objcopy -O binary build/transient_flairtenants.elf build/transient_flairtenants.bin
sz=$(wc -c < build/transient_flairtenants.bin); max=$(( 0x005C0000 - 0x00580000 )); [ "$sz" -le "$max" ] || { printf '!!! transient_flairtenants.bin (%s B) exceeds its load window\n' "$sz"; exit 1; }; n=$(od -An -tu4 -j24 -N4 build/transient_flairtenants.bin | tr -d ' '); want=$(wc -l < build/transient_imports.rsp.keep); [ "$n" = "$want" ] || { printf '!!! transient_flairtenants.bin: header n_imports=%s != %s kernel imports (gcc-nm -u of the transient objects; initech-13ig)\n' "$n" "$want"; exit 1; }; printf '>>> transient COMMAND.COM (%s): %s B in boot root, linked against %s\n' 'flairtenants' "$sz" 'build/kernel_flairtenants.elf'
harness/diff/fat_diff/boot_system.mk:9: update target 'build/boot_system' due to: harness/diff/fat_diff/boot_system.c
cc -std=c11 -O2 -Wall -Wextra -Werror harness/diff/fat_diff/boot_system.c -o build/boot_system
Makefile:11074: update target 'build/flair_tenants.img' due to: build/mbr.bin build/stage2.bin build/IO.SYS build/INITDOS.SYS build/flairovl_kernel_flairtenants.ovl build/transient_flairtenants.bin build/boot_system tools/mk_boot_system.sh
sh tools/mk_boot_system.sh build/flair_tenants.img build/mbr.bin build/stage2.bin build/IO.SYS build/INITDOS.SYS build/boot_system build/transient_flairtenants.bin build/flairovl_kernel_flairtenants.ovl
printf ">>> flair-tenants image: %s (HELLO+NOTES co-resident; COMMAND.COM in boot FAT root)\n" "build/flair_tenants.img"
Makefile:11115: update target 'build/kmain_flairtenants_int.o' due to: os/milton/kmain.c os/milton/ansi_cursor.h os/milton/finder_actions.h os/milton/power_cold.h build/serial_trace_pack os/milton/kmain_cold.h os/apps/ref_demo_menu.h os/milton/boot_info.h os/milton/io.h os/milton/console.h os/milton/idt.h os/milton/pic.h os/milton/int21.h os/milton/loader.h os/milton/test_prog.h os/milton/psp.h os/milton/sft.h os/milton/ata.h os/milton/fat12.h os/milton/fileio_fat.h os/milton/desktop_db.h os/milton/blockdev.h os/milton/kbd.h os/milton/mouse.h os/milton/mouse_pack.h os/milton/pit.h os/milton/sysinit.h os/milton/command.h os/flair/heap.h os/flair/surface.h os/flair/shell.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/event.h os/flair/system_key.h os/flair/power_notice.h os/flair/process.h os/apps/ref_tenant.h spec/event_model.h spec/memory_map.h spec/dos_structs.h spec/region_algebra.h spec/flair_tenants_demo.h spec/assets/menu_canon.h spec/assets/palette.h spec/assets/color_canon.h spec/assets/cursors.h os/flair/tbxgate.h spec/toolbox_gate.h os/flair/scrap.h os/flair/stdfile.h os/flair/list.h os/flair/text.h os/milton/transient.h build/dos_messages.h os/flair/cursor.h os/milton/boot_screen.h spec/assets/boot_logo.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -DFLAIR_LIVE_INTERACTIVE -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -E os/milton/kmain.c -o build/kmain_flairtenants_int.o.i
build/serial_trace_pack build/kmain_flairtenants_int.o.i build/kmain_flairtenants_int.o.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -DFLAIR_LIVE_INTERACTIVE -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -c build/kmain_flairtenants_int.o.c -o build/kmain_flairtenants_int.o
Makefile:11140: update target 'build/kernel_flairtenants_int.elf' due to: build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/kmain_flairtenants_int.o os/milton/kernel.ld build/transient_imports.rsp
gcc -m32 -march=i386 -nostdlib -no-pie -flto -Oz -flto-partition=one -fno-jump-tables -ffunction-sections -fdata-sections -mregparm=3 -mrtd -freg-struct-return -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Wl,--build-id=none -Xlinker --gc-sections -T os/milton/kernel.ld -o build/kernel_flairtenants_int.elf build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/kmain_flairtenants_int.o @build/transient_imports.rsp
Makefile:11143: update target 'build/kernel_flairtenants_int.bin' due to: build/kernel_flairtenants_int.elf
objcopy -O binary build/kernel_flairtenants_int.elf build/kernel_flairtenants_int.bin
sz=$(wc -c < build/kernel_flairtenants_int.bin); max=$(( 384 * 512 )); \
if [ "$sz" -gt "$max" ]; then \
	printf '!!! kernel_flairtenants_int.bin (%s bytes) exceeds KERNEL_SECTORS window (%s bytes)\n' "$sz" "$max"; \
	exit 1; \
fi; \
dd if=/dev/zero of=build/kernel_flairtenants_int.bin bs=1 seek="$sz" count="$(( max - sz ))" conv=notrunc status=none; \
printf ">>> kernel(flairtenants-interactive): %s (visible cursor + unbounded pump, padded to %d sectors)\n" "build/kernel_flairtenants_int.bin" "384"
end=$(nm build/kernel_flairtenants_int.elf | awk '$3=="_kernel_end"{print "0x"$1}'); lowest=$(nm -n build/kernel_flairtenants_int.elf | awk '$1 ~ /^[0-9A-Fa-f]+$/ && $2 ~ /^[TtRrDdBb]$/ {print "0x"$1; exit}'); if [ -z "$end" ]; then printf '!!! kernel-end guard (flairtenants-interactive): _kernel_end symbol not found in %s\n' "build/kernel_flairtenants_int.elf"; exit 1; fi; if [ -z "$lowest" ]; then printf '!!! kernel-floor guard (flairtenants-interactive): no defined symbol address found in %s\n' "build/kernel_flairtenants_int.elf"; exit 1; fi; endv=$(( end )); lowv=$(( lowest )); base=$(( 0x00500000 )); ceil=$(( 0x00580000 )); if [ "$lowv" -lt "$base" ]; then printf '!!! kernel-floor guard (flairtenants-interactive): lowest symbol=0x%x below KERNEL_BASE=0x%x -- LOAD_LOW relapse\n' "$lowv" "$base"; exit 1; fi; if [ "$endv" -gt "$ceil" ]; then printf '!!! kernel-end guard (flairtenants-interactive): _kernel_end=0x%x passes KERNEL_IMAGE_CEIL=0x%x (transient base; kernel stack above) -- shrink the kernel or re-ratify the runway sub-windows; bump KERNEL_SECTORS too if loaded bytes grew\n' "$endv" "$ceil"; exit 1; fi; printf '>>> kernel placement guard (flairtenants-interactive): lowest=0x%x >= KERNEL_BASE=0x%x; _kernel_end=0x%x <= KERNEL_IMAGE_CEIL=0x%x OK\n' "$lowv" "$base" "$endv" "$ceil"
printf ">>> flair-tenants INTERACTIVE image: %s (visible cursor + unbounded pump)\n" "build/kernel_flairtenants_int.bin"
Makefile:11056: update target 'build/flairovl_kernel_flairtenants_int.elf' due to: build/kernel_flairtenants_int.elf build/overlay_hdr.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o os/milton/overlay.ld build/transient_imports.rsp
objcopy $(sed 's/^/-N /' build/ovl_exports.txt) build/kernel_flairtenants_int.elf build/flairovl_kernel_flairtenants_int.elf.syms
ld -m elf_i386 -T os/milton/overlay.ld --just-symbols=build/flairovl_kernel_flairtenants_int.elf.syms -o build/flairovl_kernel_flairtenants_int.elf build/overlay_hdr.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o
Makefile:11056: update target 'build/flairovl_kernel_flairtenants_int.sys' due to: build/flairovl_kernel_flairtenants_int.elf build/kernel_flairtenants_int.bin build/flairovl
objcopy -O binary build/flairovl_kernel_flairtenants_int.elf build/flairovl_kernel_flairtenants_int.raw
build/flairovl pack build/flairovl_kernel_flairtenants_int.raw build/kernel_flairtenants_int.bin build/flairovl_kernel_flairtenants_int.ovl build/flairovl_kernel_flairtenants_int.sys
Makefile:11157: update target 'build/transient_flairtenants_int.elf' due to: build/kernel_flairtenants_int.elf build/transient_hdr.o build/command.o build/env.o build/batch.o os/milton/transient.ld
end=$(nm build/kernel_flairtenants_int.elf | awk '$3=="_kernel_end"{print "0x"$1}'); [ -n "$end" ] && [ $(( end )) -le $(( 0x00580000 )) ] || { printf '!!! transient(%s): kernel _kernel_end=%s reaches TRANSIENT_BASE=%s\n' 'flairtenants_int' "$end" '0x00580000'; exit 1; }
ld -m elf_i386 -T os/milton/transient.ld --just-symbols=build/kernel_flairtenants_int.elf -o build/transient_flairtenants_int.elf build/transient_hdr.o build/command.o build/env.o build/batch.o
Makefile:11157: update target 'build/transient_flairtenants_int.bin' due to: build/transient_flairtenants_int.elf
objcopy -O binary build/transient_flairtenants_int.elf build/transient_flairtenants_int.bin
sz=$(wc -c < build/transient_flairtenants_int.bin); max=$(( 0x005C0000 - 0x00580000 )); [ "$sz" -le "$max" ] || { printf '!!! transient_flairtenants_int.bin (%s B) exceeds its load window\n' "$sz"; exit 1; }; n=$(od -An -tu4 -j24 -N4 build/transient_flairtenants_int.bin | tr -d ' '); want=$(wc -l < build/transient_imports.rsp.keep); [ "$n" = "$want" ] || { printf '!!! transient_flairtenants_int.bin: header n_imports=%s != %s kernel imports (gcc-nm -u of the transient objects; initech-13ig)\n' "$n" "$want"; exit 1; }; printf '>>> transient COMMAND.COM (%s): %s B in boot root, linked against %s\n' 'flairtenants_int' "$sz" 'build/kernel_flairtenants_int.elf'
Makefile:11159: update target 'build/flair_tenants_interactive.img' due to: build/mbr.bin build/stage2.bin build/IO.SYS build/flairovl_kernel_flairtenants_int.sys build/flairovl_kernel_flairtenants_int.ovl build/transient_flairtenants_int.bin build/boot_system tools/mk_boot_system.sh
sh tools/mk_boot_system.sh build/flair_tenants_interactive.img build/mbr.bin build/stage2.bin build/IO.SYS build/flairovl_kernel_flairtenants_int.sys build/boot_system build/transient_flairtenants_int.bin build/flairovl_kernel_flairtenants_int.ovl
printf ">>> flair-tenants INTERACTIVE image: %s (visible cursor + unbounded pump; COMMAND.COM in boot FAT root)\n" "build/flair_tenants_interactive.img"

# stderr

```

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 14; UNIT 26; META 0; SIZE/LAYOUT 1.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-flair-desktop.md](gates-flair-desktop.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/diff/fat_diff/test_desktop_db.c — UNIT

`harness/diff/fat_diff/test_desktop_db.c:1`; Host oracle for the R3.1 DESKTOP.DB/TRASH persistence skeleton.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_desktop_db`, `build/test_desktop_db_mut_magic_flip`, `build/test_desktop_db_mut_write_skip`.
Run/assert gate(s): `test-desktop-db`, `test-desktop-db-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_desktop_db`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_desktop_db harness/diff/fat_diff/test_desktop_db.c os/milton/desktop_db.c \
	os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/test_finder_diskfull.c — UNIT

`harness/diff/fat_diff/test_finder_diskfull.c:1`; tdnl.62. Real FAT rails; saturated independent allocation table. A failed. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_diskfull`, `build/test_finder_diskfull_mutant`.
Run/assert gate(s): `test-finder-diskfull-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_finder_diskfull`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/test_finder_diskfull harness/diff/fat_diff/test_finder_diskfull.c os/milton/desktop_db.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/proptest/test_apple_menu.c — ORACLE-STRONG

`harness/proptest/test_apple_menu.c:1`; - the HOST ORACLE of the system Apple. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_apple_menu`.
Run/assert gate(s): `test-apple-menu`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_apple_menu.c`, `spec/apple_menu_items.h`, `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/geneva9.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/render.c`, `harness/render/render.h`, `spec/apple_menu_items.h`, `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/geneva9.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `tools/palette_extract.c`.
Canonical `make -n` compile/link (`build/test_apple_menu`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_apple_menu harness/proptest/test_apple_menu.c os/flair/apple_menu.c os/flair/menu.c os/flair/text.c os/flair/blitter.c os/flair/surface.c os/flair/atkinson/region.c harness/render/render.c os/flair/heap.c os/flair/flair_look.c -lpng
```

### harness/proptest/test_boot_logo.c — UNIT

`harness/proptest/test_boot_logo.c:1`; the boot-screen logo STRIKE oracle (bead initech-tdnl.105).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_boot_logo`, `build/test_boot_logo_mut_notch_row`.
Run/assert gate(s): `test-boot-logo`, `test-boot-logo-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/`, `spec/assets//`, `spec/assets/boot_logo.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/boot_logo.h`.
Canonical `make -n` compile/link (`build/test_boot_logo`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec/assets -Iseed -o build/test_boot_logo harness/proptest/test_boot_logo.c -lm
```

### harness/proptest/test_boot_screen.c — UNIT

`harness/proptest/test_boot_screen.c:1`; host render of the FLAIR boot screen (bead. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_boot_screen`, `build/test_boot_screen_mut_no_logo`.
Run/assert gate(s): `test-boot-screen`, `test-boot-screen-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/`, `spec/assets/boot_logo.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/boot_logo.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`.
Canonical `make -n` compile/link (`build/test_boot_screen`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/milton -Ios/flair -o build/test_boot_screen harness/proptest/test_boot_screen.c
```

### harness/proptest/test_desk_icons.c — UNIT

`harness/proptest/test_desk_icons.c:1`; the Finder icon BLITTER oracle (test-desk-icons).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_desk_icons`, `build/test_desk_icons_mutant_full_row`, `build/test_desk_icons_mutant_mask_ignored`, `build/test_desk_icons_mutant_row_off1`, `build/test_finder_icon_art`, `build/test_finder_icon_art_mut_noselect`.
Run/assert gate(s): `test-desk-icons`, `test-desk-icons-mutant`, `test-finder-icon-art`, `test-finder-icon-art-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/render/icon_capture.h`, `spec/assets/`, `spec/assets/alert_icons.h`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_small_icons.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/region_algebra.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_finder_icon_art.c`, `harness/render/icon_capture.h`, `spec/assets/alert_icons.h`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_small_icons.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/region_algebra.h`.
Canonical `make -n` compile/link (`build/test_desk_icons`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iseed -o build/test_desk_icons harness/proptest/test_desk_icons.c os/flair/finder_icon.c os/flair/surface.c os/flair/atkinson/region.c os/flair/flair_look.c -lpng
```

### harness/proptest/test_egg_jtc.c — UNIT

`harness/proptest/test_egg_jtc.c:1`; Independent transcription of operator-authored JTC contract. Law 2:. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_egg_jtc`, `build/test_egg_jtc_cell_mutant`, `build/test_egg_jtc_mutant`.
Run/assert gate(s): `test-egg-jtc`, `test-egg-jtc-cell-mutant`, `test-egg-jtc-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `tools/bin2c.c`, `tools/egg_jtc_pack.c`.
Canonical `make -n` compile/link (`build/test_egg_jtc`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -o build/test_egg_jtc harness/proptest/test_egg_jtc.c os/flair/egg_jtc_model.c build/egg_jtc_data.c os/flair/event.c
```

### harness/proptest/test_film_scene.c — UNIT

`harness/proptest/test_film_scene.c:1`; Routing oracle for M024/M025: invoke the REAL Calc tenant, capture its. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_film_busy_mut`, `build/test_film_scene`.
Run/assert gate(s): `test-film-scene`, `test-film-scene-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/calc_ledger.h`, `spec/i123/wk1_format.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_calc_tenant.c`, `spec/assets/calc_ledger.h`, `spec/i123/wk1_format.h`.
Canonical `make -n` compile/link (`build/test_film_scene`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/apps/tbx -Ios/i123/core -Ispec -Ispec/assets -ffunction-sections -fdata-sections -Ios/flair -Ios/flair/atkinson -Wl,--gc-sections -o build/test_film_scene harness/proptest/test_film_scene.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/range.c os/flair/cursor.c os/flair/surface.c
```

### harness/proptest/test_finder_cmd.c — ORACLE-STRONG

`harness/proptest/test_finder_cmd.c:1`; the HOST oracle for the R3 Finder command-table spine.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_finder_cmd`, `build/test_finder_cmd_mutant_pred_stuck_enabled`, `build/test_finder_cmd_mutant_silent_unknown`, `build/test_finder_cmd_mutant_table_bypass`.
Run/assert gate(s): `test-finder-cmd`, `test-finder-cmd-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_process.c`, `harness/render/`, `spec/assets/`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_finder_cmd`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_finder_cmd harness/proptest/test_finder_cmd.c os/flair/finder_cmd.c
```

### harness/proptest/test_finder_demo.c — UNIT

`harness/proptest/test_finder_demo.c:1`; tdnl.79: drive the REAL demo resource through the Menu Manager. */. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_demo`, `build/test_finder_demo_mutant`.
Run/assert gate(s): `test-finder-demo`, `test-finder-demo-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/render/render.c`.
Canonical `make -n` compile/link (`build/test_finder_demo`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -Ios/apps -o build/test_finder_demo harness/proptest/test_finder_demo.c os/flair/menu.c os/flair/text.c os/flair/blitter.c os/flair/surface.c os/flair/atkinson/region.c harness/render/render.c os/flair/heap.c os/flair/flair_look.c -lpng
```

### harness/proptest/test_finder_desktop.c — UNIT

`harness/proptest/test_finder_desktop.c:1`; the R3.2 Finder DESKTOP MANAGER oracle (THE ORACLE).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_desktop`, `build/test_finder_desktop_mutant_dbltick_off`, `build/test_finder_desktop_mutant_marquee_offbyone`, `build/test_finder_desktop_mutant_no_underlay`.
Run/assert gate(s): `test-finder-desktop`, `test-finder-desktop-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_desk_icons.c`, `harness/proptest/test_drag.c`, `harness/render/`, `spec/assets/`, `spec/assets/desk_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/flair_solid_traces.mk`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/desk_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/test_finder_desktop`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_finder_desktop harness/proptest/test_finder_desktop.c os/flair/event.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_finder_icon_art.c — UNIT

`harness/proptest/test_finder_icon_art.c:1`; the Finder icon ART oracle (test-finder-icon-art).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_icon_art`, `build/test_finder_icon_art_mut_noselect`.
Run/assert gate(s): `test-finder-icon-art`, `test-finder-icon-art-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/render/icon_capture.h`, `spec/assets/alert_icons.h`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_small_icons.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/region_algebra.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_desk_icons.c`, `harness/render/icon_capture.h`, `spec/assets/alert_icons.h`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_small_icons.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/region_algebra.h`.
Canonical `make -n` compile/link (`build/test_finder_icon_art`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iseed -o build/test_finder_icon_art harness/proptest/test_finder_icon_art.c os/flair/finder_icon.c os/flair/surface.c os/flair/atkinson/region.c os/flair/flair_look.c -lpng
```

### harness/proptest/test_finder_icon_oracles.c — UNIT

`harness/proptest/test_finder_icon_oracles.c:1`; Factory regression: the icon graders must follow the LOCKED scene.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_icon_oracles`, `build/test_finder_icon_oracles_mut_unselected`.
Run/assert gate(s): `test-finder-icon-oracles`, `test-finder-icon-oracles-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/color_canon.h`, `spec/dos_folder_apps.txt`, `spec/flair_finder_icons_traces.mk`, `spec/flair_trash_traces.mk`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/render/alert_capture.h`, `harness/render/icon_capture.h`, `spec/assets/color_canon.h`, `tools/ppm_flair_trash_check.c`.
Canonical `make -n` compile/link (`build/test_finder_icon_oracles`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -DICON_CAPTURES_DIR='"../system7-decomp/goldens/captures"' -o build/test_finder_icon_oracles harness/proptest/test_finder_icon_oracles.c -lpng
```

### harness/proptest/test_finder_input.c — UNIT

`harness/proptest/test_finder_input.c:1`; Manager TrackControl (system7-decomp/specs/toolbox/control-manager.md).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_input`, `build/test_finder_input_mutant`.
Run/assert gate(s): `test-finder-input`, `test-finder-input-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_process.c`.
Canonical `make -n` compile/link (`build/test_finder_input`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -o build/test_finder_input harness/proptest/test_finder_input.c os/flair/process.c os/flair/window.c os/flair/heap.c os/flair/atkinson/region.c
```

### harness/proptest/test_finder_limits.c — UNIT

`harness/proptest/test_finder_limits.c:1`; tdnl.61/.63/.84/.64. Explicit listings, actions and expected names.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/finder_host_entries`, `build/finder_host_keys`, `build/finder_host_open-all`, `build/finder_host_window-limit`, `build/test_finder_limits`.
Run/assert gate(s): `test-finder-entries-mutant`, `test-finder-keys-mutant`, `test-finder-limits`, `test-finder-open-all-mutant`, `test-finder-window-limit-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_finder_windows.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/finder_host_entries`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS -DFINDER_MUT_ICON64 -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/finder_host_entries harness/proptest/test_finder_limits.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_finder_list_view.c — UNIT

`harness/proptest/test_finder_list_view.c:1`; tdnl.44 host oracle. Ref: PRD 6.3/6.5; design F1.3/F3.3;. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_finder_list_view`.
Run/assert gate(s): `test-finder-list-view`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/render/`, `spec/assets/`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_finder_windows.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/test_finder_list_view`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_finder_list_view harness/proptest/test_finder_list_view.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c os/flair/finder_menu.c
```

### harness/proptest/test_finder_menu.c — ORACLE-STRONG

`harness/proptest/test_finder_menu.c:1`; the HOST oracle for THE FINDER MENU BAR RESOURCE.. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_finder_menu`, `build/test_finder_menu_mutant_cmdchar_dup`, `build/test_finder_menu_mutant_dead_item`, `build/test_finder_menu_mutant_enable_stuck`.
Run/assert gate(s): `test-finder-menu`, `test-finder-menu-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/render/`, `spec/assets/`, `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/geneva9.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/render.c`, `harness/render/render.h`, `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/geneva9.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `tools/palette_extract.c`.
Canonical `make -n` compile/link (`build/test_finder_menu`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_finder_menu harness/proptest/test_finder_menu.c os/flair/finder_menu.c os/flair/finder_cmd.c os/flair/menu.c os/flair/text.c os/flair/blitter.c os/flair/surface.c os/flair/atkinson/region.c harness/render/render.c os/flair/heap.c os/flair/flair_look.c
```

### harness/proptest/test_finder_notices.c — UNIT

`harness/proptest/test_finder_notices.c:1`; tdnl.62/.66/.72/.57: expected words are the operator/bead wording.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/finder_host_disk-alert`, `build/finder_host_launch-alert`, `build/finder_host_move-alert`, `build/test_finder_notices`.
Run/assert gate(s): `test-finder-disk-alert-mutant`, `test-finder-launch-alert-mutant`, `test-finder-move-alert-mutant`, `test-finder-notices`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_finder_windows.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/finder_host_disk-alert`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS -DFINDER_MUT_NO_DISK_ALERT -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/finder_host_disk-alert harness/proptest/test_finder_notices.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_finder_ops.c — UNIT

`harness/proptest/test_finder_ops.c:1`; the R3.4a Finder FILE-OPERATIONS oracle (THE ORACLE).. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/test_finder_ops`, `build/test_finder_ops_mutant_abs`, `build/test_finder_ops_mutant_arrange`, `build/test_finder_ops_mutant_hilite`, `build/test_finder_ops_mutant_locked_purge`, `build/test_finder_ops_mutant_noconfirm`, `build/test_finder_ops_mutant_powernosave`, `build/test_finder_ops_mutant_scrollpaint`, `build/test_finder_ops_mutant_trashroot`, `build/test_finder_replace`, `build/test_finder_restore`.
Run/assert gate(s): `test-finder-lock-mutant`, `test-finder-ops`, `test-finder-ops-mutant`, `test-finder-replace`, `test-finder-restore`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_finder_windows.c`, `harness/render/`, `spec/assets/`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_finder_replace.c`, `harness/proptest/test_finder_restore.c`, `harness/proptest/test_finder_windows.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/test_finder_ops`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_finder_ops harness/proptest/test_finder_ops.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_finder_replace.c — UNIT

`harness/proptest/test_finder_replace.c:1`; tdnl.57: Replace preserves the displaced item through the SAME Trash. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/finder_host_replace`, `build/test_finder_replace`.
Run/assert gate(s): `test-finder-replace`, `test-finder-replace-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_finder_ops.c`, `harness/proptest/test_finder_windows.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/finder_host_replace`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS -DFINDER_MUT_REPLACE_CANCEL -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/finder_host_replace harness/proptest/test_finder_replace.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_finder_restore.c — UNIT

`harness/proptest/test_finder_restore.c:1`; tdnl.57: Trash rename survives encode/decode, Put Away uses original name. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/finder_host_restore`, `build/test_finder_restore`.
Run/assert gate(s): `test-finder-restore`, `test-finder-restore-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_finder_ops.c`, `harness/proptest/test_finder_windows.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/finder_host_restore`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS -DFINDER_MUT_ORIGINAL_LOST -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/finder_host_restore harness/proptest/test_finder_restore.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_finder_windows.c — UNIT

`harness/proptest/test_finder_windows.c:1`; the R3.3 Finder DISK WINDOWS oracle (THE ORACLE).. deterministic assertions; mentions of properties/differentials include checks owned by separate suites, not sufficient to call this a strong oracle.
Build target(s): `build/finder_host_disk-alert`, `build/finder_host_entries`, `build/finder_host_keys`, `build/finder_host_launch-alert`, `build/finder_host_move-alert`, `build/finder_host_open-all`, `build/finder_host_replace`, `build/finder_host_restore`, `build/finder_host_window-limit`, `build/test_finder_limits`, `build/test_finder_list_view`, `build/test_finder_notices`, `build/test_finder_ops`, `build/test_finder_ops_mutant_abs`, `build/test_finder_ops_mutant_arrange`, `build/test_finder_ops_mutant_hilite`, `build/test_finder_ops_mutant_locked_purge`, `build/test_finder_ops_mutant_noconfirm`, `build/test_finder_ops_mutant_powernosave`, `build/test_finder_ops_mutant_scrollpaint`, `build/test_finder_ops_mutant_trashroot`, `build/test_finder_replace`, `build/test_finder_restore`, `build/test_finder_windows`, `build/test_finder_windows_mutant_cleanup_unsorted`, `build/test_finder_windows_mutant_dots_shown`, `build/test_finder_windows_mutant_open_app_dark`, `build/test_finder_windows_mutant_open_no_app`, `build/test_finder_windows_mutant_purge_keeps_views`, `build/test_finder_windows_mutant_service_shown`, `build/test_finder_windows_mutant_singleton_dup`, `build/test_finder_windows_mutant_view_unchecked`, `build/test_finder_windows_mutant_vollabel_shown`, `build/test_menu_handlers`.
Run/assert gate(s): `test-finder-disk-alert-mutant`, `test-finder-entries-mutant`, `test-finder-keys-mutant`, `test-finder-launch-alert-mutant`, `test-finder-limits`, `test-finder-list-view`, `test-finder-lock-mutant`, `test-finder-move-alert-mutant`, `test-finder-notices`, `test-finder-open-all-mutant`, `test-finder-ops`, `test-finder-ops-mutant`, `test-finder-replace`, `test-finder-replace-mutant`, `test-finder-restore`, `test-finder-restore-mutant`, `test-finder-window-limit-mutant`, `test-finder-windows`, `test-finder-windows-mutant`, `test-menu-handlers`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/qemu.c`, `harness/proptest/test_finder_desktop.c`, `harness/proptest/test_finder_menu.c`, `harness/render/`, `spec/assets/`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/flair_disk_windows_traces.mk`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_finder_limits.c`, `harness/proptest/test_finder_list_view.c`, `harness/proptest/test_finder_notices.c`, `harness/proptest/test_finder_ops.c`, `harness/proptest/test_finder_replace.c`, `harness/proptest/test_finder_restore.c`, `harness/proptest/test_menu_handlers.c`, `harness/render/render.c`, `spec/assets/desk_icons.h`, `spec/assets/finder_icons.h`, `spec/assets/finder_list_titles.h`, `spec/assets/finder_small_icons.h`, `spec/assets/geneva9.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
Canonical `make -n` compile/link (`build/finder_host_disk-alert`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DWINDOW_ENABLE_R1_OPS -DFINDER_MUT_NO_DISK_ALERT -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/finder_host_disk-alert harness/proptest/test_finder_notices.c os/flair/event.c os/flair/finder_windows.c os/flair/finder_ops.c os/flair/finder_desktop.c os/flair/finder_icon.c os/flair/finder_cmd.c os/flair/window.c os/flair/desktop.c os/flair/blitter.c os/flair/surface.c os/flair/heap.c os/flair/chrome.c os/flair/atkinson/region.c os/flair/flair_look.c
```

### harness/proptest/test_flair_routing.c — ORACLE-STRONG

`harness/proptest/test_flair_routing.c:1`; the R3.7 O-1-style HOST routing suite over. property/exhaustive or independent differential checks (inferred from source assertions and loops).
Build target(s): `build/test_flair_routing`.
Run/assert gate(s): `test-flair-routing`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_process.c`, `spec/assets/chicago12.h`, `spec/assets/geneva12.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/cp_dialog.h`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/find_data.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/toolbox_gate.h`, `spec/window_record.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/expect_abort.h`, `harness/proptest/flair_routing_stubs.c`, `harness/render/render.c`, `harness/render/render.h`, `seed/test_assert.h`, `spec/assets/chicago12.h`, `spec/assets/geneva12.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/cp_dialog.h`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/find_data.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/toolbox_gate.h`, `spec/window_record.h`, `tools/palette_extract.c`.
Canonical `make -n` compile/link (`build/test_flair_routing`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -Iharness/render -Iseed -o build/test_flair_routing harness/proptest/test_flair_routing.c harness/proptest/flair_routing_stubs.c os/flair/tbxgate.c os/flair/event.c os/flair/process.c os/flair/window.c os/flair/heap.c os/flair/atkinson/region.c os/milton/psp.c os/milton/sft.c os/flair/chrome.c os/flair/surface.c os/flair/flair_look.c harness/render/render.c
```

### harness/proptest/test_menubar_clock.c — UNIT

`harness/proptest/test_menubar_clock.c:1`; tdnl.13, host oracle FIRST. Literal hours / boundary readings are independent. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_menubar_clock`.
Run/assert gate(s): `test-menubar-clock`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/geneva9.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/menubar_clock_oracle.c`, `harness/menubar_clock_oracle.h`, `harness/render/render.c`, `harness/render/render.h`, `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/geneva9.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/flair_skins.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `tools/palette_extract.c`.
Canonical `make -n` compile/link (`build/test_menubar_clock`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -Iharness -o build/test_menubar_clock harness/proptest/test_menubar_clock.c harness/menubar_clock_oracle.c os/flair/menu.c os/flair/text.c os/flair/blitter.c os/flair/surface.c os/flair/atkinson/region.c harness/render/render.c os/flair/heap.c os/flair/flair_look.c -lpng
```

### harness/proptest/test_power_notice.c — UNIT

`harness/proptest/test_power_notice.c:1`; B04 / audit L022: exact authored explanation, app identity, and physical. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_power_notice`, `build/test_power_notice_mutant`.
Run/assert gate(s): `test-calcda-power`, `test-calcda-power-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/chicago12.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/chicago12.h`.
Canonical `make -n` compile/link (`build/test_power_notice`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ispec/assets -o build/test_power_notice harness/proptest/test_power_notice.c
```

### harness/proptest/test_shell.c — UNIT

`harness/proptest/test_shell.c:1`; the FLAIR DESKTOP SHELL M4 GATE (THE ORACLE; ADR-0004 D-8).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/tbx_mut_SHELL_MUT_MENU_COUNT`, `build/tbx_mut_SHELL_MUT_SHORT_SURFACE`, `build/test_shell`, `build/test_shell_mutant_ignoreoverlay`, `build/test_shell_mutant_modalbehind`, `build/test_shell_mutant_nomodal`, `build/test_shell_mutant_onemenubar`.
Run/assert gate(s): `test-flair-shell`, `test-flair-shell-mutant`, `test-tbx-mutant-SHELL_MUT_MENU_COUNT`, `test-tbx-mutant-SHELL_MUT_SHORT_SURFACE`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_drag.c`, `harness/render/`, `harness/render/render.h`, `spec/assets/`, `spec/assets/apple_glyph.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/expect_abort.h`, `harness/render/render.c`, `harness/render/render.h`, `spec/assets/apple_glyph.h`, `spec/assets/menu_canon.h`, `spec/assets/palette.h`, `spec/chrome_metrics.h`, `spec/grafport.h`, `spec/imaging.h`, `spec/region_algebra.h`, `spec/window_record.h`, `tools/palette_extract.c`.
Canonical `make -n` compile/link (`build/test_shell`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -o build/test_shell harness/proptest/test_shell.c os/flair/shell.c harness/render/render.c os/flair/surface.c os/flair/heap.c os/flair/atkinson/region.c os/flair/window.c os/flair/blitter.c os/flair/chrome.c os/flair/menu.c os/flair/text.c os/flair/control.c os/flair/dialog.c os/flair/event.c os/flair/desktop.c os/flair/flair_look.c
```

### harness/proptest/test_skin_era_frozen.c — SIZE/LAYOUT

`harness/proptest/test_skin_era_frozen.c:1`; the ACCRETION digest oracle (test-skin-era-frozen).. memory/layout, byte/size policy or legacy map constraints; preserve functional safety invariants separately.
Build target(s): `build/test_skin_era_frozen`, `build/test_skin_era_frozen_mutant`, `build/test_skin_era_frozen_mutant_baserow`.
Run/assert gate(s): `test-skin-era-frozen`, `test-skin-era-frozen-mutant`, `test-skin-era-frozen-mutant-baserow`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_skin_era_frozen.c`, `spec/assets/`, `spec/assets/color_canon.h`, `spec/flair_skins.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/color_canon.h`, `spec/flair_skins.h`.
Canonical `make -n` compile/link (`build/test_skin_era_frozen`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ispec/assets -o build/test_skin_era_frozen harness/proptest/test_skin_era_frozen.c
```

### harness/proptest/test_skin_teal.c — UNIT

`harness/proptest/test_skin_teal.c:1`; the AUTHORED-grade skin oracle (test-skin-teal).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_skin_teal`, `build/test_skin_teal_mutant`, `build/test_skin_teal_mutant_plat`.
Run/assert gate(s): `test-skin-teal`, `test-skin-teal-mutant`, `test-skin-teal-mutant-plat`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_skin_teal.c`, `spec/assets/`, `spec/assets/color_canon.h`, `spec/flair_skins.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/color_canon.h`, `spec/flair_skins.h`.
Canonical `make -n` compile/link (`build/test_skin_teal`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ispec/assets -o build/test_skin_teal harness/proptest/test_skin_teal.c
```

### tools/ppm_finder_demo_check.c — ORACLE-STRONG

`tools/ppm_finder_demo_check.c:1`; tdnl.79: disabled ink comes from the independent Mac OS 8.1 capture.. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_finder_demo_check`.
Run/assert gate(s): `test-flair-finder-demo`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Canonical `make -n` compile/link (`build/ppm_finder_demo_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/ppm_finder_demo_check tools/ppm_finder_demo_check.c -lpng
```

### tools/ppm_flair_app_launch_check.c — ORACLE-STRONG

`tools/ppm_flair_app_launch_check.c:1`; - the R3.7 APP LAUNCH screendump grader. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_app_launch_check`.
Run/assert gate(s): `test-flair-app-launch`, `test-flair-fg-close`, `test-flair-finder-input`, `test-flair-paint`, `test-flair-tenant-menu`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/`, `spec/assets//`, `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/font_linkage.h`, `spec/assets/menu_canon.h`, `spec/chicago12_nfnt_golden.h`, `spec/chrome_metrics.h`, `spec/flair_app_launch_trace.golden`, `spec/flair_app_launch_traces.mk`, `spec/toolbox_gate.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `spec/assets/apple_glyph.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/font_linkage.h`, `spec/chicago12_nfnt_golden.h`.
Canonical `make -n` compile/link (`build/ppm_flair_app_launch_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Ispec -o build/ppm_flair_app_launch_check tools/ppm_flair_app_launch_check.c
```

### tools/ppm_flair_appswitch_check.c — UNIT

`tools/ppm_flair_appswitch_check.c:1`; the O-5 EMU app-switch oracle's screendump. structural/behavioral regression against authored geometry or implementation captures; no independent period/reference differential established for this checker.
Build target(s): `build/ppm_flair_appswitch_check`.
Run/assert gate(s): `test-flair-appswitch`, `test-flair-appswitch-mutant`, `test-flair-desktop-8bpp`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/color_canon.h`, `spec/chrome_metrics.h`, `spec/flair_appswitch_trace.mk`, `spec/flair_tenants_demo.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/color_canon.h`, `spec/chrome_metrics.h`, `spec/flair_tenants_demo.h`.
Canonical `make -n` compile/link (`build/ppm_flair_appswitch_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ispec/assets -o build/ppm_flair_appswitch_check tools/ppm_flair_appswitch_check.c
```

### tools/ppm_flair_boot_screen_check.c — ORACLE-STRONG

`tools/ppm_flair_boot_screen_check.c:1`; the FLAIR BOOT SCREEN pixel oracle. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_boot_screen_check`.
Run/assert gate(s): `test-boot-screen`, `test-boot-screen-mutant`, `test-flair-boot-screen`, `test-flair-boot-screen-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/boot_logo.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `spec/assets/boot_logo.h`.
Canonical `make -n` compile/link (`build/ppm_flair_boot_screen_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -o build/ppm_flair_boot_screen_check tools/ppm_flair_boot_screen_check.c -lpng
```

### tools/ppm_flair_desktop_icons_check.c — ORACLE-STRONG

`tools/ppm_flair_desktop_icons_check.c:1`; the R3.2 Finder DESKTOP MANAGER. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_desktop_icons_check`.
Run/assert gate(s): `test-flair-desktop-icons`, `test-flair-desktop-icons-bochs-pixel`, `test-flair-desktop-icons-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/bochs_leg_check.sh`, `harness/proptest/test_desk_icons.c`, `harness/proptest/test_finder_desktop.c`, `harness/render/`, `harness/render/icon_capture.h`, `spec/assets/`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/flair_desktop_icons_traces.mk`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/icon_capture.h`, `spec/assets/color_canon.h`.
Canonical `make -n` compile/link (`build/ppm_flair_desktop_icons_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iharness/render -DICON_CAPTURES_DIR='"/home/tobiasosborne/Projects/system7-decomp/goldens/captures"' -o build/ppm_flair_desktop_icons_check tools/ppm_flair_desktop_icons_check.c -lpng
```

### tools/ppm_flair_disk_windows_check.c — ORACLE-STRONG

`tools/ppm_flair_disk_windows_check.c:1`; initech-zc68: root roster README/APPS/DOS/AUTOEXEC, NEWFOLD at index4.. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_disk_windows_check`.
Run/assert gate(s): `test-dos-folder`, `test-dos-folder-mutant`, `test-flair-disk-windows`, `test-flair-empty-trash`, `test-flair-finder-cmds`, `test-flair-finder-follow`, `test-flair-finder-follow-mutant`, `test-flair-finder-restore`, `test-flair-finder-service`, `test-flair-finder-service-mutant`, `test-flair-scroll`, `test-flair-scroll-mutant`, `test-flair-trash-purge-views`, `test-flair-trash-purge-views-mutant`, `test-flair-trash-window`, `test-flair-trash-window-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dos_folder.sh`, `harness/diff/fat_diff/fixtures/flagship_readme.txt`, `harness/diff/fat_diff/trash_purge_views.sh`, `harness/proptest/test_finder_menu.c`, `harness/proptest/test_finder_windows.c`, `harness/render/`, `harness/render//`, `harness/render/icon_capture.h`, `harness/verify_fixture_volume.sh`, `spec/assets/`, `spec/assets/color_canon.h`, `spec/assets/finder_icons.h`, `spec/chrome_metrics.h`, `spec/dos_folder_commands.txt`, `spec/dos_folder_externals.txt`, `spec/flair_disk_windows_traces.mk`, `spec/flair_file_ops_traces.mk`, `spec/flair_finder_follow_traces.mk`, `spec/flair_scroll_traces.mk`, `spec/flair_tenants_demo.h`, `spec/flair_trash_traces.mk`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/icon_capture.h`, `spec/assets/color_canon.h`.
Canonical `make -n` compile/link (`build/ppm_flair_disk_windows_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -Iharness/render -DICON_CAPTURES_DIR='"/home/tobiasosborne/Projects/system7-decomp/goldens/captures"' -o build/ppm_flair_disk_windows_check tools/ppm_flair_disk_windows_check.c -lpng
```

### tools/ppm_flair_egg_jtc_check.c — UNIT

`tools/ppm_flair_egg_jtc_check.c:1`; Factory C, initech-2ter (a). Pixel-exact crop vs a SELF-MINTED authored. structural/behavioral regression against authored geometry or implementation captures; no independent period/reference differential established for this checker.
Build target(s): `build/ppm_flair_egg_jtc_check`.
Run/assert gate(s): `test-flair-egg-jtc`, `test-ovl-behaviour-mutant`, `test-ovl-lazy`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/easter/jtc_alert.png`, `fixtures/easter/jtc_mat.png`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/ppm_flair_egg_jtc_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/ppm_flair_egg_jtc_check tools/ppm_flair_egg_jtc_check.c -lpng
```

### tools/ppm_flair_file_copy_check.c — ORACLE-STRONG

`tools/ppm_flair_file_copy_check.c:1`; - the FILE COPY emulator screendump. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_file_copy_check`.
Run/assert gate(s): `test-flair-file-copy`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/render/progress_golden.h`, `spec/assets/chicago12.h`, `spec/progress_platinum.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/progress_golden.h`, `spec/assets/chicago12.h`.
Canonical `make -n` compile/link (`build/ppm_flair_file_copy_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/render -Ispec -Ispec/assets -o build/ppm_flair_file_copy_check tools/ppm_flair_file_copy_check.c -lpng
```

### tools/ppm_flair_finder_icons_check.c — ORACLE-STRONG

`tools/ppm_flair_finder_icons_check.c:1`; - the booted-image Finder icon grader. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_finder_icons_check`.
Run/assert gate(s): `test-flair-finder-icons`, `test-flair-finder-icons-mutant`, `test-flair-trash-edges`, `test-flair-trash-full`, `test-flair-trash-full-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/render/icon_capture.h`, `spec/flair_finder_icons_traces.mk`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/icon_capture.h`.
Canonical `make -n` compile/link (`build/ppm_flair_finder_icons_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness/render -o build/ppm_flair_finder_icons_check tools/ppm_flair_finder_icons_check.c -lpng
```

### tools/ppm_flair_menubar_clock_check.c — ORACLE-STRONG

`tools/ppm_flair_menubar_clock_check.c:1`; tdnl.13 emu pixel gate: independent capture + original Chicago NFNT.. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_menubar_clock_check`.
Run/assert gate(s): `test-flair-menubar-clock`, `test-flair-menubar-clock-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/menubar_clock_oracle.c`, `harness/menubar_clock_oracle.h`.
Canonical `make -n` compile/link (`build/ppm_flair_menubar_clock_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness -o build/ppm_flair_menubar_clock_check tools/ppm_flair_menubar_clock_check.c harness/menubar_clock_oracle.c -lpng
```

### tools/ppm_flair_power_notice_check.c — ORACLE-STRONG

`tools/ppm_flair_power_notice_check.c:1`; initech-0xs6: refused-power notice. Reuse the existing independently. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_power_notice_check`.
Run/assert gate(s): `test-flair-calcda-power-mutant`, `test-flair-power-notice`, `test-flair-power-notice-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/calcda_power_check.sh`, `harness/emu/power_notice_check.sh`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/chicago12_nfnt_golden.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/render/alert_capture.h`, `harness/render/icon_capture.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/chicago12_nfnt_golden.h`, `tools/ppm_flair_trash_check.c`.
Canonical `make -n` compile/link (`build/ppm_flair_power_notice_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -DICON_CAPTURES_DIR='"/home/tobiasosborne/Projects/system7-decomp/goldens/captures"' -o build/ppm_flair_power_notice_check tools/ppm_flair_power_notice_check.c -lpng
```

### tools/ppm_flair_shutdown_check.c — UNIT

`tools/ppm_flair_shutdown_check.c:1`; the SHUT-DOWN SCREEN screendump oracle. structural/behavioral regression against authored geometry or implementation captures; no independent period/reference differential established for this checker.
Build target(s): `build/ppm_flair_shutdown_check`.
Run/assert gate(s): `test-flair-calcda-power`, `test-flair-shutdown`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/emu/calcda_power_check.sh`, `harness/render/alert_capture.h`, `spec/assets/`, `spec/assets/color_canon.h`, `spec/flair_power_traces.mk`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `spec/assets/color_canon.h`.
Canonical `make -n` compile/link (`build/ppm_flair_shutdown_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -o build/ppm_flair_shutdown_check tools/ppm_flair_shutdown_check.c
```

### tools/ppm_flair_trash_check.c — ORACLE-STRONG

`tools/ppm_flair_trash_check.c:1`; the EMPTY TRASH screendump STRUCTURE oracle. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_power_notice_check`, `build/test_finder_icon_oracles_mut_unselected`, `build/ppm_flair_trash_check`, `build/ppm_flair_trash_check_mut_locked_unselected`, `build/test_finder_icon_oracles`.
Run/assert gate(s): `test-finder-icon-oracles`, `test-finder-icon-oracles-mutant`, `test-flair-alert-chrome`, `test-flair-calc-quit`, `test-flair-calcda-power-mutant`, `test-flair-dialog-pressed-golden`, `test-flair-dialog-tracking`, `test-flair-empty-trash`, `test-flair-exit-to-dos`, `test-flair-finder-disk-alert-mutant`, `test-flair-finder-diskfull`, `test-flair-finder-launch-alert-mutant`, `test-flair-finder-launch-alerts`, `test-flair-finder-limits`, `test-flair-finder-move-alert-mutant`, `test-flair-finder-refused`, `test-flair-finder-replace`, `test-flair-i123-quitguard`, `test-flair-icon-oracles-mutant`, `test-flair-itext-badname`, `test-flair-itext-badname-mutant`, `test-flair-itext-full`, `test-flair-itext-full-mutant`, `test-flair-locked-purge`, `test-flair-locked-trash`, `test-flair-locked-trash-mutant`, `test-flair-power-notice`, `test-flair-power-notice-mutant`, `test-flair-trash-edges`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fixtures/flagship_ledger.txt`, `harness/diff/fat_diff/fixtures/flagship_readme.txt`, `harness/diff/fat_diff/locked_trash.sh`, `harness/emu/calcda_power_check.sh`, `harness/emu/power_notice_check.sh`, `harness/render/alert_capture.h`, `harness/render/icon_capture.h`, `spec/assets/`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/chicago12_nfnt_golden.h`, `spec/dos_folder_apps.txt`, `spec/flair_trash_traces.mk`, `spec/toolbox_gate.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults), `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_finder_icon_oracles.c`, `harness/render/alert_capture.h`, `harness/render/icon_capture.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/chicago12_nfnt_golden.h`, `tools/ppm_flair_power_notice_check.c`.
Canonical `make -n` compile/link (`build/ppm_flair_power_notice_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -DICON_CAPTURES_DIR='"/home/tobiasosborne/Projects/system7-decomp/goldens/captures"' -o build/ppm_flair_power_notice_check tools/ppm_flair_power_notice_check.c -lpng
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-flair-desktop.md](gates-flair-desktop.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**164 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 176 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-app-launch

#  recipe to execute (from 'Makefile', line 23302):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_data_tfx_badmbar.img`, `build/flair_data_tfx_crash.img`, `build/flair_data_tfx_noreg.img`, `build/flair_tenants.img`.
Input trace `APPL_TRACE` at make variable (generated/included):
```text
grep -E '^(TENANT-|FLAIR-DISPATCH app=)' $(1) > $(1).trace && grep -v '^#' spec/flair_app_launch_trace.golden > $(1).golden && cmp -s $(1).trace $(1).golden
```
Input trace `FLAIR_APP_LAUNCH_CLOSE_SPEC` at `spec/flair_app_launch_traces.mk:109`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m35:48,l1,l0,kx,m-35:-48,l1,l0,l1,l0
```
Input trace `FLAIR_APP_LAUNCH_CYCLE_SPEC` at `spec/flair_app_launch_traces.mk:93`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m65:88,l1,l0,m-65:-88,l1,l0,l1,l0,m65:88,l1,l0
```
Input trace `FLAIR_APP_LAUNCH_DOUBLE_SPEC` at `spec/flair_app_launch_traces.mk:86`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m4:0,m-4:0,l1,l0,l1,l0,l1,l0,kesc,m65:99,m0:89,l1,l0,l1,l0
```
Input trace `FLAIR_APP_LAUNCH_PRE_SPEC` at `spec/flair_app_launch_traces.mk:127`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,m110:113,m110:113,m110:112,m110:0,m105:0
```
Input trace `FLAIR_APP_LAUNCH_SHOW_SPEC` at `spec/flair_app_launch_traces.mk:117`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m110:113,m110:113,m110:112,m110:0,m105:0
```
Input trace `FLAIR_APP_LAUNCH_SPEC` at `spec/flair_app_launch_traces.mk:58`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m65:88,l1,l0,m120:100,m120:100,m120:50,m120:0
```
Input trace `FLAIR_APP_MENUBAR_SPEC` at `spec/flair_app_launch_traces.mk:149`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,l0,l1,l0,m72:-92,l1,l0,m-7:90,m0:90,l1,l0
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call APPL_ALIVE,build/flair_appl_launch.serial) || { printf '!!! test-flair-app-launch [1/10] FAIL: the desktop did not survive the launch trace\n'; grep -E '^(PANIC|TENANT-|HALTED)' build/flair_appl_launch.serial; exit 1; }
@$(call APPL_TRACE,build/flair_appl_launch.serial) || { printf '!!! test-flair-app-launch [1/10] FAIL: the launch trace differs from the locked golden %s\n' 'spec/flair_app_launch_trace.golden'; diff -u build/flair_appl_launch.serial.golden build/flair_appl_launch.serial.trace; exit 1; }
@$(call APPL_ALIVE,build/flair_appl_show.serial) || { printf '!!! test-flair-app-launch [2/10] FAIL: the desktop did not survive\n'; exit 1; }
@grep -qx 'FLAIR-DISPATCH app=TENANTFX' build/flair_appl_show.serial || { printf '!!! test-flair-app-launch [2/10] FAIL: the tenant never became foreground\n'; exit 1; }
@build/ppm_flair_app_launch_check show build/flair_appl_show.ppm || { printf '!!! test-flair-app-launch [2/10] FAIL: the resident tenant window is not what the tenant drew\n'; exit 1; }
@build/ppm_flair_app_launch_check bar-tenant build/flair_appl_show.ppm || { printf '!!! test-flair-app-launch [2/10] FAIL: band 2 is not the OWN menu bar of the foreground tenant (SETMBAR, initech-cnpm)\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_appl_pre.serial) || { printf '!!! test-flair-app-launch [3/10] FAIL: the baseline boot did not survive\n'; exit 1; }
@if grep -q '^TENANT-' build/flair_appl_pre.serial; then printf '!!! test-flair-app-launch [3/10] FAIL: the baseline (single click) LAUNCHED something\n'; exit 1; fi
@cmp -s build/flair_appl_pre.ppm build/flair_appl_launch.ppm || { printf '!!! test-flair-app-launch [3/10] FAIL: the post-exit frame differs from the never-launched baseline (a tenant pixel, a stale foreground or a wrong band-2 bar survived the exit)\n'; cmp -l build/flair_appl_pre.ppm build/flair_appl_launch.ppm | wc -l; exit 1; }
@build/ppm_flair_app_launch_check bar-finder build/flair_appl_launch.ppm || { printf '!!! test-flair-app-launch [3/10] FAIL: after the exit band 2 is not the Finder bar (the previous foreground bar was not restored)\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_appl_double.serial) && $(call APPL_DOUBLE,build/flair_appl_double.serial) || { printf '!!! test-flair-app-launch [4/10] FAIL: DOUBLE_LAUNCH -- want exactly one REGISTER, one SLOT-BUSY, one clean exit\n'; grep -E '^(TENANT-(REGISTER|SLOT|EXIT|LOAD)|PANIC)' build/flair_appl_double.serial; exit 1; }
@$(call APPL_ALIVE,build/flair_appl_cycle.serial) && $(call APPL_CYCLE,build/flair_appl_cycle.serial) || { printf '!!! test-flair-app-launch [5/10] FAIL: EXIT_LEAK -- two cycles must exit cleanly with EQUAL heap avail\n'; grep -E '^(TENANT-(EXIT|HEAPAVAIL|LOAD)|PANIC)' build/flair_appl_cycle.serial; exit 1; }
```

### test-flair-file-ops

#  recipe to execute (from 'Makefile', line 21355):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_fileops_between_data.img`, `build/flair_fileops_hover_data.img`, `build/flair_fileops_into_data.img`, `build/flair_fileops_refuse_data.img`, `build/flair_fileops_trash_data.img`, `build/flair_tenants.img`.
Input trace `FLAIR_FILEOPS_BETWEEN_SPEC` at `spec/flair_file_ops_traces.mk:76`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m-48:20,l1,m-45:39,m0:39,m0:2,l0,m8:-100,m0:-32,l1,l0,m100:65,m100:65,m100:65,m100:65,m100:65,m82:65
```
Input trace `FLAIR_FILEOPS_HOVER_SPEC` at `spec/flair_file_ops_traces.mk:158`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m34:0,m34:0
```
Input trace `FLAIR_FILEOPS_INTO_SPEC` at `spec/flair_file_ops_traces.mk:48`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m34:0,m34:0,m0:4,l0,m100:71,m100:71,m100:71,m100:71,m97:70
```
Input trace `FLAIR_FILEOPS_REFUSE_SPEC` at `spec/flair_file_ops_traces.mk:115`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:-8,m100:-8,m100:-8,m100:-7,m100:-7,m45:0,m0:2,l0,kesc,m-100:9,m-100:9,m-100:9,m-100:9,m-77:0,l1,l0,l1,l0,m77:-12,l1,m100:100,m50:50,l0,m-100:-100,m-50:-70,l1,l0,m-77:32,l1,m100:62,m100:62,m100:62,m27:62,m0:2,l0,m100:54,m70:54
```
Input trace `FLAIR_FILEOPS_TRASH_SPEC` at `spec/flair_file_ops_traces.mk:141`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,kctrl-n,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,kctrl-n,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-100:-64,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m20:38
```
Trace file: `spec/flair_file_ops_traces.mk`.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@command -v mdir >/dev/null 2>&1 && command -v mtype >/dev/null 2>&1 || { printf '!!! test-flair-file-ops FAIL: mtools (mdir/mtype/mcopy) missing\n'; exit 1; }
@mdir -i build/flair_fileops_into_data.img ::/APPS | grep -q '^README   TXT ' || { printf '!!! test-flair-file-ops FAIL: mtools does not see README.TXT in ::/APPS\n'; exit 1; }
@mtype -i build/flair_fileops_into_data.img ::/APPS/README.TXT | cmp -s - harness/diff/fat_diff/fixtures/flagship_readme.txt || { printf '!!! test-flair-file-ops FAIL: ::/APPS/README.TXT bytes differ from the fixture\n'; exit 1; }
@! mdir -i build/flair_fileops_into_data.img :: | grep -q '^README   TXT ' || { printf '!!! test-flair-file-ops FAIL: README.TXT is STILL in the root after the move\n'; exit 1; }
@[ -s "build/flair_fileops_hover.ppm" ] || { printf '!!! test-flair-file-ops FAIL: HOVER screendump missing\n'; exit 1; }
@build/ppm_flair_drop_target_check "build/flair_fileops_hover.ppm" || { printf '!!! test-flair-file-ops FAIL: the mid-drag frame does not show the highlighted target + gray outline\n'; exit 1; }
@mcopy -n -i build/flair_fileops_between_data.img ::/TENANTFX.EXE - 2>/dev/null | cmp -s - build/tenantfx.exe || { printf '!!! test-flair-file-ops FAIL: mtools does not read ::/TENANTFX.EXE back byte-identical to the shipped build\n'; exit 1; }
@! mdir -i build/flair_fileops_between_data.img ::/APPS | grep -q '^TENANTFX EXE' || { printf '!!! test-flair-file-ops FAIL: TENANTFX.EXE is STILL in ::/APPS\n'; exit 1; }
@grep -Eq '^FLAIR-DRAG win [0-9]+ \(40,80\)->\(190,230\)$$' "build/flair_fileops_refuse.serial" || { printf '!!! test-flair-file-ops FAIL: the APPS window was not dragged to (190,230), so the root APPS icon was not uncovered\n'; grep -E '^(FLAIR-DRAG|FINDER)' "build/flair_fileops_refuse.serial"; exit 1; }
@! grep -qE '^(FINDER-MOVE |FINDER-TRASH |DESKTOP-DB-SAVE)' "build/flair_fileops_refuse.serial" || { printf '!!! test-flair-file-ops FAIL: a refused drop still committed something\n'; grep -E '^(FINDER|DESKTOP)' "build/flair_fileops_refuse.serial"; exit 1; }
for n in '^README   TXT ' '^APPS .*<DIR>' '^DESKTOP  DB ' '^TRASH .*<DIR>'; do grep -qE "$$n" build/flair_fileops_refuse.root.lst || { printf '!!! test-flair-file-ops FAIL: after the refusals the root lost %s\n' "$$n"; cat build/flair_fileops_refuse.root.lst; exit 1; }; done
@mdir -i build/flair_fileops_refuse_data.img ::/APPS | grep -q '^TENANTFX EXE' && ! mdir -i build/flair_fileops_refuse_data.img ::/APPS | grep -q '^APPS ' || { printf '!!! test-flair-file-ops FAIL: ::/APPS changed on the refused CYCLE drop\n'; exit 1; }
```

### test-flair-finder-alerts

#  recipe to execute (from 'harness/finder_alerts.mk', line 319):

Boot/data images named by direct prerequisites or expanded recipe: passed by recursive helper; see full recipe in gate inventory.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

### test-flair-trash-edges

#  recipe to execute (from 'harness/finder_trash_edges.mk', line 114):

Boot/data images named by direct prerequisites or expanded recipe: `build/finder_full_fixture.img`, `build/flair_data.img`, `build/flair_tenants_mut_hold.img`, `build/trash_edge_full_delete_data.img`.
Input trace `FLAIR_TRASH_EDGE_FULL_ALERT_SPEC` at `spec/flair_trash_edges_traces.mk:38`:
```text
kret,m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m20:38
```
Input trace `FLAIR_TRASH_EDGE_FULL_DELETE_SPEC` at `spec/flair_trash_edges_traces.mk:47`:
```text
kret,m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,kret,l1,l0,l1,l0,m20:38
```
Input trace `FLAIR_TRASH_EDGE_NOTRASH_OPEN_SPEC` at `spec/flair_trash_edges_traces.mk:56`:
```text
kret,m100:61,m100:61,m80:60,l1,l0,l1,l0,m20:38
```
Input trace `FLAIR_TRASH_EDGE_PUTAWAY_SPEC` at `spec/flair_trash_edges_traces.mk:74`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:8,m-100:8,m-100:8,m-100:7,m-100:7,m-45:0,l1,m100:64,m100:64,m100:64,m100:64,m100:62,m45:0,m0:2,l0,m-100:-80,m-100:-80,m-100:-80,m-41:-80,l1,m100:80,m100:80,m100:80,m41:78,m0:2,l0,l1,l0,l1,l0,kctrl-n,kctrl-a,kctrl-y,kctrl-w,kctrl-n,m20:38
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@command -v mdir >/dev/null 2>&1 && command -v mtype >/dev/null 2>&1 && command -v fsck.fat >/dev/null 2>&1 || { printf '!!! test-flair-trash-edges FAIL: mtools / fsck.fat (dosfstools) missing\n'; exit 1; }
@$(call APPL_ALIVE,build/trash_edge_full_alert.serial) || { printf '!!! test-flair-trash-edges [1] FAIL: the guest did not finish alive\n'; exit 1; }
@$(call TE_FULL_ALERT,trash_edge_full_alert) || { grep -E '^(TRASH|FINDER|DESK)' build/trash_edge_full_alert.serial; printf '!!! test-flair-trash-edges [1] FAIL: the full-volume drop did not put up the Delete Now confirm (or something was removed before OK)\n'; exit 1; }
@build/ppm_flair_trash_check chrome build/trash_edge_full_alert.ppm || { printf '!!! test-flair-trash-edges [1] FAIL: the Delete Now confirm is not the two-button alert chrome on screen\n'; exit 1; }
@build/ppm_flair_finder_icons_check "../system7-decomp/goldens/captures" build/trash_edge_full_alert.ppm CAUTION:UNSEL@156,100/E7E7E7 || { printf '!!! test-flair-trash-edges [1] FAIL: the Delete Now confirm does not carry the captured CAUTION icon\n'; exit 1; }
@$(call APPL_ALIVE,build/trash_edge_full_delete.serial) || { printf '!!! test-flair-trash-edges [2] FAIL: the guest did not finish alive\n'; exit 1; }
@$(call TE_FULL_DELETE,trash_edge_full_delete) || { grep -E '^(TRASH|FINDER|DESK)' build/trash_edge_full_delete.serial; fsck.fat -n build/trash_edge_full_delete_data.img | tail -3; printf '!!! test-flair-trash-edges [2] FAIL: OK did not remove README.TXT in place, reserve \\TRASH and open it (mtools/fsck)\n'; exit 1; }
@printf '>>> test-flair-trash-edges [2/4]: OK -> README.TXT removed in place, \\TRASH made in the freed cluster and opened; mtools + fsck.fat clean\n'
@$(call APPL_ALIVE,build/trash_edge_notrash.serial) || { printf '!!! test-flair-trash-edges [3] FAIL: the guest did not finish alive\n'; exit 1; }
@$(call TE_NOTRASH,trash_edge_notrash) || { grep -E '^(TRASH|FINDER)' build/trash_edge_notrash.serial; printf '!!! test-flair-trash-edges [3] FAIL: opening the unavailable Trash did not name the Trash\n'; exit 1; }
@build/ppm_flair_trash_check notice build/trash_edge_notrash.ppm || { printf '!!! test-flair-trash-edges [3] FAIL: the notice is not on screen\n'; exit 1; }
@$(call APPL_ALIVE,build/trash_edge_putaway.serial) || { printf '!!! test-flair-trash-edges [4] FAIL: the guest did not finish alive\n'; exit 1; }
```

### test-flair-power-notice

#  recipe to execute (from 'harness/emu/calcda_power.mk', line 87):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_calcda_gate.img`, `build/power_text_cancel_data.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qxF 'FINDER-POWER-CANCEL app=TEXT.EXE declined' build/power_text_notice.serial
@build/ppm_flair_power_notice_check build/power_text_notice.ppm
@sh harness/emu/power_notice_check.sh build/power_text_cancel.serial
@fsck.fat -n build/power_text_cancel_data.img > build/power_text_cancel.fsck
```

The remaining legacy emulator entries cover Finder icon/list/window/drag/menu gestures, trash and file operations, keyboard focus, film scene/pixel fidelity, power/Exit-to-DOS and Bochs parity. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/apps/ref_demo_menu.h:1`|1|1|`os/apps/ref_demo_menu.h:7` `FINDER_MUT_DEMO_LIVE`|
|`os/apps/ref_tenant.c:1`|0|0|none|
|`os/apps/ref_tenant.h:1`|0|0|none|
|`os/flair/apple_menu.c:1`|1|1|`os/flair/apple_menu.c:32` `APPLE_MENU_MUT_LIST_ALL`|
|`os/flair/apple_menu.h:1`|0|0|none|
|`os/flair/desktop.c:1`|5|3|`os/flair/desktop.c:238` `DESKTOP_MUTATE_NO_PAINTALL_CLEAR`; `os/flair/desktop.c:264` `DRAG_MUTATE_SKIP_EXPOSED`; `os/flair/desktop.c:268` `DRAG_MUTATE_NO_CLIP`; `os/flair/desktop.c:304` `DRAG_MUTATE_NO_CLIP`; `os/flair/desktop.c:339` `DESKTOP_MUTATE_NO_PAINTALL_CLEAR`|
|`os/flair/desktop.h:1`|0|0|none|
|`os/flair/egg_jtc.c:1`|4|4|`os/flair/egg_jtc.c:78` `EGG_MUT_OVL_ONE_LABEL`, `MUTANT`; `os/flair/egg_jtc.c:85` `EGG_MUT_OVL_LEAK`; `os/flair/egg_jtc.c:95` `EGG_MUT_OVL_LEAK`; `os/flair/egg_jtc.c:98` `EGG_MUT_OVL_UD2`|
|`os/flair/egg_jtc.h:1`|0|0|none|
|`os/flair/egg_jtc_model.c:1`|3|3|`os/flair/egg_jtc_model.c:12` `EGG_MUT_CELL_WALK`; `os/flair/egg_jtc_model.c:24` `EGG_MUT_PAD_MISSING`; `os/flair/egg_jtc_model.c:58` `EGG_MUT_TRIGGER_OFF_BY_ONE`|
|`os/flair/egg_jtc_model.h:1`|0|0|none|
|`os/flair/film_scene.h:1`|0|0|none|
|`os/flair/film_startup.h:1`|1|1|`os/flair/film_startup.h:12` `PANIC_MUTATE_STARTUP_OOM`|
|`os/flair/finder_cmd.c:1`|6|5|`os/flair/finder_cmd.c:110` `FINDER_CMD_MUT_TABLE_BYPASS`; `os/flair/finder_cmd.c:181` `FINDER_CMD_MUT_NEWFOLDER_IN_TRASH`; `os/flair/finder_cmd.c:341` `FINDER_CMD_MUT_TABLE_BYPASS`; `os/flair/finder_cmd.c:359` `FINDER_CMD_MUT_SILENT_UNKNOWN`; `os/flair/finder_cmd.c:380` `FINDER_CMD_MUT_PRED_STUCK_ENABLED`; `os/flair/finder_cmd.c:399` `FINDER_CMD_MUT_EXEC_SWALLOWS`|
|`os/flair/finder_cmd.h:1`|0|0|none|
|`os/flair/finder_desktop.c:1`|5|5|`os/flair/finder_desktop.c:151` `FINDER_DESK_MUT_TRASH_NEVER_FULL`; `os/flair/finder_desktop.c:384` `FINDER_DESK_MUT_MARQUEE_OFFBYONE`; `os/flair/finder_desktop.c:438` `FINDER_DESK_MUT_DBLTICK_OFF`; `os/flair/finder_desktop.c:887` `FINDER_DESK_MUT_NO_HILITE`; `os/flair/finder_desktop.c:927` `FINDER_DESK_MUT_NO_UNDERLAY`|
|`os/flair/finder_desktop.h:1`|0|0|none|
|`os/flair/finder_icon.c:1`|4|3|`os/flair/finder_icon.c:67` `DESK_ICON_MUT_ROW_OFF1`; `os/flair/finder_icon.c:75` `DESK_ICON_MUT_MASK_IGNORED`; `os/flair/finder_icon.c:118` `DESK_ICON_MUT_NO_SELECT`; `os/flair/finder_icon.c:125` `DESK_ICON_MUT_NO_SELECT`|
|`os/flair/finder_icon.h:1`|0|0|none|
|`os/flair/finder_menu.c:1`|5|4|`os/flair/finder_menu.c:130` `FINDER_MENU_MUT_CMDCHAR_DUP`; `os/flair/finder_menu.c:200` `FINDER_MENU_MUT_DEAD_ITEM`; `os/flair/finder_menu.c:289` `FINDER_MENU_MUT_DEAD_ITEM`; `os/flair/finder_menu.c:296` `FINDER_MENU_MUT_STUB_ENABLED`; `os/flair/finder_menu.c:339` `FINDER_MENU_MUT_ENABLE_STUCK`|
|`os/flair/finder_menu.h:1`|0|0|none|
|`os/flair/finder_ops.c:1`|0|0|none|
|`os/flair/finder_ops.h:1`|0|0|none|
|`os/flair/finder_ops_cold.c:1`|13|11|`os/flair/finder_ops_cold.c:101` `FINDER_OPS_MUT_REMOVE_NO_META`; `os/flair/finder_ops_cold.c:121` `FINDER_OPS_MUT_COUNT_CELL`; `os/flair/finder_ops_cold.c:129` `FINDER_OPS_MUT_COUNT_CELL`; `os/flair/finder_ops_cold.c:138` `FINDER_OPS_MUT_NO_ORIGIN`; `os/flair/finder_ops_cold.c:152` `FINDER_MUT_ORIGINAL_LOST`; `os/flair/finder_ops_cold.c:206` `FINDER_OPS_MUT_NO_SERVICE_GUARD`; `os/flair/finder_ops_cold.c:241` `FINDER_OPS_MUT_NO_SERVICE_GUARD`; `os/flair/finder_ops_cold.c:258` `FINDER_OPS_MUT_NO_CYCLE`; `os/flair/finder_ops_cold.c:275` `FINDER_OPS_MUT_LOCKED_STAGE`; `os/flair/finder_ops_cold.c:298` `FINDER_OPS_MUT_TRASH_NO_STAGE`; `os/flair/finder_ops_cold.c:351` `FINDER_OPS_MUT_DROP_NO_META`; `os/flair/finder_ops_cold.c:386` `FINDER_MUT_REPLACE_CANCEL`; `os/flair/finder_ops_cold.c:465` `FINDER_MUT_PUTAWAY_REPOPULATE`|
|`os/flair/finder_ops_private.h:1`|0|0|none|
|`os/flair/finder_windows.c:1`|26|25|`os/flair/finder_windows.c:97` `FINDER_VIEW_MUT_SORT_UNSTABLE`; `os/flair/finder_windows.c:202` `FINDER_WIN_MUT_VOLLABEL_SHOWN`; `os/flair/finder_windows.c:291` `FINDER_VIEW_MUT_SIZE_COLUMN_OFFBYONE`; `os/flair/finder_windows.c:307` `FINDER_VIEW_MUT_HEADER_IGNORED`; `os/flair/finder_windows.c:452` `FINDER_VIEW_MUT_SMALL_ICON_SHIFT`; `os/flair/finder_windows.c:479` `FINDER_VIEW_MUT_HEADER_FACE`; `os/flair/finder_windows.c:632` `FINDER_WIN_MUT_SINGLETON_DUP`; `os/flair/finder_windows.c:709` `FINDER_WIN_MUT_DOTS_SHOWN`; `os/flair/finder_windows.c:724` `FINDER_WIN_MUT_SERVICE_SHOWN`; `os/flair/finder_windows.c:766` `FINDER_WIN_MUT_ABS_COORDS`; `os/flair/finder_windows.c:814` `FINDER_WIN_MUT_SCROLL_PAINT_ONLY`; `os/flair/finder_windows.c:924` `FINDER_MUT_ICON64`; `os/flair/finder_windows.c:991` `FINDER_MUT_WINDOW4`; `os/flair/finder_windows.c:1088` `FINDER_WIN_MUT_TRASH_ROOT`; `os/flair/finder_windows.c:1141` `FINDER_WIN_MUT_VIEW_UNCHECKED`; `os/flair/finder_windows.c:1219` `FINDER_WIN_MUT_CLEANUP_UNSORTED`; `os/flair/finder_windows.c:1301` `FINDER_WIN_MUT_SCROLL_PAINT_ONLY`; `os/flair/finder_windows.c:1422` `FINDER_VIEW_MUT_SAVE_ICONS`; `os/flair/finder_windows.c:1587` `FINDER_MUT_OPEN_ONE`; `os/flair/finder_windows.c:1591` `FINDER_MUT_OPEN_NO_APP`; `os/flair/finder_windows.c:1638` `FINDER_WIN_MUT_ARRANGE_NOOP`; `os/flair/finder_windows.c:1685` `FINDER_WIN_MUT_EMPTY_NO_CONFIRM`; `os/flair/finder_windows.c:1714` `FINDER_WIN_MUT_POWER_NO_SAVE`; `os/flair/finder_windows.c:1752` `FINDER_WIN_MUT_EXIT_DOS_UNWIRED`; `os/flair/finder_windows.c:1822` `FINDER_MUT_OPEN_APP_DARK`; `os/flair/finder_windows.c:1899` `FINDER_MUT_NO_KEYS`|
|`os/flair/finder_windows.h:1`|0|0|none|
|`os/flair/finder_windows_cold.c:1`|9|8|`os/flair/finder_windows_cold.c:89` `FINDER_WIN_MUT_PURGE_KEEPS_VIEWS`; `os/flair/finder_windows_cold.c:104` `FINDER_WIN_MUT_LOCKED_PURGE`; `os/flair/finder_windows_cold.c:241` `FINDER_WIN_MUT_DELETE_NO_WALK`; `os/flair/finder_windows_cold.c:248` `FINDER_WIN_MUT_DELETE_KEEP_VIEW`; `os/flair/finder_windows_cold.c:351` `FINDER_MUT_NO_MOVE_ALERT`; `os/flair/finder_windows_cold.c:357` `FINDER_MUT_NO_DISK_ALERT`; `os/flair/finder_windows_cold.c:371` `FINDER_MUT_NO_LAUNCH_ALERT`; `os/flair/finder_windows_cold.c:383` `FINDER_MUT_NO_MOVE_ALERT`; `os/flair/finder_windows_cold.c:403` `FINDER_MUT_NOTRASH_GENERIC`|
|`os/flair/finder_windows_private.h:1`|0|0|none|
|`os/flair/flair_look.c:1`|2|2|`os/flair/flair_look.c:84` `FLAIR_PART_MAP_MUT_DERIVED`; `os/flair/flair_look.c:257` `FLAIR_MUT_SKIN_WRONG_ERA`|
|`os/flair/flair_look.h:1`|0|0|none|
|`os/flair/power_notice.h:1`|1|1|`os/flair/power_notice.h:10` `POWER_MUT_SILENT_CANCEL`|
|`os/flair/shell.c:1`|7|5|`os/flair/shell.c:127` `SHELL_MUT_SHORT_SURFACE`; `os/flair/shell.c:159` `SHELL_MUT_MENU_COUNT`; `os/flair/shell.c:339` `SHELL_MUT_SHORT_SURFACE`; `os/flair/shell.c:359` `SHELL_MUTATE_MODAL_BEHIND`; `os/flair/shell.c:387` `SHELL_MUTATE_ONE_MENUBAR`; `os/flair/shell.c:400` `SHELL_MUTATE_MODAL_BEHIND`; `os/flair/shell.c:405` `SHELL_MUTATE_NO_MODAL`|
|`os/flair/shell.h:1`|0|0|none|
|`os/flair/shell_menus.h:1`|1|1|`os/flair/shell_menus.h:38` `SHELL_MENUS_MUT_ABOUT_LIVE`|
|`os/milton/about_box.c:1`|0|0|none|
|`os/milton/about_box.h:1`|0|0|none|
|`os/milton/desktop_db.c:1`|1|1|`os/milton/desktop_db.c:73` `DB_MUT_PHANTOM`|
|`os/milton/desktop_db.h:1`|0|0|none|
|`os/milton/finder_actions.c:1`|4|4|`os/milton/finder_actions.c:20` `KMAIN_MUT_MKDIR_CUT`; `os/milton/finder_actions.c:284` `KMAIN_MUT_NO_DELETE_NOW`; `os/milton/finder_actions.c:413` `KMAIN_MUT_LOCKED_NOTICE`; `os/milton/finder_actions.c:758` `KMAIN_MUT_OPEN_NO_LAUNCH`|
|`os/milton/finder_actions.h:1`|0|0|none|
|`os/milton/kmain_cold.c:1`|1|1|`os/milton/kmain_cold.c:28` `KMAIN_ABOUT_MUT_NONE`|
|`os/milton/kmain_cold.h:1`|0|0|none|
|`os/milton/power.h:1`|1|1|`os/milton/power.h:48` `POWER_MUT_NO_PULSE`|
|`os/milton/power_cold.c:1`|6|6|`os/milton/power_cold.c:194` `KMAIN_MUT_SHUTDOWN_RETURNS`; `os/milton/power_cold.c:247` `KMAIN_MUT_POWER_NO_ASK`; `os/milton/power_cold.c:291` `KMAIN_MUT_POWER_NO_FLUSH`; `os/milton/power_cold.c:344` `KMAIN_MUT_COMMAND_MESSAGE`; `os/milton/power_cold.c:385` `KMAIN_MUT_COMMAND_MAGIC`; `os/milton/power_cold.c:432` `KMAIN_MUT_EXIT_NO_TEARDOWN`|
|`os/milton/power_cold.h:1`|0|0|none|
kmain.c mutation sites are counted under kernel because the physical file is counted once. Desktop-owned ranges above contain most GUI KMAIN_MUT_* / FLAIR_LIVE_MUTATE_* branches; move those with the extracted functions before turning them into patches.

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `os/apps/ref_demo_menu.h:5` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `os/apps/ref_tenant.c:77` → `os/flair/process.h`: literal cross-component include; evaluate in its #if context.
- `os/apps/ref_tenant.c:78` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/apps/ref_tenant.c:81` → `os/flair/blitter.h`: literal cross-component include; evaluate in its #if context.
- `os/apps/ref_tenant.c:82` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/apps/ref_tenant.c:83` → `os/flair/heap.h`: literal cross-component include; evaluate in its #if context.
- `os/apps/ref_tenant.h:29` → `os/flair/process.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/apple_menu.h:19` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/desktop.c:68` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/desktop.c:69` → `os/flair/blitter.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/desktop.c:70` → `os/flair/chrome.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/desktop.c:71` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/desktop.h:83` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/desktop.h:84` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/egg_jtc.c:16` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/egg_jtc.c:17` → `os/flair/blitter.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/egg_jtc.c:19` → `os/flair/text.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/egg_jtc.h:4` → `os/flair/process.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/film_scene.h:7` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/film_scene.h:8` → `os/flair/cursor.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/film_startup.h:7` → `os/milton/panic_screen.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/film_startup.h:8` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.c:19` → `os/flair/event.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.c:33` → `os/flair/asset_lz.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.c:34` → `os/flair/asset_lz_data.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.c:62` → `os/flair/blitter.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.h:106` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.h:108` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_desktop.h:109` → `os/flair/process.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_icon.c:30` → `os/flair/asset_lz.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_icon.c:31` → `os/flair/asset_lz_data.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_icon.h:53` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_menu.h:95` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_windows.c:48` → `os/flair/blitter.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_windows.c:49` → `os/flair/heap.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_windows.h:135` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_windows.h:138` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/finder_windows.h:139` → `os/flair/process.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/flair_look.c:47` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.c:64` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.c:65` → `os/flair/dialog.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.c:66` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.c:67` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.h:87` → `os/flair/surface.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.h:89` → `os/flair/window.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.h:90` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.h:91` → `os/flair/dialog.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell.h:92` → `os/flair/control.h`: literal cross-component include; evaluate in its #if context.
- `os/flair/shell_menus.h:32` → `os/flair/menu.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/about_box.c:14` → `os/milton/int21.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/desktop_db.h:11` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.c:5` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.c:6` → `os/flair/tbxgate.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.c:8` → `os/flair/event.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.h:9` → `os/flair/dialog.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.h:11` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.h:12` → `os/flair/event.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power.h:43` → `os/milton/io.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.c:20` → `os/flair/tbxgate.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.c:23` → `os/milton/console.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.c:24` → `os/milton/int21.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.c:25` → `os/milton/kbd.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.c:26` → `os/milton/transient.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.h:10` → `os/milton/console.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.h:11` → `os/milton/psp.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/power_cold.h:12` → `os/milton/fat12.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/finder_actions.h:43` → `g_finder_* globals`: replace shared buffers/state globals with a desktop context/service table.
- `os/milton/kmain.c:3408` → `FAT12 directory enumeration`: Finder filesystem provider directly binds FAT internals; replace with named file service.
- `os/milton/kmain.c:5234` → `boot filesystem + overlay + transient resident imports`: separate platform loading from desktop event policy.
- `os/apps/ref_tenant.c:77` → `process/window/blitter/surface/heap structures`: HELLO/NOTES bypass INT 81h; convert to disk examples or retain explicitly as privileged fixtures.

### kmain.c split map (all 8,207 physical lines)

a = kernel; b = production desktop/Finder; c = compiled-in scenarios or test policy. Ranges are ownership guidance from source inspection, not preprocessor-executed LOC. Mutation branches are interleaved and enumerated in section 6.

|Lines|Class|Disposition / role|
|---|---|---|
|1–385|a+b|includes, shared declarations and adapters; separate by referenced subsystem; `os/milton/kmain.c:1`|
|386–995|a|DOS callbacks, process/EXEC/TSR/keyboard/idle/filesystem plumbing; CHKDSK cache adapter also serves dos; `os/milton/kmain.c:386`|
|996–1536|b|FLAIR frame, surface/palette and composed scene construction; `os/milton/kmain.c:996`|
|1537–3151|b|live cursor/input tracking, window/menu/modal/chrome and launch helpers; `os/milton/kmain.c:1537`|
|3152–5233|b|Finder bindings, filesystem provider, menus, alerts, Standard File, Apple menu/DA services; `os/milton/kmain.c:3152`|
|5234–5467|a+b+c|boot-volume provider, FLAIR.OVL loader and OVL_TEST* scenarios; loader service belongs kernel, desktop imports retire; `os/milton/kmain.c:5234`|
|5468–5745|b|Exit-to-DOS teardown, foreground activation, Finder keyboard/menu dispatch; `os/milton/kmain.c:5468`|
|5746–5936|c|TBX_HEADLESS_SMOKE_* compiled-in app/tenant driving and copy smoke keys; `os/milton/kmain.c:5746`|
|5937–6214|a|kernel entry, boot-info/placement validation, console/interrupt/sysinit/platform foundation; `os/milton/kmain.c:5937`|
|6215–6754|b+c|static/live desktop construction, HELLO/NOTES/Finder setup, input hooks; personality/mutation guards interleaved; `os/milton/kmain.c:6215`|
|6755–7268|b+c|production event pump plus tick-budget/record/interactive/test exits; ordinary image uses bounded oracle mode; `os/milton/kmain.c:6755`|
|7269–7407|c|fault/spurious and FPU_TEST/FPU_MF diagnostics; `os/milton/kmain.c:7269`|
|7408–7618|a+c|DOS filesystem setup and baked TYPE/DIR demo programs; keep mount service, remove baked demos; `os/milton/kmain.c:7408`|
|7619–8073|c|EXEC/nested/overlay/MZ/memory/write/absolute disk/multiopen/date/exit/sysinit test scenarios; `os/milton/kmain.c:7619`|
|8074–8207|a+c|shell REPL dispatch vs keyboard/IRQstorm/CONIN/VECT test personalities and terminal halt; `os/milton/kmain.c:8074`|

### Every BOOT_* personality

A program can currently execute privileged instructions because applications are flat ring 0. The recommendations below do not claim that interrupt/CR0 probes are safe ordinary DOS programs under future protection. “Special kernel” is needed only to observe failures before any loader/program can run; most current probes execute after startup.

|Selector|Condition sites|What it exercises|Replacement / special-kernel assessment|
|---|---|---|---|
|BOOT_ABSDISK|`os/milton/kmain.c:7897`|INT25/26 absolute disk guest|disk test program; yes|
|BOOT_CONIN|`os/milton/kmain.c:8140`|cooked/live console input|console test program; yes|
|BOOT_DATETIME|`os/milton/kmain.c:7936`|DOS date/time interrupt functions|disk test program; yes|
|BOOT_DOSMSG_FAULT|`os/milton/kmain.c:8084`|controlled DOS diagnostic on startup fault|test program/fault injection at service boundary; yes, with injectable device/provider|
|BOOT_EXEC|`os/milton/kmain.c:7639`|load and execute ordinary COM; register/return integrity|shell-run launcher and child programs; yes|
|BOOT_EXEC_NESTED|`os/milton/kmain.c:7619`|nested EXEC stack/PSP/handle restoration|parent and child programs on disk; yes|
|BOOT_EXEC_OVERLAY|`os/milton/kmain.c:7626`|DOS overlay/load service behavior|disk launcher + overlay fixture; yes|
|BOOT_EXITH|`os/milton/kmain.c:7951`|child exit reclaims handles|disk launcher/program; yes|
|BOOT_FLAIR_LIVE|`os/milton/kmain.c:32`, `os/milton/kmain.c:68`, `os/milton/kmain.c:830`, `os/milton/kmain.c:847`, `os/milton/kmain.c:876`, `os/milton/kmain.c:996`, `os/milton/kmain.c:1537`, `os/milton/kmain.c:3152`, `os/milton/kmain.c:6215`, `os/milton/kmain.c:6221`, `os/milton/kmain.c:6267`|live chrome/tenant desktop personality|production desktop launch mode; external traces replace bounded test builds; no special kernel; launch desktop normally|
|BOOT_FLAIR_SHELL|`os/milton/kmain.c:68`, `os/milton/kmain.c:996`, `os/milton/kmain.c:6215`, `os/milton/kmain.c:6250`|static composed desktop frame|desktop screenshot mode/external driver; no special kernel|
|BOOT_FPU_MF|`os/milton/kmain.c:7310`, `os/milton/kmain.c:7376`|native unmasked x87 exception #MF reaches panic|privileged diagnostic program; panic ends boot; yes under current ring-0 app model; isolate privileged test|
|BOOT_FPU_TEST|`os/milton/kmain.c:7310`|x87 initialization/control word/exact arithmetic|privileged diagnostic program; yes under ring-0 app model; absent-FPU variant remains emulator configuration|
|BOOT_IRQSTORM|`os/milton/kmain.c:8116`|timer IRQ continues while INT21 paths execute|disk program observing progress + external timer trace; yes; timing/EOI counters need observability service|
|BOOT_KBD_ECHO|`os/milton/kmain.c:8074`|keyboard interrupt and echo path|disk program reading CON; yes|
|BOOT_MEMTEST|`os/milton/kmain.c:7797`|MCB allocate/free/resize and expected failure|disk INT21 program; yes|
|BOOT_MULTIOPEN|`os/milton/kmain.c:7920`|independent open positions/handles|disk INT21 program; yes|
|BOOT_MZEXEC|`os/milton/kmain.c:7712`|MZ relocation and return behavior|MZ file and shell-run launcher; yes|
|BOOT_MZFOREIGN|`os/milton/kmain.c:7762`|reject foreign/invalid MZ|bad file and shell-run launcher; yes|
|BOOT_SECONDARY_SHELL|`os/milton/kmain.c:8107`|primary-slave shell/startup image choice|boot configuration or explicit mount/drive service; configuration; boot-device selection genuinely occurs before shell, no test-only kernel needed|
|BOOT_SELFTEST_FAULT|`os/milton/kmain.c:7269`|divide error panic / no triple fault|privileged faulting disk program; yes under current ring-0 model; truly early-init fault tests need diagnostic boot hook|
|BOOT_SHELL|`os/milton/kmain.c:830`, `os/milton/kmain.c:860`, `os/milton/kmain.c:7609`, `os/milton/kmain.c:8083`|COMMAND interpreter entry|production shell, not a test; no special kernel|
|BOOT_SPURIOUS|`os/milton/kmain.c:7286`|unhandled vector / IRQ7 / IRQ15 return & EOI discipline|privileged interrupt diagnostic; yes under ring-0 model; not ordinary unprivileged DOS semantics|
|BOOT_SYSINIT|`os/milton/kmain.c:8008`|CONFIG.SYS sizing, loading and device initialization|on-disk config + external boot checks; boot path must be exercised; no need to recompile kernel; early-init failure probes may require an injectable provider|
|BOOT_VECT|`os/milton/kmain.c:8157`|DOS interrupt vector services|disk INT21 program; yes|
|BOOT_WRITE|`os/milton/kmain.c:7884`|DOS create/write/flush/readback path|disk INT21 program; yes|

Non-BOOT scenario selectors also matter: `TBX_HEADLESS_SMOKE_*` around kmain:5746, `FLAIR_LIVE_TENANTS`, `FLAIR_LIVE_INTERACTIVE`, `FLAIR_LIVE_*TICKS`, `OVL_TEST_*`, recording selectors and manager mutant defines. They are not additional BOOT personalities. The default `image` is a bounded tenants oracle, while the interactive image is a separate link. Move replay inputs and completion logic outside the product.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Merge `_cold.c`/`_private.h` size splits with their owning implementation, retire FLAIR.OVL resident/import stub synthesis and COMMAND transient fingerprinting as their replacements become available. Keep actual DOS overlay semantics and independently useful loader validation. Remove the 384-sector/192-KiB policing, padding-only mutants, variant-wide byte ceilings and recording/bounded personality switches from production builds.
