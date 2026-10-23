# DAS source triage

## Auditor corrections and reference boundaries

- L001 is WRONG in proposing that absence of required media must never panic.
  CLAUDE.md Rule 2 allows a contextual fail-loud halt. The accepted film runner
  supplies both images (harness/film_scene.mk:14). There is still a D: the
  no-data trace says FAT mount failed, while the visible screen falsely says
  FLAIR heap exhausted. B08 owns that diagnostic, not a new promise to run the
  disk-launched CALC tenant without its required volume. See
  docs/audits/2026-10-23-das/evidence/film-serial.log and shots/180-film-rest.png.
- L042 is WRONG as a defect in the current progress widget's treatment. Its
  etched 14-pixel body is the independently minted Mac OS 8.1 CDEF 5 treatment
  (spec/progress_platinum.h:11-18, :65), and its lavender-to-teal substitution
  is ratified by ADR-0004-AMENDMENT-DEC-10-Sys8-Platinum-Base-Era.md Sec 6
  ("lavender->teal is canon"). os/flair/control.c:935 applies it. CLAUDE.md
  Law 4 names the Mac OS 8 base era, and Law 2 forbids replacing that oracle
  with preview.webp pixels. The screenshot correctly identifies a difference
  from the prop; it does not overturn the shared widget contract. Any separate
  film-only treatment belongs in the existing initech-c7u1 fidelity discussion.
- L026 remains a usability feature request, not evidence of a broken native
  Scrapbook scrollbar. The guide's printed p.152 explicitly allows clipped
  previews; the bar browses clips, while the implemented arrow keys additionally
  scroll text. B18 adds discoverable mouse reading without inventing a reference
  requirement. L036 likewise concerns an unlabeled edit snapshot, not a stopped
  system clock. Polling must not overwrite a user's draft date/time.

## Scope and verdict discipline

Audit: docs/audits/2026-10-23-das/{REPORT.md,NITS.tsv,evidence/}, L001-L045.
Audited image commit: e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
Source citations pinned to /home/tobias/Projects/initech-os commit
3e44338f53906c23e54fc747a3d689a5d7e0ee2a, 2026-10-23. HEAD advanced to
c995e23f7d366e5999abbc155cad41dcce6835bf during inspection through unrelated
audit additions; the inspected source/spec/harness/manual/ADR paths have no
committed or working-tree differences from the pinned source revision.
Read CLAUDE.md, the 2026-10-05 triage model, this report, AGENTS.md,
PRD 1.4/6.1/6.3/6.4/6.5/Appendices A-B and relevant sources/specs.

All source paths are repository-relative; ../system7-decomp means the sibling
reference repository. Each evidence path in BEADS.tsv is fully relative to
InitechOS. In the table and batch prose, bare shots/ or evidence/ paths are
relative to docs/audits/2026-10-23-das/. Line citations describe inspected
source, not the older image's line numbers. No repository writes, builds,
emulator runs, tracker mutations or live tracker queries were performed.
Acceptance oracles below are proposed work, not passing-test claims. One small
read-only calculation summed the checked-in Chicago advances for L045.

D means a defect in an existing behavior/consumer. F means that the reported
entry point was never implemented, even where shared primitives exist.
Acknowledged limitations in app comments are not automatically deliberate
canon. In particular Paint's bar-carrier scope does not turn unbuilt drawing
and picture I/O into shipped functionality; they are bounded F pickups, not
claims that full Photoshop parity was promised. DOC means only the shipped
manual is wrong. BY-DESIGN here protects the ratified progress treatment.
DUP and LANE are the requested single tracking verdicts, with the underlying
source classification recorded in the note. No missing SSIM implementation,
16-bit compatibility, cooperative scheduling, 116%, 570-, hourglass, teal
desktop or stacked-bar behavior is refiled as a defect.

OPEN-BEADS.txt and LANES-IN-FLIGHT.txt are the authoritative supplied snapshot.
The older offline .beads/issues.jsonl was consulted only to distinguish broad
scope: initech-ncfu is full lineStarts/multistyle and List Manager follow-up;
initech-bf2 is caption/rate implementation; initech-cqej is shutdown acceptance
coverage. None names the specific new adapter/rendering defects below.
The six film-fidelity findings map directly to the current initech-c7u1 title;
initech-wlu is its broader M5 umbrella and is not another create candidate.
Multiple disk-tenant capacity remains on initech-smp0/initech-62hy. B06 only
repairs relaunch identity/activation and the current refusal message.

## Every finding, once

