# FLAIR Toolbox

The reusable graphics and application managers. The Finder and desktop shell
are built independently in desktop/ and linked by system/. No application or desktop policy is linked
into this library. Dependencies: flair/region (region/include/), sdk/include
(interface records), and, for the host-only real-DOS tests, kernel/include.
There is no target dependency on kernel headers.

Public hosted Dialog binding: make host-dialog builds host/libdialog.a under
build/flair, including its private rendering support. Clients use only
include/dialog.h, include/cp_dialog_items.h and the declared region/SDK records.
The flat-item adapter is the same one used by the target gate. Control Panels
links this archive for its real press/release tests; its app never reads FLAIR's
private renderer or compiles private manager sources.

File replacement follows GOLDEN-MASTER items 3/4/6 and the phase-2 operator
recovery ruling. FILE COPY creates an exclusive same-directory stage, transfers
and closes it, then compares its bytes against the source before replacement.
Progress calls never publish, including for a zero-byte source. The modal
explicitly commits at its completion tick, with no further cancellable input
turn before its result. The previous destination moves to the first absent
NAME.~00..~99 only during commit; a failed publication restores it. Physical
Escape, Control-period (the PC command shortcut), the Cancel button and the
close box reach one modal cancellation path. Mouse controls fire on release
inside; close and tenant teardown remove only an incomplete
stage. The copy engine refuses resolved source/destination identity and DOS
devices, and checks every transfer, verification and close result.
Cancellation and rate-file cleanup retry a failed CLOSE once and retain any
still-open handle until cleanup succeeds or the tenant session is reaped.
A pending close blocks a new copy; cleanup never deletes a stage still open.
A refused stage DELETE keeps its name owned for the next cleanup attempt.
The SDK retries cleanup once; persistent refusal names the exact temporary
file and prevents further saves from consuming additional temporary names.
Later calls retry that owned cleanup and can save once storage accepts it.
Early transfer/close errors and later commit refusals share this cleanup.

If publication and restore both fail, both complete streams remain and the
error notice names the exact previous-version path. This is not crash atomicity.
The current three-line alert can conservatively show 60 path characters, so a
longer recovery name refuses before staging; no recovery name is truncated.
The SDK save helper uses the same sequence and existing ALERT verb. Successful
replacement remains successful if only backup cleanup fails. Removing a
retained backup after that error still depends on the kernel unlink repair.

The desktop binds the data volume once. Every file-session dispatch temporarily
selects that volume and root CWDs for all mounted drives, then restores the
caller PSP, all per-drive CWDs, default drive and selected I/O drive. Explicit
foreign-drive paths use that drive's root. Resolved file identity is the triple
of volume, parent directory and directory slot; the internal platform record
now supplies these fields. The INT 81h numbers, arguments and records are
unchanged.

tests/test_filecopy_disk.c fault-injects each DOS operation and sector write
through the real Toolbox adapter, InitechDOS dispatcher and FAT12 backend.
mtools independently decodes the resulting destination bytes. It also checks
cancel positions, aliases, devices, temporary collisions, disk exhaustion,
readback detection of silent corruption and both failed-restore streams.
test_desktop_db_disk checks persisted positions and Trash origins with literal
IDB1 records, independent disk readback and fresh FAT-cache reloads.
test_tbx_volume_disk checks two real mounted disks and caller-state restoration.
test_copy_modal compiles the actual desktop copy loop with the Event Manager,
Dialog Manager, copy engine and real DOS/FAT. Its five Set-1/mouse cancellation
traces preserve literal old/source bytes as read by mtools. Presentation and
app-completion bindings alone are hosted. make smoke builds the product image
and a separate test tenant, then drives nine real INT 81h scenarios in QEMU:
all cancel controls, empty source, success and refused operations. It verifies
one delivered completion and independently reads the disk. The live Cancel
button/close box are authored GM3 policy required by the phase-2 operator;
the two-item film tableau retains its original geometry.
The live copy selects the canon hourglass before opening the dialog and
restores the prior cursor selection on success, cancel and early/late error.
The booted smoke reads P6 pixels independently against transcribed frozen
hourglass rows and compares the restored cursor area to its idle capture.
It waits for the painted midpoint and uses a slower valid configured rate
so a snapshot inside ShieldCursor cannot masquerade as a missing cursor.

