# WL-0171 -- SAMIR size round 2: 4,077 B reclaimed by calling-convention and pass policy

Lane samirsize (branch lane/samirsize, worktree lane-samirsize), single
lane. Nothing pushed; CLAUDE.md, HANDOFF and beads untouched. Every QEMU
gate ran one at a time with INITECH_QMP_STDIO=1 (scratchpad env.sh).

## Context

test-samir-size was RED on main:

    RED SAMIR_SIZE: 98587 B > 98175 B reclaim cap / 102256 B headroom cap

At that round, the two caps (harness/diff/dbf_diff/test_samir_size.c): the
*reclaim cap* was ARRIVAL - RECOVER = 106367 - 8192 = 98175 B, the round-1
target pinned against the fixed arrival measurement; the *headroom cap* was
the linker budget minus 4 KiB = 106352 - 4096 = 102256 B. Round 1
(docs/worklog/samir-size-round1.md) reached 97915 B with
SAMIR_SIZE_CFLAGS + the byte-pair message catalogue; the fns lanes since
then grew the image to 98587 B (docs/worklog/samirfns-fourth-launch.md:
7,780 B recovered, 412 B short). Brief: reclaim at least 1,500 B with no
behavioural change, never raise a cap.

Operator ruling 2026-10-24 (initech-9swot; WL-0182) retired the arrival-reclaim
clause. The 4096 B floor against the samir.ld PAL-memory budget remains live.
The measurements and verdicts below record this round's historical policy.

## Measure first

Baseline ELF (build/samir_com/SAMIR.elf at 104f9e7f): .text 94354 B,
.rodata 4219 B (713 strings, 2973 B total -- the catalogue is already
packed, so strings are not where the bytes are), alignment 14 B, BSS
48504 B; 390 text symbols. The sorted symbol table (every text/rodata
symbol > 128 B, 150 rows) is reproduced at the end of this shard; the
head of it:

| B | sym | B | sym |
| ---: | --- | ---: | --- |
| 4816 | xb_call_builtin | 1481 | d_total |
| 2573 | ndx_build | 1457 | lbl_run.constprop.0 |
| 2333 | fl_memfile | 1454 | d_sort |
| 2077 | samir_msg_data (r) | 1414 | tr_format_numeric |
| 1854 | ndx_insert_key | 1317 | eval_binop |
| 1629 | xb_lex.constprop.0 | 1174 | wa_open_impl.constprop.0 |
| 1570 | set_cmd_hook.lto_priv.0 | 1133 | rp_record |

Source-level candidates from the brief were measured and rejected as
not worth a behavioural risk: duplicated strings (`strings -n 3 | uniq -d`
finds 16 short duplicates, < 100 B); the only genuine clone pair is
read_exact + two constprop variants (121 B total); -Os on cold handlers
cannot beat -Oz (-Os is the larger profile); rodata is 4 % of the image.
The mass is .text, so the lever is codegen policy, as in round 1.

## Method: one flag at a time, then the kept subset

Each row is a clean `make -B samir-com` with SAMIR_SIZE_CFLAGS overridden
on the command line (scratchpad try.sh), 4 s per build, GCC 13.3.0
(Ubuntu 13.3.0-6ubuntu2~24.04.1). Baseline 98587 B.

