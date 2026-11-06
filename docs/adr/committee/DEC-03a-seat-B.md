<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0009 Amendment DEC-03a -- Committee Memo, Seat B (SAMIR / oracle / publications / desktop / W4 re-grade)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Committee Working Paper (non-binding until chair synthesis)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0009-A2-SEAT-B |
| Bead | initech-yndl2 |
| Seat | B -- SAMIR, oracles, publications, desktop; the W4 re-grade. Seat A (period plausibility, loader, memory map) is covered elsewhere. |
| Question put | Operator ruling 2026-11-06: InitechMZ image + heap may load above 1 MB; SAMIR.OVL stays as it is; packaging B and the shrink programme are retired. |
| Evidence base | Read-only file readings (Makefile, samir_{size,ovl}.mk, merge_samir.mk, os/samir, kmain.c, memory_map.h, pubs/, ship plan, beads export, ../dbase3-decomp/specs). Nothing was built or run. |

The seat finds no objection to the ruling. It finds places where careless execution would weaken an
oracle without anyone deciding to, and one (F-5) where the premise ("a console-window tenant") is not
what the code does. They are recorded so the amendment decides them on purpose.

---

## 1. Findings

**F-1. The size gates are one arithmetic, and it is the arithmetic being retired.**
`harness/diff/dbf_diff/test_samir_size.c` fixes BUDGET = ENV_BLOCK - PROGRAM_IMAGE -
PROGRAM_BSS_RESERVE - PAL_ARENA - 0x20 = 93,776 B (enum at :33-36; exits 2 if it ever differs,
:67), caps the resident module at BUDGET - 4096, and `samir.ld` repeats the same sum as two
link-time ASSERTs (64 KiB reserve; `_samir_load_end + 0x10000 + 0x8090 + 0x20 <= 0x6F000 -
0x40100`). Today: module 88,192 B, headroom 5,584 B (WL-0194; the 91,296 B in the brief is
the file with MZ header and relocations). Precedent: WL-0182 (initech-9swot) retired the
98,175 B reclaim clause and kept the 4 KiB floor. DEC-03a is the second act of the same kind and
removes the floor's premise.

**F-2. Not every gate with "size" in its name is a size gate.** `samir_size.mk` also holds
`test-samir-messages`, `test-samir-memwidth(-emu)(-mutant)` and `samir-size-goldens`; their
`SAMIR_SIZE_MUTATE_*` mutants are behavioural (message pairing, memvar column width against
real-III+ captures). An executor who retires by name-grep would delete real-dBASE oracles.
`test-samir-size-emu(-mutant)` is likewise an alias for boot / write / canon-y2k.

**F-3. The overlay machinery has no load-address dependency.** The manager relocates by the
linked symbol `__samir_base` (os/samir/core/ovl.c:121, :366), not by a constant. The only literal
addresses are the two `samir.ld` ASSERTs, `test_samir_size.c`, and one pure-arithmetic vector in
`test_samir_ovl.c:78-80` (any base is valid there). `e_minalloc = 2058` (Makefile:29029) is a
loader contract pin, not a squeeze, and stays meaningful in a high window.

**F-4. New code should NOT use `SAMIR_OVL_TEXT` -- and for W4 it cannot.** The packer forbids
overlay code that reaches `samir_do`, `samir_repl` or `samir_ovl_call` (Makefile:29024, mutant
REENTER in `test-samir-ovl-static-mutant`), because the manager refuses a swap while an overlay
runs (`ovl.c:386`, "live swap"). ASSIST, BROWSE-with-commands and any app that executes dot
commands re-enter the interpreter, so under the current rule they are resident by construction.
The "nested-swap" question parked on initech-1x6x2.18 (`samir_msg_data`) is the same wall seen
from the other side; with the squeeze gone it need not be answered. Hence: the six existing
overlays (file, report, status, set, memfile, forms; slot 33,792 B; 131 cold functions) stay
untouched as period flavour; new code is resident by default; an author may still overlay a
genuinely cold, leaf-only command, but nothing requires it.