The menu bar uses Initech's operator-approved three-block mark. Its 16 by 15
transparent strike is independent of the retained 33px menu hit slot. The
unchanged approved mask and provenance live in tests/goldens/marks; test_menu
compares the product data and every normal/pulled rendered pixel against it.

Managers:

Empty menu titles highlight while held and have no panel, shadow, selectable
row or below-bar damage. This is the authored Inside Macintosh IV-55 (128K
ROM) choice for Paint's seven structural headings. test_menu_empty checks
literal zero geometry, pixel conservation and streaming hold/drag/release;
tests/smoke-empty-menu.sh drives the real Paint graph and independently reads
all seven held/released images plus File-to-empty restoration.

- Surface: bounded pixel writes and depth packing.
- Blitter: region-clipped fills and copies.
- Text/fonts: Chicago and Geneva strikes, metrics, styled/clipped text runs.
- TextEdit: editable text, layout, selection, caret and scrap operations.
- Chrome: Platinum window and widget drawing.
- Window: geometry, z-order, visible regions, exposure and update damage.
  Moving a collapsed document translates its saved expanded frame, so expansion
  preserves the dragged location and full size. The pre-zoom user frame remains
  independent; literal geometry and owner-grid damage tests cover both states.
- Menu: proportional bars, pull-down layout, tracking and keyboard equivalents.
- Control: buttons, checkboxes, radios, scroll bars and progress bars.
- Dialog: item routing, modal handling and standard alert/file-copy furniture.
- Event: deterministic raw ring, cooked events, mouse tracking and ticks.
- List: cell layout and selection.
- Scrap: typed desktop scrap storage.
- Resource: resource-map reader.
- Heap: caller-backed, classed arena allocation and reclamation.
- Cursor: save-under, masking and shielded cursor drawing.
- Look: part/skin/palette resolution, including authored canon colors.
- Process: cooperative app lifecycle, activation, dispatch and teardown.
- Standard File: directory list and get/put file selection model.
- Toolbox gate: validated INT 81h verbs and resident disk-tenant lifecycle.