| ID | Verdict | Owner | Note |
| --- | --- | --- | --- |
| L001 | D | B08 | Failed required-media/asset launch is mislabeled heap exhaustion on screen (kmain.c:7084, :921); panic itself is allowed. |
| L002 | D | B01 | DOS and GUI receive the same key (kbd.c:351/364); BREAK ON polls the retained 03h at int21.c:5389. |
| L003 | LANE | initech-mcep | in lane initech-mcep; D: itext.c:329 truncates before the full write/close succeeds at :338-340. |
| L004 | LANE | initech-ycvk | in lane initech-ycvk (queued); D: tpsrep_program.asm:827 ignores FCB write status and restores EAX at :830. |
| L005 | LANE | initech-oobx | in lane initech-oobx; D: itext.c:308 compares the normalized previous output byte, suppressing consecutive LF. |
| L006 | LANE | initech-qxft | in lane initech-qxft (queued); D: calc.c:172 copies fixed LEDGER.TXT, while :146 reloads fixture labels on launch. |
| L007 | LANE | initech-qxft | in lane initech-qxft (queued); D: calc.c:170 exits on Quit/GOAWAY without checking dirty or pending entry. |
| L008 | LANE | initech-mcep | in lane initech-mcep; D: notepad.c:116 retains empty defaults on corrupt load, then :146/321 recreates storage on close. |
| L009 | LANE | initech-qxft | in lane initech-qxft (queued); D: calc.c:29-30 fixes the paths; tbxgate.c:1755 truncates TABLES.TXT without asking. |
| L010 | F | B11 | No keyboard-extension entry point: itext.c:444 and notepad.c:403 call collapse/move-only FlairTEKey (textedit.c:377). |
| L011 | F | B12 | Undo is disabled and has no route/history (itext.c:103, :485); it was never built. |
| L012 | D | B02 | Existing resizable editor consumes fixed CONT_W/CONT_H/WRAP_W (itext.c:74, :501, :521). |
| L013 | D | B02 | show_caret records was after changing g_scroll (itext.c:227, :235); partial paint can retain stale line pixels. |
| L014 | LANE | initech-oobx | in lane initech-oobx; D: itext.c:414 returns silently when FlairTECanReplace refuses capacity. |
| L015 | F | B17 | Geneva 12 is authored only for 20h..7Eh; geneva12.h:161 sends high bytes to one missing box. File bytes are preserved. |
| L016 | F | B10 | Standard File never built a caret/range editor (kmain.c:4975, :5092); command-modified printable keys are appended. |
| L017 | LANE | initech-oobx | in lane initech-oobx; D: kmain.c:5140 requires sf_name_ok but has no invalid-name feedback branch. |
| L018 | D | B03 | calc.c:186 accepts CR only, while cooked Return arrives as LF; Down takes the separate working commit route. |
| L019 | D | B03 | calc.c:172 ignores request refusal and :175 discards FILECOPY completion/error events; diagnostics remain serial only. |
| L020 | F | B14 | Real copy absorbs input (kmain.c:5282); dialog.c:1103 specifies no cancel item. Staged film dismissal is a different path. |
| L021 | F | B13 | calcda.c:52 disables Copy/Paste and :109 routes only Close; the shared Scrap exists but this adapter does not. |
| L022 | D | B04 | Close-only Calculator fails literal-Quit search (tbxgate.c:1550); kmain.c:5720 cancels with serial-only explanation. |
| L023 | F | B15 | paint.c:102 only erases the canvas on update; :107 only handles Quit. Bar-carrier scope is acknowledged, not drawing parity. |
| L024 | F | B16 | Open is disabled at paint.c:39; no chooser or decoder route is present in tbx_app_event at :99. |
| L025 | F | B16 | Save is disabled at paint.c:39; no retained image writer or chooser route exists. |
| L026 | F | B18 | Extra mouse intra-clip reading is unbuilt. Existing bar intentionally browses clips (notepad.c:190/:341); manual p.152 permits clipped previews. |
| L027 | D | B05 | Preference redraw paints desktop/content then only MenuClock (kmain.c:7397-7404), omitting full system menu bar. |
| L028 | LANE | initech-ycvk | in lane initech-ycvk (queued); D: tpsrep_program.asm:135 accepts atoi zero without a validity flag, :138 substitutes one. |
| L029 | LANE | initech-ycvk | in lane initech-ycvk (queued); D: tpsrep_program.asm:718-726 skips every nondigit in numeric fields and returns zero for XYZ. |
| L030 | D | B06 | tbxgate.c:1404 refuses occupied slot before testing identity; kmain.c:3132 collapses it to generic launch error. Not a second-slot request. |
| L031 | D | B07 | film_scene.h:12 shrinks ledger to 300x220 while calc.c:92 draws its total at local y=194 outside the content bottom. |
| L032 | F | B19 | calcda.c:102 presses immediately and :75 paints readout only; process.c:856 drops release events. Held-state feedback is unbuilt. |
| L033 | F | B20 | cursor.c:224 only switches arrow/hourglass; itext has no text-hit-region cursor request. |
| L034 | F | B20 | itext.c:210 always paints the active caret; process.c:856 drops idle/null delivery. Samples alone would not prove no blink. |
| L035 | F | B10 | kmain.c:5192 copies default name and :4983 draws end caret; there is no initial range selection. |
| L036 | F | B21 | cpanels.c:63 samples time on panel entry; core.c:71 keeps an edit snapshot. Missing live view/clear snapshot label, not a stopped RTC. |
| L037 | DOC | B22 | s2-leaving-flair.tex:71 and :87-91 assume no AUTOEXEC; Makefile:10408/10433 installs it. Shipped prompt behavior is correct. |
| L038 | DUP | initech-c7u1 | Same next film-fidelity pass; calc.c:120 rasterizes a flat disk with no separation, perspective or extrusion. |
| L039 | DUP | initech-c7u1 | Same film chart treatment; calc.c:115 uses black/gray/navy shades and :118 white background. Preserve 116 and ratified chrome colors. |
| L040 | DUP | initech-c7u1 | Same film chart label placement; calc.c:134 prints a right-hand list and :139 adds a visible sum. Reference puts labels on wedges. |
| L041 | DUP | initech-c7u1 | Same film composition gap; kmain.c:892 creates two scene windows and :7082 adds Calc only; no reached Drive A Files view is arranged. |
| L042 | BY-DESIGN | CLAUDE Law 4 / DEC-10 | Current 14-pixel etched/teal progress treatment is locked in progress_platinum.h:11, :50, :65 and control.c:935. Do not replace the correct shared widget from a photo. |
| L043 | DUP | initech-c7u1 | Same film dialog furniture gap; dialog.c:1099 constructs a standalone two-item movable dialog, not a document with scroll gutters. |
| L044 | DUP | initech-c7u1 | Same film worksheet furniture gap; calc.c:14 installs only File and :74 begins directly with the A/B grid. Keep canonical Photoshop bar. |
| L045 | D | B09 | notepad.c:299 supplies a 49-byte/296-pixel line; gate limits are 47 bytes/285 pixels (toolbox_gate.h:689-690), and notice drops refusal. |

## Already dispatched: no new TSV rows

