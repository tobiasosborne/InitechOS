# SAMIR overlays (InitechBase overlays Phase B)

Bead initech-5etom.1 (epic initech-5etom). Status: operator accepted the design on 2026-10-25 (Sec 10). Phase B is
independent of Phase A and rebased onto main c2ba6e16; review fixes and their
acceptance are recorded in docs/worklog/WL-0185-samir-overlay-review.md.

SAMIR was a flat `.COM` at its size wall (100,777 B against a 102,256 B cap:
the `samir.ld` PAL-memory budget with the 4 KiB headroom floor). This note
records how it became `SAMIR.EXE` (an InitechMZ resident core) plus
`SAMIR.OVL` (overlaid command families), as dBASE III PLUS is `DBASE.EXE`
plus `DBASE.OVL` (ADR-0009 DEC-03 named this the authentic end state).

## 1. Result

| | before overlays | after rebase and review fixes |
|---|---:|---:|
| loaded program bytes | 100,777 (SAMIR.COM) | 82,032 (SAMIR.EXE load module) |
| module budget (Sec 5) | 106,352 | 93,776 |
| headroom over the budget | 5,575 | 11,744 |
| headroom over the 4 KiB floor | 1,479 | 7,648 |
| SAMIR.EXE file | -- | 85,376 (32 B header + 828 relocations + module) |
| SAMIR.OVL file | -- | 26,624 (5 overlays, 512-byte multiple) |
| slot (= largest overlay) | -- | 9,728 |

Historical pre-rebase dry run (not committed): the parked `lane/wave1-say` diff (main...lane/wave1-say:
@...SAY, SET DEVICE, ROW/COL/PROW/PCOL) applies cleanly on top and gives an
81,104 B resident (SET DEVICE lands in the SET overlay, now 2,820 B):
headroom 12,672 B, 8,576 B above the floor. On main it needed 103,246 B
against a 102,256 B cap.

## 2. Ground truth (Law 1)

- **Packaging.** `DBASE.EXE` 133,632 B (MZ) and one raw overlay file
  `DBASE.OVL` 266,240 B holding many segments, each with its own fixup list
  (`../dbase3-decomp/re/ghidra-plan.md` Sec 0.1/0.2;
  `docs/research/overlays-period-ground-truth.md` Sec 1.1/1.2). Every OVL is
  a 512-byte multiple; an overlay matches one release of its EXE (research
  Sec 1.1, Sec 4). WordPerfect 5.1 likewise ships `WP.EXE` beside one
  `WP.FIL` (`../wordperfect51-decomp/mint/work/WP51/`).
- **How DBASE.EXE loads DBASE.OVL.** A byte scan of
  `goldens/dbase-iii-plus-1.1/extracted/disk1/DBASE.EXE` finds no
  `MOV AX,4B03h` and no `MOV AH,4Bh`; the only EXEC is one `MOV AX,4B00h`
  (file offset 0x2CFC, RUN). Next to the `DBASE.OVL` string (0x20010) the
  overlay manager opens the file (`MOV AH,3Dh; INT 21h` at 0x204B4, with an
  FCB fallback `AH=0Fh` at 0x2049C), positions it with `LSEEK AH=42h` to a
  paragraph-scaled offset (0x204CB..0x204FE) and reads with `AH=3Fh`
  (0x20553; FCB fallback `AH=27h` at 0x2053B) -- `ndisasm -b16` of
  0x20480..0x20580. A multi-segment overlay file is read by the program's
  own manager; EXEC AL=03h loads one WHOLE file (IBM DOS 3.30 TR
  p. 6-196/6-197). The only local period binary using `MOV AX,4B03h` is
  Turbo Pascal 5.5 `INSTALL.EXE` (one site).
- **Overlay discipline.** TP5 keeps one overlay buffer and requires FAR
  frames for code active across a swap, "unpredictable and possibly
  catastrophic" otherwise (RG5 p. 160); TP5 routes procedure pointers to
  overlaid routines through stubs (p. 169); a missing overlay is a hard
  failure (p. 162, run-time error 208). The file stays open while the
  program runs (PRG3 R4-4).
