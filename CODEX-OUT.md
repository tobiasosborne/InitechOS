NDX is merge-ready on lane/wave1-ndx-r, based on fetched origin/main 4aaa0f72. Integration commit: 8f2bc5fc. No push, merge into main, bd, Bochs or full make test was run.

The seven lane commits were replayed in order; lane-brief commit 15222bc2 was skipped:

| Original | Replayed |
| --- | --- |
| 65cf35d0 | e21fff80 |
| 37804a78 | 5653ba77 |
| ab5fca21 | 770b7fd8 |
| 24bbea83 | 9db8da5f |
| fa880e3c | eaf4a6ed |
| 5b56d615 | 3d7fef4b |
| 133537a7 | 1808e757 |

Conflict resolutions:

- Makefile: retained F4 marker-derived lists, W2 rules and main's EXE/OVL machinery. ndx moved PENDING -> ACCEPTED; ndxdisplay has its own ACCEPTED marker. Classifier emulator mutants now build/install EXE/OVL pairs.
- eval.h/workarea.c: retained last_update and appended index_name at the END. PAL slots remain 30; no slot was added. Existing main PAL bindings, including SAY, remain intact; the host curdrive binding pins C: deterministically.
- Manuals: retained generated entry includes; ported NDX into entries/fn/ndx.tex in both manuals. The obsolete monolithic u6-entries.tex stays deleted. Main's set_options.def remains intact.
- fns_mint.sh: kept main's shared mint_capture.sh and added explicit procedure-library setup for programs containing PROCEDURE definitions. All W2 evidence and adapter paths remain intact.
- proc.c/query.c: retained guarded error binding and the STATUS overlay annotation while replaying routine-return clearing and case-preserving filename rendering.
- Function harness: retained completed-trace checks and the unchanged transcript comparator, installed both EXE and OVL, used writable private boot copies for real C: captures, and added per-case locks so classifier mutants cannot clobber one another under -j64. The alternate filename remains GOLD.OUT as in the reviewed lane.

The first merge-vector run also caught index I/O normalization uppercasing native directory names. ndx_io_name now canonicalizes only the file component and preserves directory spelling. Main's independent K05 view and mint-005 USE INDEX/list-layout regressions pass. Display spelling remains separately owned and unchanged.

Placement and lifetime:

NDX(), cursor access, deferred-error/guard completion, procedures, index open/accessors and the copying display-name setter remain resident. INDEX ON / REINDEX and ndx_build/ndx_rebuild execute in FILE; STATUS rendering stays in STATUS; SET INDEX parsing stays in SET. The ELF confirms the resident helpers below __ovl_slot=0x15490 and d_index/ndx_build/ndx_rebuild inside the overlay slot. NOCROSSREFS and the static call-graph gate pass.

Display text is copied into the same resident-owned allocation as ndx_index. The constructor guards the allocation and retained tail pointer; the setter guards the owned destination and copies bytes rather than retaining the FILE/SET caller's pointer. REINDEX preserves that allocation. index_name is covered by samir_bind_cursor's lifetime guard and by the context audit at every overlay boundary. No new API retains a pointer into the slot. Placement is documented in docs/design/samir-overlays.md.

Sizes:

| Item | Bytes |
| --- | ---: |
| Resident load module | 87184 |
| Resident budget | 93776 |
| Headroom | 6592 |
| Headroom above the 4096-byte floor | 2496 |
| SAMIR.EXE file | 90672 |
| Largest overlay / slot (FILE) | 9888 |
| SAMIR.OVL file | 27136 |

Against main's 84528-byte resident, the resident delta is +2656 bytes. PAL slot count and memory-budget assertions are unchanged.

Every make command below used these command-line assignments:
```text
DBASE3_DECOMP=/home/tobiasosborne/Projects/dbase3-decomp
LOTUS123_DECOMP=/home/tobiasosborne/Projects/lotus123-decomp
SYSTEM7_DECOMP=/home/tobiasosborne/Projects/system7-decomp
WIN31_DECOMP=/home/tobiasosborne/Projects/win31-decomp
```

Commands and exit codes:

- git fetch origin main: 0. git switch -c lane/wave1-ndx-r origin/main: 0.
- git cherry-pick of the seven listed commits: conflict stops (1), resolved faithfully; final git cherry-pick --continue: 0.
- Initial make -j8 test-samir-fns-ndx test-samir-fns-ndxdisplay test-samir-size test-samir-ovl-static test-pubs-lockstep-src samir-fns-goldens: 0.
- Cold make -j64 test-merge-samir: 2. The run exposed the directory-normalization regression, unstaged marker-file dependency issue, boot-output snapshot isolation, and emulator infrastructure failures. The source/fixture issues were repaired and staged.
- Repair checks for test-samir-view, test-samir-use and test-samir-list-layout: green; the combined repair invocation initially exited 2 while C: output was still isolated in a disposable QEMU snapshot. After enabling --boot-rw on the private boot copy, both NDX emulator gates exited 0.
- Warm make -j64 test-merge-samir: 0, all 336 gates green.
- Separate make -j8 invocation of all targets listed below: 0.
- Final make test-pubs-lockstep-src after a manual-entry whitespace cleanup: 0.

Separate verification targets:
```text
test-samir-fns-ndx test-samir-fns-ndx-mutant
test-samir-fns-ndx-emu test-samir-fns-ndx-emu-mutant
test-samir-fns-ndxdisplay test-samir-fns-ndxdisplay-mutant
test-samir-fns-ndxdisplay-emu test-samir-fns-ndxdisplay-emu-mutant
test-samir-fns-ndx-deferred-mutant
test-samir-fns-ndx-handler-name-mutant test-samir-fns-ndx-handler-depth-mutant
test-samir-fns-ndx-handler-name-emu-mutant test-samir-fns-ndx-handler-depth-emu-mutant
test-samir-size test-samir-ovl-static test-pubs-lockstep-src samir-fns-goldens
```

Verbatim final verdicts:
```text
>>> test-merge-samir: 336 gates (test-pubs-lockstep-src: yes; no LaTeX)
>>> test-merge-samir: ALL GREEN
samir-size: resident 87184 B; headroom 6592 B (budget 93776 B); slot 9888 B = overlay 0 (9888 B) of 5; SAMIR.OVL 27136 B
>>> test-samir-fns-ndx: green
>>> test-samir-fns-ndxdisplay: green
>>> test-samir-size: green (resident >=4096 B headroom against the samir.ld PAL-memory budget; slot == largest overlay)
>>> samir-fns-goldens: green (SF-index/listoff/recsize/type/errors/struct/unknown/division/numlist/exprcol real III+ captures pinned)
>>> test-samir-ovl-static: green (relocation kinds, slot, headers, call-graph rule; re-pack byte-identical)
>>> SAMIR_FNS_MUTANT_ndx RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndxdisplay RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_deferred RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_handler_name RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_handler_depth RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> test-samir-fns-ndx-emu: green
>>> SAMIR_FNS_MUTANT_ndx emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> test-samir-fns-ndxdisplay-emu: green
>>> SAMIR_FNS_MUTANT_ndx_handler_name emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndx_handler_depth emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> SAMIR_FNS_MUTANT_ndxdisplay emu RED:   FAIL harness/diff/dbf_diff/test_samir_fns.c:231: FNS transcript byte-identical to real III+
>>> test-pubs-lockstep-src: pubs/manuals/initechbase-volume-one: 149 shipped, 149 documented, 2 DRAFT
>>> test-pubs-lockstep-src: pubs/manuals/initechbase-quick-reference: 149 shipped, 149 documented, 2 DRAFT
>>> test-pubs-lockstep-src: green (293 DotExample typed-line checks, 1 Transcript pairs; restricted source grammar, no LaTeX run)
```

Both native III+ goldens are byte-identical to the reviewed lane. All changed text is ASCII; source whitespace checks pass with native golden spacing and mint provenance preserved. The final report commit also removes one blank line at the end of the new manual entry. Existing untracked archive reports and logs were retained.

Logs: build/ndx-merge/initial.log, merge-cold.log, repairs.log, emu-repair.log, merge-warm.log, targeted-final.log and pubs-final.log. No oracle was relaxed.
