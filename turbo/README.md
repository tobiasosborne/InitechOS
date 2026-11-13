# Turbo Initech

Two compilers: `seed/` is the C host bootstrap; `compiler/turbo.pas` is the
Pascal compiler. The seed emits assembly for the host bootstrap. The Pascal
compiler emits assembly plus native flat COM and extended InitechMZ executables;
no host assembler/linker is needed to finish those native programs. A self-host
certificate is still pending.

`make` builds the seed, the FPC host compiler and resident `TURBO.COM`.
Despite its name, TURBO.COM is an extended InitechMZ container: a flat 32-bit
module linked at zero, relocated to 0x600000 by the content-dispatch loader.
Startup retains the loader's separate 64 KiB stack. Descriptor reserve covers
BSS including alignment; files and exit use INT 21h, diagnostics use the
inherited serial helpers. No kernel private headers or code are included.

`make test` composes 6 seed and 39 compiler host entries, including 17 FPC
execution references and independent native byte comparisons (17 flat, 18 extended). The seed-built
and Pascal-generated compiler are packaged with factory/mz/mzlink and checked
by the independent mzcheck against the actual extended limits: 262144 module
bytes, resident+reserve before 0x7ef000, native heap header, separate guard
and stack. These replace the obsolete conventional compiler placement gates;
small conventional programs still use their original runtime/link script.

`make smoke` builds the product image, boots it on QEMU and Bochs with 8 MiB,
and runs the packaged compiler from a separate TEST data disk. mtools extracts
the generated programs and validates the extended containers. QEMU compiles
and runs all 17 corpus programs; Bochs runs control, file and tiny cases. Stdout
is compared with independently authored goldens, also executed under FPC;
mtools compares file payloads with those written by FPC. No test material is placed on the product boot disk. Logs and
images remain in build/turbo/smoke. This proves native compilation and execution
without a host assembler/linker in the loop. The separate guest TIMER utility
uses INT 21h GET TIME to bracket compilation (1-second clock resolution). Emulator work never runs in tier 0.

FPC, NASM, GNU binutils and mtools are required; missing tools fail. The source
semantics and independently authored corpus/protocol goldens are unchanged.
Runtime is a checked-in NASM byte table with a tier-0 regeneration comparison;
see compiler/NATIVE.md. Five encoder/fixup/relocation/layout mutants are killed.
`make mutants` runs component patch mutants. `selfhost` and `ddc` explicitly
fail until the in-OS native compilation proofs exist. See ROADMAP.md.