- initech-mcep: L003 and L008. Atomic replacement alone is not enough for L008:
  retain the corrupt original and require visible recovery/explicit replacement.
  The existing harness/emu/notepad.mk:95 corrupt-file test expects empty defaults
  and serial telemetry; it never proves that closing preserves recoverable bytes.
  Extend that acceptance to the one-byte-checksum fixture and subsequent close.
- initech-oobx: L005, L014 and L017. Preserve LF-only blank lines, visibly refuse
  capacity, and explain invalid 8.3 names. B10 shares the filename UI but does
  not create another validation item.
- initech-ycvk (queued): L004, L028 and L029. Check each FCB write/flush/close,
  propagate a nonzero result, and validate both numeric configuration and DBF
  numeric data. Keep the 2432-byte independent normal-report golden green and
  mutation-prove the full-disk/non-numeric legs.
- initech-qxft (queued): L006, L007 and L009. Serialize edited worksheet state,
  guard every exit including pending input, and identify/confirm replacement.
  B03 adds only Return/error handling; B14 adds only cancellation. Coordinate
  shared file-layer work with initech-mcep rather than duplicating it.

## New developer pickups in user-harm order

The data-preservation lanes above take precedence. Among new work, GUI Copy can
end the whole desktop and strand unsaved work; then come blocked workflows and
unreachable content, specific failure diagnostics, missing feature entry points,
and polish. Priority 1 is highest. Related causes share one pickup; broad new
Paint functionality is split into bounded drawing and picture-file mechanisms.
Each GUI pickup needs its host oracle, locked emulator trace and record-flair
clip before closure (CLAUDE.md Rule 14), with a named mechanism mutant that
actually turns its acceptance red. New oracle legs named below must be built;
existing suite names alone are not claims that the finding is covered.

### B01 - Keep GUI command keys out of the DOS Control-Break input path (P1)

Covers: L002. New bug candidate.

Root cause: os/milton/kbd.c:351 queues each key in the DOS cooked ring before :364 also posts it to FLAIR; :254 translates Ctrl-C to 03h. os/milton/int21.c:5389 polls that stale byte for BREAK ON and :5392 invokes INT23, whose default terminates at :5341. The desktop itself polls DOS time at os/milton/kmain.c:7412. This is shared input ownership, not a failing Scrap put.

Ground truth: CLAUDE.md Rule 2 does not authorize killing a GUI session for Copy; the local Macintosh System Software Users Guide V6.0, printed pp.125 and 152, describes accessory/document clipboard work. spec/toolbox_gate.h Sec 12 defines the Scrap seam; ADR-0003 Amendment DEC-16 governs the DOS break checkpoints.

Pickup: Give the active GUI and DOS console explicit input ownership across entry, exit and SAMIR suspension. Consume GUI chords once; preserve authentic DOS Ctrl-C and extended-key/typeahead behavior when DOS owns input. Do not fix this by globally disabling BREAK or removing INT23.

Acceptance: Extend keyboard/ring and INT21 host ownership tests, then lock copybreak in QEMU and a Bochs leg: Copy, idle through a clock poll, Paste, drag, close and relaunch must work with no INT23-BREAK/INT21-EXIT. A restored dual-ring leak mutant must terminate/fail this oracle; DOS prompt and child Ctrl-C controls must still pass. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/262-copybreak-before.png, docs/audits/2026-10-23-das/shots/263-copybreak-after-copy.png, docs/audits/2026-10-23-das/shots/265-copybreak-unresponsive.png, docs/audits/2026-10-23-das/evidence/copybreak-serial.log, docs/audits/2026-10-23-das/evidence/scraplimit-serial.log.

### B02 - Use current text-window geometry and repaint every scroll change (P2)

Covers: L012, L013. New bug candidate.

Root cause: os/apps/itext/itext.c:74 fixes CONT_W/CONT_H/WRAP_W and :501 initializes wrapping once; :521 handles updates without adopting resized content geometry. show_caret changes g_scroll at :227 before capturing was at :235, so a successful bar round-trip hides that movement and leaves g_full false. paint skips earlier rows at :186.

Ground truth: ../system7-decomp/specs/toolbox/textedit.md Sec 4 defines wrapping by destRect and clipping/scrolling by viewRect. The audit LINES25 and WRAP fixtures independently expose stale pixels and unreachable text.

Pickup: Share actual content width/height among wrapping, drawing, hit testing, scroll extent and caret following. Capture the pre-change scroll offset and fully repaint whenever it changes. Preserve text bytes and selection across resize; a horizontal route is acceptable only if it actually exposes all text.

Acceptance: Extend test-itext-route/test-textedit with shrink/grow, coordinate and line-visibility assertions. Extend test-flair-itext-scroll with the LINES25 typing sequence and compare the immediate framebuffer with a forced full repaint. Proposed ITEXT_MUT_FIXED_GEOMETRY and ITEXT_MUT_SCROLL_PARTIAL mutants must fail the geometry and immediate-repaint checks respectively. Require a locked trace and record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/083-text-wrap-original.png, docs/audits/2026-10-23-das/shots/087-text-wrap-shrunk.png, docs/audits/2026-10-23-das/shots/088-text-caret-offscreen.png, docs/audits/2026-10-23-das/shots/093-text-scroll-before-refresh.png, docs/audits/2026-10-23-das/shots/094-text-scroll-after-refresh.png, docs/audits/2026-10-23-das/evidence/fixtures/LINES25.TXT, docs/audits/2026-10-23-das/evidence/text-serial.log.

### B03 - Commit ledger Return input and display file-copy failures (P2)

Covers: L018, L019. New bug candidate.

Root cause: os/apps/calc/calc.c:186 commits only CR although the event path delivers LF (the paired handling is documented at os/apps/itext/itext.c:454). calc.c:172 drops the immediate tbx_file_copy result and :175 drops every TBX_EVT_FILECOPY completion. os/milton/kmain.c:5258 returns rate/open errors through that event.

