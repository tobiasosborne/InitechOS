<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0003 Amendment DEC-19 -- INT 21h AH=4Bh AL=03h LOAD OVERLAY in 32-Bit Flat Mode

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

---

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0003-A8 |
| Title | ADR-0003 Amendment DEC-19: INT 21h AH=4Bh AL=03h Load Overlay in 32-Bit Flat Mode |
| Version | 0.9 (draft for ratification) |
| Status | **PROPOSED** -- awaiting the operator's ratification. The implementation below is landed behind its oracles; the decision is not binding until ratified. |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | STAPLER Programme, overlays Phase A coding lane |
| Date | 2026-10-25 |
| Ruling served | Operator 2026-10-25: period-correct overlays now, the `DBASE.EXE`/`DBASE.OVL` pattern (SAMIR.COM at 103,246 B is over the 4 KiB headroom cap under the `samir.ld` PAL budget) |
| Supersedes | **Partial, on ratification** -- the deferral clause ADR-0003 Amendment DEC-08a.7 ("Overlays deferred") and the DEC-08a Sec 3 "MAY NOT claim: InitechOS supports overlays (deferred)" line. Everything else in DEC-08a is retained and extended, not replaced. |
| Related Documents | ADR-0003 (InitechDOS); Amendment DEC-04a (flat INT 21h calling convention); Amendment DEC-08a (InitechMZ, DEC-08a.4 content dispatch, DEC-08a.5 untagged-MZ panic, DEC-08a.7 overlays deferred); Amendment DEC-14 (user-pointer validation, code 0009h); ADR-0009 DEC-03 (MZ + overlay as the authentic SAMIR end state); ADR-0014 (kernel overlays -- a different mechanism, unaffected) |
| Evidence base (Law 1) | IBM DOS 3.30 Technical Reference (`../initech-manuals-ref/dos33/80X0945_DOS_3.30_Technical_Reference_Apr87.pdf`): printed p. 6-196 (PDF 319), p. 6-197 (PDF 320), p. 6-199 (PDF 322), p. 6-42 (PDF 165), pp. 11-4/11-5 (PDF 415/416); [docs/research/overlays-period-ground-truth.md](../research/overlays-period-ground-truth.md) Sec 3 |
| Related Issues | initech-dewv (this amendment, Phase A); initech-5etom (overlays epic; Phase B splits SAMIR into an InitechMZ resident core + SAMIR.OVL); initech-dtw.4 (the deferred overlay bead DEC-08a.7 named); initech-x0i (86Box/Bochs legs for the EXEC family) |
| Retention | 7 years following decommission, per RECORDS-SCHED-014 |

---

## 1. Context

SAMIR (InitechBase) has outgrown its flat `.COM`. The operator ruled for the
period mechanism: a resident executable plus an overlay file it loads on
demand, exactly as dBASE III PLUS ships `DBASE.EXE` beside `DBASE.OVL`
(ADR-0009 DEC-03; research file Sec 1). Phase B of the epic will split SAMIR;
Phase A, this amendment, supplies the operating-system service that split
calls. DEC-08a.7 deferred it and promised that "the header and relocation
machinery are defined so the overlay loader extends, not replaces, this
format". This amendment collects on that promise.

**Period ground truth.** TR p. 6-196, function 4BH, AL value 03H: "Load, do not
create the program segment prefix, and do not begin execution. This is useful
in loading program overlays." TR p. 6-197: for AL = 03H the block at ES:BX is
"WORD segment address where file will be loaded" and "WORD relocation factor to
be applied to the image". TR p. 11-5: "For function 3, to load an overlay, no
PSP is built, and EXEC assumes the calling program has already allocated memory
to load the new program into -- it will not allocate memory for it"; the caller
"should then allocate a block (based on the size of the program to be loaded)";
and "load overlay does not check to see if the calling program actually owns
the memory block ... If the calling program does not own the memory into which
the overlay is being loaded, there is a chance that the program being loaded
will overlay one of the control blocks that DOS uses to keep track of memory
blocks." TR p. 11-4: both functions "perform their loading in the same way
(relocation is performed for .EXE files)". TR p. 6-199: errors are returned in
AX; TR p. 6-42 lists them (2 file not found, 3 path not found, 9 invalid memory
block address, 11 invalid format).