- **Missing overlay.** `DBASE.EXE` carries, at file offset 0x186E0, the
  AH=09h string `\r` + 8 spaces + `Insert System Disk 2 and press ENTER, or
  press Ctrl-C to abort.` + 6 spaces. What dBASE does at run time without its
  OVL is not minted (research gap G-1): SAMIR's refusal is modelled on that
  string and claims nothing about dBASE's behaviour.

## 3. What is resident, what is overlaid

Measured on the final link (`nm -S`, `samir_ovl_pack` summary). A family is
cold when it is a whole dot-prompt verb family that real programs do not run
in tight loops; everything an expression, a navigation or a record edit
touches stays resident.

| Overlay (id) | Bytes | Contents |
|---|---:|---|
| FILE (0) | 9,724 | TYPE, RENAME, ERASE, COPY FILE, RUN/!, SORT, TOTAL, INDEX, REINDEX, APPEND FROM, PACK, ZAP (`cmd/mutate.c` d_*/m_pack/m_zap) + `ndx_build`/`ndx_rebuild` (`fs/ndx.c`; only these verbs build indexes) |
| REPORT (1) | 6,604 | REPORT FORM, LABEL FORM (`cmd/report.c`) |
| STATUS (2) | 3,022 | LIST/DISPLAY STATUS, LIST/DISPLAY MEMORY (`cmd/query.c` q_inspect/qi_*) |
| SET (3) | 3,200 | the SET sub-option parser (`cmd/set.c`); the SET state and every getter stay resident |
| MEMFILE (4) | 2,041 | SAVE TO / RESTORE FROM (`cmd/flow.c` fl_memfile) |

Resident (82,032 B): crt0, the PAL (`pal_milton.c`), soft-float, lexer,
parser, evaluator and every built-in function, value/rt, DBF/DBT/NDX
read/seek/maintain, work areas, navigation, the flow spine (DO WHILE/IF/
CASE, memvars, STORE, RELEASE), procedures (DO, PARAMETERS, ACCEPT, INPUT,
WAIT, ON ...), the query verbs (LIST, DISPLAY, ?, ??, LOCATE, CONTINUE, SEEK,
FIND, GO, SKIP), the record edits (REPLACE, APPEND [BLANK], DELETE, RECALL),
the REPL, every command hook's claim (verb recognition) and the overlay
manager and retained-pointer guards. String literals and mergeable constants of overlay
functions also stay resident (Sec 4).

NDX() and deferred-error/guard completion stay resident with the interpreter.
Index display spelling is copied into the resident-owned `ndx_index` allocation;
FILE (INDEX/REINDEX) and SET never publish an overlay pointer. Resident display
accessors and the copying setter let STATUS and SET use that metadata without
cross-overlay calls. The allocation and retained display pointer use the lifetime
guard, and `index_name` is audited as the final cursor-vtable slot at binding and
at every overlay boundary.