Ground truth: PRD 6.5 promises an editable grid; spec/toolbox_gate.h Sec 12 defines FILECOPY error/completion delivery. Commit-on-Return and visible Save failure are ordinary document contracts, rather than measured Lotus UI claims.

Pickup: Accept both cooked Return encodings. Display a bounded, gate-valid explanation for immediate and asynchronous copy errors, identifying missing input/configuration where known and retaining pending entry/state. Keep document persistence and replacement confirmation on initech-qxft.

Acceptance: Extend harness/proptest/test_calc_tenant.c/test_calc_file_copy.c with CR/LF and immediate/completion refusal cases. Replay apps, copy-zero/bad/empty/missing and missingledger; assert committed value and visible error, with old TABLES.TXT unchanged on refused setup. Restore CR-only and dropped-error mutants to prove failure. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/101-ledger-return.png, docs/audits/2026-10-23-das/shots/102-ledger-committed.png, docs/audits/2026-10-23-das/shots/171-copy-zero-end.png, docs/audits/2026-10-23-das/shots/175-copy-bad-end.png, docs/audits/2026-10-23-das/shots/177-copy-empty-end.png, docs/audits/2026-10-23-das/shots/179-copy-missing-end.png, docs/audits/2026-10-23-das/shots/251-copy-missingledger.png, docs/audits/2026-10-23-das/evidence/extracted-copy/summary.json, docs/audits/2026-10-23-das/evidence/apps-serial.log, docs/audits/2026-10-23-das/evidence/missingledger-serial.log.

### B04 - Let Calculator participate in shutdown and explain a refused quit (P2)

Covers: L022. New bug candidate.

Root cause: os/apps/calcda/calcda.c:43 installs only Close W and :110 handles only that menu word. os/flair/tbxgate.c:1550 searches for a literal enabled Quit item, returns -1 at :1557, and os/milton/kmain.c:5720 explains cannot-ask only to serial before silently cancelling.

Ground truth: The Macintosh guide printed p.125 documents Close as an accessory lifecycle route. spec/toolbox_gate.h Sec 11 and kmain.c:5695 cite cancellable shutdown. This is Calculator integration, not the separate initech-cqej missing declining-app/Bochs acceptance work.

Pickup: Provide an explicit quit/close-request contract for a documentless accessory and wire Calculator to it; retain Close W. Any genuinely unaskable/declining app must leave the desktop usable with a visible reason. Do not kill document tenants as a generic fallback.

Acceptance: Extend test-calcda routing and shutdown host tests; lock Calculator -> Finder -> Shut Down and Restart emulator legs, plus a dirty Text cancel control. A Close-only/unaskable mutant must fail the Calculator leg. Coordinate Bochs coverage with initech-cqej and record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/232-calc-shutdown.png, docs/audits/2026-10-23-das/evidence/calcdeep-serial.log, docs/audits/2026-10-23-das/shots/260-paint-shutdown-refusal.png, docs/audits/2026-10-23-das/shots/243-text-shutdown-completed.png.

### B05 - Redraw both menu bars after applying desktop preferences (P2)

Covers: L027. New bug candidate.

Root cause: os/flair/cp_service.h:53 marks pattern/clock changes dirty. os/milton/kmain.c:7397 responds with desktop_paint_all and content repaint but draws only MenuClock at :7403. The required complete menu-bar passes exist in os/flair/shell.c:369, including the system bar at :378.

Ground truth: PRD 6.3 and CLAUDE.md Law 4 require working menus and the deliberate stacked bars. shell.c:369 defines their compositor order; the intentional second bar is not the defect.

Pickup: Use the normal composition path to repaint both installed bars and the clock after a preference redraw, preserving current foreground menus, modal layering and cursor save-under. Apply the same rule to temporary Sound preview if it triggers dirty redraw.

Acceptance: Extend test-flair-cpanels-settings/persist with pattern, clock show/hide and mouse changes followed by exit and actual menu activation. Grade complete bar strips against independent pre-change captures under a locked clock. Omitting the system-bar repaint must make a mutant red. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/062-pattern-applied.png, docs/audits/2026-10-23-das/shots/069-special-menu.png, docs/audits/2026-10-23-das/shots/071-after-restart.png, docs/audits/2026-10-23-das/shots/249-mouse-fast-motion.png, docs/audits/2026-10-23-das/evidence/main-serial.log, docs/audits/2026-10-23-das/evidence/cpdeep-serial.log.

### B06 - Activate an already-running accessory and report slot capacity clearly (P2)

Covers: L030. New bug candidate.

Root cause: os/flair/tbxgate.c:1404 refuses every occupied-slot launch before comparing the requested identity with g_slot.file. os/milton/kmain.c:3132 maps all negative launch results to FINDER_WIN_ERR_LAUNCH, so same-accessory activation and a different-app capacity refusal become the same generic error.

Ground truth: Macintosh System Software Users Guide V6.0 printed p.125 (evidence/references/mac-guide.txt:5664) explicitly says choosing the accessory again activates its window.

Pickup: Compare full installed application identity and activate the existing tenant for the same accessory without reinitializing it. Explain the current slot limit for a different app. Additional co-resident slots remain on initech-smp0/initech-62hy; this batch does not refile that architecture feature.

Acceptance: Add host launch/activation tests for same executable, same basename in different directories and different app. Replay dalife and Finder/Apple repeated launches; edited Note Pad bytes and page must survive with no reentry. A mutant restoring unconditional busy must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/234-note-duplicate-launch.png, docs/audits/2026-10-23-das/shots/044-scrapbook-second-app.png, docs/audits/2026-10-23-das/evidence/dalife-serial.log, docs/audits/2026-10-23-das/evidence/main-serial.log.

### B07 - Keep the film ledger total within its reachable content area (P2)

