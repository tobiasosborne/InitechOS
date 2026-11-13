# Legacy memory classification

Provenance: the supplied final `.beads/issues.jsonl` has **1,343 `_type: issue` rows and zero memories**. The audit snapshot `b571f1e35cc386803ccd763f427c1cabe41866b9:.beads/issues.jsonl` contains the same 1,343 issue IDs plus **134 `_type: memory` rows**. Those 134 records were recovered by read-only `git show`; their exact key/value objects are preserved in [source-memories.json](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/triage/source-memories.json). No tracker CLI was run.

The reboot README is the authority. These are archival classifications, not executable instructions or restored rulings. Every retained technical/preference value is quoted **in full**, including historical context; the validity note limits what should be carried. RETIRED records are individually indexed with their exact key and a reason. Numbers below are stable 1-based positions in the recovered snapshot, not issue IDs.

| Classification | Count |
|---|---:|
| STILL-TRUE-TECHNICAL | 29 |
| OPERATOR-PREFERENCE | 8 |
| RETIRED | 97 |
| **Total** | **134** |

## STILL-TRUE-TECHNICAL

### 001. `bochs-rfb-display-does-not-render-vga-mode`

**STILL-TRUE-TECHNICAL.** Retain the measured limitation of this Bochs RFB/mode13 build, not a claim that all Bochs versions or VBE modes are black. Temporary client paths are historical.

> Bochs RFB display does NOT render VGA mode 0x13 (initech-6pj/564): even a pristine REAL-MODE fill of mode 0x13 (index 1 everywhere + DAC[1] programmed), halted before the kernel, comes back ALL BLACK over the RFB display plugin (raw framebuffer bytes 0/345600 nonzero; RFB serves the guest as 720x530 true-color 3-3-2). The OS writes the mode-0x13 framebuffer CORRECTLY -- proven 4 independent ways: (1) stage2 real-mode write/readback at 0xA0000 returns the written byte; (2) Bochs debugger write-watchpoint on 0xAF9FF fires (kernel fill reaches the high framebuffer byte); (3) host oracle test-console 8bpp 57 checks; (4) serial shows BANNER-BEGIN/text/BANNER-END through console_putc. So Bochs's RFB plugin (the only headless display in this build) cannot display chained 256-color mode 0x13 -- a Bochs RFB limitation, NOT an InitechOS bug. CONSEQUENCE: the Bochs tri-emulator gate must assert on SERIAL markers + no-triple-fault (resolution/display-independent, the rigorous Rule-5 signal), not a screendump. A visual Bochs screendump of mode 0x13 is not achievable via RFB here; mode 0x12 (640x480x16 planar) MIGHT render on RFB (untested) but needs a planar renderer. /tmp/rfb_capture.py is the (working) RFB framebuffer-capture client.

### 006. `mac-os-8-1-platinum-mint-traps-findings`

**STILL-TRUE-TECHNICAL.** Retain Mac OS 8.1 widget/resource facts and hold-drag-release capture technique. Window offsets, local /tmp paths and timings are setup-specific; discover them again.

> Mac OS 8.1 Platinum mint traps + findings (initech-slwi, 2026-08-16): (1) Title-bar widget order in OS 8 Platinum is close-box LEFT, then at right ZOOM box then COLLAPSE box -- the RIGHTMOST box is the collapse/windowshade (verified by effect). (2) /tmp/os81/lib.sh win_origin reads the child 'Basilisk II' window origin dynamically (+80,+97 on the current Xvfb/metacity setup) -- use it instead of hardcoding. (3) Menus: xdotool mousedown on the menu title, mousemove to the item, then mouseup selects; 'import -window' works fine while button 1 is held, so hierarchical Apple->Control Panels->Appearance is drivable in 3 hold-and-shoot steps. (4) machfs Volume.name reports 'Untitled' regardless -- do NOT trust it; read drVN as a Pascal string at image offset 1024+36 (gave 'Initech81'). machfs parses a 500MB HFS image fine (~15s, 781 files, 726 with parseable resource forks). (5) OS 8.1 has NO 'layo' and NO 'thme' resources anywhere on the volume: the System 7 Finder 'layo' was replaced by 'fvew'/'fcds'/'fmn2'/'fdm2'/'fabi'/'fcol' in the OS 8 Finder, and OS 8.1 shipped Platinum-only (no theme switching). (6) 'wctb' -4096 does not exist; the System suitcase has 'wctb' 0 / 'cctb' 0 and they are still the LEGACY black/white System-7 tables -- Platinum's 3D greys are hard-coded inside the Appearance Extension's WDEF/CDEF 68K code, so screen captures are the only practical colour ground truth. (7) The Platinum control taxonomy IS recoverable: System Folder:Extensions:Appearance Extension carries 28 NAMED CDEFs (Bevel Buttons, DisclosureTriangles, Progress Bar, Little Arrows, Chasing Arrows, Tabs, Separator Lines, Group Box, Image Well, Popup Arrow, Placard, Clock, User Pane, Edit Text, Static Text, Picture, Icon, Window Header, List Box, Push Buttons, Scroll Bars, Popup Button, Radio Group) plus WDEF 0/63/64/65/66/67/124 and MDEF/MBDF 0/63.

### 008. `rule-6-mutation-mutants-that-elide-the-sole`

**STILL-TRUE-TECHNICAL.** Retain the compiler-warning mechanism: a mutation must compile before its behavioral failure is evidence. This does not reinstate mandatory mutant gates.

> Rule-6 mutation mutants that ELIDE the sole call site of a static helper trip -Werror=unused-function -> the mutant fails to BUILD instead of failing the oracle ASSERTION, which makes 'mutant correctly RED' ambiguous (build error vs. real bite). Fix: annotate the helper with __attribute__((unused)) so the mutant still COMPILES and the oracle bites on the assertion. Seen in os/milton/sft.c (SFT_MUTATE_DUP2_NO_RELEASE elides the only sft_release call).

### 011. `testing-gap-lesson-2026-06-20-the-host`

**STILL-TRUE-TECHNICAL.** Retain the seam distinction: a host model does not execute the real interrupt/loader path. Old target names and mandatory gate lists are retired; the reboot interface-smoke tier covers this concern.

> TESTING-GAP LESSON 2026-06-20: the HOST oracle test-exec-unit and the EMU gate test-exec are DIFFERENT -- test-exec-unit (in TEST_UNIT_GATES) does NOT exercise the real int 0x21 AH=4Bh do_exec path that the emu test-exec gate does. An EXEC-path / int21 / loader edit can pass 'make test-unit' (all host gates) yet break test-exec on the emulator. After ANY change to do_exec / the loader EXEC seam / int21 dispatch, re-run the EMU EXEC gates: test-exec, test-program, test-zs24-exec, test-ut6d, test-samir-boot (Rule 5). Do not commit EXEC-path changes on host-green alone.

