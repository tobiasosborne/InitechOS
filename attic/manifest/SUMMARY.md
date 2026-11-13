# Legacy component import manifest

Read-only research at `legacy-final` / `c5d18538049986bbbb27018c351405acf814ebbc`. No tracked file was changed, no tests/gates/emulators were run, and no bd tracker commands were used. All report/script/archive output is in this directory.

The allowed initial `make -qp` probe encountered GNU make’s included-makefile remake behavior (`harness/emu/vesa8.mk:16`): it created the DAC generator/header/stamp under ignored build/. It did not run a gate or change tracked files. Subsequent probes used `make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= <artifact>`. This parse-time side effect is a factory defect worth removing.

## Component table

Source files/LOC count owned production and source-support files (C/headers/ASM/linker/Pascal/definition tables; no test mains, fixtures/examples or spec data). kmain.c is counted once in kernel; desktop extraction ranges are in both manifests. Factory source-support includes generic host helpers and builders. Test counts are unique host main/script programs, with META restricted to factory/process/spec-inventory checks; target variants are enumerated without double counting. Classes based on body scanning are marked inferred in each entry.

|Component|Source files|Physical LOC|Strong|Unit|Meta|Size/layout|Legacy emu entries|External corpus dependencies|
|---|---:|---:|---:|---:|---:|---:|---:|---|
|[boot](boot.md)|3|1,807|0|0|0|0|6|none directly named|
|[kernel](kernel.md)|97|41,123|14|59|0|3|137|none directly named|
|[dos](dos.md)|40|12,072|2|20|1|0|80|none directly named|
|[flair](flair.md)|51|19,811|31|35|0|0|24|system7-decomp; win31-decomp|
|[flair-desktop](flair-desktop.md)|46|12,569|14|26|0|1|164|system7-decomp|
|[sdk](sdk.md)|5|1,025|0|0|0|0|0|none directly named|
|[apps-calc](apps-calc.md)|1|312|1|2|0|0|5|system7-decomp; lotus123-decomp (shared core/oracle)|
|[apps-calcda](apps-calcda.md)|5|894|2|1|0|0|5|system7-decomp|
|[apps-cpanels](apps-cpanels.md)|3|198|1|2|0|0|25|system7-decomp|
|[apps-itext](apps-itext.md)|2|726|1|1|0|0|14|system7-decomp|
|[apps-notepad](apps-notepad.md)|1|427|1|1|0|0|12|system7-decomp|
|[apps-paint](apps-paint.md)|1|109|1|1|0|0|3|system7-decomp|
|[apps-fonts](apps-fonts.md)|1|101|1|0|0|0|2|system7-decomp|
|[apps-ctenant](apps-ctenant.md)|2|208|0|0|0|0|2|none directly named|
|[apps-i123](apps-i123.md)|12|7,802|6|10|0|0|27|lotus123-decomp|
|[apps-iword](apps-iword.md)|9|6,315|7|4|0|0|33|wordperfect51-decomp|
|[apps-samir](apps-samir.md)|49|32,588|35|47|1|1|229|dbase3-decomp|
|[tps](tps.md)|19|15,280|1|2|0|0|18|FPC independent compiler; turbopascal-decomp for original mint provenance|
|[factory](factory.md)|305|37,345|0|0|26|1|13|all corpora through readers/mint tooling|
|[goldens](goldens.md)|0|0|0|0|0|0|0|all six decomp repositories; manuals provenance|

The emulator column is an exclusive primary-owner redistribution of the **799 declared active emulator entries**, preserving the audit denominator. These are not boots. [gate-attribution.tsv](gate-attribution.tsv) preserves old gate-map family, new owner and declared tier. Some host entries also run emulators transitively; per-component section 5 gives structural emulator-associated totals. Product support is shared: SDK has no exclusive gate names but uses CTENANT/protection smokes.

## Recommended import order

Host-first core imports in steps 3–6 do not need boot/kernel. Their production executable packaging waits for the later runtime interfaces. The dependency graph has cycles today; this order describes breaking them with explicit adapters rather than preserving the monolithic link.