Covers: L031. New bug candidate.

Root cause: os/flair/film_scene.h:12 shrinks the ledger frame to 300x220. os/apps/calc/calc.c:74 still paints a 353x252 body and :92 draws the total at local (204,194); the smaller document content bottom clips it. The app has no scroll extent setup or update-time geometry adaptation.

Ground truth: PRD 6.5 and Appendix A require the live ledger and its total; the audit reference film-preview.webp shows that total. The existing authored rest-state geometry contradicts its own implemented content, independently of the missing expense-table fidelity work.

Pickup: Derive layout/scroll bounds from the current content rectangle or set a minimum fitting film size. Make the total reachable in the supplied rest state and after user shrink/grow. Keep its amount, 570- and the 116% chart unchanged. Coordinate visual layout with initech-c7u1.

Acceptance: Extend test-film-scene/test_calc_tenant.c with content-bounds assertions; extend test-flair-live-scene to dismiss the modal, move the chart aside and inspect the total before and after resize. A proposed FILM_MUT_CLIP_TOTAL mutant restoring the clipping size/fixed-layout pair must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/187-film-data-escape.png, docs/audits/2026-10-23-das/shots/190-film-ledger-edit.png, docs/audits/2026-10-23-das/shots/100-ledger-launch.png, docs/audits/2026-10-23-das/evidence/references/film-preview.webp, docs/audits/2026-10-23-das/evidence/filmdata-serial.log.

### B08 - Show the actual film startup failure on the panic screen (P2)

Covers: L001. New bug candidate.

Root cause: os/milton/kmain.c:7084 sends missing-volume, missing-APPS and any CALC load failure to flair_desktop_oom at :7086. That sink prints only FLAIR heap exhausted on screen at :921, including when no allocation failed. harness/film_scene.mk:14 supplies the data image to the accepted scene leg.

Ground truth: CLAUDE.md Rule 2 permits a contextual panic for an invariant failure. evidence/film-serial.log reports FAT mount failure before the launch panic; shots/180-film-rest.png visibly misidentifies it as heap exhaustion. The audit request that absent required media never panic is not the project policy.

Pickup: Preserve the halt policy but carry the mount/load status and a short accurate visible reason identifying the missing required data disk or CALC asset. Distinguish allocation failures. State the two-image requirement in the launch instructions; a new single-disk packaging promise is not implied by this defect.

Acceptance: Add a host failure-to-message mapping test and emulator no-data/missing-APPS/missing-CALC controls alongside the normal attached-data scene leg. The screen must show the real cause and the normal control must boot. A mutant routing all statuses to OOM must fail the pixel/text check. Record the GUI/panic acceptance clip.

Audit evidence: docs/audits/2026-10-23-das/shots/180-film-rest.png, docs/audits/2026-10-23-das/shots/185-film-panic-unresponsive.png, docs/audits/2026-10-23-das/shots/186-film-data-rest.png, docs/audits/2026-10-23-das/evidence/film-serial.log, docs/audits/2026-10-23-das/evidence/filmdata-serial.log.

### B09 - Make Scrapbook capacity notices fit the alert contract (P2)

Covers: L045. New bug candidate.

Root cause: os/apps/notepad/notepad.c:299 passes a 49-byte single line to notice, which ignores tbx_alert failure at :128. spec/toolbox_gate.h:689 allows only 47 bytes plus NUL per line and :690 caps it at 285 Chicago pixels. os/flair/tbxgate.c:882 rejects its byte length; its measured advance is also 296 pixels, so simply increasing byte capacity would not fix it.

Ground truth: spec/notepad/store.h locks the 32-clip capacity; spec/toolbox_gate.h Sec 11 locks alert limits. The ordinary capacity-feedback contract is the auditor stated usability basis, not a claim of changed clip limits.

Pickup: Split/shorten the full and too-large notices into valid lines, check the alert result, and give an actionable visible message without modifying any clip. Audit the nearby TEXT-only notice against the same constraints. Keep the strict gate validator.

Acceptance: Extend test-scrapbook-route with the real alert byte/width validator rather than a success-only alert mock. Replay scrapcapacity at 32/33 and an oversized-clip control; independently decode SCRAPBK.DAT and compare unchanged records. A proposed DA_MUT_OVERLONG_NOTICE mutant restoring the overlong line must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/272-scrap-32-real.png, docs/audits/2026-10-23-das/shots/273-scrap-full-real.png, docs/audits/2026-10-23-das/evidence/scrapcapacity-serial.log, docs/audits/2026-10-23-das/evidence/extracted-scrapcapacity/SCRAPBK.DAT.

### B10 - Give Standard File filename fields caret and selection editing (P2)

Covers: L016, L035. New feature candidate.

Root cause: os/milton/kmain.c:4975 draws only an end caret; :5092 handles end Backspace and :5093 appends printable bytes without checking command modifiers. :5192 copies the default name without selection state. The filename field has no caret/range model.

Ground truth: The independent Mac OS 8 save-dialog capture evidence/references/s8_sf_saveas_default.png shows the selected default name; ../system7-decomp/specs/toolbox/textedit.md Sec 2 supplies the range model. Home/End key details are the audit usability expectation and need a captured contract before claiming native parity.

Pickup: Use the shared edit model for selection, cursor movement, replacement and deletion; initially select the supplied default name. Ctrl-A must select rather than append A. Keep filename validation feedback on initech-oobx (L017), and retain DOS 8.3 policy.

Acceptance: Extend Standard File host UI routing tests and test-stdfile, then lock new-name typing, Ctrl-A, Home/End, Shift selection and replacement cancellation in an emulator Save As trace. A no-initial-selection or append-command-letter mutant must fail. Require independent capture pixels and a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/028-text-save-dialog.png, docs/audits/2026-10-23-das/shots/029-text-saved.png, docs/audits/2026-10-23-das/shots/035-save-name.png, docs/audits/2026-10-23-das/shots/036-save-result.png, docs/audits/2026-10-23-das/evidence/references/s8_sf_saveas_default.png, docs/audits/2026-10-23-das/evidence/main-serial.log.