## 2. Decisions

### DEC-19.1 -- The service

INT 21h AH=4Bh with AL=03h loads a file into caller-supplied memory and
returns. No PSP is built, no environment is passed, no memory is allocated, no
code runs, no EXEC frame or exit hook is taken and InDOS is not handed off
(nothing executes). EDX names the file; the path is resolved by the SAME code
AL=00h uses (absolute, CWD-relative and `\SUB\FILE` paths, drive prefix,
`0003h` for a missing component), and the file is located the same way. The
caller transfers control into the overlay itself, as a period program does.
Subfunctions other than 00h and 03h remain `0001h` invalid function (TR
p. 6-196 lists only these two).

*Implementation:* `os/milton/int21.c` `do_exec` -> `do_exec_overlay` -> the
`int21_overlay_fn` seam; `os/milton/kmain.c` `loader_overlay_by_name` (bound
beside every `int21_set_exec_backend(loader_exec_by_name)`);
`os/milton/loader.c` `loader_load_overlay` -> `loader_overlay_core`.

### DEC-19.2 -- The parameter block (locked, Rule 8)

On the flat substrate (DEC-04a) the two WORD segments become flat dwords. EBX
points to `exec_overlay_block_t`, locked in
[spec/dos_structs.h](../../spec/dos_structs.h):

| Offset | Size | Field | Real DOS 3.30 (TR p. 6-197) |
|---|---|---|---|
| 00h | 4 | `load_addr` -- flat linear load address | WORD load segment |
| 04h | 4 | `reloc_factor` -- added to each relocated dword | WORD relocation factor |

`_Static_assert(sizeof == 8)` in the spec header and the `offsetof` checks of
`test_overlay.c` (B0) hold the code to the layout. The block is mandatory (it
carries the load address): `EBX == 0` or an unreadable block is refused with
`0009h`, unlike AL=00h, whose optional block degrades to a no-argument launch.

### DEC-19.3 -- What is loaded, and the relocation rule

Format is chosen by content, never by extension (DEC-08a.4):

- **Tagged InitechMZ** (`'MZ'`/`'ZM'`, `e_res[0] == 0x4943`): the load module
  -- bytes `[e_cparhdr*16, image_bytes)` -- is placed at `load_addr`. For each
  relocation entry (a uint32 module-relative offset, DEC-08a.1) the dword at
  `load_addr + offset` has `reloc_factor` ADDED to it (mod 2^32). This is the
  flat image of DOS's "relocation factor added to each fixup", exactly as
  DEC-08a.1 is the flat image of the EXEC fixup, and it reuses the same
  `mz_parse_header` / `mz_apply_relocs` with the factor as the base. The
  common call passes `reloc_factor == load_addr`; a different factor (load
  here, run there) is honored. The **footprint** is the load module only: the
  MZ header and the relocation table are never written, so a caller that sized
  its block from the module gets nothing past it (TR p. 11-5). `e_minalloc`,
  `e_maxalloc`, `e_cs:e_ip` and `e_ss:e_sp` are ignored -- nothing is allocated
  or run. Every relocation offset is validated against the module BEFORE any
  byte is written, so a malformed overlay leaves the destination untouched.
- **Raw file** (anything else): copied verbatim, `file_size` bytes; the factor
  is ignored, as DOS ignores it for a non-.EXE image. An empty file loads zero
  bytes.
- **Untagged MZ** (a genuine 16-bit DOS .EXE): the kernel PANICS
  (`PANIC foreign-mz`, DEC-08a.5), exactly as AL=00h does. Overlaying 16-bit
  code into a flat program would be the same misexecution DEC-08a.5 forbids,
  one CALL later.

