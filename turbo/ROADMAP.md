# Turbo Initech: path to the golden master

This is an engineering assessment of the imported code, not an item-7 claim.
Evidence is the actual host run, source routines and contracts cited below.
The seed is host C; the resident compiler is Pascal. They are separate programs.

1. Today the 17 gen_* corpus programs exercise signed 32-bit integer arithmetic,
   boolean complete evaluation, char/ord/chr, ShortString operations, consts,
   if/while/for/repeat, routines with locals/value/var parameters, recursion and
   forward, static arrays (including negative bounds), named scalar-field records,
   record arrays/copies, and untyped byte files. See fixtures and tests/run.sh.
   FPC execution agrees with all 17 hand-computed stdout goldens. Tier 0 proves
   our emitted assembly assembles/links deterministically; guest equivalence
   still needs the deferred differential. No reals, pointers/heap, units, sets,
   objects, arbitrary nested aggregates or general Pascal compatibility is proved.
   case is explicitly refused (parse_case_unsupported); later calls need forward.

2. The current pipeline is host cc -> initechc; initechc compiles the Pascal
   source -> NASM ELF32 object -> host ld with start_dos/fileio runtime -> objcopy
   -> resident flat COM. All assembly, linking and image construction are host
   operations. Once launched in DOS the compiler reopens its fixed input for LEX,
   PARSE, TYPE and GEN, then writes a fixed assembly file with BlockWrite.
   Host extraction/NASM/ld/objcopy are still needed to run that output. The FPC
   build of the same compiler is a host testing/bootstrap path, not the artifact.

3. It CAN compile its own source today through that host pipeline: self-protocols
   reaches EOF, PARSE-OK, TYPE-OK; self-generation produces deterministic assembly
   twice and links both targets. No missing language feature blocks this measured
   case. Before rename: source 156643 bytes/5033 lines; self assembly 765205 bytes,
   1938 labels; seed-built COM 89591 bytes, Pascal-generated COM 112995 bytes.
   BSS is 71704 bytes; seed-linked end 0x67710, Pascal-linked end 0x6d27c. This
   does not execute the resulting compiler or prove K2==K3. See rename evidence
   in compiler/tests/IMPORT.md for the final source/assembly sizes.

4. Capacity is already tight. SourceMax=262144, PoolMax=6144, NameMax=704,
   SymbolMax=1024, HeaderParamMax=PendingNameMax=32 (source const section).
   Self TYPE dump has 818 SYM rows and no FIELD rows; self lexer sees 537 distinct
   identifier spellings totalling 5359 bytes (keywords separately; generator
   string constants can add pool entries). Global bytes=71704. The Pascal-linked
   end leaves only 7556 bytes below 0x6f000; the seed-linked image has 30960.
   Enlarging tables/emitter/IDE needs a measured memory plan, not just larger
   maxima. Eight MiB physical memory does not remove this fixed low arena limit.

5. Recommend direct machine-code emission for the existing stack-machine backend.
   Gen* routines already select a small fixed instruction vocabulary: pushes,
   loads/stores, arithmetic, setcc, branches, calls and fixed string/file helpers.
   Add explicit text/data/BSS placement, little-endian encoders, near rel32 label
   and routine fixups, and reproducible startup/runtime inclusion. Prefer fixed
   instruction sizes plus counting/emission passes and streamed disk output to a
   large in-memory code buffer. Use NASM-assembled byte fixtures as the independent
   encoding oracle; Intel 80386 instruction encodings are the design authority.
   Planning estimate: 1500-2500 Pascal lines, 15-30 KiB target code plus 8-16 KiB
   fixup/state tables (not a measured result). The current arena cannot absorb
   this estimate: first resolve placement/relocation and loader allocation through
   the public DOS ABI, or reduce existing code/state with mechanical evidence.
   An in-OS assembler/linker would also need a parser, relocations, object/symbol
   format and image layout policy, plus resident tools: a larger second toolchain
   to trust and bootstrap. Do not implement both. Keep NASM as a factory oracle.

6. With final source S and native emitter, K1 is the seed-built resident compiler
   (host assembler/linker allowed ONLY for this bootstrap). Run K1(S) inside DOS
   -> K2.COM; run K2(S) inside DOS -> K3.COM. Reset source/config/input filesystem
   identically; compare meaningful entire COM bytes using host cmp after FAT
   extraction and verify both executions succeed. K2==K3 is the certificate;
   K1==K2 is neither necessary nor the promised check. Include runtime bytes,
   BSS/layout metadata if introduced, fixed label ordering and no dates/paths.

7. DDC needs the independent FPC-built HOST compiler to compile the same S into
   target compiler D1; execute D1(S) in DOS -> D2. Compare D2 with the converged
   seed path K2/K3. The FPC executable itself is a different host architecture
   binary and must never be byte-compared with a COM. Document language/RTL
   assumptions, tool hashes/source identity and deterministic layout. Add negative
   controls for a contaminated seed path and changed compiler source. The legacy
   selfhost/ddc targets are only failing stubs; there is no DDC proof to import.

8. Minimal blue IDE requires a text buffer, insertion/deletion/navigation, explicit
   save, compile diagnostics with line/column, and EXEC/run/return. kernel/int21.c
   dispatches 01/02/06/07/08/09/0A/0B/0C console services, 3C-40/42 files,
   41/56 delete/rename, 4B EXEC, 4C exit, 4D return status, 48/49/4A allocation.
   Its ANSI apply layer plus kernel/ansi.c and console.c support cursor/erase/SGR;
   verify public routing/configuration on the product image. kernel/idt.c has no
   BIOS INT 10h/16h display/keyboard compatibility gate; do not call them from
   protected-mode Pascal. Existing compiler output is privileged COM1 serial,
   not screen output; add a DOS console RTL and input/key bridge, structured
   diagnostics, safe save failure handling and parameter/file selection. All OS
   access must use INT 21h, with no direct kernel/private header dependency.

9. Main risks: low-memory growth; fixed tables/string temp lifetimes; signed
   arithmetic/overflow corner cases; var-parameter ABI; short/failed writes and
   output file handles; deterministic fixups; cooperative responsiveness; loader
   return/EXEC context; FPC semantic differences. The file RTL currently closes
   an old handle on Assign/Reset/Rewrite but exposes no source Close builtin.
   A compiler that only writes serial protocols cannot be mistaken for an IDE.

10. Proposed milestones, each mechanically accepted:
    (a) Restore image smokes: all deferred gates, 17/17 guest/FPC stdout plus disk
        files, wrong-value mutants, no timeout/fault on QEMU and Bochs.
    (b) Memory/ABI plan: measure code+BSS+worst stack; demonstrate public allocation,
        load/run/return on 8 MiB with sentinels and failed-allocation controls.
    (c) Native backend: each encoder/fixup vs NASM bytes; all 17 programs compile
        inside DOS and execute vs FPC with zero host assembly/link after bootstrap.
    (d) Fixed point: make selfhost from clean boots, extract K2/K3, cmp equal;
        deliberate nondeterminism makes it red. Run generated compilers too.
    (e) Diverse path: make ddc, D2 equal to fixed point; contaminated path red.
    (f) IDE/finale: injected edit/save/error-correct/compile/run/self-reproduce
        session, safe failure/cancel saves, stable screens and full cert green.
