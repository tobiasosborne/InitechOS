<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0092 -- THE CANONICAL-APP GOLDENS LAND: REAL LOTUS 1-2-3 R2.2 AND REAL WORDPERFECT 5.1 RUN UNDER THE MINT HARNESS

**Date:** 2026-09-26 (evening; operator: "work alone this session, no
subagents"; media choice ratified in-session; budget stop at 60% weekly/Fable,
closed at 56%/52%)
**Beads:** CLOSED initech-qh83 (I123 P0 Law-1 gate), initech-rv9y (IWORD P0
Law-1 gate). FILED: INIT replay (P2), WK1 gaps F-1..F-5 (P2), WP5 prefix
packets + libwpd oracle (P2), specs/ distillation for both corpora (P1).
**Sister repos (local git, no remote):** `../lotus123-decomp` `dc75536`,
`../wordperfect51-decomp` `8dcaf25` (both amended to untrack runtimes).
**No artifact code changed in this repo** -- docs + beads only.

## Context
The two P0 golden-acquisition beads were `bd human`: an agent cannot choose
the media. The operator asked for "the most period correct" images of both
products; the plans fix Release 2.2 (1989) and WP 5.1. Found and verified on
archive.org, the operator ratified, and the session went on to build both
corpora on the `../dbase3-decomp` pattern and mint goldens from the REAL
programs (Law 1 / Law 2).

## What changed
1. **Media (ratified):** Lotus `lotus-1-2-3-release-2.2` (12 x 360K, IMG+IMD,
   Allways included; the ONLY complete 2.2 dump online); WordPerfect
   `word-perfect-5.1-for-dos-5.1-1992-03-3.5-720-kb-english` (golden master:
   last mainstream 5.1) + `WordPerfect5.1.1989-11-06` (first release,
   cross-check). All md5-ledgered in each corpus `SOURCES.md`, gitignored.
2. **Mint harness** (`tools/mint_drive.sh`, shared): DOSBox-X 2024.03.01 under
   Xvfb, xdotool XTEST keystrokes from a step file, per-step window screenshots
   -- the agent SEES the period program. Traps recorded in
   `lotus123-decomp/re/harness-setup.md` (no `-silent`, no autoexec `exit`,
   `videodriver=x11`, `quit warning=false`, no `windowactivate --sync`).
3. **Lotus:** pristine 123.EXE demands the one-time INIT personalization, which
   refuses every emulated floppy tried (internal DOS and booted MS-DOS 6.22).
   Bypassed with the archive.org initialized tree (75/80 files byte-identical
   to pristine; the 5 that differ are exactly INIT's products) -- runtime only,
   pristine media stay the byte authority. 6 goldens minted (MINT01-05,
   CIRC01) + 38 shipped sample sheets. `wk1_ref.py` decodes all 44 / 1071
   formulas, zero failures; every minted cached value equals the screenshot.
   Facts won: @LENGTH not @LEN; @ROUND away from zero; @MOD sign of dividend;
   @CHOOSE 0-based; @NA=-inf/@ERR=+inf in the cache; 32767 INTEGER vs 32768
   NUMBER; literal 1E300 rejected, 1E99*10 stored as 1e100 shown as `*****`;
   @IF/@INDEX fixed-arity (no count byte) -- the first reader draft's wrong
   assumption mis-parsed 18 sample formulas and was caught by the goldens.
4. **WordPerfect:** real INSTALL.EXE driven end-to-end (Custom install from one
   staged directory, no disk swaps) -> 100-file installed tree; 5 documents
   minted with Reveal Codes screenshots; `wp5_ref.py` (self-checking mirrored
   trailers) parses all to EOF, zero errors, text round-trips. Pinned: the
   variable-length group size counts bytes AFTER the 4-byte header (the
   "size = total" reading is wrong for 5.1 output); margins are in 1200/inch
   WPU; prefix area is exactly 0x147 bytes on a fresh install.

## Frictions
- Four round trips were harness plumbing (dummy SDL driver, stdin-less step
  loop, sed over-match, `pkill -f` self-kill). All now in harness-setup.md.
- INIT.EXE replay remains open (GAPS H-1); 86Box (`initech-x0i`) is the
  natural closer.

## Acceptance
Both P0 beads' acceptance text met and cited in their close reasons; the
corpora are self-describing (README/CLAUDE/SOURCES/GAPS/INDEX + re/dumps).
No `make test` run: no artifact code touched.

## Pointers -- NEXT AGENT START HERE
1. `bd ready`: the P1 specs/ distillation bead is the bridge from goldens to
   `initech-9u8w` (I123 P1) and `initech-l4tj` (IWORD P1); do it before P1 code.
2. Unchanged queue from WL-0091: initech-cnpm (SETMBAR) -> 34dh -> 6k12 ->
   p6st -> zj6w -> yjlo (now with real 1-2-3 numeric goldens to cite).
3. Operator queue shrinks: Lotus/WordPerfect media DONE. Still open: ADR-0009
   numeric ruling; tdnl.12 stage-2 chimera; InitechWord dictionary placement.