### DEC-19.4 -- Destination ownership (a deliberate deviation, the DEC-14 stance)

Real DOS does not check the target (TR p. 11-5, quoted above) and documents the
consequence: a wild overlay overwrites the MCB chain. InitechDOS enforces the
TR's own stated assumption instead (Rule 2), using the DEC-14 user-pointer
validation as its first stage. The footprint `[load_addr, load_addr + len)`
must lie wholly inside ONE region the current process owns:

- **(A) its own image/BSS window** `[PSP + 100h, first arena header)`, when the
  PSP sits below the bound MCB arena -- the top-level EXEC layout, in which the
  arena is disjoint ABOVE the image plus `PROGRAM_BSS_RESERVE`
  (`loader_prepare_core`; with no arena bound the window ends at
  `PROGRAM_ARENA_CEIL`); or
- **(B) the data area of one arena block whose MCB owner is the current PSP**
  -- an AH=48h block, which is the TR p. 11-5 procedure, or a nested child's
  own EXEC block.

NULL and 32-bit wrap fail first (`user_buf_ok`, DEC-14). The caller's PSP, any
MCB header, a free block, another process's block, the kernel and anything
outside the caller are refused with **`0009h` invalid memory block address**,
before a byte of the file is read. *Code choice:* 0009h is the DOS code for "the
memory you named is not a block you hold" (TR p. 6-42; AH=49h/4Ah return it for
exactly that) and is the code DEC-14 already assigns to a bad user buffer;
0008h (insufficient memory) would misdescribe an address fault as a quantity
fault. Implementation: `int21_overlay_dest_ok` (int21.c) over
`mcb_owned_block` (mcb.c, a read-only chain walk; a corrupt chain owns
nothing).

### DEC-19.5 -- Results

| Condition | Result |
|---|---|
| Loaded | CF clear; AX unchanged (TR documents no success value) |
| AL not 00h/03h | CF, AX = `0001h` |
| File absent, a directory, unreadable, or no overlay backend bound | CF, AX = `0002h` (the AL=00h mapping) |
| Path component missing | CF, AX = `0003h` |
| EBX block NULL/unreadable, or the footprint not caller-owned (DEC-19.4) | CF, AX = `0009h` |
| Malformed InitechMZ (truncated, header > image, out-of-module relocation) | CF, AX = `000Bh` |
| Untagged 16-bit MZ | `PANIC foreign-mz` (DEC-08a.5); never returns |

Every refusal except `0001h` is also recorded for AH=59h GET EXTENDED ERROR
(the `0001h` path is the pre-existing one and is unchanged).

### DEC-19.6 -- DEC-08a.7 un-deferred

On ratification DEC-08a.7 is superseded. The InitechMZ container, tag, parser
and relocation primitive are unchanged; the overlay path is a third consumer of
them beside EXEC (`loader_prepare_mz`) and disk tenants (`loader_prepare_tenant`).
The kernel overlay mechanism of ADR-0014 (FLAIR.OVL, kernel code linked at a
fixed window) is a different thing and is unaffected.

## 3. Honesty boundary (Law 1)

**MAY claim:** "InitechDOS implements INT 21h AH=4Bh AL=03h Load Overlay for
InitechMZ and raw files, with the caller's relocation factor applied to every
InitechMZ fixup"; "an overlay can only be loaded into memory the calling
program owns". **MAY NOT claim:** that InitechOS loads genuine 16-bit DOS
overlays (it panics on them); that the relocation arithmetic is DOS's (DOS adds
a segment to a 16-bit word, InitechDOS adds a flat factor to a 32-bit dword);
that DOS checks ownership (it does not -- DEC-19.4 is our deviation); anything
about what dBASE does without its overlay (research gap G-1).

## 4. Oracle (Law 2, Rule 6)