**F-5. SAMIR is a class-2 TEXT tenant, not a console-window tenant.** ADR-0013 Sec 3.2 and
`os/milton/kmain.c:3059` (`flair_launch_text_tenant`): the hotkey SUSPENDS the pump, SAMIR takes
the whole console, and the desktop is re-presented from the offscreen on return. `os/flair/process.c`
contains no reference to SAMIR; the AC-2 split arena (RECORDS/DATA per `FlairApp`) is not touched
by a text tenant. The windowed SAMIR front-end is a separate, parked item (initech-e7qc). The
amendment should say "text tenant" or it will mis-scope the gates in F-6.

**F-6. What a high image means for the desktop is one rule: disjointness from live FLAIR state.**
The O-7 gate (Makefile:25126-25160) proves the desktop returns bit-identical because SAMIR runs
"ENTIRELY below 1 MiB ... DISJOINT from the FLAIR-heap offscreen" and the offscreen and tenant
arenas live in [0x100000,0x500000) across the suspend (kmain.c:3040-3046). A SAMIR window inside
that range, or inside the kernel runway [0x500000,0x600000) (kernel, TRANSIENT 0x580000, FLAIR.OVL
0x5C0000, stack 0x5F0000), corrupts the desktop. The smallest legal home is therefore at or above
KERNEL_CEIL (0x600000), which is above the boot RAM gate (INITECH_MIN_EXT_KB = 5120, memory_map.h:168,
kmain.c:6191). A new locked window needs a RAM-floor decision (Open question Q1). QEMU defaults and
Bochs `megs: 32` (harness/emu/bochs.c:177) back it; the floor is a contract question, not an
emulator one.

**F-7. Exit / Restart / Shut Down.** Exit to InitechDOS discards the FLAIR heap and tenants and
loads the transient shell at 0x580000; a later SAMIR launch from the prompt uses the same high
window (no conflict if the window is above 0x600000). Restart and Shut Down are cold paths
(`power_cold.c`) and carry no state. One real hazard: `kstart.asm` zeroes only the runway, so a
high window is zero on an emulator and arbitrary on a real 486 at power-on. SAMIR's crt0 zeroes
its own BSS (ADR-0009 DEC-05) but the PAL heap block is not zeroed; today's conventional window
is also reused across runs (test-samir-lifetime-desktop relaunches), so this is not new, but the
loader should zero the bound arena (Seat A) and a two-boot determinism check should say so.

**F-8. The real-dBASE oracle does not know where SAMIR lives.** `test-merge-samir`
(harness/merge_samir.mk) is every `test-samir*`, `test-flair-samir-*` and `samir-*-goldens` gate
less the Bochs legs. They grade (a) stdout transcripts byte-identical to real III+ (prog_diff, DML,
fns, canon-y2k, salami); (b) screens cell-by-cell via `ppm_text_check` against the real
G2TRV.TXT console projection; (c) files: DBF/DBT/NDX bytes after `dbf_normalization.json`,
`test-ndx-incremental` against 516 sha256-pinned real snapshots, REPORT FORM output files against
real mint 004; (d) the message catalogue. None encodes a load address. Host-vs-target:
`FLOW_MAX_REGISTRY` and `FLOW_MEMVAR_ARENA` differ by profile (flow.c:58-59: host stress 16384, target
6000). **6000 is the real III+ memvar pool (what LIST MEMORY reports), a semantic limit, not a
squeeze; it must not be raised because room appeared.** No `MEMORY()` RAM-size function exists in
the III+ inventory (grep of ../dbase3-decomp/specs finds none), so no III+ observable depends on
installed RAM. Error #43 paths (flow.c:2146, set.c:173) are fail-loud, not graded by size.