**Slot count: one.** One fixed slot is the authentic default (TP5's single
overlay buffer; dBASE's overlay area). It costs nothing against the module
budget: it lives in the loader's BSS reserve (Sec 5). Several slots would
only reduce swaps; no SAMIR gate or program measured needs that today, and
any family that later swaps hot (a W4 full-screen app) can be regrouped by
editing two lines in `samir.ld`.

## 4. Mechanism

- **Marking.** Overlay code is `SAMIR_OVL_TEXT(name)` (a section attribute,
  `.text.0ovl_<name>`; empty on the host). GCC puts that function's jump
  tables in `.rodata.0ovl_<name>`; its slot header (`SAMIR_OVL_HEADER`:
  magic, id, entry) in `.rodata.0ovl_<name>.0hdr`. With one LTO partition,
  identical string literals are emitted once in whichever function's section
  came first, so `.rodata.0ovl_*.str*/.cst*` are kept RESIDENT (the link
  proved the hazard: NOCROSSREFS_TO refused `"TO"`/`"FILE"` shared between
  mutate_cmd_hook and d_file).
- **Link.** One ELF at ORG 0 with `--emit-relocs` and per-function sections.
  `samir.ld` links every overlay at the same address (OVERLAY, the slot,
  directly after the module) and enforces two rules in the linker:
  `OVERLAY ... NOCROSSREFS` (no overlay-to-overlay reference) and
  `NOCROSSREFS_TO(<ovl> .text .rodata .data .bss)` (no resident reference to
  an overlay).
- **Pack.** `tools/samir_ovl_pack.c` writes the resident module and its
  relocation list (every `R_386_32` site), and `SAMIR.OVL`; `tools/mzlink
  --reloc-file --header-relocs --minalloc 2058` wraps the module as an
  InitechMZ with the relocation table in the header (the DBASE.EXE layout:
  2773 entries at 0x1C). The packer refuses unknown relocation types,
  absolute targets, orphan sections, a slot that is not exactly the largest
  overlay, a missing slot header, and THE CALL-GRAPH RULE (Sec 6).
- **Call.** A resident command hook claims its verb exactly as before, then
  `SAMIR_OVL_CALL(id, entry, k, ...)`. On Milton that is
  `samir_ovl_call`: if the slot holds another overlay it reads the wanted
  one (LSEEK + READ through the PAL, as DBASE.EXE does), CRC-checks it,
  adds the resident base B to each relocated dword, checks the slot header
  and calls the header's entry. On the host the macro is a direct call of
  the same entry: no manager, no target-only behaviour (ADR-0009 DEC-03).
- **Relocation.** One factor serves both images: the resident is relocated
  by the InitechMZ loader by B (`loader_prepare_mz`, DEC-08a.1) and an overlay
  is linked at `slot - B`-relative addresses, so the manager adds the same B.
  B is read from a relocated reference to `__samir_base`.

## 5. Memory

The physical arithmetic is unchanged (ADR-0009 DEC-03/04,
`spec/memory_map.h`): module + 64 KiB BSS reserve + the PAL's AH=48h block +
0x20 (MCB, rounding) must end below ENV_BLOCK, with 4096 B to spare. The
slot sits in the reserve. To make room, the PAL heap's two independently
reclaimed regions swapped places: the 16,528 B region moved from the reserve
into the AH=48h block (now 2057 paragraphs = 0x8090) and the 3,952 B region
moved into the reserve above `.bss`. Region sizes, list order and
non-adjacency are unchanged, so allocation behaviour is identical. Module
budget: 106,352 - 16,528 + 3,952 = 93,776 B (`test_samir_size.c` pins it).
Reserve use: slot 9,728 + `.bss` 46,996 + region 3,952 (+ alignment) =
60,704 of 65,536 B (link-time ASSERT). `e_minalloc` = 2058 paragraphs, so
the loader refuses (DOS 08h) rather than start SAMIR heap-starved
(DEC-08a.3). Knob for later waves: the slot may grow to about 14.5 KB; past
that, move the 3,952 B region back to the arena (budget -3,952).

## 6. Rules and their checks

1. Resident code never names an overlay symbol -- linker (NOCROSSREFS_TO).
2. No overlay refers to another overlay -- linker (OVERLAY NOCROSSREFS).
3. No returning into an evicted overlay: overlay code calls only resident
   code that can never swap the slot. Static: the packer builds the call
   graph from every relocation (direct calls and function-address
   references) and refuses when overlay code reaches `samir_ovl_call` or an
   interpreter entry that could run another command (`samir_do`,
   `samir_repl`, `run_block`, `exec_command`, `fl_trap`, `fl_escape`); the
   first name must exist, so a renamed manager cannot disable the check.
   Run time (defence in depth, indirect calls): the manager rejects any
   reentry while an overlay frame is on the stack, including same-id calls.
   The PAL prints the invariant, loaded/requested ids, depth and pointer on
   stderr, then raises UD2 into the kernel panic (PC LOAD LETTER).
4. Code pointers kept in resident state are never overlay addresses (the
   command hooks registered in the chain are the resident claims; an overlay
   passes its own callbacks only for the duration of a resident call, e.g.
   `ndx_rebuild(..., d_key, ...)`). Runtime registration checks in
   `samir/lifetime.h` reject slot addresses in output/resolver/error/cursor/PAL
   bindings, command hooks, suspend hooks and retained codec ownership state.
   The manager audits every public context pointer, nested cursor vtable and
   PAL slot before entry and after return, before permitting any eviction.
   This also catches direct writes bypassing the binding APIs. Synchronous
   NDX key-provider arguments are not retained and remain permitted.

## 7. Packaging and names

The program the manuals and users name is **SAMIR** (typed `samir` at the
prompt). Its files are `SAMIR.EXE` and `SAMIR.OVL` on the InitechBase System
diskette, period-shaped like `DBASE.EXE`/`DBASE.OVL`. Every build (shipped
and every mutant) writes `X.EXE` and `X.OVL` side by side; every harness and
image builder installs both (`harness/diff/dbf_diff/samir_install.h`, the
`samir_pack` recipe, fixture contracts list both files). The FLAIR text
tenant launches `SAMIR.EXE` (`os/milton/kmain.c` FLAIR_TEXT_TENANT_NAME).
`SAMIR.OVL` is opened in the current directory at start-up and held open for
the session.

## 8. Failure model

`SAMIR.OVL` is verified whole before the first dot prompt (header, version,
overlay count, slot length, the stamp the packer patched into the resident,
and the CRC-32 of the whole body: the ADR-0014 DEC-14-5 Alternative 5A
policy, ratified for the kernel's overlay), and each overlay is CRC-checked
again on every load. Missing, foreign/stale or corrupt -> the prompt
`\r        Insert System Disk and press ENTER, or press Ctrl-C to abort.      `
(`spec/samir/ovl_format.h`); ENTER retries (re-opens), Ctrl-C or end of
input aborts. At start-up the abort exits with return code 1 before any
database is open; mid-session (the file replaced under a running SAMIR) the
abort closes every work area as QUIT does, then exits with 1.

## 9. Gates

`harness/diff/dbf_diff/samir_ovl.mk` and `samir_size.mk`: test-samir-ovl
(+mutant: NO_RELOC, NO_STAMP), test-samir-ovl-static (+mutant: CROSS,
RESREF, REENTER), test-samir-ovl-emu (+mutant: SKIP_OPEN, NO_STAMP, NO_CRC),
test-samir-ovl-swap-emu (+mutant: NO_SWAP, NO_RELOC), test-samir-size
(+mutant: PAD, SLOT), test-samir-size-repro (EXE and OVL). The swap gate
reuses the real-III+ DML-status program, which alternates the SET, STATUS
and FILE overlays; its Bochs leg is test-samir-dml-status-emu-bochs.

## 10. Accepted operator decisions (2026-10-25)

1. The multi-segment DBASE.EXE-style PAL manager is accepted. Phase B neither
   requires nor introduces an EXEC AL=03h / DEC-19 kernel service.
2. The Insert System Disk wording is accepted. G-1 still limits claims about
   real dBASE's missing-overlay behaviour; it does not block this wording.
3. The heap-region swap and 93,776 B module budget are accepted, with the
   original physical window and 4 KiB floor intact.
4. The `samir-exe*` target rename is accepted. EXE and OVL are grouped build
   outputs; all default-image consumers declare both dependencies. The output
   gate deletes only OVL in a private BUILD and proves the pair is rebuilt
   byte-identically.

## 11. Review regression gates

`test-samir-ovl-guard-emu-mutant` builds the SET callback escape that static
reachability missed, plus direct context-data, command-hook, PAL-hook,
cursor-vtable, error-hook, suspend-hook and indirect live-reentry regressions.
Each completed deliberate-fault trace proves its specific context diagnostic,
exact loaded/requested/depth values, kernel vector 06 panic and absence of a
return to unsafe code. Relocation omission is likewise stopped before calling
an entry outside the slot; its trace completes at the diagnostic kernel panic.
NO_SWAP remains a wrong-output differential with a completed shell trace.

The short-read PAL mutant marks DBF handles only, clearing marks on close or
handle reuse. It leaves startup OVL reads and command input intact. Its gate
requires the actual USE command, error 15, no overlay refusal, absent LIST rows
and F2's complete trace. `test-samir-mzlink-io` checks a valid relocation stream,
a partial dword, a read error and an injected fclose error.