### B11 - Add anchored Shift-arrow selection to the shared text applications (P2)

Covers: L010. New feature candidate.

Root cause: os/apps/itext/itext.c:444 sends arrow tokens without Shift state; os/apps/notepad/notepad.c:403 does the same. os/flair/textedit.c:351 has only collapse/move semantics and :377 sets both selection ends to the same offset. Shift-click selection already exists, but keyboard extension does not.

Ground truth: ../system7-decomp/specs/toolbox/textedit.md Sec 2 defines half-open selection and active endpoint state. Shift-arrow itself is the auditor stated period-editing expectation; capture its exact boundary/reversal rules independently rather than claiming the generic TEKey spec proves them.

Pickup: Add an anchor/moving-end keyboard adapter shared by Text and Note Pad. Support left/right/up/down extension, reversal, collapse without Shift and document/wrapped-line boundaries while preserving text.

Acceptance: Extend test-textedit/test-itext-route/test-notepad-route with independently specified selections and keyboard sequences; replay the three-line Shift-Left case in both apps. A mutant dropping Shift or moving the anchor must fail. Add a locked emulator trace and record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/025-text-typed.png, docs/audits/2026-10-23-das/shots/026-text-shift-select.png, docs/audits/2026-10-23-das/shots/097-note-long.png, docs/audits/2026-10-23-das/shots/098-note-shift.png, docs/audits/2026-10-23-das/evidence/main-serial.log, docs/audits/2026-10-23-das/evidence/text-serial.log.

### B12 - Provide bounded Undo for destructive InitechText edits (P2)

Covers: L011. New feature candidate.

Root cause: os/apps/itext/itext.c:103 permanently disables Undo; the edit dispatch at :485 has no E_UNDO case and replace_sel at :412 retains no inverse edit. This is a never-built Undo entry point, not an enabled command that lost its history.

Ground truth: Independent Mac OS 8 evidence/references/s8_simpletext_editmenu.png shows Undo; the local TextEdit selection contract supplies offsets. Capture the period one-level edit/undo behavior before choosing coalescing or redo semantics.

Pickup: First pickup: bounded one-level recovery of Cut/Clear/Paste/typing, including selection and dirty state, with no-op behavior when nothing is undoable. Reset history safely for New/Open. Reuse the document engine rather than a private second editor.

Acceptance: Extend test-itext-route with exact before/after bytes for Cut -> Undo, paste replacement and capacity rejection. Replay main and verify both visible restored text and saved bytes. A no-snapshot/no-undo mutant must fail. Record a record-flair clip; do not claim unimplemented multi-level history.

Audit evidence: docs/audits/2026-10-23-das/shots/027-text-cut-undo.png, docs/audits/2026-10-23-das/evidence/references/s8_simpletext_editmenu.png, docs/audits/2026-10-23-das/evidence/main-serial.log.

### B13 - Connect Calculator Copy and Paste to the shared Scrap (P2)

Covers: L021. New feature candidate.

Root cause: os/apps/calcda/calcda.c:48 disables all Edit rows, including Copy/Paste at :52; :109 handles only File Close. No result-to-Scrap or Scrap-to-number path exists although the shared Scrap gate is implemented.

Ground truth: Macintosh System Software Users Guide V6.0 printed p.127 / PDF p.147 explicitly supports copying results and pasting numbers; evidence/references/mac-calc-147.png is the independent page. spec/calcda/calcda_vectors.tsv governs existing arithmetic.

Pickup: Enable result Copy and validated numeric Paste through the existing text Scrap. Preserve calculator state on invalid/empty clipboard data and define accepted numeric syntax from a real-Mac fixture. Coordinate B01 input isolation; arithmetic must keep its existing semantics.

Acceptance: Extend test-calcda and the tenant route mock with exact Scrap strings and independent numeric paste vectors. Lock Calculator Copy -> close -> Text Paste and Text Copy -> close -> Calculator Paste emulator legs. A drop-Copy/ignore-Paste mutant must fail; record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/024-calc-edit-settled.png, docs/audits/2026-10-23-das/evidence/references/mac-calc-147.png, docs/audits/2026-10-23-das/evidence/main-serial.log.

### B14 - Add cancellation to the real modal FILE COPY operation (P2)

Covers: L020. New feature candidate.

Root cause: os/milton/kmain.c:5282 absorbs every event in the real copy loop and never checks Cancel/Escape. os/flair/dialog.c:1101 builds only two display items with no cancel item at :1103. No cancellation route is implemented.

Ground truth: PRD 6.5 / Appendix B defines the paced real FILE COPY operation; spec/toolbox_gate.h Sec 12 defines completion. A Cancel affordance is the auditor stated long-operation usability expectation, not evidence that the staged film prop writes files.

Pickup: Add a visible cancel control and Escape handling with one explicit cancelled completion. Coordinate atomic staging with initech-mcep and destination/persistence semantics with initech-qxft so cancellation preserves the old destination and discards only incomplete temporary output. Preserve the configured progress rate.

Acceptance: Extend test_calc_file_copy host tests for cancellation before open, mid-copy and near completion; replay apps at a slow rate on copied media and compare old destination/source bytes with mtools. An ignored-cancel mutant must fail promptly and unchanged-disk assertions must hold. Add a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/107-ledger-copy-cancel.png, docs/audits/2026-10-23-das/evidence/apps-serial.log.

### B15 - Build a bounded usable InitechPaint drawing surface (P2)

Covers: L023. New feature candidate.

Root cause: os/apps/paint/paint.c:102 handles update by filling a white canvas; the only other route at :107 is Quit. There is no image model, tool state or mouse stroke consumer. :34 disables all other menu rows.