**F-9. The squeeze does not vanish; it moves to the kernel.** HANDOFF 2026-11-06: largest
kernel 167,360 B under a 169,984 B cap (2,624 B margin); the parked tsr-exec-relocation lane
(initech-h4bk) has unknown kernel-size impact. A loader change to bind a high window must land
inside that margin or the cap must be re-ruled. The amendment should say so, or the grind merely
changes address.

**F-10. The size CFLAGS are residue, not law.** `SAMIR_SIZE_CFLAGS` (Makefile:28947-29002; -Oz,
-fno-* passes, -mregparm=3 -mrtd, LTO one partition) were bought byte by byte. The ABI flags are
shared with `samir_crt0.asm` and `pal_milton.c` and must not be touched casually. Recommendation:
freeze, make no further size rounds, and defer any -O2 flip to its own bead (Q4).

## 2. Gate dispositions

"Retire" removes the check and records the reason here; none is silent. "Informational" prints,
never fails.

| Gate / check | Where | Disposition | Reason |
|---|---|---|---|
| Resident <= BUDGET - 4096 ("RED SAMIR_SIZE") | test_samir_size.c | **Retarget**: print headroom as information; fail only above a runaway ceiling (chair sets, Q3; suggest 256 KiB module) | Floor premise (no room) retired; a ceiling still catches a pathological link |
| `BUDGET != 93776` contract exit 2 | test_samir_size.c:67 | **Retire** | Pins the retired window arithmetic |
| Slot == largest overlay rounded to 16, every overlay fits ("RED SAMIR_SLOT") | test_samir_size.c | **Keep** | Overlay-format correctness, not size pressure |
| MZ tagged InitechMZ; `e_minalloc` == PAL block + MCB (2058) | test_samir_size.c | **Keep**, re-expressed against the new loader contract | Loader must still enforce the arena SAMIR assumes |
| test-samir-size-mutant: PAD floor-1 RED / floor GREEN | samir_size.mk | **Retarget** to the new ceiling (ceiling+1 RED, ceiling GREEN); SLOT mutant **Keep** | Rule 6: the retargeted gate must still bite |
| test-samir-size-repro | samir_size.mk | **Keep** | Rule 11, not a size check |
| test-samir-size-emu, -emu-mutant | samir_size.mk | **Keep** (rename optional) | Aliases boot / write / canon-y2k |
| test-samir-messages, -memwidth, samir-size-goldens (+mutants) | samir_size.mk | **Keep -- do not retire by name** | Real-III+ captures (F-2) |
| samir.ld ASSERT 64 KiB BSS reserve | samir.ld | **Retarget** to the size of the new window | Same invariant, new constant |
| samir.ld ASSERT module + PAL block <= 0x6F000-0x40100 | samir.ld | **Retire**, replace with an ASSERT against the new window length | Retired arithmetic |
| samir.ld NOCROSSREFS, NOCROSSREFS_TO; packer --forbid / call-graph | samir.ld, Makefile:29024 | **Keep** | Correctness of overlays (F-4) |
| test-samir-ovl, -mutant, -static(-mutant), -emu(-mutant), -swap-emu(-mutant), -guard-emu-mutant, -lifetime, -output, mzlink-io | samir_ovl.mk | **Keep** all | Overlay stays as is |
| SAMIR_SIZE_CFLAGS policy | Makefile:28947 | **Keep, frozen** | F-10 |
| test-flair-samir-suspend, -mutant, -bochs | Makefile:25179 | **Keep, re-prove** | F-6 |
| test-samir-lifetime-emu(-mutant), -desktop(-mutant) | Makefile:29068, 29103 | **Keep, re-prove** | Relaunch through hotkey and COMMAND.COM |
| test-arena-disjoint(-mutant) | Makefile:28762 | **Keep, extend**: add a high-window case; the overlap mutant must still go RED | Asserts the arena is disjoint from image and stack |
| test-memory-map(-mutant), test-hardware-spec(-mutant) | Makefile:28858, 28786 | **Strengthen**: new window pairs against FLAIR heap, kernel, TRANSIENT, OVERLAY, stack; a hardware.json row; RAM floor | Locked-data act (Rule 8), stricter not looser |
| Kernel size cap 169,984 B gates | Makefile (kernel size family) | **Keep** | F-9 |
| test-samir-boot, -write, -canon-*, -dml-*, -fns-*, -core-emu, -report-emu | many | **Keep, re-run** | Behaviour; load address invisible (F-8) |