Public headers are include/*.h, with shared font/icon/color inputs in
include/assets/. The matching manager header is its API; window_record.h,
control_record.h, dialog_record.h, menu_record.h, imaging.h, grafport.h and
drawing_ops.h carry the shared types and constants. flair_platform.h is the
host binding, tbxgate.h the OS-side pump interface. tbxfile.h, tbxmenu.h,
cp_service.h, cp_dialog_items.h, tbx_layout.h, rdoc.h and ring_report.h remain
private implementation helpers.
The asset decoders are public because asset headers use them. Clients add the
public include directories and build/flair/generated for packed storage.

make builds 20 freestanding ELF32 i386 objects and build/flair/libflair.a with
plain TARGET_CC/TARGET_CFLAGS from mk/config.mk. It also invokes region/
make target to build build/flair/region/target/region.o; link that object
alongside libflair.a. The archive does not duplicate the region implementation. Generated fonts,
palette/canon, DAC and packed assets come from component-local inputs using
host C tools under build/flair/tools. No LTO, per-file size flags or executable
overlay packing remains. make test runs all host programs with one result line
each, logs under build/flair/host and isolated scratch under build/flair/runs.
Use -j32 for parallel compilation; the serial clean run is under a minute.
All seeded legacy sweeps run by default; THOROUGH=1 does not shorten any suite.
make test TESTS='test_window test_menu' selects registered entries for debugging.

The host tests use independent pixel/owner grids, text/layout models and
reference-derived Mac resources and screen crops. SYSTEM7_DECOMP and
WIN31_DECOMP default through mk/config.mk. Missing, changed or malformed corpus
inputs fail loudly. Twenty runtime reference inputs are SHA-256 pinned in
tests/REFERENCE-INPUTS.sha256; the five captured/transcribed golden headers cite
their sources. tests/IMPORT.md gives every program, counts and the honest
reference/self-derived split. In particular, DAC/skin consistency, compressed
storage and the pre-compaction part-map transcript are UNIT/REGRESSION, not
proof of historical fidelity. Authored logo art and teal substitutions remain
authored policy. No pixel or user-visible behavior is intentionally changed.

The platform binding supplies private file sessions, DOS-form file/clock calls,
DTA exchange, memory accounting, tenant loading and transfer, desktop pattern,
speaker, serial diagnostics and panic. Clock ticks and the master arena already
have explicit Event Manager / tbx_bind inputs. Console-to-GUI hand-off belongs
to the desktop; the Toolbox does not seize the console. NOTES-FOR-KERNEL.md maps
every binding to the legacy routine and states ordering/lifetime requirements.
Routing uses real tbxgate/process/window/event and the real blitter on a 32-bit
surface, with fail-loud platform stubs for services outside its scope. FILLRECT
clips in the wide local coordinate domain before adding an origin or narrowing;
an independent reader subtracts two literal origins from every framebuffer
pixel, including distant and extreme endpoints. The same fixture proves distinct
window tokens and consumed-once presentation requests. NEWWINDOW bounds every
endpoint before width/height subtraction; extreme refusals preserve record
storage, window count and the first valid token, including under UBSan.
File integration uses tests/platform_dos.c over the real existing InitechDOS
dispatcher. No fake success replaces those DOS checks.

The SDK owns all app-facing gate, event, geometry and Control Panels records.
GrafPort includes geometry.h rather than declaring Point again. The former
cp_dialog_items adapter is private to FLAIR (unchanged from legacy-final's
spec/cp_dialog.h). tbx_layout.h pins registration and event wire offsets and
compares every meaningful menu-manager field with the SDK records in i386
builds. Host routing uses fixed i386 wire offsets, never native host pointers.
The DOS find buffer is opaque storage sized from the SDK's DTA payload offset
and length; only the platform binding interprets its DOS resume state.

Host-only integration debt: tests/platform_dos.c and test_tbx_file.c use
only kernel public headers, and the
file suites compile existing kernel translation units directly. There
are no borrowed headers, source views or app GrafPort compatibility paths.
The target never sees kernel include paths. Move the host adapter into the
kernel's host binding when that binding is published. The diagnostic catalog
in tests/platform is a frozen legacy fixture, not product code or an oracle.

The target platform binding and INT 81h assembly are integrated in desktop/.
Disabled and pressed CDEF capture gaps remain explicit. tests/SMOKES.md indexes
250 legacy emulator gates and their traces/checkers; those individual checks
remain with the factory and desktop owners. The product smoke exercises the
current booted pump, rather than certifying every historical scenario.

make mutants runs 203 patches through the shared strict runner: passing baseline,
zero-fuzz application, successful compilation, and the same runtime failure twice.
Each patch selects a killing host program. tools/run-mutants.sh groups exact
selections and runs up to MUTANT_JOBS=16 groups concurrently with private scratch;
the measured sweep is 85 seconds on this machine. The shared runner itself is
serial; native parallel scheduling and baseline sharing across patch workers
would remove this local wrapper and the serial tail of large selection groups.
Twelve patches under mutants/pending/ still need their named legacy emulator gates.
The phase-2 strict sweep compiled and ran all sixteen original pending patches:
the two FILE COPY patches are now killed twice by test_filecopy_disk. The raster
routing fixture also kills drawing-not-presented and window-token-alias twice;
twelve remain pending. No compile failure
was counted as a kill. The copy patches were refreshed against staged copying,
with independent destination bytes and a literal configured-rate assertion.
None was deleted as equivalent. tests/MUTATION-RESULTS.tsv records all 207 macros,
their original 248 guard entries, test selection, disposition and recipe evidence.
This includes five hooks omitted from the original 202-macro/243-entry inventory:
the two clock faults, modal passthrough, close hiding and update visibility.
Shared scrollbar patches are counted once under control; surface/asset decoding
under blitter, look resolution under chrome, font storage under text, and the
menu gate helpers under menu. No product behavior changes.

Shared build conventions: command stamps rebuild on tool/flag changes; C
translation units use -MMD dependencies. TEST_BUDGET_S is 30 seconds for the
complete tier-0 selection; measured execution fits the default, excluding builds.
Standalone tests verify their consumed files in goldens/CORPORA.lock; the
top-level test run verifies the full lock once before parallel execution.
The graph-based import audit now checks all-reachable sources and headers.
The remaining DAC generator omission probe is an external patch, killed by
test_flair_dac (index 255 differs from the independently locked white entry).
make test builds and audits libflair.a before its consumers run. libflair.a uses the
shared target rules; region-target asks region for its single library/object build.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: resident
Interface export-pure: flair/include/asset_lz.h
Interface export-records: flair/include/assets/boot_logo.h
Interface export-pure: flair/include/assets/chicago12.h chicago12_aw chicago12_lb chicago12_rows
Interface export-pure: flair/include/assets/color_canon.h
Interface export-pure: flair/include/assets/desk_icons.h
Interface export-records: flair/include/assets/finder_icons.h
Interface export-records: flair/include/assets/finder_list_titles.h
Interface export-records: flair/include/assets/finder_small_icons.h
Interface export-records: flair/include/assets/font_linkage.h
Interface export-pure: flair/include/assets/font_linkage.h
Interface export-pure: flair/include/assets/geneva12.h geneva12_aw geneva12_lb geneva12_rows
Interface export-pure: flair/include/assets/geneva9.h geneva9 geneva9_advance
Interface export-records: flair/include/assets/menu_canon.h
Interface export-pure: flair/include/assets/palette.h
Interface export-pure: flair/include/blitter.h blitter_blit_clipped blitter_fill_rect_clipped
Interface export-pure: flair/include/chrome.h flair_draw_document_window flair_draw_document_window_scroll flair_draw_movable_dbox_chrome flair_draw_rdoc_window flair_draw_window_scrollbar flair_draw_window_widget
Interface export-binding: flair/include/chrome_metrics.h
Interface export-binding: flair/include/control.h DrawControl DrawControlDefaultRing GetControlValue SetControlValue TestControl TrackControl control_init ctrl_progress_fill_px ctrl_thumb_y ctrl_value_from_thumb_y
Interface export-binding: flair/include/cursor.h flair_cursor_hide flair_cursor_init flair_cursor_is_busy flair_cursor_move flair_cursor_set_busy flair_cursor_shield flair_cursor_show flair_cursor_unshield
Interface export-binding: flair/include/dialog.h DialogHandleEvent DialogModalDispatch DisposeDialog DrawDialog FileCopyCancelableDialog FileCopyDialog FindDialogItem GetDialogItem GetDialogItemText GetNewDialog ModalDialog MoveDialog NewDialog SetDialogItem SetDialogItemText ShowDialog
Interface export-binding: flair/include/event.h GetNextEvent WaitNextEvent flair_event_init flair_event_set_keyboard_observer flair_event_set_yield flair_event_warp flair_raw_post flair_tick_advance flair_tick_count
Interface export-binding: flair/include/filecopy_cancel.h
Interface export-pure: flair/include/flair_look.h flair_look_default_skin flair_look_pixel flair_look_pixel_depth flair_look_pixel_for_skin flair_look_small_icon_pixel
Interface export-binding: flair/include/flair_platform.h flair_platform_bind_files flair_platform_desktop_pattern flair_platform_dos flair_platform_file_info_get flair_platform_files_close flair_platform_files_init flair_platform_files_live flair_platform_load flair_platform_memory flair_platform_panic flair_platform_path_is_device flair_platform_serial flair_platform_speaker flair_platform_swap_dta flair_platform_tenant_abort flair_platform_tenant_call
Interface export-binding: flair/include/grafport.h
Interface export-pure: flair/include/heap.h flair_alloc flair_free flair_heap_avail flair_heap_init
Interface export-binding: flair/include/imaging.h
Interface export-binding: flair/include/list.h FlairLAddRow FlairLClick FlairLGetCell FlairLGetSelect FlairLSetCell FlairLSetSelect FlairList_init
Interface export-binding: flair/include/menu.h DrawMenuBar HiliteMenu MenuBar_hilite_rect MenuBar_hit MenuBar_menu MenuBar_menu_id MenuBar_set_apple_menu MenuBar_title_w MenuBar_title_x MenuClock_attach MenuClock_draw MenuClock_enable MenuClock_format MenuClock_set MenuClock_text MenuInfo_item_at MenuInfo_item_selectable MenuInfo_panel_footprint_rect MenuInfo_panel_rect MenuKey MenuSelect MenuTrack_begin MenuTrack_result MenuTrack_step flair_draw_menu_panel flair_menu_track
Interface export-records: flair/include/ostype.h
Interface export-binding: flair/include/ostype.h
Interface export-binding: flair/include/process.h FlairProcessList_init FlairProcess_activate FlairProcess_admit FlairProcess_close_window FlairProcess_kill FlairProcess_launch FlairProcess_register FlairProcess_sync_active FlairProcess_terminate flair_app_dispatch flair_route_updates
Interface export-pure: flair/include/scrap.h FlairGetScrap FlairInfoScrap FlairPutScrap FlairScrap_init FlairZeroScrap
Interface export-binding: flair/include/stdfile.h FlairSF_click FlairSF_count FlairSF_doCancel FlairSF_doOpen FlairSF_navigate FlairSF_open FlairSF_select FlairSF_up
Interface export-pure: flair/include/surface.h surface_blit surface_clear surface_fill_span
Interface export-binding: flair/include/tbxgate.h tbx_alert_answer tbx_ask_quit tbx_bind tbx_close_ask tbx_fault_triage tbx_fc_close tbx_fc_commit tbx_fc_open tbx_fc_rate tbx_fc_step tbx_filecopy_answer tbx_gate_dispatch tbx_launch tbx_menu_choice tbx_menu_key tbx_owns_window tbx_preferences_boot tbx_preferences_dirty tbx_preferences_tick tbx_reap tbx_resident_name tbx_sf_answer tbx_take_affirm tbx_take_alert tbx_take_bar_dirty tbx_take_drew tbx_take_filecopy tbx_take_mbar_redraw tbx_take_sf tbx_take_track tbx_track_push
Interface export-pure: flair/include/text.h region_contains_point surface_blit text_chicago_row text_draw_run text_font_from_txfont text_geneva12_row
Interface export-pure: flair/include/textedit.h FlairTECanReplace FlairTEClick FlairTECopy FlairTECut FlairTEDelete FlairTEGetOffset FlairTEGetPoint FlairTEGetText FlairTEInsert FlairTEKey FlairTELineHeight FlairTEPaste FlairTESetSelect FlairTESetText FlairTE_init
Interface export-binding: flair/include/window.h CalcDocContentRect CalcRDocContentRect CollapseWindow ComputeVisible ConstrainWindowSize DisposeWindow DragWindow FindWindow HideWindow MoveWindow NewDocumentWindow NewWindow SelectWindow SetWTitle ShowWindow SizeWindow WindowFrameRect WindowMgr_clip_update_to_visible WindowMgr_init WindowMgr_invalidate WindowMgr_invalidate_desktop WindowMgr_validate WindowMgr_window_index WindowScrollAttach WindowScrollBand WindowScrollDetach WindowScrollFindPart WindowScrollOf ZoomWindow
Interface export-binding: flair/include/window_record.h
Interface export-binding: flair/include/winscroll.h
Interface export-records: build/flair/generated/asset_lz_data.h
Interface export-records: build/flair/generated/boot_logo.h
Interface export-records: build/flair/generated/chicago12.h
Interface export-records: build/flair/generated/chicago_packed_data.h
Interface export-records: build/flair/generated/color_canon.h
Interface export-records: build/flair/generated/flair_dac.h
Interface export-records: build/flair/generated/geneva12.h
Interface export-records: build/flair/generated/geneva12_packed_data.h
Interface export-records: build/flair/generated/palette.h
Interface use-gate: sdk/include/tbx.h
Interface use-pure: flair/region/include/region.h
Interface use-binding: flair/include/flair_platform.h
Interface use-pure: flair/region/include/region_algebra.h
Interface export-binding: flair/event.c flair_double_click_ticks
<!-- end-interface-declarations -->
