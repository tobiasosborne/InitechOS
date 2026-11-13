# tps

Includes C seed compiler and Pascal Turbo Initech compiler; retain the independent FPC host path and the K2/K3 self-hosting ladder. Generated compiler programs are DOS programs, not Toolbox tenants.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

19 production/source-support files; 15,280 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/tps/tps.pas:1`|5033|$MODE DELPHI}{$B+}{$H-}|
|`seed/ast.c:1`|436|arena allocator, node constructors, and S-expression dump for the|
|`seed/ast.h:1`|753|AST node types for the seed Pascal front-end scaffold subset.|
|`seed/codegen.c:1`|2793|Step B seed codegen: AST -> nasm Intel-syntax 32-bit x86.|
|`seed/codegen.h:1`|71|Step B of the seed Pascal cross-compiler: x86 code emission.|
|`seed/initechc.c:1`|183|thin driver for the seed Pascal cross-compiler front end.|
|`seed/lexer.c:1`|459|single-pass lexer for the seed Pascal front-end subset.|
|`seed/lexer.h:1`|50|single-pass lexer over a source buffer for the seed front end.|
|`seed/parser.c:1`|2075|recursive-descent parser for the seed Pascal front-end subset.|
|`seed/parser.h:1`|268|recursive-descent parser for the seed Pascal front-end subset.|
|`seed/rt/fileio.asm:1`|212|B8 thin file-I/O RTL over InitechDOS INT 21h.|
|`seed/rt/seed.ld:1`|35|linker script for seed-compiled freestanding binaries.|
|`seed/rt/seed_dos.ld:1`|45|flat .COM link script for seed-compiled InitechDOS programs.|
|`seed/rt/start.asm:1`|133|freestanding runtime for the seed cross-compiler's output.|
|`seed/rt/start_dos.asm:1`|153|InitechDOS-load runtime for the seed cross-compiler's output.|
|`seed/token.c:1`|77|human-readable token-kind names for dumps and diagnostics.|
|`seed/token.h:1`|183|token kinds and the Token struct for the seed Pascal front end.|
|`seed/typecheck.c:1`|2152|minimal static type checker for the seed Pascal subset.|
|`seed/typecheck.h:1`|169|minimal static type checker for the seed Pascal subset.|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`os/tps/.gitkeep`|binary|.gitkeep; source/contract input (role inferred from path)|
|`os/tps/CODEGEN.md:1`|186|Turbo Initech x86 Emission Contract|
|`os/tps/CORPUS.md:1`|89|Turbo Initech Shared Compiler Corpus Register|
|`os/tps/LEXER-DUMP.md:1`|44|Turbo Initech lexer dump format|
|`os/tps/PARSER-TRACE.md:1`|86|Turbo Initech parser trace format|
|`os/tps/SYMBOLS-DUMP.md:1`|139|Turbo Initech Symbol Table Dump Interface|
|`os/tps/fixtures/compiler_corpus.bat:1`|binary|compiler_corpus.bat; source/contract input (role inferred from path)|
|`os/tps/fixtures/gen_array_deep.golden:1`|1|FILLSUM=55 SUM=15 REV=54321 NESTED=4 BSUM=510 CSUM=10 FLAGSUM=2 SCANPOS=4|
|`os/tps/fixtures/gen_array_deep.pas:1`|161|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_array_shared.golden:1`|1|SUM=15 REV=54321 NESTED=2 BSUM=510 CSUM=20 F1=TRUE F2=FALSE LSUM=45|
|`os/tps/fixtures/gen_array_shared.pas:1`|131|$MODE DELPHI}{$B+}|
|`os/tps/fixtures/gen_bool_deep.golden:1`|1|EQT=TRUE EQF=FALSE NET=TRUE NEF=FALSE LTT=TRUE LTF=FALSE LET=TRUE LEF=FALSE GTT=TRUE GTF=FALSE GET=TRUE GEF=FALSE ANDTT=TRUE ANDTF=FALSE ANDFF=FALSE ORTT=TRUE ORTF=TRUE ORFF=FALSE NOTT=FALSE|
|`os/tps/fixtures/gen_bool_deep.pas:1`|115|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_char_deep.golden:1`|1|ORDA=65 CHRA=A ORDCHR65=66 CHRORDB=B CHRWRAP=A CONSTUSE=11 NEGC=-1 CHRCONST=A BOOLCONSTT=TRUE BOOLCONSTF=FALSE CMPIF=EQ CMPLT=LT SCAN=5 ORDBOOLT=1 ORDBOOLF=0 WCH=Z|
|`os/tps/fixtures/gen_char_deep.pas:1`|118|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_control_deep.golden:1`|1|SUM=15 IF1=LT IF2=LE DE=Y NEST=18 FORTO=10 FORDOWN=10 FORONCE=3 REPEAT=6|
|`os/tps/fixtures/gen_control_deep.pas:1`|140|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_file_shared.golden:1`|1|FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR|
|`os/tps/fixtures/gen_file_shared.pas:1`|48|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_fileio_deep.golden:1`|1|FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR|
|`os/tps/fixtures/gen_fileio_deep.pas:1`|51|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_func_deep.golden:1`|1|FACT5=120 SUMTO=15 EVEN7=FALSE ODD7=TRUE SWAPA=8 SWAPB=3 ALIASG=41 ALIASG2=42 VALINDEP=5 COMPOSE=10|
|`os/tps/fixtures/gen_func_deep.pas:1`|153|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_func_shared.golden:1`|1|120 TRUE FALSE 2 1|
|`os/tps/fixtures/gen_func_shared.pas:1`|78|$MODE DELPHI}{$B+}|
|`os/tps/fixtures/gen_local_record_array.golden:1`|1|LOCALREC=63 COPY=27 SOURCE=7 FLAGS=1|
|`os/tps/fixtures/gen_local_record_array.pas:1`|53|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_record_deep.golden:1`|1|LOCALREC=607 KIND=42 CH=X OK=TRUE BKIND=43 BOK=FALSE T2KIND=43 T2CH=X TKIND=43 T2KIND2=999 NESTED=1010 SUMKIND=1150 FIRSTOK=2 SCANCH=C|
|`os/tps/fixtures/gen_record_deep.pas:1`|181|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_record_shared.golden:1`|1|KIND=7 VAL=40 OK=FALSE BKIND=17 BOK=TRUE T2KIND=99 TKIND=17 T2VAL=12 KSUM=118|
|`os/tps/fixtures/gen_record_shared.pas:1`|109|$MODE DELPHI}{$B+}|
|`os/tps/fixtures/gen_recursion_depth.golden:1`|1|DEPTH=78 AND=FALSE OR=TRUE TOUCH=12 DIV=-3 MOD=-1|
|`os/tps/fixtures/gen_recursion_depth.pas:1`|52|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_string_deep.golden:1`|1|LEN0=0 CONSTSTR=Hi TRUNC=abc TLEN=3 LDEEP=abcdef RNEST=ghijkl CCAT=abcd SELFA=abc PRE=aZ EQ=TRUE EQF=FALSE NE=TRUE LTP=TRUE LTB=TRUE LE=TRUE GT=TRUE GE=TRUE CMPCH=TRUE LENV=3 LENP=3 IDXR=Y I|
|`os/tps/fixtures/gen_string_deep.pas:1`|185|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_string_shared.golden:1`|1|Hello, World! 13 H EQ LT|
|`os/tps/fixtures/gen_string_shared.pas:1`|53|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_string_temporaries.golden:1`|1|DEEP=ABCDE DIRECT=ABCDE LEN=5 QUOTE=It's|
|`os/tps/fixtures/gen_string_temporaries.pas:1`|31|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/gen_tiny.golden:1`|1|gen_tiny.golden; source/contract input (role inferred from path)|
|`os/tps/fixtures/gen_tiny.pas:1`|30|$MODE DELPHI}{$B+}{$H-}|
|`os/tps/fixtures/lex_badchar.golden:1`|4|TPS-LEX-BEGIN|
|`os/tps/fixtures/lex_badchar.pas:1`|1|lex_badchar.pas; source/contract input (role inferred from path)|
|`os/tps/fixtures/lex_basic.golden:1`|73|TPS-LEX-BEGIN|
|`os/tps/fixtures/lex_basic.pas:1`|11|program MiXeD_Main;|
|`os/tps/fixtures/lex_unterminated_brace.golden:1`|4|TPS-LEX-BEGIN|
|`os/tps/fixtures/lex_unterminated_brace.pas:1`|1|lex_unterminated_brace.pas; source/contract input (role inferred from path)|
|`os/tps/fixtures/lex_unterminated_paren.golden:1`|4|TPS-LEX-BEGIN|
|`os/tps/fixtures/lex_unterminated_paren.pas:1`|1|lex_unterminated_paren.pas; source/contract input (role inferred from path)|
|`os/tps/fixtures/lex_unterminated_string.golden:1`|4|TPS-LEX-BEGIN|
|`os/tps/fixtures/lex_unterminated_string.pas:1`|1|program 'open|
|`os/tps/fixtures/parse_bad_factor.golden:1`|1|TPS-PARSE-ERROR pos=53 line=5 col=1 expected=FACTOR found=KW_END|
|`os/tps/fixtures/parse_bad_factor.pas:1`|5|program BadFactor;|
|`os/tps/fixtures/parse_case_unsupported.golden:1`|1|TPS-PARSE-ERROR pos=50 line=4 col=3 expected=KW_END found=KW_CASE|
|`os/tps/fixtures/parse_case_unsupported.pas:1`|6|program CaseUnsupported;|
|`os/tps/fixtures/parse_misplaced_var.golden:1`|1|TPS-PARSE-ERROR pos=31 line=3 col=3 expected=KW_END found=KW_VAR|
|`os/tps/fixtures/parse_misplaced_var.pas:1`|4|program MisplacedVar;|
|`os/tps/fixtures/parse_missing_semi.golden:1`|1|TPS-PARSE-ERROR pos=21 line=2 col=1 expected=SEMI found=KW_BEGIN|
|`os/tps/fixtures/parse_missing_semi.pas:1`|3|program MissingSemi|
|`os/tps/fixtures/parse_mut_else.pas:1`|7|program ElseMutant;|
|`os/tps/fixtures/parse_mut_precedence.pas:1`|5|program PrecedenceMutant;|
|`os/tps/fixtures/parse_rich.golden:1`|651|TPS-PARSE-BEGIN|
|`os/tps/fixtures/parse_rich.pas:1`|41|program ParseRich;|
|`os/tps/fixtures/parse_unbalanced_end.golden:1`|1|TPS-PARSE-ERROR pos=52 line=5 col=6 expected=KW_END found=DOT|
|`os/tps/fixtures/parse_unbalanced_end.pas:1`|5|program Unbalanced;|
|`os/tps/fixtures/type_bad_assignment.golden:1`|1|TPS-TYPE-ERROR line=5 col=3 type mismatch in assignment: expected integer got boolean|
|`os/tps/fixtures/type_bad_assignment.pas:1`|6|program BadAssignment;|
|`os/tps/fixtures/type_bad_duplicate.golden:1`|1|TPS-TYPE-ERROR line=4 col=3 duplicate identifier: count|
|`os/tps/fixtures/type_bad_duplicate.pas:1`|6|program BadDuplicate;|
|`os/tps/fixtures/type_bad_file.golden:1`|1|TPS-TYPE-ERROR line=5 col=3 file variables cannot be assigned|
|`os/tps/fixtures/type_bad_file.pas:1`|6|program BadFile;|
|`os/tps/fixtures/type_bad_string_bound.golden:1`|1|TPS-TYPE-ERROR line=4 col=16 array bound requires an integer literal or integer constant|
|`os/tps/fixtures/type_bad_string_bound.pas:1`|6|program BadStringBound;|
|`os/tps/fixtures/type_bad_unknown.golden:1`|1|TPS-TYPE-ERROR line=3 col=9 unknown type: missing|
|`os/tps/fixtures/type_bad_unknown.pas:1`|5|program BadUnknown;|
|`os/tps/fixtures/type_bad_varparam.golden:1`|1|TPS-TYPE-ERROR line=7 col=10 var parameter requires a variable: argument 1 of setone|
|`os/tps/fixtures/type_bad_varparam.pas:1`|8|program BadVarParam;|
|`os/tps/fixtures/type_rich.golden:1`|30|TPS-TYPE-BEGIN|
|`os/tps/fixtures/type_rich.pas:1`|54|program TypeRich;|
|`seed/README.md:1`|13|seed/ — the seed cross-compiler (C)|
|`seed/examples/arith/array.pas:1`|160|B5: static arrays (array[lo..hi] of T) + indexed access as|
|`seed/examples/arith/bool.pas:1`|111|relational operators (= <> < <= > >=), boolean type, and/or/not|
|`seed/examples/arith/char.pas:1`|114|const declarations, the char type, char literals, and ord/chr|
|`seed/examples/arith/control.pas:1`|136|if/then/else + while/do (primitive), plus for/repeat (sugar,|
|`seed/examples/arith/divmod.pas:1`|9|Pascal div/mod ; expect "D=3 M=2" (17 div 5 = 3, 17 mod 5 = 2).|
|`seed/examples/arith/fileio.pas:1`|51|$MODE DELPHI}{$B+}{$H-}|
|`seed/examples/arith/func.pas:1`|152|B4: procedures/functions, stack frames, value + var params,|
|`seed/examples/arith/negative.pas:1`|7|unary-minus / negative decimal ; expect "N=-5" (0 - 5).|
|`seed/examples/arith/negative_divmod.pas:1`|32|negative-operand Pascal div/mod (beads initech-tf3c;|
|`seed/examples/arith/parens.pas:1`|6|parens override precedence ; expect "R=20" (beads initech-znb). }|
|`seed/examples/arith/precedence.pas:1`|6|* binds tighter than + ; expect "R=14" (beads initech-znb). }|
|`seed/examples/arith/record.pas:1`|180|B6: records + field access (incl. array-of-record),|
|`seed/examples/arith/string.pas:1`|184|B7: fixed/ShortString strings (length/index/compare/concat)|
|`seed/examples/fpc/array_shared.pas:1`|131|$MODE DELPHI}{$B+}|
|`seed/examples/fpc/file_shared.pas:1`|43|$MODE DELPHI}{$B+}{$H-}|
|`seed/examples/fpc/func_shared.pas:1`|78|$MODE DELPHI}{$B+}|
|`seed/examples/fpc/record_shared.pas:1`|109|$MODE DELPHI}{$B+}|
|`seed/examples/fpc/string_shared.pas:1`|53|$MODE DELPHI}{$B+}{$H-}|
|`seed/examples/hello.pas:1`|17|the seed front-end smoke sample (beads initech-znb).|
|`seed/examples/smoke.pas:1`|8|canonical end-to-end seed codegen smoke (beads initech-znb).|
|`seed/test_assert.h:1`|54|tiny zero-dependency assert harness for the seed front end.|
|`seed/test_lexer.c:1`|338|unit tests for the seed Pascal lexer.|
|`seed/test_parser.c:1`|904|unit tests for the seed Pascal recursive-descent parser.|
|`spec/tps_ritual.json:1`|89|"_comment": [|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|—|No cross-component quoted includes found in production sources|—|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-tps.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/initechc build/tps_lex/TPS.COM
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:12110: update target 'build/initechc' due to: seed/initechc.c seed/token.c seed/lexer.c seed/ast.c seed/parser.c seed/typecheck.c seed/codegen.c
cc -std=c11 -Wall -Wextra -Werror -Iseed -o build/initechc seed/initechc.c seed/token.c seed/lexer.c seed/ast.c seed/parser.c seed/typecheck.c seed/codegen.c
Makefile:17711: target 'build/tps_lex' does not exist
mkdir -p build/tps_lex
Makefile:17836: update target 'build/tps_lex/tps_seed.s' due to: os/tps/tps.pas build/initechc
build/initechc --emit-asm -o build/tps_lex/tps_seed.s os/tps/tps.pas \
	|| { printf '!!! test-tps-lex-build FAIL: seed rejected %s (TPS source bug unless a DEC-02 pin diverged)\n' "os/tps/tps.pas"; exit 1; }
Makefile:17840: update target 'build/tps_lex/tps_seed.o' due to: build/tps_lex/tps_seed.s
nasm -f elf32 build/tps_lex/tps_seed.s -o build/tps_lex/tps_seed.o
Makefile:17482: update target 'build/seed_rt_dos.o' due to: seed/rt/start_dos.asm
nasm -f elf32 seed/rt/start_dos.asm -o build/seed_rt_dos.o
Makefile:12258: update target 'build/seed_fileio_rt.o' due to: seed/rt/fileio.asm
nasm -f elf32 seed/rt/fileio.asm -o build/seed_fileio_rt.o
Makefile:17846: update target 'build/tps_lex/tps_seed_dos.elf' due to: build/tps_lex/tps_seed.o build/seed_rt_dos.o build/seed_fileio_rt.o seed/rt/seed_dos.ld
ld -m elf_i386 -T seed/rt/seed_dos.ld -o build/tps_lex/tps_seed_dos.elf build/seed_rt_dos.o build/seed_fileio_rt.o build/tps_lex/tps_seed.o
Makefile:17849: update target 'build/tps_lex/TPS.COM' due to: build/tps_lex/tps_seed_dos.elf
objcopy -O binary build/tps_lex/tps_seed_dos.elf build/tps_lex/TPS.COM
printf '>>> TPS.COM: %s bytes (seed-compiled B9.4 lexer+parser+typecheck+codegen, flat .COM @0x40100)\n' "$(stat -c%s build/tps_lex/TPS.COM)"

# stderr

```

### dry-tps-fpc.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tps_lex/tps_fpc
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:17711: target 'build/tps_lex' does not exist
mkdir -p build/tps_lex
Makefile:17714: update target 'build/tps_lex/tps_fpc' due to: os/tps/tps.pas
command -v fpc >/dev/null 2>&1 \
	|| { printf '!!! test-tps-lex-fpc FAIL: fpc is not installed\n'; exit 1; }
fpc -B -O- -v0 -FUbuild/tps_lex -FEbuild/tps_lex -obuild/tps_lex/tps_fpc os/tps/tps.pas >/dev/null \
	|| { printf '!!! test-tps-lex-fpc FAIL: fpc rejected %s\n' "os/tps/tps.pas"; exit 1; }

# stderr

```

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 1; UNIT 2; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-tps.md](gates-tps.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### os/tps/tps.pas — ORACLE-STRONG

`os/tps/tps.pas:1`; $MODE DELPHI}{$B+}{$H-}. host FPC compiler; lexer/parser/type/codegen comparison and independent FPC execution suites.
Build target(s): `build/tps_lex/tps_fpc`.
Run/assert gate(s): `test-tps-lex-build`, `test-tps-lex-fpc`, `test-tps-lex-mutant`, `test-tps-parse-build`, `test-tps-parse-fpc`, `test-tps-parse-mutant`, `test-tps-type-build`, `test-tps-type-fpc`, `test-tps-type-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `os/tps/fixtures/lex_basic.golden`, `os/tps/fixtures/lex_basic.pas`, `os/tps/fixtures/parse_mut_else.pas`, `os/tps/fixtures/parse_mut_precedence.pas`, `os/tps/fixtures/parse_rich.golden`, `os/tps/fixtures/type_bad_assignment.golden`, `os/tps/fixtures/type_bad_assignment.pas`, `os/tps/fixtures/type_bad_duplicate.pas`, `os/tps/fixtures/type_rich.golden`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tps_lex/tps_fpc
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:17711: target 'build/tps_lex' does not exist
mkdir -p build/tps_lex
Makefile:17714: update target 'build/tps_lex/tps_fpc' due to: os/tps/tps.pas
command -v fpc >/dev/null 2>&1 \
	|| { printf '!!! test-tps-lex-fpc FAIL: fpc is not installed\n'; exit 1; }
fpc -B -O- -v0 -FUbuild/tps_lex -FEbuild/tps_lex -obuild/tps_lex/tps_fpc os/tps/tps.pas >/dev/null \
	|| { printf '!!! test-tps-lex-fpc FAIL: fpc rejected %s\n' "os/tps/tps.pas"; exit 1; }

# stderr

```