## 3. Publications impact

Because SAMIR.OVL stays, most of the books do not change. Sentences that would:

- **Unchanged:** `initechbase-volume-one/g1-installing.tex:51-56` (SAMIR.EXE reads parts of itself
  from SAMIR.OVL; "Insert System Disk ..." prompt); `u4-environment.tex:53-56` ("InitechBase takes
  the memory it needs when it starts"); message 43 in `u7-entries.tex:282`; SCREENS line 3.
- **Change only if the RAM floor rises (Q1):** the requirements sentence "At least 5 MB of memory:
  the first megabyte and 4 MB of extended memory" in `initechdos-users-guide/ch1-introduction.tex:64`,
  `flair-getting-started/ch2-setup.tex:25`, `flair-users-guide/appb-requirements.tex:18`, and the
  trouble entry `flair-getting-started/ch5-trouble.tex:57`; plus their Law-1 comment blocks, which cite
  FLAIR_HEAP_MIN. **Pre-existing drift:** the kernel actually gates at INITECH_MIN_EXT_KB = 5120 KiB
  extended (6 MB total), not 4 MB extended; the docs lane should correct both in one pass.
- **Screens:** `initechdos-reference/screens/chkdsk.txt`, `chkfile.txt` and
  `initechdos-users-guide/screens/chkdsk.txt` print "118160 bytes total memory / free" from the live
  MCB chain (chkdsk.c:339-343): CHKDSK is a flat .COM, so they change only if the arena window of flat
  .COMs changes. **Restrict the high window to InitechMZ images and these three screens are untouched.**
  `initechbase-volume-one/screens/gs-dir.txt` ("1294336 bytes free") is DISK free and moves with every
  change to the byte size of SAMIR.EXE/SAMIR.OVL on the samir diskette (pubs.mk:269-270); the re-mint
  is already owed (WL-0194), and growth from here makes it a standing per-session chore, not a defect.
- **Gates that go red:** `test-pubs-screens` (gs-dir; chkdsk screens only if the flat window moves);
  `test-pubs-lockstep-src` stays green (it checks shipped names against .tex entries, not memory);
  `test-pubs-tps` and `test-pubs-examples` are unaffected unless an example transcript prints a memory
  figure; `test-hardware-spec` is named in the requirements chapters' `\Gates{}` lines.

## 4. W4 re-grade -- replacement acceptance (operator decision to encode)

The full-screen apps (initech-j7lld) are NOT graded byte-exact against real III+ screen dumps.
Three layers replace that, applied to every app: **(L1)** data-effect oracle -- the files an app
writes are byte-exact (after `dbf_normalization.json`), because real dBASE must read them (format
specs in ../dbase3-decomp/specs/file-formats); **(L2)** a locked injected key-trace replay with a
structural screen check (named strings and regions present, cursor/highlight moves, status line
content), each check mutation-proven (Rule 6); **(L3)** the Rule 14 record-flair clip in
`build/clips/`, repro-proven, for the operator's eye. Screen spec: specs/full-screen/assist-ui-and-editors.md.

| App (spec section) | L1: data-effect oracle | L2: key-trace structure | Byte-exact screen kept? |
|---|---|---|---|
| BROWSE (Sec 5) | .DBF/.DBT/.NDX after edits, same trace run on real DBASE.EXE via the mint harness | F1 menu toggles; arrows/PgDn move cell and record; status line shows record n; Ctrl-End saves | No -- pure behaviour |
| EDIT / CHANGE / APPEND (Sec 4) | .DBF (and .DBT for memo edit); Esc abandons == file unchanged; Ctrl-W saves | field navigation; key-help status line; APPEND adds exactly one record | No; shares the W3 READ screens, which stay byte-exact where already minted |
| CREATE / MODIFY STRUCTURE (Sec 6) | .DBF header and surviving records after MODIFY (dbf.md) | field table rows, type cycling, Ctrl-End save / Esc abandon | **Yes** for the empty CREATE table (static, Sec 6.1 verbatim from HELP.DBS) |
| MODIFY COMMAND (Sec 9) | .PRG bytes after a locked edit trace, plus any backup file the mint shows | text entry, cursor keys, save/abandon, status line | No |
| REPORT designer (Sec 8.1) | .FRM bytes (frm.md); real-made .FRM re-saved unchanged is identical; SAMIR-made .FRM run by real REPORT FORM gives SAMIR's own output (existing test-samir-report-emu) | designer sections reachable; box edits | No |
| LABEL designer (Sec 8.2) | .LBL bytes (lbl.md), same two round trips | label dimensions form | No |
| QUERY / SCREEN / VIEW (Sec 7) | .QRY (qry.md), .SCR plus generated .FMT (scr-and-fmt.md), .VUE (vue.md); generated .FMT run under SAMIR and real III+ yields the same W3 form screen | filter form rows; painter cursor and draw keys | The generated-.FMT form screen, **yes** (it is a W3 screen) |
| ASSIST (Sec 1-2) | the commands ASSIST issues, as a transcript, plus resulting files, equal real III+ for a locked menu walk | pad order Set Up, Create, Update, Position, Retrieve, Organize, Modify, Tools (Sec 2.1-2.8) | No -- pure behaviour |
| HELP (Sec 3) | none (read-only) | topic index complete vs HELP_topics.txt (Sec 0); F1 overlay, PgUp/PgDn, ENTER> prompt | Main menu page and a few sampled topic pages, only where the mint captures them cheaply and the text is InitechBase-authored (Q5) |
| CONFIG.DB (config-db.md) | `DISPLAY STATUS` / `LIST MEMORY` **transcripts** after startup with a given CONFIG.DB equal real III+ (transcripts, not screens) | n/a | Transcripts yes; no screens |

Rule 14 applies to all ten. Layer L2 is the new load-bearing layer; a structural check that has
never gone red is decoration, so each carries a named mutant (drop the status line, break Tab
order). The amendment should also record the class itself as locked data
(`spec/samir/screen_regrade.json`, one row per app naming its three layers), so the W5 ship gate can
verify the list rather than trust prose, and so the weakening from byte-exact is a recorded Rule 8
act and not a silent one.

## 5. Bead and plan dispositions

| Bead | Disposition |
|---|---|
| initech-5etom (packaging epic) | Close as delivered in part: AL=03h (DEC-19) and Phase B (5etom.1, closed) shipped. Remaining scope -- multi-overlay, per-image gates, DBASE.EXE/OVL parity -- retired by DEC-03a. |
| initech-1x6x2.18 (shrink candidates, `samir_msg_data` nested swap) | Close won't-do, superseded. Siblings .6 and earlier size rounds are already closed; no other open size child found under 1x6x2 by title (re-run `bd list` to confirm). |
| NEW: retarget samir size gates and samir.ld ASSERTs | File as the implementation bead for Sec 2. RED-first: a link over the old window must go green, over the ceiling red. |
| initech-9x8cn (W4.1 kit) | Stays. Acceptance text re-graded to L1/L2/L3; the W4.0 full-screen mint harness is demoted to optional (HELP and STRUCTURE goldens only). |
| initech-j7lld (W4 apps epic) | Stays, description rewritten to Sec 4; children inherit the per-app rows. |
| initech-y29t4 (W5 ship) | Wording: "shipped-with-real-golden" unchanged for verbs, functions, SET options, messages; full-screen apps accepted per DEC-03a Sec 4 and listed in `screen_regrade.json`; the ship certificate states the non-byte-exact class explicitly. |
| initech-p3nn0 (W3 forms) | Unchanged; GET/READ/@ screens keep the byte-exact `test-samir-forms-emu` machinery (Q5). Slices 2-4 are the next critical-path work. |
| initech-h4bk (TSR relocation) | Cross-link: relocating InitechMZ above residents becomes a natural fit for the high window (Seat A). |

Replacement critical path (supersedes ship plan Sec 3):
1. W3 slices 2-4 (GET, READ, PICTURE/FUNCTION/RANGE, CLEAR GETS) -- finish PRD 6.6 forms, byte-exact.
2. DEC-03a lands: window constant, RAM floor, loader bind (Seat A), size-gate retarget, suspend re-proof.
3. W4.1 kit (key loop, menus, field editor, status line), graded at L1/L2/L3.
4. W4 apps in big strokes, up to four concurrent lanes, no overlay work and no shrink rounds.
5. W2a/W2b/W2c fan-outs on merge trains; they no longer wait on packaging.
6. W5 sweep and ship gate: bugs, message reachability, `screen_regrade.json` check, full vector + Bochs.
7. Capacity freed from packaging and shrink goes to Turbo Initech (M7/M8 fixpoint) and FLAIR fidelity.

## 6. Risks

1. **Kernel cap (F-9).** 2,624 B of margin; the loader change and the parked tsr-exec lane compete for it.
2. **Silent oracle weakening.** The size floor and the W4 byte-exact bar both go away. Both are recorded
   here and must be recorded in the amendment; the retargeted ceiling and L2 mutants are what keep them gates.
3. **RAM floor.** A new window above 0x600000 changes the hardware contract, `hardware.json`, three
   requirements chapters, and exposes the 4 MB / 5 MB manual drift.
4. **Determinism on real hardware (F-7).** High window contents are arbitrary at power-on.
5. **L2 can rot.** Without named mutants, structural checks pass against broken screens; review should
   refuse an L2 check without its mutant. Name-grep retirement (F-2) is the other likely executor error.
6. **FLOW_MEMVAR_ARENA drift.** Raising 6000 "because there is room" would break LIST MEMORY parity.
7. **Honest labelling.** SAMIR.OVL is now period flavour by choice, not necessity; DEC-03's wording
   ("authentic end-state", "loading above 1 MB REJECTED") must be amended to say so (Law 3 honesty).
8. **Diskette capacity.** A larger resident still has to fit the 1.44 MB System diskette the manuals describe.

## 7. Open questions for the chair

- **Q1.** Address, length and RAM floor of the high window (this seat suggests starting at KERNEL_CEIL,
  0x600000, with INITECH_MIN_EXT_KB raised to match). Who edits the three requirements chapters?
- **Q2.** Does the window apply to InitechMZ images only (recommended: leaves CHKDSK screens and flat
  .COM behaviour alone) or to every program?
- **Q3.** The runaway ceiling for the retargeted `test-samir-size` (suggest 256 KiB module; or none).
- **Q4.** Freeze `SAMIR_SIZE_CFLAGS` (recommended) or schedule an -O2 return as its own bead?
- **Q5.** Does the re-grade stop at W4 (recommended; W3 keeps byte-exact screens where already minted),
  and may HELP pages be byte-exact if their text is InitechBase-authored rather than III+ verbatim?
- **Q6.** Rule on new overlays: "none by default" (recommended) or "forbidden"?
- **Q7.** Is the kernel-cap interplay (F-9) ruled here or in a sibling? **Q8.** Accept
  `spec/samir/screen_regrade.json` as the locked record of the non-byte-exact class?

*-- End of Seat B memo --*

<!-- Tedium certified compliant with NFR-7. The floor was removed; the ceiling was installed; the overlay you do not need is retained in full. -->
