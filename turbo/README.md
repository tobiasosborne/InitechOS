# Turbo Initech

Two compilers: `seed/` is the C host bootstrap; `compiler/turbo.pas` is the
Pascal compiler. Both currently emit assembly. NASM, ld and objcopy finish
programs on the host. No native emission or self-host certificate is claimed.

`make` builds the seed, the FPC host compiler and resident `TURBO.COM`.
Despite its name, TURBO.COM is an extended InitechMZ container: a flat 32-bit
module linked at zero, relocated to 0x600000 by the content-dispatch loader.
Startup retains the loader's separate 64 KiB stack. Descriptor reserve covers
BSS including alignment; files and exit use INT 21h, diagnostics use the
inherited serial helpers. No kernel private headers or code are included.

`make test` composes 6 seed and 38 compiler host entries, including 17 FPC
execution references and deterministic assembly/link checks. The seed-built
and Pascal-generated compiler are packaged with factory/mz/mzlink and checked
by the independent mzcheck against the actual extended limits: 262144 module
bytes, resident+reserve before 0x7ef000, native heap header, separate guard
and stack. These replace the obsolete conventional compiler placement gates;
small conventional programs still use their original runtime/link script.

`make smoke` builds the product image, boots it on QEMU and Bochs with 8 MiB,
and runs the packaged compiler from a separate TEST data disk. mtools extracts
its assembly and cmp compares it with the host Pascal compiler's output for
that source. No test material is placed on the product boot disk. Logs and
images remain in build/turbo/smoke. This proves compilation to assembly, not
execution of that assembly's program. Emulator work never runs in tier 0.

FPC, NASM, GNU binutils and mtools are required; missing tools fail. The source
semantics and independently authored corpus/protocol goldens are unchanged.
`make mutants` runs component patch mutants. `selfhost` and `ddc` explicitly
fail until the in-OS native compilation proofs exist. See ROADMAP.md.
