# Native executable emission

Authority for instruction encodings: Intel 80386 Programmer's Reference
Manual, section 17.2 and instruction tables:
https://pdos.csail.mit.edu/6.1810/2017/readings/i386/s17_02.htm
Container authority: kernel/include/initechmz_extended.h and documented
loader_high_plan accounting in kernel/loader_validation.c. No kernel source
is compiled or included in the compiler or its independent format checker.

The existing AST-free stack-machine generator retains its NASM output. Its
fixed instruction vocabulary also feeds Pascal instruction encoders, with
explicit near branches, fixed label/routine tables and counting/emission
passes. Output streams in 255-byte chunks. No program-sized code buffer is
allocated. NativeAbsolute records all absolute dword sites, including inline
string addresses and the runtime's BSS/rodata pointers. The counted sorted
sites precede the module in a paragraph-aligned v1 extended MZ header.

TURBOOUT.EXE is always the extended form. TURBOOUT.COM is also emitted when
its code+BSS fits [0x40100,0x6f000). Extended modules are limited to 262144
bytes, and aligned module+reserve must leave the native heap header before
0x7ef000. The loader supplies the separate 65536-byte stack and 4096-byte
guard. Startup preserves ESP; five NOPs keep its offsets equal to flat startup.
The extended descriptor reserves BSS and its preceding alignment gap, with
no requested heap and maxalloc=FFFF. Output contains no dates or host paths.

Runtime is the permitted checked-in NASM-derived byte table, not instruction
encoder output. tests/runtime.sh assembles both source runtimes and fileio,
links them with tests/runtime.ld, regenerates the Pascal tables with the C
helper and compares the complete tables in tier 0. Dynamic dwords explicitly
use NativeAbsolute or rel32 encoders. The table test was observed red before
regeneration, and its final forms are green. UART helpers poll LSR bit 5 to
avoid Bochs transmit FIFO overflow. Hardware authority: PC16550D data sheet,
section 8.6.3 (THRE), also compatible with 16450 mode:
https://media.digikey.com/pdf/Data%20Sheets/Texas%20Instruments%20PDFs/PC16550D.pdf
All three assembly runtimes have the same serial helpers.

Independent oracles in tests/run.sh: NASM/ld/objcopy byte equality for all 17
flat corpus programs; NASM/ld/objcopy plus mzlink byte equality for all 17
extended corpus programs AND turbo.pas. mzlink derives sites independently by
linking at zero and 0x01010180, which preserves section alignment and changes
every absolute dword's low byte. mzcheck independently accepts every emitted
container. Both generation runs repeat these comparisons and compare artifacts.
The self-source exceeds the flat arena, so its real placement check is extended.
No source-language feature was added; seed capacity increased for the larger
compiler, but it still only emits assembly.

Five patch mutants are killed: wrong ModRM, wrong rel32 base, off-by-one label
fixup, missing relocation and wrong BSS placement. Byte comparisons first went
red on a genuinely missing inline-string relocation. Tests never derive
expected relocation bytes from the compiler's own table.