| Flag (alone, on baseline) | B | delta |
| --- | ---: | ---: |
| -mregparm=3 -mrtd | 96699 | -1888 |
| -mrtd | 97051 | -1536 |
| -mregparm=3 | 97819 | -768 |
| -fno-optimize-sibling-calls (not taken: changes stack depth) | 98011 | -576 |
| -fno-ipa-sra | 98331 | -256 |
| -flto-partition=one | 98350 | -237 |
| -flto-partition=none | 98383 | -204 |
| -mtune=generic | 98395 | -192 |
| -fno-forward-propagate | 98459 | -128 |
| -fno-shrink-wrap | 98491 | -96 |
| -fno-defer-pop | 98523 | -64 |
| -fipa-pta | 98555 | -32 |
| -fwhole-program, -fno-ipa-cp-clone, -fmerge-all-constants, -falign-*=1, -fno-unroll-loops, -mtune=i386, -fcf-protection=none | 98587 | 0 |
| -fno-guess-branch-probability | 98651 | +64 |
| -fno-tree-switch-conversion | 98621 | +34 |
| -fno-cse-follow-jumps, -fno-tree-dse | 98779 | +192 |
| -fno-strict-aliasing, -fno-tree-vrp | 98875 | +288 |
| -fno-gcse, -fno-ivopts | 99035 | +448 |
| -fno-expensive-optimizations | 99067 | +480 |
| -fno-thread-jumps | 99131 | +544 |
| -freorder-blocks-algorithm=simple | 99291 | +704 |
| -fno-tree-ter | 99803 | +1216 |
| -fno-ipa-icf | 99995 | +1408 |
| -finline-functions-called-once | 100127 | +1540 |
| -fno-crossjumping | 102619 | +4032 |
| -fno-inline-small-functions | 102658 | +4071 |
| -fno-inline | 102914 | +4327 |

On top of -mregparm=3 -mrtd -flto-partition=one -fno-ipa-sra
-mtune=generic -fno-forward-propagate -fno-shrink-wrap -fno-defer-pop
(95758 B):

| Added | B | delta |
| --- | ---: | ---: |
| -fno-tree-dominator-opts | 94894 | -864 |
| -fno-stack-clash-protection | 95598 | -160 |
| -fno-tree-sra | 95598 | -160 |
| -fno-tree-tail-merge | 95598 | -160 |
| -mstringop-strategy=byte_loop | 95630 | -128 (reverses to +800 with the DOM/SRA set; not taken) |
| -fno-code-hoisting, -fno-tree-slsr | 95694 | -64 |
| -fno-move-loop-invariants, -fno-early-inlining | 95726 | -32 |
| -fno-caller-saves, -fno-rerun-cse-after-loop, -fno-dse, -fno-ipa-profile, -Wl,-O1, --param early-inlining-insns=2, ... | 95758 | 0 |
| -fno-ipa-modref, -fno-tree-fre, -fno-tree-coalesce-vars | 95822 | +64 |
| -fno-split-wide-types, -fira-region=all, -fno-cprop-registers | 95854 | +96 |
| -fno-if-conversion(2) | 95918 | +160 |
| -fno-ira-share-*-slots | 96046 | +288 |
| -fno-tree-reassoc | 96302 | +544 |
| -fno-tree-sink | 96398 | +640 |
| -fno-ipa-cp | 97014 | +1256 |

## What changed (Makefile only, SAMIR_SIZE_CFLAGS)

Three named commits, each a clean build repeated and compared byte for
byte, each with test-samir-size green:

| Step | Commit | Flags added | Before | After | delta |
| --- | --- | --- | ---: | ---: | ---: |
| 1 | e5cfa852 | -mregparm=3 -mrtd | 98587 | 96699 | -1888 |
| 2 | 1ea72b1a | -flto-partition=one -fno-ipa-sra -mtune=generic | 96699 | 96014 | -685 |
| 3 | 02471284 | -fno-forward-propagate -fno-shrink-wrap -fno-defer-pop -fno-stack-clash-protection -fno-tree-dominator-opts -fno-tree-tail-merge -fno-tree-sra | 96014 | 94510 | -1504 |

Total 98587 -> 94510 B: 4,077 B reclaimed this round (brief asked for
1,500). Against the fixed arrival the gate now reads
`samir-size: 94510 B; reclaimed 11857 B; headroom 11842 B (budget 106352 B)`.
Final ELF: .text 90317 B, .rodata 4174 B, alignment 19 B, BSS 48464 B;
366 static + 20 global text symbols, 158 symbols > 128 B. Neither cap,
samir.ld, spec/memory_map.h nor any oracle changed.

## Why each step is behaviour-neutral (Law 1 citations)