1. **goldens** — freeze/hash captures, corpus/license/provenance and independent readers first; prevents importing a rewrite of the oracle.
2. **factory** — minimal host assertion helpers, deterministic image/FAT builders and emulator drivers; omit old process wrappers.
3. **tps** — C seed/FPC tests are already isolated; establish compiler oracles early without boot dependency.
4. **apps-samir** — PAL host backend and strongest period transcript/format suites can import independently; DOS packaging waits for kernel.
5. **apps-i123** — pure worksheet/WK1/formula core before CALC; independent Lotus readers/captures give strong protection.
6. **apps-iword** — pure document/WP5/search core with independent WP reader before GUI binding.
7. **boot** — handoff + chunked loader removes the constraint that caused variant/overlay/size machinery.
8. **kernel** — platform/FAT/MCB/PSP/SFT/INT21/loader; expose explicit services; leave desktop orchestration out.
9. **dos** — INT21-backed COMMAND and utility programs; remove exact-address transient ABI.
10. **flair** — region first, then surface/blitter/text/heap/event/window/menu/control/dialog/textedit/list/scrap/resource/process and gate; narrow kernel adapters.
11. **sdk** — frozen flat INT81 contract, C runtime/tenant linker and shared host fake transport; finalize before app faces.
12. **apps-ctenant** — smallest complete tenant proves SDK load/register/draw/event/exit seam.
13. **apps-calcda** — pure core and real Macintosh key/click/readout oracle.
14. **apps-cpanels** — settings/dialog/clock service boundary and record ownership.
15. **apps-itext** — TextEdit/font library and gate-driven document save/scrap behavior.
16. **apps-notepad** — reuse the now-explicit TextEdit/font library for Notepad and Scrapbook.
17. **apps-calc** — reuse imported i123 core for the film calculator.
18. **apps-fonts** — specimen confirms text/font/style/title ABI.
19. **apps-paint** — small specimen app; keep visible disabled menus honest.
20. **flair-desktop** — compose Finder/film/system menus against stable kernel/Toolbox/SDK; extract production kmain slices last.

## Complete spec ownership

156 tracked files. Each row is an ownership recommendation; file existence and line counts are measured. Trace .mk files contain useful input programs even when their build/rule machinery is legacy-only.