### seed/test_lexer.c — UNIT

`seed/test_lexer.c:1`; unit tests for the seed Pascal lexer.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_lexer`, `build/test_lexer_mut_num_range`.
Run/assert gate(s): `test-seed`, `test-seed-num-range-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_lexer`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed -o build/test_lexer seed/test_lexer.c seed/token.c seed/lexer.c seed/ast.c seed/parser.c seed/typecheck.c seed/codegen.c
```

### seed/test_parser.c — UNIT

`seed/test_parser.c:1`; unit tests for the seed Pascal recursive-descent parser.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_parser`, `build/test_parser_mut`.
Run/assert gate(s): `test-seed`, `test-seed-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_parser`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed -o build/test_parser seed/test_parser.c seed/token.c seed/lexer.c seed/ast.c seed/parser.c seed/typecheck.c seed/codegen.c
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-tps.md](gates-tps.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**18 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 29 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-compiler-os

#  recipe to execute (from 'Makefile', line 18588):

Boot/data images named by direct prerequisites or expanded recipe: `build/tps_compiler.img`, `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys-after-ms 60000 --quit-after "SHELL-EXIT" makes the harness wait for SHELL-EXIT (printed only
--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
@if grep -q 'triple_fault=1' "build/tps_lex/tps_compiler_os.report"; then \
	printf '!!! test-compiler-os FAIL: corpus boot triple-faulted\n'; exit 1; fi
	|| { printf '!!! test-compiler-os FAIL: no serial capture\n'; exit 1; }
	if test "$$(grep -Fxc "$$begin" build/tps_lex/compiler_os.normalized)" -ne 1 \
	   || test "$$(grep -Fxc "$$end" build/tps_lex/compiler_os.normalized)" -ne 1; then \
		printf '!!! test-compiler-os FAIL: %s markers missing/non-singular\n' "$$name"; \
	if diff -u "$$fpc" "$$got" >/dev/null; then \
		printf '!!! test-compiler-os FAIL: TPS/InitechDOS vs FPC differs for %s:\n' "$$name"; \
		diff -u "$$fpc" "$$got" || true; failed=1; \
	|| { printf '!!! test-compiler-os FAIL: differential_pass_rate must be 100%%\n'; exit 1; }
@grep -q '^SHELL-EXIT$$' "build/tps_compiler_os.serial" \
```

### test-tps-lex-os

#  recipe to execute (from 'Makefile', line 17895):

Boot/data images named by direct prerequisites or expanded recipe: `build/tps_lex.img`, `build/tracer_secondary.img`.
Input trace `TPS_LEX_OS_KEYS` at make variable (generated/included):
```text
t,p,s,ret,e,x,i,t,ret
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys "t,p,s,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
@if grep -q 'triple_fault=1' "build/tps_lex/os.report"; then \
	printf '!!! test-tps-lex-os FAIL: TPS.COM triple-faulted\n'; exit 1; fi
@grep -q '^SHELL-READY$$' build/tps_lex_os.serial \
	|| { printf '!!! test-tps-lex-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' build/tps_lex_os.serial; then \
	printf '!!! test-tps-lex-os FAIL: B8 RTL error:\n'; grep 'FILEIO-ERROR' build/tps_lex_os.serial; exit 1; fi
@test "$$(grep -c '^TPS-LEX-BEGIN$$' build/tps_lex/os.normalized)" -eq 1 \
	&& test "$$(grep -c '^TPS-LEX-END$$' build/tps_lex/os.normalized)" -eq 1 \
	|| { printf '!!! test-tps-lex-os FAIL: anchored dump markers are missing/non-singular\n'; exit 1; }
@! grep -q '^TPS-LEX-ERROR' build/tps_lex/os.dump \
	|| { printf '!!! test-tps-lex-os FAIL: TPS.COM emitted a lexer error\n'; exit 1; }
```

### test-tps-parse-os

#  recipe to execute (from 'Makefile', line 18076):

Boot/data images named by direct prerequisites or expanded recipe: `build/tps_parse.img`, `build/tracer_secondary.img`.
Input trace `TPS_PARSE_OS_KEYS` at make variable (generated/included):
```text
t,p,s,ret,e,x,i,t,ret
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys "t,p,s,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
@if grep -q 'triple_fault=1' "build/tps_lex/parse_os.report"; then \
	printf '!!! test-tps-parse-os FAIL: TPS.COM triple-faulted\n'; exit 1; fi
@grep -q '^SHELL-READY$$' build/tps_parse_os.serial \
	|| { printf '!!! test-tps-parse-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' build/tps_parse_os.serial; then \
	printf '!!! test-tps-parse-os FAIL: B8 RTL error:\n'; grep 'FILEIO-ERROR' build/tps_parse_os.serial; exit 1; fi
@test "$$(grep -c '^TPS-PARSE-BEGIN$$' build/tps_lex/parse_os.normalized)" -eq 1 \
	&& test "$$(grep -c '^TPS-PARSE-END$$' build/tps_lex/parse_os.normalized)" -eq 1 \
	|| { printf '!!! test-tps-parse-os FAIL: anchored parse markers are missing/non-singular\n'; exit 1; }
@! grep -q '^TPS-PARSE-ERROR' build/tps_lex/parse_os.dump \
	|| { printf '!!! test-tps-parse-os FAIL: TPS.COM emitted a parser error\n'; exit 1; }
```

### test-tps-type-os

#  recipe to execute (from 'Makefile', line 18240):

Boot/data images named by direct prerequisites or expanded recipe: `build/tps_type.img`, `build/tracer_secondary.img`.
Input trace `TPS_TYPE_OS_KEYS` at make variable (generated/included):
```text
t,p,s,ret,e,x,i,t,ret
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys "t,p,s,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--keys-after-ms 60000 --quit-after "SHELL-EXIT" \
@if grep -q 'triple_fault=1' "build/tps_lex/type_os.report"; then \
	printf '!!! test-tps-type-os FAIL: TPS.COM triple-faulted\n'; exit 1; fi
@grep -q '^SHELL-READY$$' build/tps_type_os.serial \
	|| { printf '!!! test-tps-type-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' build/tps_type_os.serial; then \
	printf '!!! test-tps-type-os FAIL: B8 RTL error:\n'; grep 'FILEIO-ERROR' build/tps_type_os.serial; exit 1; fi
	test "$$(grep -c "^$$marker$$" build/tps_lex/type_os.normalized)" -eq 1 \
		|| { printf '!!! test-tps-type-os FAIL: %s marker missing/non-singular\n' "$$marker"; exit 1; }; \
@diff -u build/tps_lex/type_rich.full build/tps_lex/type_os.full \
	|| { printf '!!! test-tps-type-os FAIL: full seed/InitechDOS triple bracket differs from fpc\n'; exit 1; }
```

### test-seed-fileio-os

#  recipe to execute (from 'Makefile', line 17521):

Boot/data images named by direct prerequisites or expanded recipe: `build/seed_fileio.img`, `build/tracer_secondary.img`.
Input trace `SEED_FILEIO_OS_KEYS` at make variable (generated/included):
```text
f,i,l,e,i,o,ret,e,x,i,t,ret
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys "f,i,l,e,i,o,ret,e,x,i,t,ret" --keys-after "SHELL-READY" \
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--quit-after "SHELL-DONE" \
@if grep -q 'triple_fault=1' "build/seed_fileio_os.report"; then \
	printf '!!! test-seed-fileio-os FAIL: TRIPLE FAULT running FILEIO.COM\n'; exit 1; fi
@grep -q '^SHELL-READY$$' "build/seed_fileio_os.serial" \
	|| { printf '!!! test-seed-fileio-os FAIL: SHELL-READY missing\n'; exit 1; }
@if grep -q 'FILEIO-ERROR' "build/seed_fileio_os.serial"; then \
	printf '!!! test-seed-fileio-os FAIL: the RTL hit a CF error path:\n'; \
	grep 'FILEIO-ERROR' "build/seed_fileio_os.serial"; exit 1; fi
@grep -qF 'FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR' "build/seed_fileio_os.serial" \
	|| { printf '!!! test-seed-fileio-os FAIL: exact FILEIO line missing on serial\nexpected: %s\ngot:\n' "FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR"; \
	     grep '^FILEIO W1=' "build/seed_fileio_os.serial" || printf '  (no FILEIO result line at all)\n'; exit 1; }
```

The remaining legacy emulator entries cover lex/parse/type/codegen parity, arithmetic/file I/O runtime, compiler corpus execution, independent FPC comparison, self-host/DDC and targeted negative controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/tps/tps.pas:1`|0|0|none|
|`seed/ast.c:1`|0|0|none|
|`seed/ast.h:1`|0|0|none|
|`seed/codegen.c:1`|22|15|`seed/codegen.c:219` `SEED_MUT_NONDET`; `seed/codegen.c:224` `SEED_MUT_FILEIO_SHORT_WRITE`, `SEED_MUT_FILEIO_WRONG_HANDLE`; `seed/codegen.c:570` `SEED_MUT_CODEGEN_FRAME_OFF4`; `seed/codegen.c:594` `SEED_MUT_CODEGEN_FIELD_OFF4`; `seed/codegen.c:613` `SEED_MUT_CODEGEN_VARPARAM_COPY`; `seed/codegen.c:924` `SEED_MUT_CODEGEN_ARRAY_LO_SKIP`; `seed/codegen.c:940` `SEED_MUT_CODEGEN_STRIDE`; `seed/codegen.c:949` `SEED_MUT_CODEGEN_REC_STRIDE`; `seed/codegen.c:1298` `SEED_MUT_CODEGEN_SETL_AS_SETG`; `seed/codegen.c:1305` `SEED_MUT_CODEGEN_SETL_AS_SETG`; `seed/codegen.c:1351` `SEED_MUT_CODEGEN_MUL_AS_ADD`; `seed/codegen.c:1385` `SEED_MUT_CODEGEN_SETL_AS_SETG`; `seed/codegen.c:1405` `SEED_MUT_CODEGEN_SETL_AS_SETG`; `seed/codegen.c:1579` `SEED_MUT_CODEGEN_CHR_OFF1`; `seed/codegen.c:1669` `SEED_MUT_CODEGEN_BRANCH_INVERT`; `seed/codegen.c:1943` `SEED_MUT_FILEIO_SHORT_WRITE`; `seed/codegen.c:1948` `SEED_MUT_FILEIO_WRONG_HANDLE`; `seed/codegen.c:2039` `SEED_MUT_CODEGEN_STR_TEMP_CLOBBER`; `seed/codegen.c:2546` `SEED_MUT_CODEGEN_STR_CMP_NOLEN`; `seed/codegen.c:2709` `SEED_MUT_FILEIO_SHORT_WRITE`; `seed/codegen.c:2711` `SEED_MUT_FILEIO_WRONG_HANDLE`; `seed/codegen.c:2727` `SEED_MUT_NONDET`|
|`seed/codegen.h:1`|0|0|none|
|`seed/initechc.c:1`|0|0|none|
|`seed/lexer.c:1`|1|1|`seed/lexer.c:274` `SEED_MUT_NUM_RANGE`|
|`seed/lexer.h:1`|0|0|none|
|`seed/parser.c:1`|1|1|`seed/parser.c:553` `SEED_MUT_PARSE_ADD_AS_MUL`|
|`seed/parser.h:1`|0|0|none|
|`seed/rt/fileio.asm:1`|0|0|none|
|`seed/rt/seed.ld:1`|0|0|none|
|`seed/rt/seed_dos.ld:1`|0|0|none|
|`seed/rt/start.asm:1`|0|0|none|
|`seed/rt/start_dos.asm:1`|0|0|none|
|`seed/token.c:1`|0|0|none|
|`seed/token.h:1`|0|0|none|
|`seed/typecheck.c:1`|0|0|none|
|`seed/typecheck.h:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