Ground truth: PRD 6.5 says Paint primarily carries the canonical Photoshop menu; it does not establish full Photoshop parity. PRD 1.4 and the auditor stated paint-program expectation ground this additional feature. CLAUDE.md freezes the menu titles, not a ban on drawing.

Pickup: First bounded pickup: retained monochrome raster plus pencil/eraser strokes that survive repaint, movement and resize; expose actual tool selection. Fill, selection, patterns and layers remain explicit later slices on the feature. Keep the canonical menu titles and do not enable nonexistent actions.

Acceptance: Add a host raster/stroke oracle with independently specified boundary pixels, clipping and erase expectations; lock an emulator draw -> obscure/reveal -> resize trace and grade the retained picture. A drop-stroke or clear-on-update mutant must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/126-paint-actual.png, docs/audits/2026-10-23-das/shots/127-paint-draw-actual.png, docs/audits/2026-10-23-das/shots/129-paint-edit-actual.png, docs/audits/2026-10-23-das/shots/132-paint-select-actual.png, docs/audits/2026-10-23-das/shots/134-paint-window-actual.png, docs/audits/2026-10-23-das/evidence/painttps-serial.log.

### B16 - Add Paint picture Open and Save through one verified native format (P2)

Covers: L024, L025. New feature candidate.

Root cause: os/apps/paint/paint.c:39 declares Open and Save with ITEM/G=0 and :99 has no file dispatch, chooser or codec call. The missing image file workflow is separate from the absent drawing engine.

Ground truth: PRD 1.4 supplies Standard File and application services; the specific picture persistence requirement is the auditor stated ordinary paint-program expectation. Choose and lock a period native bitmap format against independent real-software files before implementation; this audit supplies no verified picture-format golden.

Pickup: After B15 defines retained pixels, implement one bounded independently verified bitmap import/export and shared Standard File Open/Save. Include dirty-close protection and safe failure/cancel preservation through the atomic file layer. Broader format/layer compatibility stays open on the feature.

Acceptance: Add host codec tests with independent period files/readback, plus a draw -> Save -> quit -> Open emulator leg with exact pixel comparison and full-disk/invalid-file controls. A dropped row or no-file-write mutant must fail. Require a locked trace and record-flair clip; a private codec round-trip alone is insufficient.

Audit evidence: docs/audits/2026-10-23-das/shots/128-paint-file-actual.png, docs/audits/2026-10-23-das/evidence/painttps-serial.log.

### B17 - Define and render the extended MacRoman text repertoire (P2)

Covers: L015. New feature candidate.

Root cause: spec/assets/geneva12.h:25 bounds the authored strike to 20h..7Eh and :161 maps every other byte to the same missing-glyph box. os/apps/itext/itext.c:153 draws through that face; the audit extraction proves the original high-bit bytes survive Save.

Ground truth: ../system7-decomp/specs/fonts/geneva.md:91 records a real NFNT extending through MacRoman code 217; that Geneva 9 reference establishes the repertoire, not Geneva 12 artwork. PRD 6.4 and CLAUDE.md require clean-room authored glyphs. A matching independent 12-point extended specimen/metrics must be acquired before asserting glyph fidelity.

Pickup: Lock the supported single-byte encoding and author its non-ASCII display repertoire with measured metrics and honest missing-glyph behavior. Keep on-disk bytes intact. initech-6grv concerns the shared Geneva 9 shape mismatch; it is not this Geneva 12 repertoire gap.

Acceptance: Extend font-art/metrics and text-render host oracles against an independent extended-font specimen, then open EIGHT.TXT in an emulator and extract unchanged bytes after Save. A mutant mapping one supported accented code back to missing must fail its pixel oracle. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/078-text-eightbit.png, docs/audits/2026-10-23-das/evidence/fixtures/EIGHT.TXT, docs/audits/2026-10-23-das/evidence/extracted-text/comparison.json, docs/audits/2026-10-23-das/evidence/text-serial.log.

### B18 - Offer mouse reading of long Scrapbook text clips (P3)

Covers: L026. New feature candidate.

Root cause: os/apps/notepad/notepad.c:190 configures the shared bar for clip count/page position; :341 uses it to turn clips. Only Down/Up keys alter intra-clip g_scroll at :398. Mouse scrolling within a clip has no entry point.

Ground truth: The audit explicitly bases this on usability. The Macintosh guide printed p.152 (evidence/references/mac-guide.txt:6764) warns that an oversized Scrapbook image may be fully visible only after pasting elsewhere; its bar browses clips. Do not describe the existing clip-navigation model as disproved by a native scrolling reference.

Pickup: Add a discoverable mouse route to read hidden lines while preserving clip navigation and persistence. Specify whether to add separate text scrolling controls or a clear mode, rather than silently repurposing the existing page bar. Priority is polish because keyboard reading already works.

Acceptance: Extend test-scrapbook-route with one long clip and several short clips; emulator mouse actions must expose LAST and still reach previous/next clips. A mutant ignoring intra-clip mouse steps must fail. Check saved clips unchanged and record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/257-scrap-long-before-mouse.png, docs/audits/2026-10-23-das/shots/258-scrap-long-mouse-no-scroll.png, docs/audits/2026-10-23-das/shots/259-scrap-long-key-scroll.png, docs/audits/2026-10-23-das/evidence/lastui-serial.log.

### B19 - Render Calculator held-key feedback through release delivery (P3)

Covers: L032. New feature candidate.

Root cause: os/apps/calcda/calcda.c:102 executes immediately on mouseDown and command at :75 repaints only the readout. There is no retained pressed key or release case. os/flair/process.c:849 routes keyboard down/autoKey but its default at :856 drops mouseUp/keyUp.

Ground truth: Independent real Mac OS 8 Calculator capture evidence/references/s8_calc_key7_pressed.png shows the held 7 key inverted; spec/calcda/calcda_layout.h defines this app layout.