Step 1 is an ABI change for the whole image, not a semantic one. GCC 13
manual, x86 Options: -mregparm=NUM passes the first NUM integer args in
EAX/EDX/ECX; -mrtd makes the callee pop its args (ret imm16) for every
non-variadic prototyped function. The policy is safe here because (a)
SAMIR_SIZE_CFLAGS is carried by every TU compile, by SAMIR_LINK (so the
LTO re-codegen sees it) and by all nine COM recipes (grep: 9 SAMIR_LINK
uses, 9 recipes); (b) the libcall helpers (__muldf3, __divdf3,
__fixdfdi, __floatdidf, udivmod64, memcpy/memset) are all compiled from
os/samir sources under the same flags -- -nostdlib links no libgcc; GCC
emits libcalls under the -mregparm/-mrtd convention when the callee type
is non-variadic (i386.cc ix86_return_pops_args / init_cumulative_args),
which is the Linux i386 kernel's long-standing -mregparm=3 arrangement;
(c) there is no varargs function in os/samir (grep `...)`), no setjmp, no
unprototyped declaration or function pointer (grep), and -Wextra's
-Wcast-function-type plus -Werror already forbid mismatched pointer
casts; (d) the one asm caller, boot/samir_crt0.asm, calls
samir_milton_entry(void) -- nothing to pop -- and pal_milton.c's INT 21h
wrappers bind EAX/EBX/ECX/EDX/EDI by constraint, not by convention. The
disassembly shows 117 `ret $imm` sites. Period precedent: Borland's
register (-pr) and `pascal` (callee-pops) conventions.