### 013. `interactive-in-emulator-oracles-inject-keystrokes-via-the`

**STILL-TRUE-TECHNICAL.** QMP key injection and serial-marker synchronization remain reusable techniques. CLI spellings and historical uppercasing claims must be checked against the replacement runner.

> Interactive in-emulator oracles inject keystrokes via the harness: qemu_main --keys "d,i,r,ret" (+ --keys-after <serial-marker>) sends QMP send-key qcode events (letters/digits/ret/spc/dot) after the marker. Built in initech-43b (harness/emu/qemu.c qmp_session). DOS commands are injected lowercase; the shell/COMMAND.COM upcases. This is how test-kbd/test-conin/test-shell drive the guest.

### 014. `sister-repo-dbase3-decomp-git-branch-master-is`

**STILL-TRUE-TECHNICAL.** Retain the III+ 1.1 corpus identity and independently measured file/language facts. Old counts, sibling paths and any subsequently corrected capture are archival, not frozen new-tree requirements.

> Sister repo ../dbase3-decomp (git, branch master) is the dBASE III PLUS 1.1 reconstruction knowledge base that grounds SAMIR/InitechBase M6. It is spec+ground-truth corpus, NOT code. Target is III+ 1.1 ONLY (NOT plain III, NOT IV). Adapted Laws: bytes win over docs; tag confidence [verified]/[documented]/[oracle-resolves]/[inferred]; mark III-vs-IV deltas. Layout: specs/{file-formats,language,commands,functions,full-screen,reports-labels,environment,runtime}/ (byte-verified specs), re/ (RE+MINT harness), goldens/ (real DBASE.EXE/OVL + 10 .dbf/11 .ndx/etc fixtures, gitignored), oracles/ (Harbour etc, semantic-only NOT byte authority), GAPS.md (open A-S), SOURCES.md. MINT harness is LIVE: dosbox-x runs real DBASE.EXE headless (SDL_VIDEODRIVER=dummy, inline ON ERROR, timeout -k, SET ALTERNATE capture). Key minted ground truth for SAMIR: .dbf header YY,MM,DD + +1/+2 terminator variants + optional 0x1A EOF; .ndx numeric keys = RAW LE IEEE-754 doubles compared ARITHMETICALLY (no sign-flip), date keys = Julian Day Number as double; C+N is ERROR #9 'Data type mismatch.' (NO auto-stringification -- that is a PLUS/IV feature); SET EXACT default OFF = directional begins-with; SET DECIMALS=2; rounding ties round toward +infinity (STR(2.5,2,0)=3, STR(-2.5,2,0)=-2); 570- trailing-minus is APP-LEVEL not a dBASE PICTURE (native: leading -, @( parens, @X DB, @C CR); 1/0 -> *-fill overflow not error; DTOS absent in III+; SET DATE AMERICAN/CENTURY OFF. The initech-os SAMIR brief docs/research/dbase-ground-truth.md is the narrower oracle-driven SUBSET of this corpus.

### 024. `reference-2026-10-04-original-period-manuals-are`

**STILL-TRUE-TECHNICAL.** Keep the manifest/reference-set pointer; the old /home/tobias location is historical and availability must be checked before use.