Pickup: Deliver the needed release/track state through the App Contract and retain one pressed key for painting; clear it on release, drag-out or deactivation according to an independently captured rule. Preserve one arithmetic action per accepted press.

Acceptance: Extend test-calcda routing/render tests with down/held/drag/release and key equivalents. Add a target screenshot during the hold matched to the independent pressed capture. A no-pressed-paint or lost-release mutant must fail; record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/023-calc-held-seven-settled.png, docs/audits/2026-10-23-das/evidence/references/s8_calc_key7_pressed.png, docs/audits/2026-10-23-das/evidence/main-serial.log.

### B20 - Provide text-surface I-beam feedback and timed caret blinking (P3)

Covers: L033, L034. New feature candidate.

Root cause: os/flair/cursor.c:224 selects only busy/hourglass or arrow, while os/milton/kmain.c:1555 only moves the pointer; InitechText has no cursor-shape request. itext.c:210 always draws a solid active insertion caret and acknowledges absent blink at :40. os/flair/process.c:856 drops null/idle events, so a tenant cannot drive TEIdle.

Ground truth: Macintosh guide printed p.28 describes the I-beam/insertion point; ../system7-decomp/specs/toolbox/textedit.md Sec 2 and TEIdle define active caret blinking. CLAUDE.md retains the hourglass during busy operations.

Pickup: Add bounded cursor ownership/hit regions and a cooperative time/idle delivery contract; use an I-beam only over editable text and blink only the active insertion point. Preserve selection drawing and busy-hourglass precedence; reset shape on leaving, deactivation or exit.

Acceptance: Extend cursor/TextEdit/tenant host tests with an injected clock and inside/outside/focus changes. Use phase-separated target samples (not one-second aliasing) and a pointer-crossing trace; mutants forcing arrow or holding caret on must fail independently. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/084-text-pointer.png, docs/audits/2026-10-23-das/shots/085-text-caret1.png, docs/audits/2026-10-23-das/shots/086-text-caret2.png, docs/audits/2026-10-23-das/evidence/text-serial.log.

### B21 - Make Date and Time edit-snapshot behavior clear (P3)

Covers: L036. New task candidate.

Root cause: os/apps/cpanels/cpanels.c:63 reads the clock only when entering a panel; core.c:71 retains original_time for Cancel and :40 renders state.time as Time. There is no live polling while the edit transaction remains open; core.c:84 commits that edited snapshot on OK.

Ground truth: The report states a usability expectation, not a measured real-Mac live-update requirement. The existing cp_state transaction and Cancel/OK behavior in os/apps/cpanels/core.c are the local contract; no evidence shows that the underlying system clock stops.

Pickup: First pickup: label the displayed value clearly as an editable snapshot and explain when OK sets it, or design live display separated from dirty edited fields. Do not overwrite a user edit with polling or certify that the panel is a frozen system clock.

Acceptance: Extend test-cpanels with a locked clock advanced across a minute while the panel stays open, with both untouched and edited fields, Cancel and OK. Target samples must show the agreed snapshot label or live display and unchanged edits. A missing-label/frozen-live-view mutant must fail. Record a record-flair clip.

Audit evidence: docs/audits/2026-10-23-das/shots/055-date-time.png, docs/audits/2026-10-23-das/shots/056-date-leap2000.png, docs/audits/2026-10-23-das/shots/059-date-cancel.png, docs/audits/2026-10-23-das/evidence/main-serial.log.

### B22 - Correct the TPS manual exit procedure for shipped AUTOEXEC media (P3)

Covers: L037. New task candidate.

Root cause: pubs/manuals/tps-report-generator/s2-leaving-flair.tex:71 says the data volume has no AUTOEXEC.BAT and :87-91 inserts date/time prompts. Makefile:10408 writes @ECHO OFF and PATH=A:\DOS, installed at :10433. os/milton/command.c:4781 runs an existing AUTOEXEC and :4797 prompts for date/time only when it is absent.

Ground truth: The shipped distribution recipe and evidence/extracted-tps/AUTOEXEC.BAT are the current ground truth. The DOS startup behavior is correct; this is stale shipped documentation.

Pickup: Update the prose and numbered procedure to the immediate A:\> prompt, retain an explicitly conditional no-AUTOEXEC case if useful, and check procedure step references/screens against the current media. Do not remove AUTOEXEC or add artificial prompts to match stale prose.

Acceptance: Run the applicable test-pubs manual build/render checks and a pristine-media Exit to InitechDOS emulator leg whose transcript reaches A:\> without startup questions. Bind the procedure check to that independent transcript; a mutant restoring unconditional date/time steps must make the check fail. No system behavior change is needed.

Audit evidence: docs/audits/2026-10-23-das/shots/139-dos-start.png, docs/audits/2026-10-23-das/evidence/extracted-tps/AUTOEXEC.BAT, docs/audits/2026-10-23-das/evidence/painttps-serial.log.

## Completion accounting

- 45 findings, exactly one table row per L-id.
- 11 D, 15 F and 1 DOC findings remain new, grouped into 22 pickups.
- 11 findings are LANE (four dispatched owners); 6 are DUP of initech-c7u1;
  1 is BY-DESIGN (L042). There are no new rows for any of these 18 findings.
- BEADS.tsv has a header and 22 candidate rows: 9 bugs, 11 features, 2 tasks;
  1 P1, 16 P2, 5 P3. Each remaining finding is assigned exactly once.
- No tracker items were created. No fix or oracle success is claimed.
- Remaining reference work is explicit: exact Shift-arrow/Undo and Calculator
  paste rules, extended Geneva 12 specimens, and Paint's first supported image
  format need independent fixtures. The stale simpletext-stdfile.md path cited
  in app comments is absent in the current sibling checkout; the retained
  independent audit PNGs and present toolbox/textedit.md were used instead.