|File|Owner|Disposition|
|---|---|---|
|`spec/CANON-MANIFEST.md:1`|factory|LEGACY-ONLY process/layout policy: retain at tag|
|`spec/README.md:1`|factory|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/absdisk_int2526.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/apple_menu_items.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/.gitkeep`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/alert_icons.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/apple_glyph.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/boot_logo.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/calc_ledger.h:1`|apps-calc|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/calc_ledger.json:1`|apps-calc|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/calcda_geneva9.h:1`|apps-calcda|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/chicago12.art:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/chicago12.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/clut.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/clut.json:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/color_canon.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/color_canon.json:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/cursors.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/desk_icons.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/finder_icons.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/finder_list_titles.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/finder_small_icons.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/font_linkage.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/geneva12.art:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/geneva12.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/geneva9.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/menu_canon.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/palette.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/assets/palette.json:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/block_device_contract.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/block_device_contract.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/calcda/calcda_layout.h:1`|apps-calcda|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/calcda/calcda_vectors.tsv:1`|apps-calcda|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/chicago12_nfnt_golden.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/chimera_element_map.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/chrome_fidelity_golden.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/chrome_metrics.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/chrome_metrics.json:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/control_record.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/cp_dialog.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/cpanels/record.json:1`|apps-cpanels|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/cpanels_record.h:1`|apps-cpanels|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dialog_record.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_autoexec_bat_baseline.txt:1`|dos|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_banner.txt:1`|dos|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_config_sys_baseline.txt:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_folder_apps.txt:1`|dos|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_folder_commands.txt:1`|dos|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_folder_externals.txt:1`|dos|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_folder_traces.mk:1`|dos|input trace → retain data, replace make orchestration|
|`spec/dos_messages.json:1`|dos|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/dos_structs.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/drawing_ops.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/drive_letters.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/drive_letters.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/easter/jump_to_conclusions.json:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/event_model.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/find_data.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/flair_app_launch_trace.golden:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/flair_app_launch_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_appswitch_trace.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_calc_traces.mk:1`|apps-calc|input trace → retain data, replace make orchestration|
|`spec/flair_calcda_traces.mk:1`|apps-calcda|input trace → retain data, replace make orchestration|
|`spec/flair_cpanels_traces.mk:1`|apps-cpanels|input trace → retain data, replace make orchestration|
|`spec/flair_ctenant_traces.mk:1`|apps-ctenant|input trace → retain data, replace make orchestration|
|`spec/flair_desktop_icons_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_disk_windows_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_egg_jtc_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_exit_dos_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_fg_close_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_file_ops_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_file_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_film_scene_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_finder_alerts_trace.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_finder_cmds_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_finder_follow_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_finder_icons_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_finder_open_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_fonts_traces.mk:1`|apps-fonts|input trace → retain data, replace make orchestration|
|`spec/flair_fullsave_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_grow_box_trace.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_i123_copy_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_entry_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_file_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_hit_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_keys_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_menu_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_quit_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_resize_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_i123_wide_traces.mk:1`|apps-i123|input trace → retain data, replace make orchestration|
|`spec/flair_input_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_itext_traces.mk:1`|apps-itext|input trace → retain data, replace make orchestration|
|`spec/flair_iword_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_launchkey_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_list_view_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_menu_look_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_menubar_clock_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_notepad_traces.mk:1`|apps-notepad|input trace → retain data, replace make orchestration|
|`spec/flair_power_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_protection_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_scroll_look_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_scroll_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_skins.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/flair_solid_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_tenants_demo.h:1`|flair-desktop|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/flair_toolbox_hardening_trace.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_trash_edges_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/flair_trash_traces.mk:1`|flair-desktop|input trace → retain data, replace make orchestration|
|`spec/fpu_contract.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/gate_map.json:1`|factory|LEGACY-ONLY process/layout policy: retain at tag|
|`spec/grafport.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/hardware.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123/era_shortfalls.md:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123/menu_level_visits.tsv:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123/menu_ri_offsets.tsv:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123/menu_tree.golden:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123/wk1_format.h:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123/wk1_formula.h:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/i123_copy_smoke_keys.h:1`|apps-i123|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/imaging.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/initechmz_extended.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/int21h_calling_convention.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/int21h_register.json:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/iword/fkeys.h:1`|apps-iword|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/iword/fkeys.json:1`|apps-iword|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/iword/listfiles.h:1`|apps-iword|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/iword/reveal_codes.h:1`|apps-iword|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/iword/settings.h:1`|apps-iword|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/iword/wp5_format.h:1`|apps-iword|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/mbr_partition_contract.json:1`|boot|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/memory_map.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/menu_look_golden.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/menu_record.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/notepad/store.h:1`|apps-notepad|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/notepad/store.json:1`|apps-notepad|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/overlay_format.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/progress_platinum.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/pubs_inconsistencies.json:1`|factory|LEGACY-ONLY process/layout policy: retain at tag|
|`spec/region_algebra.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/README.md:1`|factory|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/dbase_msg_codes.tsv:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/dbf_format.h:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/dbf_normalization.json:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/memory_contract.h:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/ndx_format.h:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/ovl_format.h:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/screen_regrade.json:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/samir/xbase_coercion.json:1`|apps-samir|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/scrollbar_look_golden.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/ssim_params.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/toolbox_gate.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/tps_ritual.json:1`|tps|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/tsr_contract.h:1`|kernel|contract/asset/corpus → keep with provenance; publish shared interface headers|
|`spec/win95ism_guardrails.md:1`|factory|LEGACY-ONLY process/layout policy: retain at tag|
|`spec/window_record.h:1`|flair|contract/asset/corpus → keep with provenance; publish shared interface headers|

## Unassigned requested-tree files

- `os/README.md`
- `os/apps/.gitkeep`

## Generated headers and their generators

Explicit generated header rules plus checked-in generated data headers (the latter are inferred from file comments and generator sources; not all have an automatic make rule).

|Header|Generator|Inputs / recipe evidence|
|---|---|---|
|`build/chicago_packed_data.h`|`tools/chicago_pack.c:1`|`build/chicago_pack spec/assets/chicago12.h`; #  recipe to execute (from 'harness/proptest/kernel_size_r4.mk', line 122):|
|`build/dos_messages.h`|awk / recipe|`spec/dos_messages.json | build`; #  recipe to execute (from 'Makefile', line 28325):|
|`build/dos_messages.inc`|awk / recipe|`build/dos_messages.h | build`; #  recipe to execute (from 'Makefile', line 28357):|
|`build/flair_dac.h`|`tools/flair_dac_gen.c:1`|`build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json`; #  recipe to execute (from 'harness/emu/vesa8.mk', line 24):|
|`build/geneva12_packed_data.h`|`tools/chicago_pack.c:1`|`build/geneva12_pack spec/assets/geneva12.h`; #  recipe to execute (from 'harness/proptest/geneva12.mk', line 51):|
|`spec/assets/palette.h`|`tools/palette_extract.c:1`|`spec/assets/palette.json build/palette_extract`; #  recipe to execute (from 'Makefile', line 12063):|
|`spec/iword/fkeys.h`|`tools/iword_fkeys_gen.c:1`|`spec/iword/fkeys.json build/iword_fkeys_gen`; #  recipe to execute (from 'harness/diff/wp5_diff/p3.mk', line 5):|
|`os/flair/asset_lz_data.h:1`|`tools/asset_lz.c:1`|spec/assets/{desk_icons,finder_icons,alert_icons,finder_small_icons,boot_logo}.h; see Makefile ASSET_LZ_LOCKED (source-comment/generator association)|
|`os/flair/chicago_packed.h:1`|`tools/chicago_pack.c:1`|spec/assets/chicago12.h; runtime decoder wrapper; actual generated bytes build/chicago_packed_data.h (source-comment/generator association)|
|`os/flair/geneva12_packed.h:1`|no generator located: harness/proptest/geneva12_pack.c|spec/assets/geneva12.h; runtime decoder wrapper; actual generated bytes build/geneva12_packed_data.h (source-comment/generator association)|
|`os/i123/core/menu_data.h:1`|no generator located: NO TRACKED GENERATOR|123.RI menu strings transcribed at spec/i123/menu_ri_offsets.tsv; header:1 cites offsets; do not invent a generator (source-comment/generator association)|
|`os/samir/include/samir/messages_packed.h:1`|`tools/samir_msg_pack.c:1`|spec/samir/dbase_msg_codes.tsv only (argv[1], tools/samir_msg_pack.c:18); TSV provenance is real DBASE.MSG (source-comment/generator association)|
|`spec/assets/boot_logo.h:1`|`tools/boot_logo_gen.c:1`|compiled-in authored geometry from pubs/common/initech-logo.tex and docs/pubs/DESIGN-REFERENCE.md; generator:4–22; NO period raster input (source-comment/generator association)|
|`spec/assets/calc_ledger.h:1`|`tools/calc_ledger_gen.c:1`|spec/assets/calc_ledger.json (source-comment/generator association)|
|`spec/assets/chicago12.h:1`|`tools/font_art2h.c:1`|spec/assets/chicago12.art (source-comment/generator association)|
|`spec/assets/geneva12.h:1`|`tools/geneva12_art2h.c:1`|spec/assets/geneva12.art (source-comment/generator association)|
|`spec/block_device_contract.h:1`|`tools/gen_block_device_spec.c:1`|spec/block_device_contract.json (source-comment/generator association)|
|`spec/drive_letters.h:1`|`tools/gen_drive_letters.c:1`|spec/drive_letters.json (source-comment/generator association)|
|`spec/assets/color_canon.h:1`|`tools/color_canon_extract.c:1`|spec/assets/color_canon.json (source-comment/generator association)|
|`spec/assets/finder_small_icons.h:1`|`tools/mint_finder_small_icons.c:1`|system7 goldens/captures/s8_finder_list_view.png and s8_finder_list_view_bysize.png; tools/mint_finder_small_icons.c:1–16; color_canon.h (source-comment/generator association)|

Generated linker/assembly inputs also matter: build/samir.ld from tools/samir_memory_ld.c + os/samir/boot/samir.ld; transient/FLAIR overlay import tables and stubs from tools/mk_ovl_tables.sh and exact resident ELF symbols (retire these packaging leaks). No fresh generation was requested as verification.

## External sibling corpora and vendoring envelope

Measured filesystem subset sizes below are conservative envelopes, not proven minimal transitive closures. They cover the prominent statically named directories and readers; dynamically constructed/sprintf/wildcard paths can need more. Pin the listed sibling revision AND actual private/captured data hashes: git revisions alone do not pin ignored mint trees. Exact files/sizes and source references are in external-*.txt; external-*.sha256 pins every measured file in those envelopes. Missing paths are explicit, not counted as zero-byte available data.

|Sibling / root variable|Components|Conservative subset|Files / bytes available|Missing paths / revision|
|---|---|---|---:|---|
|`../system7-decomp` / `SYSTEM7_DECOMP`|flair, flair-desktop, apps-calc, apps-calcda, apps-cpanels, apps-itext, apps-notepad, apps-paint, apps-fonts|`goldens/captures`, `goldens/resources`, `mint/work/geneva12`|710 / 6,317,485|none in measured envelope; `2363daa0de896732276d45fbea82bff32b7a31ee`|
|`../win31-decomp` / `WIN31_DECOMP`|flair|`oracles/wine/server/region.c`, `oracles/wine/server/request.h`, `oracles/wine/server/user.h`, `oracles/wine/COPYING.LIB`|4 / 74,029|none in measured envelope; `27312fd6a311f22d5bc907d3db0703123574418e`|
|`../dbase3-decomp` / `DBASE3_DECOMP`|apps-samir|`goldens`, `mint/work`, `mint/samirctl`, `re/mint-results-004.md`, `re/mint-results-005.md`|1,118 / 20,247,266|none in measured envelope; `04820a43474a63123be4b4c122cff97ec2a5fb1c`|
|`../lotus123-decomp` / `LOTUS123_DECOMP`|apps-calc, apps-i123|`goldens/minted`, `goldens/lotus-123-2.2-installed-ref/extracted/installed/123.RI`, `tools/wk1_ref.py`|102 / 253,930|none in measured envelope; `95eed7cc35b651dfef56624e02c7b9e12669d1fa`|
|`../wordperfect51-decomp` / `WORDPERFECT51_DECOMP`|apps-iword|`goldens/minted`, `mint/work/WP51`, `mint/work/LISTF`, `tools/wp5_ref.py`|796 / 10,478,104|none in measured envelope; `e983c8dad9ce92fce02052cc0262640873ee50ad`|
|`../turbopascal-decomp` / `no shared *_DECOMP root; bootstrap provenance`|tps|`goldens/tp55/tp55.zip`|1 / 985,839|none in measured envelope; `a92f4369cce9ae91d54b5249adf1f35da476c6a5`|
|`../initech-manuals-ref` / `no shared *_DECOMP root; bootstrap provenance`|goldens/factory provenance|`MANIFEST.sha256`|1 / 11,306|none in measured envelope; `221122215f3906b562ad251f6cc565044d7cabc9`|

`FPC` is an external executable oracle rather than a sibling-repo dataset. Existing fail-loud roots/defaults/opt-outs are Makefile:29–47 and harness/oracle_policy.h:25. `tools/bootstrap/doctor.sh:92`–103 lists original binaries and mandatory captures. Real period applications/ROMs/captures may require separate rights/access; this manifest does not copy them. The private manuals repository is reference acquisition/provenance, not a required runtime dependency of every host test.

## App Contract and resident bypasses

The complete AX/name/arity/status table and per-app call sites are in [sdk.md](sdk.md). It includes cp_dialog.h’s 0x92–0x96 extension, reserved/dissolved codes and dynamic event delivery distinctions. HELLO/NOTES and Finder are the resident direct-call exceptions; TEXT/Notepad/Scrapbook directly link the shared TextEdit model. CALC reuses i123 core. No SAMIR/TPS INT81 dependency should be invented: they are DOS programs.

## Highest-priority coupling repairs

- **kernel** `os/milton/kmain.c:77` → flair heap/window/menu/process/Finder managers: move production desktop orchestration to flair/desktop; source slice map below.
- **kernel** `os/milton/kmain.c:110` → os/apps/ref_tenant.h: HELLO/NOTES are resident reference tenants, not disk programs.
- **kernel** `os/milton/int21.c:42` → os/milton/ansi.c; os/milton/loader.c:50 -> mz.c: textually folded translation units are build coupling; inspect include sites listed above.
- **flair** `os/flair/tbxgate.c:84` → os/milton/loader.h: expose a narrow tenant-loader service instead of loader internals.
- **flair** `os/flair/tbxfile.h:8` → os/milton/int21.h + sft.h: shadow PSP/JFT lifetime and DOS I/O adapter reaches into kernel state.
- **flair** `os/flair/tbxgate.c:93` → os/milton/io.h: speaker/port I/O needs an explicit platform service.
- **flair** `os/flair/cp_service.h:10` → os/milton/pit.h: control-panel service reads platform clock/tick state.
- **flair** `spec/cp_dialog.h:28` → DialogRecord/ControlRecord implementation adapter: split public flat ABI from conditional in-header Toolbox implementation.
- **flair-desktop** `os/milton/finder_actions.h:43` → g_finder_* globals: replace shared buffers/state globals with a desktop context/service table.
- **flair-desktop** `os/milton/kmain.c:3408` → FAT12 directory enumeration: Finder filesystem provider directly binds FAT internals; replace with named file service.
- **flair-desktop** `os/milton/kmain.c:5234` → boot filesystem + overlay + transient resident imports: separate platform loading from desktop event policy.
- **flair-desktop** `os/apps/ref_tenant.c:77` → process/window/blitter/surface/heap structures: HELLO/NOTES bypass INT 81h; convert to disk examples or retain explicitly as privileged fixtures.
- **dos** `os/milton/transient.h:77` → resident kernel symbol address fingerprint: transient import table ties shell image to exact kernel ELF.
- **dos** `os/milton/command.c:3067` → harness/diff/fat_diff/shell_files_host.h: host-only backend seam is useful; move into a dedicated adapter.
- **dos** `os/milton/command.c:3070` → harness/diff/fat_diff/batch_break_host.h: host-only test injection reaches factory header.
- **sdk** `os/apps/tbx/tbx.h:50` → TBX_IN_IMAGE: loaded-PROGBITS requirement inflates disk image; expose readable/writable span contract.
- **sdk** `os/apps/tbx/tbx_rt.c:85` → cdecl/regparm(0) asm trap shim: runtime must preserve trap ABI despite i123 whole-image register ABI.
- **apps-calc** `os/apps/calc/calc.c:10` → os/i123/core/i123.h: extract/re-export worksheet core as an explicit reusable library.
- **apps-calc** `Makefile:10750` → CALC links i123 sources: build ownership dependency must be explicit in component makefile.
- **apps-itext** `Makefile:10657` → os/flair/textedit.c + os/apps/itext/te_fonts.c: direct model library reuse; define a supported model/font API rather than private FLAIR headers.
- **apps-notepad** `Makefile:10669` → os/flair/textedit.c + os/apps/itext/te_fonts.c: shared TextEdit/font dependency is currently owned by another app; give it library ownership.
- **apps-i123** `Makefile:10618` → per-image regparm/rtd flags: keep tbx_trap cdecl/regparm(0) and change all image TUs together.
- **apps-i123** `os/i123/app/face_a.c:77` → spec/cpanels_record.h: shared settings data is public contract; do not import Control Panels tenant implementation.
- **apps-samir** `os/samir/pal/pal_milton.c:99` → spec/memory_map.h PROGRAM_STACK_BOT: app memory policy currently imports the global physical map; request limits through PAL/loader contract.
- **apps-samir** `Makefile:28967` → SAMIR ELF section/call-graph packing: app-owned overlays are distinct from FLAIR kernel overlays; preserve correctness while replacing only the size policy.

## Completeness and limits

Source/include paths and line counts use the detached tag. Build commands are printed, not executed; they do not prove compilation succeeds on this host. Cross-include resolution uses local headers and target include directories (i123 menu.h is resolved to its own core). Conditional edges are retained even when host-only. All 799 emulator entries receive exactly one primary owner; cross-exercised components can select the same smoke without duplicating that denominator. Host classification and semantic owners are research judgments with explicit evidence, not a dynamic coverage measurement.

The original report’s prior scratch paths refer to the former main checkout; this directory uses the detached legacy checkout directly. The initial query’s parse-time ignored-build side effect is documented above. [classification.json](classification.json), [assignment.tsv](assignment.tsv), [gate-attribution.tsv](gate-attribution.tsv), [rules.json](rules.json), [dry-host.json](dry-host.json) and per-component dry-production traces permit independent review.
