# goldens

Data ownership differs from semantic ownership: period captures and input/output corpora must be pinned, while spec contracts stay beside their component/public interface. Local fixture hashes and external envelope SHA-256 manifests are provided; sibling revisions and existing mint hash catalogs preserve provenance.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

0 production/source-support files; 0 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`fixtures/README.md:1`|14|fixtures/ — the frame still(s) and golden files|
|`fixtures/easter/README.md:1`|27|Jump to Conclusions: authored regression stills|
|`fixtures/easter/jtc_alert.png:1`|binary|jtc_alert.png; source/contract input (role inferred from path)|
|`fixtures/easter/jtc_mat.png:1`|binary|jtc_mat.png; source/contract input (role inferred from path)|
|`fixtures/flair-cpanels-bars/README.md:1`|29|Preference repaint references (initech-c8w7 / B05)|
|`fixtures/flair-cpanels-bars/cpanels.png:1`|binary|cpanels.png; source/contract input (role inferred from path)|
|`fixtures/flair-cpanels-bars/hello.png:1`|binary|hello.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/BLOCK.WK1:1`|binary|BLOCK.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-copy/BLOCK.ref:1`|binary|BLOCK.ref; source/contract input (role inferred from path)|
|`fixtures/i123-copy/OVER.WK1:1`|binary|OVER.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-copy/OVER.ref:1`|binary|OVER.ref; source/contract input (role inferred from path)|
|`fixtures/i123-copy/README.md:1`|45|Real Lotus R2.2 Copy / POINT evidence (B11, initech-fyji)|
|`fixtures/i123-copy/ROW.WK1:1`|binary|ROW.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-copy/ROW.ref:1`|binary|ROW.ref; source/contract input (role inferred from path)|
|`fixtures/i123-copy/SHA256SUMS:1`|binary|SHA256SUMS; source/contract input (role inferred from path)|
|`fixtures/i123-copy/SINGLE.WK1:1`|binary|SINGLE.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-copy/SINGLE.ref:1`|binary|SINGLE.ref; source/contract input (role inferred from path)|
|`fixtures/i123-copy/auto_anchor.png:1`|binary|auto_anchor.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/backspace.png:1`|binary|backspace.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/block.png:1`|binary|block.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/block.steps:1`|20|Real R2.2: a two-dimensional FROM does not tile in either axis.|
|`fixtures/i123-copy/escape.png:1`|binary|escape.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/overlap.png:1`|binary|overlap.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/point.steps:1`|22|Real R2.2: automatic anchor; period; Esc keeps current; Backspace restores.|
|`fixtures/i123-copy/point_range.png:1`|binary|point_range.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/row.png:1`|binary|row.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/single.png:1`|binary|single.png; source/contract input (role inferred from path)|
|`fixtures/i123-copy/single.steps:1`|14|Real R2.2, fresh sheet: a single-cell FROM fills every cell of TO.|
|`fixtures/i123-copy/tile.steps:1`|24|Real R2.2, fresh sheet: row -> nonmultiple block and overlap.|
|`fixtures/i123-copy/unanchor.png:1`|binary|unanchor.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/00_start.png:1`|binary|00_start.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/01_hello.png:1`|binary|01_hello.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/02_f2.png:1`|binary|02_f2.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/03_f2_esc1.png:1`|binary|03_f2_esc1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/04_f2_esc2.png:1`|binary|04_f2_esc2.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/05_f2_commit.png:1`|binary|05_f2_commit.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/06_f5_prompt.png:1`|binary|06_f5_prompt.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/07_f5_typed.png:1`|binary|07_f5_typed.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/08_f5_z100.png:1`|binary|08_f5_z100.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/09_f5_onscreen.png:1`|binary|09_f5_onscreen.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/10_f5_bad.png:1`|binary|10_f5_bad.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/11_f5_bad_esc.png:1`|binary|11_f5_bad_esc.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/12_pgdn.png:1`|binary|12_pgdn.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/13_pgdn2.png:1`|binary|13_pgdn2.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/14_pgup.png:1`|binary|14_pgup.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/15_pgdn_mid.png:1`|binary|15_pgdn_mid.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/16_tab.png:1`|binary|16_tab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/17_tab2.png:1`|binary|17_tab2.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/18_backtab.png:1`|binary|18_backtab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/19_tab_mid.png:1`|binary|19_tab_mid.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/20_backtab_a1.png:1`|binary|20_backtab_a1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/21_pgup_a1.png:1`|binary|21_pgup_a1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/30_a1.png:1`|binary|30_a1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/31_d1.png:1`|binary|31_d1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/32_d1_tab.png:1`|binary|32_d1_tab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/33_tab2.png:1`|binary|33_tab2.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/34_backtab.png:1`|binary|34_backtab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/35_goto_k1.png:1`|binary|35_goto_k1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/36_k1_backtab.png:1`|binary|36_k1_backtab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/37_goto_a21.png:1`|binary|37_goto_a21.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/38_a21_up.png:1`|binary|38_a21_up.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/39_a20_pgdn.png:1`|binary|39_a20_pgdn.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/40_iv1.png:1`|binary|40_iv1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/41_iv1_tab.png:1`|binary|41_iv1_tab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/42_a8190.png:1`|binary|42_a8190.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/43_a8190_pgdn.png:1`|binary|43_a8190_pgdn.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/44_f2_value.png:1`|binary|44_f2_value.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/45_f2_cleared_enter.png:1`|binary|45_f2_cleared_enter.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/46_label_cleared_enter.png:1`|binary|46_label_cleared_enter.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/47_f2_empty.png:1`|binary|47_f2_empty.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/48_f2_empty_enter.png:1`|binary|48_f2_empty_enter.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/49_edit_down.png:1`|binary|49_edit_down.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/50_bad_value.png:1`|binary|50_bad_value.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/51_bad_value_esc.png:1`|binary|51_bad_value_esc.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/60_goto_d21.png:1`|binary|60_goto_d21.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/61_h1.png:1`|binary|61_h1.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/62_h1_backtab.png:1`|binary|62_h1_backtab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/63_h1_tab.png:1`|binary|63_h1_tab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/64_a25.png:1`|binary|64_a25.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/65_a25_pgup.png:1`|binary|65_a25_pgup.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/66_pgup_top.png:1`|binary|66_pgup_top.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/67_label_tab.png:1`|binary|67_label_tab.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/68_label_pgdn.png:1`|binary|68_label_pgdn.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/69_goto_default.png:1`|binary|69_goto_default.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/70_typed240.png:1`|binary|70_typed240.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/71_stored240.png:1`|binary|71_stored240.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/72_f2_240.png:1`|binary|72_f2_240.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/73_f2_240_typeq.png:1`|binary|73_f2_240_typeq.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/74_f2_240_enter.png:1`|binary|74_f2_240_enter.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/75_saved.png:1`|binary|75_saved.png; source/contract input (role inferred from path)|
|`fixtures/i123-keys/T240.WK1:1`|binary|T240.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-keys/keys.steps:1`|92|the REAL Lotus 1-2-3 Release 2.2 driven under DOSBox-X|
|`fixtures/i123-keys/keys2.steps:1`|111|the REAL Lotus 1-2-3 Release 2.2 driven under DOSBox-X|
|`fixtures/i123-keys/keys3.steps:1`|70|the REAL Lotus 1-2-3 Release 2.2 driven under DOSBox-X|
|`fixtures/i123-keys/keys4.steps:1`|33|the REAL Lotus 1-2-3 Release 2.2 driven under DOSBox-X|
|`fixtures/i123-numeric/NUMENTRY.WK1:1`|binary|NUMENTRY.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-numeric/NUMENTRY.ref:1`|binary|NUMENTRY.ref; source/contract input (role inferred from path)|
|`fixtures/i123-numeric/NUMFMT.WK1:1`|binary|NUMFMT.WK1; source/contract input (role inferred from path)|
|`fixtures/i123-numeric/NUMFMT.ref:1`|binary|NUMFMT.ref; source/contract input (role inferred from path)|
|`fixtures/i123-numeric/README.md:1`|48|Real Lotus numeric evidence|
|`fixtures/i123-numeric/format-page1.png:1`|binary|format-page1.png; source/contract input (role inferred from path)|
|`fixtures/i123-numeric/format-page2.png:1`|binary|format-page2.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/01_w72.png:1`|binary|01_w72.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/02_w72_right.png:1`|binary|02_w72_right.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/03_w76.png:1`|binary|03_w76.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/04_w77.png:1`|binary|04_w77.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/05_w240.png:1`|binary|05_w240.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/06_w240_right.png:1`|binary|06_w240_right.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/07_w240_left.png:1`|binary|07_w240_left.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/08_w240_down.png:1`|binary|08_w240_down.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/11_w67.png:1`|binary|11_w67.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/12_w68.png:1`|binary|12_w68.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/13_w240_labels.png:1`|binary|13_w240_labels.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/14_d1.png:1`|binary|14_d1.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/15_back_a1.png:1`|binary|15_back_a1.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/16_w80.png:1`|binary|16_w80.png; source/contract input (role inferred from path)|
|`fixtures/i123-width/width.steps:1`|48|the REAL Lotus 1-2-3 Release 2.2 driven under DOSBox-X|
|`fixtures/i123-width/width2.steps:1`|52|the REAL Lotus 1-2-3 Release 2.2 driven under DOSBox-X|
|`fixtures/iword-goto/01_typed.png:1`|binary|01_typed.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/02_top.png:1`|binary|02_top.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/03_pgdn.png:1`|binary|03_pgdn.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/04_pgdn2.png:1`|binary|04_pgdn2.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/05_pgdn3.png:1`|binary|05_pgdn3.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/06_pgdn_last.png:1`|binary|06_pgdn_last.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/07_pgup.png:1`|binary|07_pgup.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/08_pgup_first.png:1`|binary|08_pgup_first.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/09_col5_line2.png:1`|binary|09_col5_line2.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/10_pgdn_col.png:1`|binary|10_pgdn_col.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/11_p2_line2.png:1`|binary|11_p2_line2.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/12_pgup_mid.png:1`|binary|12_pgup_mid.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/13_goto_prompt.png:1`|binary|13_goto_prompt.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/14_goto_3.png:1`|binary|14_goto_3.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/15_goto_3_enter.png:1`|binary|15_goto_3_enter.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/16_goto_9.png:1`|binary|16_goto_9.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/17_goto_0.png:1`|binary|17_goto_0.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/18_goto_2_esc.png:1`|binary|18_goto_2_esc.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/19_goto_2_f1.png:1`|binary|19_goto_2_f1.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/20_goto_bs.png:1`|binary|20_goto_bs.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/21_goto_bs_enter.png:1`|binary|21_goto_bs_enter.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/22_goto_12.png:1`|binary|22_goto_12.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/23_goto_12_enter.png:1`|binary|23_goto_12_enter.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/24_p2_mid.png:1`|binary|24_p2_mid.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/25_goto_up.png:1`|binary|25_goto_up.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/26_goto_down.png:1`|binary|26_goto_down.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/27_goto_char.png:1`|binary|27_goto_char.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/28_goto_twice.png:1`|binary|28_goto_twice.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/29_goto_4_soft.png:1`|binary|29_goto_4_soft.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/30_goto_2x.png:1`|binary|30_goto_2x.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/31_goto_2.png:1`|binary|31_goto_2.png; source/contract input (role inferred from path)|
|`fixtures/iword-goto/goto.steps:1`|235|- the REAL WordPerfect 5.1 (03-09-92) under|
|`fixtures/iword-listfiles-ops/MINT.txt:1`|44|initech-7aou / cx-iwordlist round 2. Independent golden: REAL WordPerfect 5.1|
|`fixtures/iword-listfiles-ops/SHA256SUMS:1`|binary|SHA256SUMS; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/VERIFY.txt:1`|137|initech-7aou / cx-iwordlist round 2 verification|
|`fixtures/iword-listfiles-ops/collisions.steps:1`|39|type Keep active.|
|`fixtures/iword-listfiles-ops/full.steps:1`|14|type BETA.TXT|
|`fixtures/iword-listfiles-ops/grids/01_list.txt:1`|51|00\|Keep active.                                                                    \||
|`fixtures/iword-listfiles-ops/grids/02_delete.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/03_move.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/04_copy.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/05_lock_delete.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/06_readonly_deleted.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/12_move_name.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/13_move_replace.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/14_copy_name.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/15_copy_replace.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/16_copy_success.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/17_move_success.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/18_active.txt:1`|51|00\|Keep active.                                                                    \||
|`fixtures/iword-listfiles-ops/grids/19_directory_copy.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/20_directory_move.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/21_bad_path.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/25_readonly_volume.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/26_readonly_copy.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/29_full_copy.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/30_readonly_rename.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/32_copy_selection.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/grids/33_rename_selection.txt:1`|51|00\|03-01-92  09:00a            Directory A:\OPS\*.*                                \||
|`fixtures/iword-listfiles-ops/mint.sh:1`|71|Real WP 5.1 capture, initech-7aou. Corpus is read-only; each case receives|
|`fixtures/iword-listfiles-ops/ops.steps:1`|23|Real WP 5.1 03-09-92, scratch work copy; Reference PDF pp. 364-370.|
|`fixtures/iword-listfiles-ops/protected.steps:1`|14|type RENAME.TXT|
|`fixtures/iword-listfiles-ops/readonly-copy.steps:1`|13|shot 26_readonly_copy|
|`fixtures/iword-listfiles-ops/readonly.steps:1`|12|shot 25_readonly_volume|
|`fixtures/iword-listfiles-ops/refusals.steps:1`|20|shot 19_directory_copy|
|`fixtures/iword-listfiles-ops/selection.steps:1`|18|type ZETA.TXT|
|`fixtures/iword-listfiles-ops/shots/01_list.png:1`|binary|01_list.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/02_delete.png:1`|binary|02_delete.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/03_move.png:1`|binary|03_move.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/04_copy.png:1`|binary|04_copy.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/05_lock_delete.png:1`|binary|05_lock_delete.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/06_readonly_deleted.png:1`|binary|06_readonly_deleted.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/12_move_name.png:1`|binary|12_move_name.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/13_move_replace.png:1`|binary|13_move_replace.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/14_copy_name.png:1`|binary|14_copy_name.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/15_copy_replace.png:1`|binary|15_copy_replace.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/16_copy_success.png:1`|binary|16_copy_success.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/17_move_success.png:1`|binary|17_move_success.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/18_active.png:1`|binary|18_active.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/19_directory_copy.png:1`|binary|19_directory_copy.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/20_directory_move.png:1`|binary|20_directory_move.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/21_bad_path.png:1`|binary|21_bad_path.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/25_readonly_volume.png:1`|binary|25_readonly_volume.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/26_readonly_copy.png:1`|binary|26_readonly_copy.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/29_full_copy.png:1`|binary|29_full_copy.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/30_readonly_rename.png:1`|binary|30_readonly_rename.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/32_copy_selection.png:1`|binary|32_copy_selection.png; source/contract input (role inferred from path)|
|`fixtures/iword-listfiles-ops/shots/33_rename_selection.png:1`|binary|33_rename_selection.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/block.png:1`|binary|block.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/editing.png:1`|binary|editing.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/exitq.png:1`|binary|exitq.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/font.png:1`|binary|font.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/format.png:1`|binary|format.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/formatline.png:1`|binary|formatline.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/reveal.png:1`|binary|reveal.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/search.png:1`|binary|search.png; source/contract input (role inferred from path)|
|`fixtures/pubs/initechword-reference/setup.png:1`|binary|setup.png; source/contract input (role inferred from path)|

### Harness-local captured/generated corpus data

Exact filenames and SHA-256 values: [local-goldens.sha256](local-goldens.sha256). These are data, not necessarily independent period oracles: mint provenance must be retained.

|Prefix|Files|Bytes|
|---|---:|---:|
|`harness/conkeys/extended.hex`|1|409|
|`harness/conkeys/extended.keys`|1|712|
|`harness/conkeys/keys.qmp`|1|126|
|`harness/conkeys/lines.txt`|1|35|
|`harness/diff/.gitkeep`|1|0|
|`harness/diff/dbf_diff/canon`|6|12863|
|`harness/diff/dbf_diff/list_off_real.sha256`|1|367|
|`harness/diff/dbf_diff/ndx_incremental.sha256`|1|38636|
|`harness/diff/dbf_diff/report_mint004.md5`|1|8674|
|`harness/diff/dbf_diff/samirctl_emu`|5|2805|
|`harness/diff/dbf_diff/screen_cursor25_gated.txt`|1|85|
|`harness/diff/dbf_diff/screen_dotprompt_gated.txt`|1|382|
|`harness/diff/dbf_diff/screen_forms_gated.txt`|1|578|
|`harness/diff/dbf_diff/screen_status_pending.txt`|1|97|
|`harness/diff/dbf_diff/use_mint005.md5`|1|455|
|`harness/diff/dbf_diff/xbase_prog_diff`|2023|1593789|
|`harness/diff/fat_diff/batch_break_mint`|15|14855|
|`harness/diff/fat_diff/chkdsk-label-golden`|4|670|
|`harness/diff/fat_diff/dos33_dir.golden`|1|119|
|`harness/diff/fat_diff/dosmsg.allow`|1|36598|
|`harness/diff/fat_diff/dosmsg.allow.baseline`|1|8439|
|`harness/diff/fat_diff/fixtures`|30|705741|
|`harness/diff/fat_diff/int21_witnesses.tsv`|1|970|
|`harness/diff/fcb/autoexec_fcbx.bat`|1|28|
|`harness/diff/fcb/fcbkept_expected.dat`|1|40|
|`harness/diff/fcb/fcbx_expected.txt`|1|973|
|`harness/diff/fcb/fcbx_fat_expected.txt`|1|95|
|`harness/diff/tps/autoexec_b.bat`|1|171|
|`harness/diff/tps/autoexec_backup.bat`|1|169|
|`harness/diff/tps/autoexec_c.bat`|1|171|
|`harness/diff/tps/keys_a.txt`|1|60|
|`harness/diff/tps/keys_c.txt`|1|20|
|`harness/diff/tps/keys_ok.txt`|1|30|
|`harness/diff/tps/old_tpsrept.bak`|1|33|
|`harness/diff/tps/old_tpsrept.txt`|1|33|
|`harness/diff/tps/tps.dbf`|1|940|
|`harness/diff/tps/tps_a_expected.txt`|1|3538|
|`harness/diff/tps/tps_b_expected.txt`|1|278|
|`harness/diff/tps/tps_c_expected.txt`|1|647|
|`harness/diff/tps/tpsmk.prg`|1|2962|
|`harness/diff/tps/tpsparm.dat`|1|93|
|`harness/diff/tps/tpsrept_expected.txt`|1|2432|
|`harness/diff/wp5_diff/listfiles_mint005.md5`|1|1870|
|`harness/emu/.gitkeep`|1|0|
|`harness/emu/ansi_probe.golden`|1|348|
|`harness/fixtures/dos33_overlay/.gitattributes`|1|100|
|`harness/fixtures/dos33_overlay/README.txt`|1|2071|
|`harness/fixtures/dos33_overlay/factor-different.bin`|1|12|
|`harness/fixtures/dos33_overlay/factor-equal.bin`|1|12|
|`harness/fixtures/dos33_overlay/version.raw`|1|44|
|`harness/fixtures/exec_overlay/README.txt`|1|1577|
|`harness/fixtures/exec_overlay/atomic-red.txt`|1|1446|
|`harness/fixtures/exec_overlay/flair-unbound-red.txt`|1|955|
|`harness/fixtures/exec_overlay/stream-device-red.txt`|1|1156|
|`harness/fixtures/exec_overlay/stream-memory-red.txt`|1|421|
|`harness/proptest/.gitkeep`|1|0|
|`harness/proptest/fixtures/flair_part_map.golden`|1|29794|
|`harness/pubs/flair.allow`|1|236|
|`harness/pubs/iword.allow`|1|234|
|`harness/pubs/pilot.allow`|1|10522|
|`harness/pubs/samir.allow`|1|239|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|—|No cross-component quoted includes found in production sources|—|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.
No production executable. Captures are inputs; minting scripts, expected-hash catalogs and normalization readers must accompany them.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 0; UNIT 0; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-goldens.md](gates-goldens.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.
No standalone host test program found for this component. This is a coverage gap, not evidence of zero assurance; relevant emulator graders and shared SDK tests are listed elsewhere.

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-goldens.md](gates-goldens.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**0 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 0 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).
No emulator gate selected: this component supplies shared data/support rather than a booted service.

The remaining legacy emulator entries cover consumer-specific data/reference grading; this row does not own runtime gates. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Do not import generated replay videos/build products, source-dependent self-referential snapshots as “independent” truth, or a hash without mint provenance. Keep legacy frame fixtures that define the golden-master film image. Retire obsolete process canon/gate-map/guardrail policy data; their source assignments remain listed for traceability.