> REFERENCE 2026-10-04: original period manuals are held locally at /home/tobias/Projects/initech-manuals-ref/ (SOURCES.md manifest; copyrighted, NOT in the public repo): complete dBASE III PLUS 1.1 printed set (Learning/Using/Programming -- Law-1 material for SAMIR, previously recorded as unobtainable), Lotus 1-2-3 R2.2 (Reference, Tutorial, Upgrader's), WordPerfect 5.1 Reference + Workbook, Turbo Pascal 6.0 (4 books), IBM DOS 3.x / MS-DOS 3.3, Apple System 6/7 guides. Physical-edition research: docs/packaging/, epic initech-gtge.

### 025. `audit-trap-2026-07-09-a-compound-bash`

**STILL-TRUE-TECHNICAL.** Retain the directory-state pitfall and explicit-path practice; old lane/gate counts are historical.

> AUDIT TRAP (2026-07-09): a compound Bash command that cd'd into .claude/worktrees/agent-* persisted the cwd across later tool calls -- subsequent Makefile/gate inspections silently read the STALE worktree (176+27 gates, 10758-line Makefile) instead of the repo root (296+61, 17298 lines). Always verify pwd (or use absolute paths) before trusting repo-state reads; stale worktrees shadow repo state convincingly.

### 030. `headless-bochs-in-this-env-only-the-rfb`

**STILL-TRUE-TECHNICAL.** Retain RFB handshake and debugger-continue requirements for the documented Bochs build. Geometry, local paths and installed display plugins are configuration-specific.

> Headless Bochs in this env: only the rfb display plugin is built (no nogui/x/sdl/term). display_library:rfb blocks ~30s waiting for a VNC client and does NOT self-proceed to boot. A minimal RFB-3.3 handshake client (security type None) connecting to port 5900 unblocks it fully headless -- no xvfb/sudo/HITL. Reusable client: /tmp/rfb_unblock.py (port hold_secs). Bochs needs ata0:enabled=1 + ata0-master cyl=2,heads=2,spt=32 for the 128-sector image, com1 mode=file for serial, and -rc with 'c' to continue past the integrated debugger.

### 034. `test-fat-initech-5cu-is-the-fat12-differential`

**STILL-TRUE-TECHNICAL.** Retain the independent FAT/mtools comparison and the sentinel test blind spot. Old test names/counts are not a factory obligation.

> test-fat (initech-5cu) is the FAT12 differential oracle: our reader (os/milton/fat12.c via harness/diff/fat_diff/fat_dump.c) vs TWO refs -- mtools (mdir/mcopy) + independent python3 (harness/diff/fat_diff/fat12_ref.py). Assertions: (A) names+sizes triple-agree (our==python==normalized mdir), (B) per-file content our==mcopy==python, (C) the 3 fat12 unit oracles, (D) tools present else fail-loud. Fixtures: hello/second/chain/empty(0B,start_cluster 0)/block.bin(1024B=exact 2-cluster, full last cluster). FINDING: the 0x00-sentinel-stop vs skip distinction is NOT exercised by a natural minted image (trailing dir entries are all 0x00 either way) -- only the SYNTHETIC garbage-after-sentinel case in test_fat12_dir.c catches a sentinel mutation. So the differential image alone underconstrains dir-scan termination; keep the synthetic unit case.

### 049. `flair-live-gui-testing-gap-the-method-2026`

**STILL-TRUE-TECHNICAL.** Retain live composited interaction as distinct evidence from host math. Old finite-pump kernels, command spelling, mouse sign and mandatory clip rules belong to the legacy implementation.

> FLAIR live-GUI testing gap + the method (2026-06-28): the Phase 4.5 waves shipped on GREEN host oracles + a single boot-frame screendump, but NEVER drove the live desktop -- so 138 real bugs (2 P0, ~20 P1) escaped (operator found ~5 in 10s). The oracles 'grade math, not composited pixels or live behaviour' (initech-pipa's words). THE METHOD (was researched in initech-l9cd but unused): drive the live desktop with build/qemu_harness --disk build/flair_live.img (or flair_tenants.img) --mouse 'm<dx>:<dy>,l1,l0' --keys SPEC --keys-after FLAIR-LIVE-READY --screendump --screendump-after <MARKER> ; markers: FLAIR-LIVE-READY/FLAIR-DRAG/FLAIR-MENU/FLAIR-CLOSE (os/milton/kmain.c). Convert PPM->PNG (pnmtopng | convert -filter point -resize 200%) and Read it; sample exact RGB from build/*.ppm with python. NOTE mouse Y is INVERTED in the harness spec (rel +y -> cursor up). Bugs filed under label gui-bughunt; full report docs/FLAIR-GUI-bug-hunt-2026-06-28.md. LESSON: a structural/host oracle is necessary but NOT sufficient for a GUI -- Law 4 requires driving the live desktop + comparing composited pixels to the ../system7-decomp goldens.

### 052. `host-unit-tests-that-stuff-a-host-pointer`

**STILL-TRUE-TECHNICAL.** A 64-bit host pointer cannot round-trip through a 32-bit artifact field. MAP_32BIT is one Linux/x86 option, not a portable API guarantee.

> Host unit tests that stuff a host pointer into an artifact boot_info/uint32_t field (e.g. fake framebuffer/font addr) must allocate in the low 4GiB -- the host has no 32-bit multilib so tests run 64-bit and a truncated pointer segfaults. Use mmap(MAP_32BIT) (see os/milton/test_console.c alloc_low). Artifact code is correct for the 32-bit target; this is a host-test-only concern.

### 061. `flair-mouse-irq12-fo-6-commit-3835b02-the`

**STILL-TRUE-TECHNICAL.** Retain slave-then-master EOI and cascade unmasking facts. Its no-Bochs-loop/ENOMODE conclusion is stale: later Bochs VBE0101h boots and IRQ paths must be assessed on the current profile.

> FLAIR mouse IRQ12 (FO-6, commit 3835b02): the dual-PIC EOI ORDER is load-bearing -- mouse_irq_handler MUST outb(0xA0,0x20) [SLAVE] THEN outb(0x20,0x20) [MASTER]; a master-only EOI leaves the slave ISR bit set and SILENTLY WEDGES all further IRQ12 (proven by the MASTER_ONLY_EOI mutant: 0 distinct FLAIR-MOUSE). A slave IRQ also needs BOTH the master cascade (IRQ2) AND the slave IRQ12 (bit4) unmasked (pic_unmask_irq12). isr.asm uses  so irq12_entry is an inert stub in the 26 non-flair kernels (mouse.o links only into flair_live) -- keeps them byte-stable. The no-wedge property is asserted by injecting >=2 mouse events and requiring >=2 DISTINCT FLAIR-MOUSE on QEMU; flair_live can't exercise IRQ12 on Bochs (640x480 ENOMODE halts before the loop) -> Bochs strict-PIC coverage is follow-up initech-<probe-bead>.

### 062. `march-i386-pin-2026-08-gui-remediation-r3`

**STILL-TRUE-TECHNICAL.** Retain host-compiler CPU-baseline and make flag-invalidation hazards. The old 386 target and clean-everything prescription are not new requirements; pin the supported 486 profile and real dependencies.

> march-i386-pin-2026-08 (GUI remediation R3.1): the freestanding artifact was compiling with the HOST gcc default arch -- cmov (P6+) throughout every kernel -- violating ADR-0001's 386+ target silently; the Bochs legs run cpu model=pentium (no cmov) and only caught it when a new Bochs-exercised path (the R3.1 DESKTOP.DB bootstrap) contained one: PANIC vec=06 after FLAIR-FAT-MOUNT-OK, QEMU green. Fix: -march=i386 pinned in KERNEL_CFLAGS + SAMIR_COM_PROFILE. TWO lessons: (1) any tri-emulator disagreement = investigate the CPU-model delta FIRST (objdump | grep -E 'cmov|bswap|xadd|cmpxchg'); (2) a CFLAGS change does NOT invalidate built objects (objects prereq sources, not the Makefile) -- ALWAYS rm build/*.o or make clean after global flag edits; the stale kernel still contained cmov after the pin until the clean rebuild.

### 068. `qemu-harness-out-must-be-a-short-path`

**STILL-TRUE-TECHNICAL.** Retain the AF_UNIX path-length diagnosis; socket addresses need short paths even when logs live deep in scratch directories.

> qemu_harness --out must be a SHORT path: the QMP unix socket sun_path is 108-char capped, so a deep scratch dir (e.g. the Claude session scratchpad) makes QEMU exit=1 with an EMPTY qemu.log and zero serial -- looks like a boot failure, is a socket bind failure. Discovered 2026-07-21 during the FLAIR solidity drive (WL-0075). Use /tmp/claude-1000/<short> or build/.

### 071. `ground-truth-for-turbo-initech-m8-turbopascal-decomp`

**STILL-TRUE-TECHNICAL.** Retain TP5.5 source/capture provenance and Integer/round/Real findings. Paths, remote availability and TP6 acquisition status are historical.

> Ground truth for Turbo Initech (M8): ../turbopascal-decomp (created 2026-10-24, bead initech-v872) holds the real Turbo Pascal 5.5 golden master (Embarcadero museum tp55.zip, sha256 a04ff655...), a dosbox-x mint harness on Xvfb :88 (tools/remint.sh), and goldens MINT00-05: IDE shots + specs/ide-geometry.md, 70 verified compiler error numbers (specs/compiler-errors.md), runtime error formats (specs/runtime-errors.md), Write/Str/Val formatting vs fpc (specs/write-formatting.md), and the initech-os corpus under real TPC (re/fpc-vs-tp55.md: TP Integer is 16-bit, round() is half-away-from-zero, Real default format differs). TP 6.0 not obtainable (GAPS G-1). Like the other decomp siblings it has NO git remote yet.

### 075. `record-flair-observer-effect-initech-l9cd-v1-1`

**STILL-TRUE-TECHNICAL.** Retain recording observer effect and the need to check dispatched actions. Extra recording kernels and Rule14 obligations are retired.

> record-flair observer effect (initech-l9cd v1.1): per-event frame dumps add ~400ms/event, which silently pushed injection past the guest pump's 250-tick live budget -- the clip LOOKED complete but the trace's back half never dispatched (solid_drag recorded no FLAIR-DRAG). A recording harness must verify the interaction's serial markers, not just count frames; the fix is a widened-budget record image variant, never a bigger default (kernel byte-identity sha-proven). Same class as HER-02: evidence that agrees with itself is not evidence.

### 088. `mutant-structural-healing-2026-07-31-wl-0076`

**STILL-TRUE-TECHNICAL.** Retain structural unreachability and masking-backstop explanations for a mutant turning green. Old required mutation proof/omission paperwork is retired.

> MUTANT STRUCTURAL HEALING (2026-07-31, WL-0076): a correctness fix can make an EXISTING mutation-proof mutant permanently green without the oracle being decoration -- the DQ3 activation seed (rqz5) made every app-switch repaint both tenants fully, so the DESKTOP_MUTATE_NO_PAINTALL_CLEAR stale-footprint stomp became UNREACHABLE in the 2-tenant appswitch trace (and separately, a NEW second backstop like desktop_validate_all can MASK a mutant -- share the knob across both backstops, the winh x ojxn lesson). When a mutant stops biting after a fix: (1) first check for a masking second backstop and put it under the SAME knob; (2) if the defect class is structurally unreachable in that scene, RE-POINT the proof to a scene where the class is still expressible (a directed host case with an extra uninvolved window beats re-tuning the emu trace), retire the emu leg with an OMISSION note at the instantiation site, and keep the oracle tier itself as a live invariant check. Never silently drop the leg and never keep a decorative one.

### 094. `basilisk-ii-mac-os-8-1-mint-traps`

**STILL-TRUE-TECHNICAL.** Retain measured Basilisk/HFS safe-shutdown and input/capture traps for that image. Exact offsets and boot/install times are local measurements.

> Basilisk II Mac OS 8.1 mint traps (initech-kd60, 2026-08-16): (1) Mac OS Setup Assistant in Startup Items HANGS BasiliskII on first boot (100% CPU, input dead) -- trash the alias from the CD boot first; baked out of boot81.img. (2) NEVER SIGKILL BasiliskII with the HFS volume mounted: the MDB still looks perfect (sig BD, blessed dirID) but the volume no longer boots -- Special->Shut Down only; keep a pristine backup. (3) Mac surface is the CHILD window named 'Basilisk II' at root +80,+97 (metacity frame lies). (4) The screenshot Mac cursor is a stale ghost -- not evidence input is dead; test with press-and-hold. (5) Initialize dialog default button is Cancel; Cmd-A does not select-all in its Name field. (6) Boot wait 45s (desktop at ~27s). (7) OS 8.1 install is ~100s in Basilisk, not wall-clock-slow.

### 098. `pm-shell-hazard-make-gate-tail-1-git`

**STILL-TRUE-TECHNICAL.** Retain Bash pipeline exit-status behavior; pipefail/logged exit codes remain relevant in any replacement build.

> PM shell hazard: 'make gate | tail -1 && git commit' commits even when the gate FAILS (the pipeline's status is tail's). Capture the gate to a log and test $? (or set -o pipefail) before any commit/push. Happened 2026-10-25 with test-adr-citations (pre-existing RED, caught after push).

### 102. `nasm-embeds-its-input-file-path-as-an`

**STILL-TRUE-TECHNICAL.** Retain NASM ELF STT_FILE path nondeterminism; normalize paths or compare the intended final binary format for K2/K3.

> NASM embeds its input file PATH as an ELF STT_FILE symbol in every .o -- any reproducibility/differential gate comparing nasm outputs from different directories will false-positive on the path bytes. Fix: cd into the build dir and assemble a bare relative filename (see test-seed-repro's NASM GOTCHA comment). This WILL matter again at the K2==K3 self-host certificate.

### 104. `dosbox-x-mint-harness-2026-09-26-lotus123`

**STILL-TRUE-TECHNICAL.** Retain DOSBox-X/xdotool and period installer observations as version-specific recipes, not universal claims about every emulator release.

> DOSBox-X mint harness (2026-09-26, lotus123-decomp/re/harness-setup.md): never pass -silent (forces SDL dummy video -> black), never end [autoexec] with exit (segfault; use -exit), set [sdl] videodriver=x11 + [dosbox] quit warning=false, drive with xdotool XTEST (not --window/XSendEvent) after windowfocus, never windowactivate --sync without a WM. Real 1-2-3 R2.2 needs INIT (unresolved under emulation; initialized ref tree runs). WP 5.1 installs from one staged dir via Custom install.

### 106. `flair-initechos-host-build-gotchas-proven-wl-0064`

**STILL-TRUE-TECHNICAL.** Retain 64/32-bit ABI truncation and bump-plus-free-list accounting facts. Prefer full-width host ownership maps over relying on truncation collisions. Old lane/build/clip rules are retired.

> FLAIR/InitechOS host-build gotchas (proven WL-0064): (1) the dev host is 64-bit with NO -m32 multilib -- host test binaries are 64-bit, so (int32_t)(uintptr_t)ptr TRUNCATES; recover owners by MATCHING (int32_t)(uintptr_t)app == w->refCon (truncated-both-sides works on host-64 AND flat-32) + a magic tag, never cast refCon back to a pointer; use a full-width void* for per-instance state. (2) the FLAIR heap (heap.c) is BUMP + per-class LIFO free-list: flair_free does NOT roll back the bump cursor and flair_heap_avail returns size-used (bump tail only) -- so a teardown/leak oracle must assert 'avail STABLE across launch/exit cycles >=2 (free-list reuse), a leak drifts it monotonically DOWN', NOT 'avail returns to pre-launch' (which reds a correct impl). (3) Law-4 eyeball as an agent: build the image -> build/qemu_harness --disk IMG --screendump --screendump-after MARKER -> pnmtopng the .ppm -> Read the .png. (4) parallel coding lanes must verify via direct gcc to /tmp, never concurrent make (build/ race).

### 109. `ruled-pnc-1-operator-2026-06-08-initechos`

**STILL-TRUE-TECHNICAL.** Retain the implemented language split: OS in C, compiler in Pascal. Old ADR authority, cross-toolchain mandate and pending-doc cleanup no longer apply.

> RULED (PNC-1, operator 2026-06-08): InitechOS is initially built in C (i686-elf cross-compile, per ADR-0002). Turbo Initich is the Pascal self-hosting compiler / North Star (ADR-0007, pending), NOT the OS implementation language. Consequence: CLAUDE.md Law 3 ('artifact is Pascal') must be revised -> OS artifact = C; Pascal confined to Turbo Initech. The seed Pascal compiler (initech-znb, built) is re-contextualized as the genesis of Turbo Initech, not the OS bootstrap. PRD §6.7/§14 to update accordingly.

### 110. `flair-compositor-oracle-gap-jmc5-qi8v-2026-07`

**STILL-TRUE-TECHNICAL.** Retain testing of the erased/background region and the stale-damage invariant. The historical bug is not automatically open today.

> FLAIR compositor oracle gap (jmc5/qi8v, 2026-07-07): tools/ppm_flair_appswitch_check.c graded an app-switch GREEN while the background NOTES window was fully erased to teal -- because it only probes the FOREGROUND (HELLO overlap + accent + menu band), never the BACKGROUND window's non-overlap footprint. This is the WL-0066 'grade math/foreground, not the whole composite' lesson recurring. RULE: any live-desktop pixel oracle MUST probe the region that the bug ERASES (the peer/background window's own footprint), not just the actor's. Also: desktop_paint_all did not clear wm->desktop_update, so init-seeded damage leaked into the pump and the first desktop_paint_damage painted it teal -- a full-repaint entry point must invalidate pending damage.

### 113. `host-discipline-addendum-2026-10-16-the-root`

**STILL-TRUE-TECHNICAL.** Retain ENOSPC as a cause of truncated supposedly mismatching outputs. Old resource quotas, build sizes and certificate cleanup mandates are retired.

> HOST DISCIPLINE ADDENDUM 2026-10-16: the root disk hit 0 bytes free at ~01:20 (a peer project's 92 GB tree + lane build dirs); the running certificate then read 'test-tps-gen-os FAIL: TPS-generated execution output is not exact' at ~01:25 -- an ENOSPC-truncated output file, NOT a regression (green alone at 01:5x after the peer freed 31 GB). Rule: check 'df -h /' before starting a vector and when an emu gate reads 'output not exact' / 'screendump missing'; remove finished lane worktrees (git worktree remove) and sibling-clone build/ dirs promptly; the full certificate itself writes about 600 MB in build/.

### 116. `system-8-pivot-re-key-lesson-2026-08`

**STILL-TRUE-TECHNICAL.** Retain equal-RGB/different-provenance lesson and stale-artifact diagnosis; old grader re-key workflow is retired.

> System 8 pivot re-key lesson (2026-08-17, 3knt/i3si): the WL-0076 behavioral-grading discipline paid off at the era flip -- only 3 of 12 emu gates (desktop legs c/d, drag LEG A, solid leg C) needed grader re-keys; every behavior-probing gate survived untouched. TRAP: Platinum's dark stripe #969696 is byte-identical to SYS7 CIDX_PIN_DARK's RGB and the well gray #C0C0C0 to win31 BTNFACE -- value-coincidence tells MUST be re-attributed with sampled-domain provenance comments or they read as leftovers and invite wrong 'dedup'. Stale-fixture trap: a failing solid gate short-circuits before refreshing later legs' dumps (clamp/raise) -- refresh ALL dumps before grading grader re-keys.

### 124. `session-close-trap-2026-11-02-git-pull`

**STILL-TRUE-TECHNICAL.** Retain Git rebase flattening of local merges and the distinction between fetch, rebase-merges and merge; no old session-close rule is reinstated.

> SESSION-CLOSE TRAP 2026-11-02: 'git pull --rebase' on a main that carries LOCAL MERGE COMMITS (lane merges not yet pushed) replays every merged lane commit linearly even when origin has not moved (rebase drops merge commits), and conflicts in the lane's files. Abort with 'git rebase --abort' (main is intact), then 'git fetch origin && git log main..origin/main' -- if empty, just 'git push'. If origin did move, use 'git pull --rebase=merges' or merge origin/main. The CLAUDE.md close protocol's 'git pull --rebase' line assumes a linear local history.

### 132. `clock-skew-gotcha-env-build-artifacts-can-acquire`

**STILL-TRUE-TECHNICAL.** Retain timestamp/skew invalidation hazard; the replacement build should model configuration inputs and reject stale evidence.

> Clock-skew gotcha (env): build/ artifacts can acquire mtimes ~tens of minutes in the FUTURE relative to the system clock, which makes GNU make skip ALL rebuilds (every target looks newer than freshly-edited sources) -> false-green/false-red oracles. Symptom: 'Clock skew detected. Your build may be incomplete.' + a mutation-proof gate (e.g. test-dosmsg-mutant [B]) fails to bite because the intermediate 'make kernel_shell.bin' is a silent no-op. FIX: 'make clean' wipes the future-dated artifacts; rebuild from clean gives clock-consistent mtimes and the gates behave. Always 'make clean' before trusting an incremental oracle run if the skew warning appears.

## OPERATOR-PREFERENCE

### 017. `operator-directive-2026-06-19-flair-is-the`

**OPERATOR-PREFERENCE.** Carry the fidelity/real-reference aesthetic. FULL GUI, 100% breadth, named laws, ADR committees and era sequencing do not override the closed GM list.

> OPERATOR DIRECTIVE (2026-06-19) -- FLAIR is the CROWN JEWEL of InitechOS corporate design. Build the FULL GUI on REAL ground truth: real MEASUREMENTS + real REVERSE-ENGINEERING / SOURCE-CODE-AS-SPEC + GOLDEN GENERATION from real era software. Every chrome metric, font strike, control, cursor, layout, and behavior must derive from actual System 7+ / Windows 3.x software run in emulators (Mini vMac / Basilisk II for Mac; dosbox-x for Win 3.1 -- already installed) OR from authoritative spec/RE sources (Inside Macintosh as published spec; open reimplementations/decomps like Advanced Mac Substitute, Executor; Wine/ReactOS GetSystemMetrics tables; Windows SDK / Petzold). Goldens are GENERATED from real software (real screenshots -> locked fixtures -> chrome/SSIM differential), NOT authored off the frame photo. The GUI must be 100 percent usable + functional, and OBVIOUSLY a bland ripoff of System 7 + Windows (the deadpan chimera, mostly-Mac + Win accents + the canon Photoshop menu bar). NO 'looks right' authoring -- Law 1 (ground truth before code) + Law 4 (look like the frame) + port-and-verify TDD against real references. NOTE: this gives the GUI APPEARANCE a real external golden (chrome/fonts/controls/cursors/layout); the ATKINSON region ENGINE internal byte-format still has no golden (QuickDraw region body is unpublished -- ADR-0005). Governs all epic k8o5 FLAIR work. Warrants a fidelity-methodology ADR (proposed ADR-0006) ratified by committee after the ground-truth research lands.

### 039. `initech-color-canon-operator-2026-06-21-definitive`

**OPERATOR-PREFERENCE.** Carry the screen-versus-print color intent as an aesthetic reference. Old rulings and the never-delete/era pipeline are not binding; use current live-frame evidence for final choices.

> INITECH COLOR CANON (operator, 2026-06-21, DEFINITIVE): The signature 'Initech teal' = VIC-20 cyan (hardware colour 3). SCREEN/phosphor value = #8DDCDC (operator motion read H180 S53 L71; alt VIC decodes #85D4DC Lospec / #87D6DD azulianblue -- prefer the NTSC 6560 / PAL 6561 decode if matching a real VIC). PIGMENT value = #57B19F for PRINTED PROPS ONLY (box/manuals/floppy labels, pre-CRT); the composite/CRT shader carries pigment toward the screen cyan. PALETTE RULE for FLAIR: System 7 (later System 8/Platinum) colours are GOLDEN, graded vs the system7-decomp goldens, EXCEPT (1) replace ALL System-7 LAVENDER tinge (#DADAFF/#CCCCFF/#B3B3DA/#8787B3/#333366 -- the 3D bevel/groove highlights) with Initech teal #8DDCDC (or closest), AND (2) the DESKTOP BACKGROUND = #8DDCDC. Use Win 3.1 colours (win31-decomp: BTNFACE #C0C0C0, navy #000080, etc.) for the additional Windows/Photoshop-side chrome bits. This teal is what gives InitechOS its distinctive character. SUPERSEDES ADR-0004 OD-4 (seafoam #6FA08E -> Initech teal #8DDCDC for the desktop). The preview.webp-sampled palette.h chrome values are NON-canonical (muddy CRT-photo samples).

### 057. `operator-2026-10-16-23-00-the-tps`

**OPERATOR-PREFERENCE.** Carry deterministic, working deadpan busywork as product design, not the lapsed per-feature gate/manual workflow.

> OPERATOR 2026-10-16 23:00: the TPS Report Generator MUST have a dedicated manual (bead under initech-ipu5) replete with mind-numbing busywork irritations (e.g. press SPACE exactly 7 times to proceed) -- these irritations are SPECIFIED, deterministic, gated program behaviour (locked spec rows + emu gate legs + a refusing mutant), played completely straight, then documented as corporate operating procedure.

### 070. `all-project-documents-adrs-worklogs-specs-prd-revisions`

**OPERATOR-PREFERENCE.** Carry the corporate prose/visual voice. Document-control headers and matrices are aesthetic options, not mandatory governance or process machinery.

> All project documents (ADRs, worklogs, specs, PRD revisions) are authored in enterprise corporate-committee 'Initech' house style -- document-control headers, defined terms, passive governance phrasing, sign-off matrices -- for period/aesthetic fidelity (ADR-0003 NFR-7, Experiential Conformance). Operator directive 2026-06-08.

### 081. `initechos-design-soul-the-recursive-joke-a-from`

**OPERATOR-PREFERENCE.** Carry the recursive-joke/deadpan/working-software design. Hardware claims must follow the reboot QEMU/Bochs GM; the old 386/physical-product ambition is R2.

> InitechOS DESIGN SOUL (the recursive joke): a from-scratch OS that at FIRST GLANCE is indistinguishable from a real early-90s corporate product (in-universe: pretend it is the OS shown in Office Space; NEVER stated in the artifact). Layered reveal: (1) looks like genuine 90s corporate software; (2) VibeOS angle -- viewer assumes a vibe-coded AI toy that can't be real; (3) surprise: it REALLY boots on 386-era hardware; (4) the software REALLY works -- with deadpan corporate absurdities played straight (InitechCalc pie chart sums to 116%, hourglass cursor, 570- trailing-minus). Aesthetic: distilled 90s office-cubicle tedium, designed-by-committee blandness. Recursive layer: legit-looking joke-ish features that ACTUALLY WORK (canonical: the vestigial FCB API for one weird DOS 1.0 program, which must be built WITH a companion utility). The presentation NEVER admits the joke.

### 086. `initechos-found-footage-clean-room-aesthetic-operator-2026`

**OPERATOR-PREFERENCE.** Carry reconstruction voice and transparent development, without restoring worklogs/ADRs or rewriting history.

> InitechOS FOUND-FOOTAGE / CLEAN-ROOM AESTHETIC (operator 2026-06-14): the project should increasingly evoke 'found footage' -- going back through the git history should read as if the operator is CLEAN-ROOMING a real Initech OS they FOUND somewhere (reconstruction/archaeology), not greenfield invention. Apply going forward to the VOICE of dev artifacts: commit messages, worklogs, and ADRs increasingly framed as recovery/reconstruction of a discovered artifact. The transparent AI/agent history is consistent -- the agents ARE the clean-room tooling. Builds steadily; do NOT retroactively rewrite history. This connects the transparent dev journey to the final-straight 'discovered on floppies' build.

### 091. `design-stance-governs-all-naming-structure-decisions-the`

**OPERATOR-PREFERENCE.** Carry generic heritage naming and corporate blandness. The never-delete/implement-all structural directive lapses; it cannot block factory removal, pruning dead commands or the closed scope.

> DESIGN STANCE (governs all naming/structure decisions): The blandness is a deliberate, rigorous design stance. Maximal genericness -> keep every canonical name and every vestigial structure, including ones that stopped meaning anything versions ago. Corporate software never deletes; it accretes and ships the cruft forever with a straight face. InitechDOS is not a parody of DOS -- it is DOS with the soul extracted and the legacy lovingly preserved. Implication: prefer generic heritage-canonical names (ADR-0003 DEC-12); implement vestigial structures (PSP, MCB arena, FCB API) IN FULL; never 'modernize away' legacy.

### 105. `initechos-build-transparency-principle-operator-2026-06-14`

**OPERATOR-PREFERENCE.** Carry transparent development versus deadpan product presentation. The physical 5.25in edition is explicitly R2.

> InitechOS BUILD-TRANSPARENCY PRINCIPLE (operator 2026-06-14): only the FINAL build must be completely straight -- the physical 5.25in floppies + period stickers/manuals/box, the 'for posterity' release (see the packaging epic). The DEV JOURNEY is intentionally transparent: public repo, AI/agent git history, CLAUDE.md/PRD/worklogs revealing intent, evolution easily discoverable -- ALL FINE AND INTENDED (part of the VibeOS-angle reveal). The joke gets STEADILY BUILT; do NOT hide the dev process or obsess over premature in-character purity. Only the product PRESENTATION LAYER (README, manuals, box, floppy labels, the OS UI) stays deadpan now.

## RETIRED

### 002. `hardware-era-note-operator-2026-06-21-the`

**RETIRED.** Historical proposed CPU-floor change is already settled by the reboot profile; soft-float and pending-ADR context are stale.

### 003. `initechos-adr-process-operator-directive-2026-06-14`

**RETIRED.** Mandatory ADR-by-agent-committee is retired process, even though corporate voice remains an aesthetic preference in record 70.

### 004. `muse-review-2026-11-05-results-lane-a`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 005. `pm-merge-gate-lists-a-gate-s-rule`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 007. `operator-directive-2026-10-24-supersedes-the-2026`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 009. `operator-directives-2026-10-04-supersede-the-2026`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 010. `size-budget-across-parallel-merges-2026-10-20`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 012. `host-oracle-hang-pattern-conin-get-returns-0`

**RETIRED.** Historical unbound-input hang was addressed by a guard; the required full-aggregate integration ritual is retired. Bind finite test input in future component tests.

### 015. `list`

**RETIRED.** Accidental value "list" contains no useful knowledge.

### 016. `merge-lesson-2026-10-17-a-conflict-resolver`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 018. `opus-lane-content-filter-trap-2026-08-25`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 019. `committee-ruling-2026-06-20-mz-exe-loader`

**RETIRED.** Original MZ ruling includes obsolete addresses, sentinel/refusal wording and overlay deferrals. Keep current flat-MZ ABI source/spec instead; do not transplant this whole record.

### 020. `operator-2026-10-03-still-unhappy-with-the`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 021. `operator-directives-2026-09-25-claude-is-pm`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 022. `operator-offer-2026-10-25-if-codex-refuses`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 023. `operator-rulings-2026-10-24-17-40-after`

**RETIRED.** Mixed overlay/cap/queue/quota ruling is explicitly reversed. Color intent is retained separately (39); date details were superseded by bkmv captures.

### 026. `open-reconciliation-pnc-1-adr-0002-mandates-c`

**RETIRED.** Pending C-versus-Pascal/MZ reconciliation was settled long ago; current language fact is retained separately (109).

### 027. `operator-2026-10-04-1-astra-gpt-6`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 028. `canonical-palette-directive-operator-2026-06-21-flair`

**RETIRED.** System7-now/System8-later staging and superseding old palette rulings are stale; surviving color preference is retained separately (39).

### 029. `flair-north-star-expanded-2026-06-27-adr`

**RETIRED.** Expanded full app-platform charter and prerequisites exceed GM; actual source/API contracts replace this old scope ruling.

### 031. `merge-workflow-v2-epic-initech-0uphq-2026-11`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 032. `operator-directives-2026-10-16-22-00-supersede`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 033. `ratified-operator-directive-2026-06-21-no-relitigation`

**RETIRED.** Architecture-by-ruling and mandatory future verb refactor are retired. Retained code may still separate policy/mechanism; inspect actual interfaces.

### 035. `codex-fan-out-recovery-2026-10-25-tmp`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 036. `flair-canonical-app-priority-operator-2026-06-20`

**RETIRED.** App priority and character/native fork are resolved; full parity and games are R2.

### 037. `flair-fidelity-target-era-axis-the-office-space`

**RETIRED.** System7-first/Platinum-later staging is completed and never-delete architecture mandate lapses. Retain live film provenance in the frame task instead.

### 038. `interrupt-foundation-initech-a5a-done-idt-at-os`

**RETIRED.** PIC remap remains useful in source, but IF=0/all-IRQs-masked and NEXT int21 task describe stale initialization state. Do not carry the record as a current kernel contract.

### 040. `night-2026-10-15-lessons-fifteen-lanes-merged`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 041. `operator-directive-2026-11-06-18-30-claude`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 042. `operator-directives-2026-10-15-supersede-the-2026`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 043. `operator-rulings-2026-11-01-initechbase-ship-w2`

**RETIRED.** Old implement-vs-omit decisions and merge-train/remote deletion rules lapse; freeze a new bounded omission list under y29t4.

### 044. `pm-merge-resolver-lesson-2026-10-17-lane`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 045. `mcb-arena-509-6-part-2-placement-the`

**RETIRED.** Original low-memory arena addresses and no-spec-change design choice are stale; use current loader/high-memory contracts.

### 046. `operator-rulings-2026-10-24-18-30-initechbase`

**RETIRED.** Implement-all III+ waves, overlays, shrinking and fan-out mandate are superseded by closed scope.

### 047. `operator-rulings-2026-10-25-phase-a-kernel`

**RETIRED.** Kernel overlay-first/cap dependency and ratification workflow are explicitly retired.

### 048. `cert-2026-10-16-full-first-person-one`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 050. `host-traps-2026-10-24-wsl-box-1`

**RETIRED.** Mixed old WSL paths, shell aliases, sandbox restrictions and quota setup are host/session-specific; no standing product knowledge.

### 051. `muse-budget-2026-11-06-operator-6-more`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 053. `beads-gotcha-a-feature-task-cannot-depend-on`

**RETIRED.** Beads command/dependency semantics are tool-version/process details, not a durable emulator/toolchain/product fact; re-read current bd help if ever needed.

### 054. `flair-app-contract-first-cut-is-built-boots`

**RETIRED.** Dated first-cut implementation/certificate and NEXT-state list; current source supersedes it.

### 055. `lane-lesson-2026-11-05-codex-lanes-that`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 056. `muse-review-budget-2026-11-05-use-1`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 058. `operator-ruling-2026-10-25-samir-overlays-now`

**RETIRED.** SAMIR overlay sequencing and fixed-size headroom floor were superseded by high-memory model/reboot.

### 059. `pm-lesson-2026-10-17-never-merge-or`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 060. `2026-10-20-sibling-cleanup-all-initech-os`

**RETIRED.** Dated clone cleanup/branch state is superseded by the final reboot README archive pointers.

### 063. `orchestration-operator-cadence-delegated-coding-subagents-mu`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 064. `cert-2026-10-16-assembled-clean-clone-initech`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 065. `muse-review-budget-2026-11-05-use-2`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 066. `operator-ruling-2026-06-20-dos-3-3`

**RETIRED.** Old Appendix-A capstone/FCB-stub acceptance no longer describes delivered FCB implementation or GM contract.

### 067. `pace-2026-10-24-17-45-operator-do`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 069. `agent-worktrees-may-spawn-from-a-stale-base`

**RETIRED.** Historical agent worktree base bug depends on obsolete harness behavior; no persistent product fact.

### 072. `muse-review-2026-11-05-t3-provider-muse`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 073. `operator-ruling-2026-10-25-samir-overlays-use`

**RETIRED.** SAMIR overlay design/rule and image-budget negotiation are historical, not a requirement for the new tree.

### 074. `orchestration-never-run-two-writers-in-the-main`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 076. `codex-audit-lane-launch-2026-10-23-works`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 077. `load-class-gate-reds-2026-10-20-on`

**RETIRED.** Bounded-pump gate kernels and load/clip quota workflow are retired; replacement tests drive the product image externally.

### 078. `codex-exec-cross-session-kill-trap-2026-08`

**RETIRED.** Historical cross-session kill diagnosis was superseded by record79; do not carry a serial-dispatch restriction.

### 079. `codex-kill-trap-update-2026-10-15-16`

**RETIRED.** Dated agent-CLI coexistence observation and announce/serialize protocol are retired, not a guarantee about current tools.

### 080. `gotcha-bd-create-graph-file-dry-run-ignores`

**RETIRED.** Old bd graph/dry-run bug is tracker/tool-version process knowledge; not a current command safety guarantee.

### 082. `operator-allowance-2026-11-05-muse-code-model`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 083. `operator-ruling-2026-10-25-record-flair-has`

**RETIRED.** Recorder-before-overlay merge dependency and holding pattern are retired.

### 084. `session-close-trap-2026-07-31-wl-0076`

**RETIRED.** Old embedded-Dolt/export-only persistence state is repository/process-specific and superseded; no bd invocation was used for this triage.

### 085. `cert-2026-10-15-00-55-main-1c93dde`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 087. `lane-pm-shells-tools-lane-env-sh-before`

**RETIRED.** Legacy lane-env/ccache setup is retired; quoting WSL PATH remains ordinary shell hygiene, not a product carryover.

### 089. `operator-2026-10-24-addendum-to-the-same`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 090. `codex-launch-addendum-2026-10-16-when-codex`

**RETIRED.** Historical agent stdin/OOM/background-job behavior is CLI/version/session-specific; old dispatch constraints lapse.

### 092. `certificate-2026-10-24-operator-ruling-tools-cert`

**RETIRED.** Sharded-certificate and gate-count acceptance ruling is explicitly retired. Its warning against unnecessary repeated acceptance is not a new workflow mandate.

### 093. `muse-2026-11-06-operator-plenty-of-muse`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 095. `flair-live-compositor-clips-each-window-s-chrome`

**RETIRED.** Historical clipped-shadow bug diagnosis is superseded by later compositor development; consult current geometry rather than preserve an unverified open bug memory.

### 096. `merge-lesson-2026-10-17-twice-tonight-kod`

**RETIRED.** Dated lane, merge, certificate, bookkeeping or session workflow; explicitly retired with the legacy factory.

### 097. `operator-2026-10-17-stopping-with-quota-unused`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 099. `host-discipline-2026-09-26-1-only-one`

**RETIRED.** Host-wide serialization, fixed-pump/Makefile-stamp and absolute lane-path rules belong to retired factory; general failure diagnosis is kept in technical records.

### 100. `initechdos-base-os-milestone-m2-codename-milton-is`

**RETIRED.** Original milestone-first and ADR-over-PRD authority were superseded by reboot decisions.

### 101. `muse-budget-2026-11-06-use-4-of`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 103. `toolchain-cdr-0001-operator-2026-06-08-interim`

**RETIRED.** Time-limited host-toolchain deviation and ADR exception process are retired; reproducibility needs a freshly pinned toolchain.

### 107. `kstart-runway-erase-trap-2026-08-25-wl`

**RETIRED.** KERNEL_SECTORS bounce/runway coupling is explicitly retired; do not restore three-way constants in the chunked loader.

### 108. `lane-workflow-2026-10-05-what-worked-what`

**RETIRED.** Old provider sandbox/clone integration/Bochs/host protocol is retired.

### 111. `flair-heresy-purge-management-decree-2026-06-21`

**RETIRED.** Heresy audit combines now-fixed gaps (live loop/SSIM) and retired laws. No current task should inherit the decree wholesale.

### 112. `flair-phase-3-committee-ruling-2026-06-21`

**RETIRED.** Old platform-expansion ordering, kernel-window budget and mandatory86Box signoff are explicitly superseded.

### 114. `merge-resolver-lesson-2026-10-20-a-scripted`

**RETIRED.** Legacy scripted merge/cap backup protocol is retired; the general Bash failure hazard survives in technical record98.

### 115. `operator-2026-10-25-15-30-over-pace`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 117. `bochs-boot-solved-initech-6pj-the-working-bochsrc`

**RETIRED.** Contains an explicitly false current conclusion: Bochs VBE640x480 is unavailable. Later VBE0101h support invalidates this recipe as a whole; salvage BIOS/CMOV/RFB observations separately.

### 118. `experiment-lesson-2026-11-06-initech-7cbu8-exp`

**RETIRED.** Dated SAMIRX experiment/cherry-pick schedule and acceptance contract; keep archive pointers in reboot README, not a live process ruling.

### 119. `new-machine-certificate-2026-10-24-wsl2-box`

**RETIRED.** Historical machine certificate/failure inventory and sharded test method are retired.

### 120. `operator-ruling-2026-10-25-merge-velocity-manual`

**RETIRED.** Once-per-session manual typesetting quota is superseded by regenerate-once-at-end.

### 121. `kernel-sectors-the-kernel-s-reserved-disk-window`

**RETIRED.** Obsolete two-copy sector constants and 0x20000 program ceiling; superseded even before the reboot and invalid for chunked loading.

### 122. `operator-acceptance-2026-10-16-11-00-law`

**RETIRED.** Old clips accepted at historical heads do not certify the new image; geometry/shortfall rulings must not override current closed scope.

### 123. `operator-ruling-2026-06-20-fcb-disposition-the`

**RETIRED.** FCB was subsequently built; old accept-with-FCB-stub milestone state is stale.

### 125. `adr-by-committee-scope-operator-refinement-2026-06`

**RETIRED.** ADR-committee process refinement is retired; it is not an owner request to spawn agents.

### 126. `codex-gpt-6-1-sol-safety-filter-refused`

**RETIRED.** Dated safety-filter/provider routing incident is not a current tool limitation or project design fact.

### 127. `lane-integration-traps-2026-10-04-1-a`

**RETIRED.** Legacy giant-Makefile registration/integration process is retired; ordinary dependency correctness should be handled in component builds.

### 128. `muse-budget-2026-11-06-use-2-of`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 129. `pm-lesson-2026-10-16-never-git-add`

**RETIRED.** Old main-tree multiwriter/commit protocol is retired; no new coordination machinery should be imported.

### 130. `quota-cli-2026-10-20-usr-bin-quota`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 131. `worktree-lane-dispatch-pattern-2026-07-14-b7`

**RETIRED.** Historical native-agent worktree lifecycle bug and shell workaround are tool-version process state, not a new-tree requirement.

### 133. `operator-directive-2026-11-05-supersedes-the-2026`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.

### 134. `operator-directives-2026-10-03-supersede-the-2026`

**RETIRED.** Dated provider/model, session/quota, dispatch or scope ruling; all such rulings lapse under the reboot README.
