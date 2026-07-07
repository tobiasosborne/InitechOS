<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0068 -- The SAMIR/xBase P1 correctness cluster: 12 bugs + a discovered Law-2 heresy (orchestrated, 7 lanes)

**Type:** post-GUI-arc correctness push. Orchestrated (7 parallel worktree lanes; committee not needed -- one design fork pre-resolved, one golden heresy surfaced + fixed at integration).
**Date:** 2026-07-07.
**Beads closed:** initech-3p9e, -ue7y, -wcb7, -qw4e (query); -0g22, -x87g (ndx/PACK); -9u0f, -eyig (fn); -jf8p (dbf); -dmrw (nav); -2hg9 (seed lexer); -winh (MILTON); -bljs (prog_diff golden heresy #2).
**Beads filed:** initech-cq0j (P3, follow-up: LOCATE NEXT n EOF()/RECNO() bounded-miss oracle vs real III+).
**Commits:** base `a8a26fa`; 13 fix commits across 7 merged lane branches + 1 integration commit; tip `a4d1bd9` (see ledger below).

## Context

WL-0067 closed the FLAIR GUI fix arc. The next-most-consequential remaining work was NOT the
GUI P2 tail but a large, coherent cluster of **P1 correctness bugs in SAMIR** (the InitechBase
dBASE-III+ engine) filed during the WL-0066 bug hunt, plus two adjacent P1s (seed lexer, MILTON
COMMAND.COM). These are correctness bugs in shipped subsystems that the canonical app suite
(Initech123/InitechWord) will build on -- and one (`3p9e`) was flagged as an **oracle heresy**
(a by-construction Law-2 violation), the project's cardinal sin. Twelve bugs total; each bead was
already adversarially-verified with exact file:line + root cause.

## Method (orchestrated; reuse the pattern)

Mapped the 12 bugs to **7 conflict-free lanes** by file ownership (every .c disjoint; only the
Makefile was a shared merge surface). Each lane ran in its own **git worktree** off `a8a26fa`,
bound to Red->Green->mutation-prove with an **independent (non-by-construction) oracle** grounded
in `../dbase3-decomp`. 2 opus + 5 sonnet dispatched in parallel (within the operator's caps).
The orchestrator integrated each lane on the main tree ONE AT A TIME, **re-running every
subsystem oracle itself** (never trusting a lane's self-report; Rule 4) + an ASCII scan, then the
full gate at the end.

- 5 sonnet lanes (F seed-lexer, E nav, D dbf, G MILTON, B fn_builtins) completed and integrated
  clean.
- Both **opus lanes hit the opus weekly rate limit mid-flight**. Committed partial work was
  preserved on-branch (A: 3p9e/ue7y/wcb7 done; C: 0g22 done + x87g WIP uncommitted). Two
  **sonnet finisher** agents took over the worktrees in place and completed `qw4e` (A) and `x87g`
  (C). The orchestrator verified the already-committed opus work end-to-end itself.

## What changed (13 fixes, each mutation-proven)

1. **3p9e (query, THE HERESY).** `?`/`??` echoed logicals as bare `T`/`F` via the same
   `q_render_val` LIST columns use; the repl oracle asserted `cap_has("\nT")` -- agreeing with the
   wrong code by construction. Fix: `q_render_val` gains `logical_dotted`; `?`/`??` echo dotted
   `.T.`/`.F.` (III+ nav-query-display.md L.803), LIST/DISPLAY columns keep bare `T`/`F` (L.689);
   the oracle de-heresied to assert `.T.`/`.F.` (which the old code FAILS -> a real oracle now).
2. **ue7y (query).** Date render hardcoded AMERICAN MM/DD/YY, ignoring SET DATE/SET CENTURY. Fix:
   honor `ctx->set_date_fmt` (BRITISH DD/MM/YY etc.) + `set_century` (4-digit year).
3. **wcb7 (query).** LOCATE RECORD n scanned n..EOF. Fix: RECORD n tests ONLY record n; FOR false
   -> FOUND()=.F., cursor stays at n (separate `record_scope` branch).
4. **qw4e (query).** LOCATE NEXT n scanned to EOF, ignoring n. Fix: `q_locate_scan` gains a
   `max_visits` bound (mirroring `q_walk`'s NEXT-n bound); coexists with wcb7's record-scope path.
5. **0g22 (ndx).** `ndx_delete_key` descent stopped at the leftmost equal-key leaf -> entries in
   later equal-key leaves undeletable. Fix: `delete_from_leaf` helper + descent that WALKS the
   equal-key run across consecutive leaves (ndx.md sec.5/6: no recno tiebreak).
6. **x87g (ndx/PACK).** PACK renumbered survivors but never updated open NDX -> stale recnos. Fix:
   full REINDEX -- `ndx_rebuild` (release fd -> `ndx_build` preserving key expr/geometry ->
   reopen); `m_pack` enumerates open NDX via `wa_index_count`/`wa_index` and rebuilds each with a
   physical-GOTO `get_key` provider over the packed table. (Per-recno delete is INSUFFICIENT:
   renumbering invalidates survivor recnos too -- the pre-ratified design.)
7. **9u0f (fn).** CTOD validated only range (dd<=31); Feb 31 etc. rolled forward. Fix:
   `xb_days_in_month` + leap rule; calendar-invalid -> blank date (JDN 0.0).
8. **eyig (fn).** ROUND used `(int64_t)` truncation-toward-zero -> wrong for negatives. Fix:
   exported `rt_floor64` (was rt.c-static; now declared in rt.h), both `fn_round` arms floor.
9. **jf8p (dbf).** `dbf_open_rw` normalized header_length to +1 form at open, breaking +2-form
   file reads (BANK/TAX.DBF). Fix: keep the true on-disk header_length; move the +1 normalization
   INTO `dbf_flush` (where the file is actually rewritten +1) -- matches dbf.h's own "ON FLUSH"
   contract, correct before AND after a flush.
10. **dmrw (nav).** SKIP -1 from virtual EOF landed at nrec-1. Fix: the skip path treats the EOF
    position as the virtual nrec+1 (matching `wac_recno`/RECNO()), leaving `wa_recno`'s raw
    contract intact.
11. **2hg9 (seed lexer).** `scan_string` wrapped the string pool only when `out==0` -> valid
    strings falsely rejected when the pool tail was small. Fix: wrap whenever the cursor isn't at
    pool start, relocating in-progress bytes with `memmove` (FACTORY code -- not the artifact).
12. **winh (MILTON).** `dos_read` lacked a Carry-Flag check -> read errors returned as byte counts,
    garbaging TYPE/COPY/batch. Fix: capture CF with `sbb` (sibling idiom), return a
    `DOS_READ_ERROR` sentinel; TYPE/COPY/batch break on it. New EMU gate drives `TYPE PRN`/`COPY
    PRN` (write-only device -> real CF=1) -- an INDEPENDENT oracle.
13. **bljs (prog_diff golden, Law-2 heresy #2 -- CAUGHT AT INTEGRATION).** After merging `3p9e`,
    `test-dbase-diff` went RED 4/8: the xbase_prog_diff Tier-0 goldens carried bare `T`/`F` on the
    whole-line `?`-echo logical lines (authored from the OLD buggy engine output -- the SAME
    heresy as 3p9e, in a second golden set). Fix: corrected ONLY whole-line `T`/`F` -> `.T.`/`.F.`
    in expr/exact/query/mutate.out (LIST columns e.g. query.out lines 2-5,9,10 correctly stayed
    bare -- proving the 3p9e engine fix is precise); updated the `SWAP_EQ_GOLDEN` mutant to
    perturb the actual value char so the mutation-proof still bites. This is a golden edit -- flagged
    for post-hoc committee/operator review (bead bljs); it STRENGTHENS toward III+ ground truth,
    not weakening.

## Why it matters

Twelve P1 correctness bugs cleared in the dBASE engine + adjacent, an oracle heresy purged in TWO
locations, and the M6 differential hardened. The subsystem is now solid ground for the canonical
app suite. The integration-caught golden heresy (bljs) is the standing lesson: a per-lane green
oracle is not enough -- **a correct engine fix can expose a contaminated golden that only a
composed main-tree re-run reveals** (Lane C ran test-dbase green in its own worktree because it
lacked A's fix). The orchestrator's own composed grading, not lane self-reports, caught it.

## Frictions / traps caught

- **Opus weekly rate limit** killed both opus lanes mid-flight. No work lost (per-bug commits on
  branch); sonnet finishers completed the remainder in the same worktrees.
- **bljs golden heresy** surfaced ONLY on the composed main tree (A + goldens), invisible to any
  single lane. Then the **mutant broke**: `swap_eq_lines` swapped byte 0, which is now `.` in the
  dotted form -> the mutation-proof silently stopped biting until the swap was re-pointed at the
  real T/F char. Both caught by the orchestrator re-running the mutant (Rule 6), not just the gate.
- Lane B exported `rt_floor64` (linkage change, rt.c-static -> rt.h) -- verified `test-samir-rt` +
  full SAMIR link clean.
- **winh x ojxn mutant interaction (caught ONLY by `make clean && make test`).** The `winh`
  dos_read CF fix is a SECOND line of defense against copy-onto-itself data loss (builtin_copy
  aborts on the corrupt same-file read). So the WL-0067 `test-copy-selfcopy-mutant` -- which
  disables ONLY the ojxn same-file guard and asserts data loss -- went RED (winh masked the loss:
  correct defense-in-depth, but the mutant could no longer reproduce the ojxn regression). Fix:
  the ojxn mutant now also builds `-DCMD_MUTATE_NO_READ_CF` so BOTH backstops are removed and the
  original truncate path is restored (HEAD kept, TAIL gone). NOT weakening -- the real ojxn gate
  and winh's own mutant still bite independently. A sub-trap: the Makefile RECIPE change didn't
  rebuild the mutant .o incrementally (mtime unchanged) -- only the CLEAN gate exercises it, which
  is exactly why `make clean && make test` is the authoritative certificate (Rule 11).

## Acceptance

`make clean && make test` = ALL GREEN (see the gate log). Every fix Red->Green->mutation-proven;
every new/modified oracle independent of the code under test (Law 2). Tri-emulator obligations
unchanged (no boot-geometry change this arc; the one new EMU gate, `test-readerr-winh`, is QEMU +
noted pending tri-emulator per initech-x0i). ASCII-clean across all lanes.

## Pointers / next

- **NEXT:** the GUI P2 tail + filed follow-ups from WL-0067 (44ab region cap, hv7u/zn61/vj28,
  Makefile KERNEL_DESKTOP_OBJ, permanent qi8v drag oracle), then the canonical app suite
  (Initech123/InitechWord) on the now-solid GUI + hardened SAMIR.
- **Follow-up filed:** initech-cq0j (P3) -- oracle-verify LOCATE NEXT n EOF()/RECNO() on a bounded
  miss vs real III+ (a nav-query-display.md [oracle-resolves] edge).
- **Review invited:** initech-bljs -- the prog_diff golden de-heresy was orchestrator-verified
  under the wind-up directive; committee ratification of the golden edit is welcome post-hoc.

## Commit ledger (a8a26fa..a4d1bd9 on command-com-default)

- Lane F: `6ed6db9` (2hg9) -> merge `ba78f1c`
- Lane E: `3c99ecb` (dmrw) -> merge `f3b39f6`
- Lane D: `d4613cb` (jf8p) -> merge `9b0a0ad`
- Lane G: `f633983` (winh) -> merge `75470ae`
- Lane B: `6c0cda0` (9u0f) + `87892e8` (eyig) -> merge `823fae8`
- Lane A: `0dae863` (3p9e) + `5b9e214` (ue7y) + `e5e42f4` (wcb7) + `48418d3` (qw4e) -> merge `748f873`
- Integration: `83106e2` (bljs golden de-heresy)
- Lane C: `a3124ba` (0g22) + `a9e2cdf` (x87g) -> merge `a4d1bd9`
- Integration: `bc98821` (winh x ojxn mutant -- disable both backstops so the ojxn mutant bites)
