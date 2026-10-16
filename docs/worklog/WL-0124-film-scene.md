# Film scene M024-M026 (initech-tdnl.102/.103/.104)

The requested beads are in .beads/issues.jsonl; bd show cannot resolve them in
this clone. Beads, CLAUDE.md, HANDOFF.md and Git metadata are left to the PM.

flair_live_interactive.img now opts into the existing FLAIR disk-tenant pump.
It mounts the existing data volume and launches APPS/CALC.EXE through tbx_launch,
INT81h and the existing process/update routing. It launches neither the Finder
nor the HELLO/NOTES examples. The two old placeholder records are hidden;
the real Calc windows are arranged through MoveWindow/SizeWindow at the exact
pre-existing fd_win_bounds: ledger(60,80)-(360,300), chart(300,120)-(560,360).
No Calc renderer, spreadsheet, event loop or tenant slot was added to the kernel.
The normal CALC.EXE remains17804 bytes; no new shipped data-volume entry lands.

The staged FILE COPY modal retains its authored placement and Photoshop bar.
CursorMgr now selects its existing frozen hourglass CURS while the scene modal
is up, through the same save-under/ShieldCursor mechanism as the arrow. Escape
accepts both cooked ASCII27 and PS/2 vkey1, as DialogHandleEvent does. Dismissal
restores the arrow and reveals the real Calc File menu; it makes no claim of
successful file I/O. The real ledger accepts edits, and the chart title drag
routes through the existing Window Manager and tenant content-update spine.

Host first: build/film_host_red.log records SCENE_ROUTING RED with the old
no-op arrangement. build/film_emu_red.log records the original interactive
scene failing ledger/grid/pie and BUSY_HOURGLASS checks before implementation.
The Calc host suite now captures the actual Toolbox fills/text, independently
checks the radius40 contour for five visible sectors, and requires all five
literal percentage labels. The film host suite invokes that real tenant and
checks the arrangement verbs, cooked Escape and busy cursor save-under.

Deliberate structural re-keys are in the existing ppm_flair_calc_check, in its
new film modes. Standard Calc mode is unchanged. Its policy coordinates move
from ledger origin(16,87), chart origin(406,122), pie centre(476,197) to film
origins(66,102)/(306,142), then chart origin(366,182), centre(436,257) after the
locked(+60,+40) drag. M025's old blank patch now requires three labels and
three grid rules; M024's original exposed patch now requires actual pie ink.
The dismissed/dragged capture requires all five sector transitions and all
five labels; it also probes the restored old footprint and the570- specimen.
The boot Saving capture grades the literal frozen hourglass silhouette at
hotspot(470,228), origin(462,220). No existing pixel gate covered the interactive
film image; the old static desktop/tick demo images retain their contracts.

Independent sources: PRD6.5/Appendix A/B (the known labels, total,570- and
40/35/18/14/9==116), spec/assets/calc_ledger.json (content transcription,
never loaded by an oracle to compute expected values), and directly read
../system7-decomp/goldens/captures/s8_doc_window_active.png (Platinum title
profile). Calc's pre-existing font-height gate retains NFNT_5415.bin provenance.
The hourglass is the deliberate AM-4 clean-room canon, not a historical Mac
wristwatch. preview.webp is never a pixel source. Unknown ledger amounts stay
TODO_FRAME. The existing small scene bounds clip the Calc footer; its full
literal total remains checked on the real tenant's Toolbox text trace and by
the standalone Calc gate. No invented rows or alternate layout were added.

Rule14: film_scene's RECORD_SCRIPTS/SPEC/MARKER/IMAGE/DATA/SETTLE block is before
record-flair. The locked trace shows the initial Saving arrangement, dismisses
it, drags the chart(+60,+40), and parks the pointer. The interactive image has
no tick lifetime to exhaust during recording. The PM records and reproves the
clip on main after merge; this lane neither records nor closes beads.

Acceptance: build/film_final.log contains test-film-scene (2032checks,0failures),
test-film-scene-mutant, test-flair-live-scene and test-flair-live-scene-mutant
GREEN. Each named host and emu mutant is RED for its own property:
CALC_MUT_NO_PIE -> PIE_DRAW; CALC_MUT_NO_LABELS -> PIE_LABELS;
CALC_MUT_MERGE_SLICES -> PIE_FIVE; CURSOR_MUT_BUSY_ARROW -> BUSY_HOURGLASS.
The same log has make image GREEN and test-kernel-headroom GREEN:181kernels,
flagship181928loaded bytes/14680headroom, binding largest183016/13592headroom,
above the4096-byte floor. build/film_regressions.log has test-calc-tenant-mutant,
test-flair-calc(-mutant), test-flair-live(-mutant), test-flair-desktop(-mutant)
and test-flair-desktop-icons(-mutant) GREEN. build/film_factory.log contains
Makefile ordering, volume contracts(19variants) and CursorMgr(+mutants) GREEN;
build/film_factory_final.log repeats final ordering/volume lint. SourceASCII
scan is clean. Every QEMU gate used INITECH_QMP_STDIO=1; no Bochs or full make
test vector was run. No shipped volume growth, so no purge/launch trace re-key.