Step 2: -flto-partition=one removes the 3-way LTRANS split (the
lto-wrapper note in every build log), so no static helper is cloned
across partitions and the link has one fewer ordering input;
-fno-ipa-sra stops the .isra.N parameter-clone copies; -mtune=generic
changes only tuning heuristics. The ISA stays pinned by -march=i386: the
objdump audit of the final ELF finds zero cmov/bswap/xadd/cmpxchg, zero
x87 (the recipe's own x87 count prints 0), zero SSE/MMX/endbr mnemonics
(the WL-0087 Bochs rule).

Step 3 switches off optimiser passes that are size-negative on this
code even under -Oz (forward propagation, shrink-wrapping, deferred
argument pops, DOM-based jump threading and copy propagation, tail
merging, scalar replacement of aggregates). All are pure
transformations with no semantic contract; the oracle set below is the
proof. -fstack-clash-protection is an Ubuntu default that inserts
page-touch probes into large frames; a flat .COM in Milton has no guard
page for the probe to hit, so the probes bought nothing.

Not taken: -fno-optimize-sibling-calls (-576 B) because turning tail
calls back into call+ret deepens the stack on recursive paths
(eval_node / run_block), which is a runtime-profile change;
-mstringop-strategy=byte_loop, whose saving reversed in combination;
-fipa-pta (-32 B, not worth a pass that is still marked experimental).

## Acceptance (final tree 02471284)

Every gate the brief names ran on the final tree, one QEMU at a time,
plus the extra COM-exercising QEMU gates chosen to prove the step-1 ABI
change (writes of doubles to disk, the exact catalogue messages, the
real-III+ DML-02 transcript). Exact tails:

| Target | rc | Tail |
| --- | ---: | --- |
| test-samir-size | 0 | samir-size: 94510 B; reclaimed 11857 B; headroom 11842 B (budget 106352 B) / test-samir-size: green (>=8192 B recovery; >=4096 B headroom) |
| test-samir-size-mutant | 0 | RED SAMIR_SIZE: 98606 B > 98175 B reclaim cap / 102256 B headroom cap (the 4096 B pad) / test-samir-size-mutant: green (SAMIR_SIZE_PAD_4096 RED; image remains below linker budget) |
| test-samir-size-repro | 0 | test-samir-size-repro: green (two clean builds byte-identical): e2b2c49baf5aa6adc5af425751e5b20c1fa9a589308fa9619495d8cdf895eacd  build/SAMIR.COM |
| test-samir | 0 | test-samir: SAMIR foundation green (PAL + rt + value + host binding + spec lock + dbf_ref + ndx_ref) |
| test-dbase | 0 | test-dbase: M6 InitechBase differential GREEN -- round-trip + program diff (100%) |
| test-dbase-diff | 0 | xbase-prog-diff: 8 checks, 0 failures / test-dbase-diff: green |
| test-samir-dml | 2 | fifteen verbs green (clear/macro/close/index/reindex/sort/total/append/save/restore/copyfile/type/rename/erase/label); test-samir-dml-run RED exactly as on main: `RED RUN: PAL has no EXEC seam; loader.c:1193 rejects nested EXEC with LOADER_ERR_BUSY` -- the host leg, compiled with the host cc, which never sees SAMIR_SIZE_CFLAGS; recorded as pre-existing in samir-size-round1.md and samirfns-fourth-launch.md |
| test-samir-core | 0 | samir-core: 286 checks, 0 failures |
| test-samir-fns | 0 | test-samir-fns-index/listoff/recsize/type: green (3 checks, 0 failures each) |
| test-samir-memwidth | 0 | test-samir-memwidth: green (PROG-04/05, DML-02 numeric memvar columns) |
| test-samir-messages | 0 | test-samir-messages: green (deterministic packing; independent catalogue decode) |
| test-samir-boot | 0 | test-samir-boot [5/5]: screendump shows SAMIR's dot prompt + LIST table on the seafoam desktop |
| test-samir-canon-y2k | 0 | test-samir-canon-y2k [4/4]: screendump shows the AR aging report on the seafoam desktop |
| test-samir-lifetime-desktop | 0 | lifetime-desktop: 241 checks, 0 failures / test-samir-lifetime-desktop: green |
| make image | 0 | image: build/initech.img (flagship alias of build/flair_tenants.img -- FLAIR desktop + HELLO/NOTES co-resident tenants) |
| test-samir-write (extra) | 0 | test-samir-write [5/5]: PERSISTED -- independent reader confirms 4 records, rec0 BAL=9999.99, rec3 BAL=5555.55 ON DISK |
| test-samir-core-emu (extra) | 0 | samir-core-emu: 18 checks, 0 failures |
| test-samir-memwidth-emu (extra) | 0 | test-samir-fns: 9 checks, 0 failures / test-samir-memwidth-emu: green (real III+ DML-02 in QEMU) |
| test-samir-messages-emu-mutant (extra) | 0 | SAMIR_SIZE_MUTATE_MSG_PAIR emu RED: FAIL test_samir_core_emu.c:53: CONCAT_PLUS: guest exact manual error 77 / test-samir-messages-emu-mutant: green |

Step 1 alone was also smoke-tested before its commit (test-samir-size
green, test-samir-boot 5/5 on the 96699 B image). Logs:
scratchpad gate-<target>.log; no Bochs leg exists for the SAMIR gates and
none was run. `make test` was not run (brief).

## Frictions

- `-fno-optimize-sibling-calls` is the one sizeable lever left on the
  table (-576 B); it was refused on principle, not measurement.
- The flag set is now eleven policy flags on top of -Oz. Round 3, if
  ever needed, should look at xb_call_builtin (4202 B, one switch over
  every built-in) and ndx_build (2405 B) at the source level rather than
  at more flags; the single-flag table above is exhaustive for GCC 13's
  size-relevant switches and nothing else bought more than 64 B.
- lane worktree .beads is 0775 (bd warning on every commit); not touched.

## Pointers

- Makefile: SAMIR_SIZE_CFLAGS (the only edit), three commits
  e5cfa852 / 1ea72b1a / 02471284 on lane/samirsize.
- harness/diff/dbf_diff/samir_size.mk, test_samir_size.c: unchanged caps.
- docs/worklog/samir-size-round1.md: the round-1 method this extends.

## Appendix: sorted symbol table, every text/rodata symbol > 128 B

Baseline (104f9e7f) -> final (02471284); `?` = symbol absent (inlined,
renamed by LTO, or merged). Produced with `nm --size-sort -S -r` on
build/samir_com/SAMIR.elf.

| final B | base B | symbol |
| ---: | ---: | --- |
| 4202 | 4816 | xb_call_builtin |
| 2405 | 2573 | ndx_build |
| 2187 | 2333 | fl_memfile |
| 2077 | 2077 | samir_msg_data |
| 1837 | 1854 | ndx_insert_key |
| 1504 | 1629 | xb_lex.constprop.0 |
| 1415 | ? | set_cmd_hook |
| 1400 | 1457 | lbl_run.constprop.0 |
| 1377 | 1454 | d_sort |
| 1341 | 1481 | d_total |
| 1327 | 1414 | tr_format_numeric |
| 1270 | 1317 | eval_binop |
| 1106 | 1107 | m_apply_replace_one |
| 1102 | 1133 | rp_record |
| 1055 | 1055 | rp_run.constprop.0 |
| 1047 | ? | query_cmd_hook |
| 1025 | 973 | q_split_clauses |
| 963 | 1174 | wa_open_impl.constprop.0 |
| 930 | ? | dbf_open_common |
| 925 | 985 | exec_command |
| 875 | 898 | d_index |
| 867 | 936 | d_file |
| 846 | ? | rp_load |
| 846 | 772 | apply_relational.constprop.0 |
| 826 | 885 | eval_node |
| 824 | 833 | df_add_sub |
| 819 | 831 | d_append_from |
| 816 | 819 | wa_resolve |
| 814 | 787 | m_split_clauses |
| 800 | 869 | q_render_record |
| 777 | 756 | parse_primary |
| 774 | ? | samir_repl |
| 769 | 794 | p_call |
| 768 | 849 | q_render_val |
| 731 | 753 | q_walk_impl |
| 725 | 757 | m_delete_recall_impl |
| 718 | 756 | q_seek_key |
| 694 | 693 | run_block |
| 687 | ? | proc_cmd_hook |
| 666 | 678 | p_do |
| 656 | 654 | q_render_heading |
| 648 | 672 | dbf_create |
| 608 | 610 | dbf_read_rec |
| 593 | 601 | __muldf3 |
| 589 | 602 | repl_do_use |
| 588 | 595 | rp_header |
| 586 | ? | mutate_cmd_hook |
| 586 | 609 | dbf_append_blank_commit |
| 572 | 590 | ndx_open_impl |
| 568 | ? | fmt_field |
| 564 | 590 | dbf_replace |
| 561 | 567 | fn_transform |
| 554 | 739 | ndx_seek |
| 533 | ? | rp_walk |
| 520 | 527 | m_replace_impl |
| 516 | 524 | m_pack |
| 507 | 540 | do_set_index |
| 502 | 520 | exec_store |
| 486 | 464 | coerce_and_write_field |
| 462 | 495 | dbf_flush |
| 461 | 474 | dbt_read |
| 458 | 446 | df_round_pack |
| 455 | 461 | fn_dtoc_impl |
| 453 | 451 | split_lines.constprop.0 |
| 450 | 472 | q_locate_impl |
| 448 | 467 | fn_ctod_impl |
| 432 | 431 | __divdf3 |
| 423 | 477 | ndx_delete_key |
| 418 | 443 | dec_format |
| 406 | 488 | xb_interp_make |
| 398 | 400 | samir_milton_entry |
| 393 | 450 | ndx_read_node |
| 374 | 390 | repl_load_prg.constprop.0 |
| 372 | 392 | dec_parse |
| 353 | 357 | fn_stuff |
| 349 | 350 | line_classify |
| 348 | 358 | samir_screen_status |
| 343 | 348 | eval_call |
| 341 | 337 | q_go |
| 337 | 349 | milton_conout |
| 336 | 362 | dbt_append |
| 330 | 325 | flow_expand |
| 301 | 332 | p_split_prompt_to.constprop.0 |
| 299 | 312 | repl_render_error |
| 296 | 298 | q_question |
| 295 | 282 | q_next_expr.constprop.0 |
| 294 | 294 | memvar_set |
| 292 | 300 | dbf_append_rec.constprop.0 |
| 290 | 309 | m_append |
| 289 | 302 | memvar_reclaim |
| 286 | 289 | do_set_filter |
| 283 | 297 | samir_do |
| 279 | 286 | repl_do_close |
| 276 | 299 | ensure_seq |
| 275 | 302 | milton_status |
| 273 | 274 | do_set_date |
| 269 | 285 | q_locate_scan |
| 266 | ? | parse_num |
| 265 | 269 | q_render_field.constprop.0 |
| 261 | ? | inorder_page |
| 259 | 276 | fn_type |
| 258 | 302 | nav_raw_skip |
| 256 | 256 | samir_msg_pairs |
| 255 | 260 | run_docase |
| 254 | 256 | xb_interp_declare_private |
| 246 | 260 | fn_maxmin |
| 236 | 242 | m_encode_key |
| 235 | 244 | wa_visible |
| 234 | 239 | m_confirm_file.constprop.0 |
| 230 | 206 | rp_total |
| 225 | 237 | xb_interp_declare_public |
| 225 | 236 | ndx_rebuild |
| 223 | 253 | samir_line_read.constprop.0 |
| 221 | ? | wa_touch_record |
| 221 | 228 | __floatdidf |
| 217 | 239 | proc_run |
| 208 | 212 | name_key |
| 204 | 235 | delete_from_leaf |
| 202 | 214 | p_wait |
| 200 | ? | insert_into_leaf |
| 194 | 198 | parse_not |
| 193 | 195 | milton_terminal_line |
| 190 | 188 | __fixdfdi |
| 187 | 209 | wa_nav_skip |
| 186 | ? | derive_dbt_path.constprop.0 |
| 184 | 184 | udivmod64 |
| 179 | ? | split_verb.constprop.0 |
| 177 | 175 | ymd_from_jdn |
| 177 | ? | wa_close |
| 174 | 190 | flow_resolve |
| 172 | 197 | fn_dmy |
| 171 | 204 | df_cmp |
| 168 | 168 | p_input |
| 163 | ? | wa_refresh |
| 163 | 131 | df_unpack2 |
| 162 | 168 | q_eval |
| 162 | 168 | p_eval |
| 162 | 168 | m_eval |
| 162 | ? | eval_expr |
| 159 | 164 | d_destination |
| 157 | 161 | dbt_open_impl.constprop.0 |
| 154 | ? | ndx_key_cmp |
| 152 | 165 | xb_interp_eval_str |
| 152 | 152 | pow10_tbl |
| 145 | 145 | m_delete_recall |
| 142 | 127 | p_accept |
| 141 | 126 | repl_mutation_error |
| 137 | 147 | d_word |
| 134 | ? | milton_cursor |
| 133 | 136 | wa_nav_goto |
| 132 | 150 | repl_verb_eq |
| 132 | ? | nav_raw_go_top |
| 132 | 134 | fn_case |
| 131 | 126 | xb_parse.constprop.0 |
| 130 | 126 | wa_name_is_open |
| 130 | 139 | p_extract_body.constprop.0 |
| 129 | 145 | q_seek_visible_visit |
| 129 | 130 | m_replace |

Baseline-only symbols > 128 B (absent from the final table):

- set_cmd_hook.lto_priv.0 (1570 B)
- query_cmd_hook.lto_priv.0 (1103 B)
- dbf_open_common.lto_priv.0 (986 B)
- rp_load.lto_priv.0 (835 B)
- samir_repl.isra.0 (776 B)
- proc_cmd_hook.lto_priv.0 (733 B)
- mutate_cmd_hook.lto_priv.0 (624 B)
- fmt_field.lto_priv.0 (596 B)
- rp_walk.lto_priv.0 (550 B)
- parse_num.lto_priv.0 (292 B)
- inorder_page.lto_priv.0 (267 B)
- wa_touch_record.lto_priv.0 (231 B)
- split_verb.constprop.0.isra.0 (221 B)
- insert_into_leaf.isra.0 (210 B)
- eval_expr.lto_priv.0 (168 B)
- wa_close.isra.0 (165 B)
- ndx_key_cmp.isra.0 (162 B)
- wa_refresh.isra.0 (160 B)
- nav_raw_go_top.lto_priv.0 (138 B)