- `make test-overlay` (host, TEST_UNIT_GATES): `os/milton/test_overlay.c`
  against the real `loader_overlay_core`, `int21_dispatch` and
  `int21_overlay_dest_ok`; literal expected bytes; canaries prove the exact
  footprint; the header-gap relocation table with 70 entries crosses the
  scratch-chunk boundary.
- `make test-overlay-mutant`: `LOADER_MUTATE_OVL_NO_FACTOR` (factor never
  applied), `INT21_MUTATE_OVL_BOUNDS_OBO` (one byte past an owned region
  accepted), `LOADER_MUTATE_OVL_ACCEPT_FOREIGN` (untagged MZ copied raw) --
  each RED for its named check.
- `make test-exec-overlay` (emulator, TEST_EMU_GATES): a resident program
  (`ovltest_program.asm`) loads an mzlink InitechMZ overlay whose relocations
  are DERIVED by `--reloc-diff`, CALLs into it, proves the factor (not the
  load address) is what is added, loads a raw file verbatim, fills an AH=48h
  block to its last byte, and is refused `0009h` one byte past it, on the
  kernel, on its PSP and on an MCB header; an untagged MZ overlay panics.
  `make test-exec-overlay-mutant` boots kernels carrying each mutant object;
  each goes RED for its named marker. QEMU only, matching test-exec,
  test-mzexec and test-exec-nested; Bochs/86Box ride initech-x0i (Rule 5 is
  owed by the whole EXEC family, not waived for this service).

## 5. Alternatives rejected

- **No ownership check (DOS-literal):** rejected; it reproduces the MCB-chain
  corruption TR p. 11-5 warns about, the class of defect Rule 2 and DEC-14
  exist to stop.
- **DEC-14 alone (NULL/wrap):** rejected; it would let a program overlay the
  kernel image.
- **The PSP:02h window `[PSP+100h, alloc_end)`:** rejected; on the flat layout
  it contains the arena headers, the environment and the stack.
- **Read the whole file to the destination, then move the module down (the
  `loader_prepare_mz` order):** rejected; it writes header and relocation-table
  bytes past the module into memory the caller did not allocate. The overlay
  path reads the header and the table into kernel scratch instead.
- **A paragraph (segment) factor:** rejected; the flat relocation adds bytes,
  never paragraphs (DEC-08a.1; the `MZ_MUTATE_RELOC_PARAGRAPH` precedent).
- **`0008h` for a destination refusal:** rejected (DEC-19.4 code choice).

## 6. Points for the operator's ratification

1. The DEC-19.4 deviation itself and its refusal code `0009h`.
2. Caller classes with no window are refused: kernel-context callers (the shell
   builtins run on the kernel PSP) and FLAIR GUI tenants (PSP in the FLAIR
   heap, outside the MCB arena). Phase B's SAMIR is an EXEC'd program and is
   covered by (A) and (B).
3. A read error maps to `0002h` (AL=00h parity) rather than a new code.
4. An empty raw file is a successful zero-byte load.
5. **Kernel bytes (BLOCKING for merge).** The service costs the flagship
   `kernel_flairtenants` 832 loaded bytes (180,208 -> 181,040 B; headroom
   16,400 -> 15,568 B), after a 96 B saving from sharing one file-find helper
   between EXEC, tenant and overlay loads. `test-kernel-headroom` stays green
   (largest kernel 182,432 B, 14,176 B headroom against the 4,096 B floor), but
   `test-kernel-size-r4` (the flagship's 16 KiB headroom floor, 180,224 B) goes
   RED by 816 B: "size round 5 is due". The flagship is the kernel Phase B's
   SAMIR runs under, so the binding cannot simply be dropped there. The floor
   was not touched; a size round (or ADR-0014 kernel overlays) or an explicit
   operator decision on the floor must precede merging.

## 7. Supersession

On ratification this Amendment supersedes ADR-0003 Amendment DEC-08a.7 and the
DEC-08a Sec 3 overlay non-claim. DEC-08a.1 through DEC-08a.6 are retained.

---

*-- End of Amendment DEC-19 --*
