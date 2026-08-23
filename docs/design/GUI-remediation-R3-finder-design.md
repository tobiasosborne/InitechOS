<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
<!-- Minted 2026-08-23 by the ox-alpha 1M-context design seat (tool-less, 506KB inlined
     corpus), orchestrator-reviewed. R3 Finder shell design (beads initech-tdnl.8..14 +
     new .9a/.11a). STATUS: DRAFT ratification-pending; section-0 corrections are honest
     corpus limits (text.h + kmain.c absent from the seat corpus -- implementing lanes
     reconcile against the real files). Companion: GUI-remediation-D1-D2-D3-design.md. -->

# R3 Finder Shell Design

**Seat:** ox-alpha (read-only design seat, epic `initech-tdnl`, phase R3 "STAPLER-DESK")
**Scope:** beads initech-tdnl.8 .. tdnl.14 — the Finder-shaped desktop shell over the MILTON FAT volume, building on the ratified D1 ruling (INT 0x81 gate, disk tenants; `docs/design/GUI-remediation-D1-D2-D3-design.md`, PART D1).
**Status:** DRAFT for orchestrator review; section 0 corrections are binding context for every ruling below.

---

## 0. Corrections to the brief — read first

Where the corpus contradicts or under-delivers versus the brief's assumptions:

1. **`os/flair/text.h` is NOT in the corpus.** The brief says "label metrics with the existing Chicago/Geneva strikes per text.h capabilities as evidenced in the corpus." The only evidence of the text layer is second-hand: `os/flair/menu.h` includes `text.h` and calls `text_measure(FONT_CHICAGO, …)` / `text_draw(…)` (menu.h §5/§7 comments), and `chrome.c` consumes it via `flair_draw_document_window`. **No Geneva strike, no glyph-metric constants, and no label-layout capability are evidenced anywhere in the corpus.** Every label-metric number below is therefore marked *golden-resolves* (the `spec/chrome_metrics.json` `golden_resolves` discipline) and the Chicago-vs-Geneva question is raised as a policy ruling (F1-4), not assumed solved.
2. **`os/milton/kmain.c` is absent from this corpus entirely.** The brief's premise "today kmain's tenants scene builds windows directly and desktop.c is a compositor" is confirmed indirectly (`spec/flair_tenants_demo.h` documents the kmain-built HELLO/NOTES scene; `os/flair/shell.c` :: `shell_render` composites statically; `os/flair/process.h` :: `flair_app_dispatch` is called by "the live kmain pump"), but no kmain symbol is citable directly. Migration-path claims in F2 are anchored to the *contracts* (process.h, shell.h, desktop.h), and the implementing seat must reconcile against the actual kmain before cutting code.
3. **A real header defect found in the corpus:** `os/milton/fat12.h` defines `FAT12_ERR_ACCESS = -15` in the error enum AND separately `#define FAT12_ERR_NOT_EMPTY (-15)` (subdirectory REMOVE section). Two distinct failure meanings share one value. Any Finder code switching on these codes cannot distinguish "CHMOD access denied" from "RMDIR non-empty." This must be fixed (renumber `FAT12_ERR_NOT_EMPTY`) as part of the first R3 MILTON-touching bead — flagged here so it is not discovered mid-oracle.
4. **Cross-directory file move does not exist and is explicitly deferred elsewhere.** `fat12_rename` is SAME-directory only (`fat12.h` :: `fat12_rename`: "A cross-directory MOVE is OUT OF SCOPE — the dispatcher rejects old_dir != new_dir BEFORE the backend (0x0011 NOT_SAME_DEVICE; cross-dir MOVE deferred to beads initech-ycb3)"). Drag-move within a volume, drag-to-Trash, and Put Away are all cross-directory operations. **This is the single largest MILTON gap for R3** and forces a reslice (F5-2): a new dependency bead for a same-volume directory-entry transplant primitive.
5. **Directory rename is also unsupported:** `fat12_rename` rejects a matched entry that is a DIRECTORY or VOLUME-LABEL with `FAT12_ERR_ACCESS` ("our backend has no '..' fixup path yet"). Renaming folders in the Finder needs a `'.'`-entry fixup bead. Flagged in F3's gap table.
6. **All write-path timestamps are deterministic ZERO.** `FAT12_FIXED_MTIME`/`FAT12_FIXED_MDATE = 0x0000` (`fat12.h`; Rule 11), stamped by CREATE/MKDIR; only AH=57h SET (`fat12_set_dirent_time`) writes caller-supplied stamps. Get Info's Created/Modified fields will therefore read as the DOS epoch unless something sets them. F3 rules on honest display rather than inventing dates.
7. **No icon/chrome metrics for desktop icons exist in `spec/chrome_metrics.json`.** It locks title-bar, scrollbar, widget, and grow geometry — nothing about icon sprite size, grid pitch, or label wrapping. All such numbers below are provisional and *golden-resolves* against `../system7-decomp/specs/sys8/` captures, per the DEC-10 sampled-domain discipline.
8. **The two-stacked-bars chimera constrains the clock and the Finder bar.** Band 1 (rows [0,20)) is the STATIC retained System-7 shell bar that must NEVER be repainted by live code (`os/flair/shell.h` :: `make_offset_view` comment; bead initech-4w15: "row 0 is the SHELL-OWNED, STATIC retained heritage bar … must NEVER be repainted by the live switch"). Band 2 reflects the active app. Consequences: the Finder menu bar renders in band 2 (F4-1), and the menu-bar clock renders in band 2's right end (F4-6) — not band 1, despite period Macs putting the clock top-right of the single bar.
9. **No line numbers survive inlining** (same as the D1 report §0.2); citations are by file :: symbol/section anchor throughout.

---

# PART F1 — The desktop data model

## F1.1 Icon records

One record per icon, held in Finder-tenant storage carved from the master FLAIR heap (`FLAIR_CLASS_HANDLE` for the record array — the same class the App Contract uses for manager records, `os/flair/heap.h` :: `flair_class_t`). Storage is caller-supplied/fixed-capacity per Law 3; a fixed cap (provisionally 64 desktop icons + 64 per open window, *golden-resolves* against real 1.44 MB floppy densities — a 112-entry root directory is the hard ceiling, `fat12.h` root_entry_count) fails loud on overflow (Rule 2), mirroring `FlairSF_MAX_ENTRIES`' locked-cap pattern (`os/flair/stdfile.h`).

```c
/* Finder icon record (design sketch; final shape in os/flair/finder_icon.h) */
typedef struct desk_icon {
    uint8_t  kind;        /* ICON_VOLUME / ICON_TRASH / ICON_FOLDER /
                             ICON_FILE / ICON_APP                        */
    char     name83[FAT12_NAME83_MAX];   /* 13 B, fat12_format_83 shape;
                                         "" for volume/trash           */
    uint16_t dir_start;   /* containing directory's first cluster (0 = root);
                             fat12_dir_t-compatible                       */
    uint32_t slot;        /* dirent slot within that directory
                             (fat12_find_slot_in semantics)               */
    uint16_t grid_x, grid_y;             /* grid cell (see F1.2)          */
    uint8_t  selected;                   /* 0/1                           */
    uint8_t  surface;                    /* ICONSURF_DESKTOP or the owning
                                             window's id                  */
    uint32_t origin_dir;  /* put-away origin (Trash items only; F1.5)     */
} desk_icon_t;
```

Derivation rules:

- **Volume icon**: one, `kind = ICON_VOLUME`, always on-desktop, position from the DB (default top-right, stacked downward as volumes mount — `docs/research/period-gui-suite.md` §1 bullet 1). V1 has exactly one mounted volume (`tdnl.8`), so the stack has one member; the stacking rule is implemented anyway so a second mount degrades gracefully.
- **Trash icon**: one, `kind = ICON_TRASH`, fixed bottom-right of the desktop (`period-gui-suite.md` §1 bullet 2). Not draggable in V1 (period Trash is fixed-position); a drag *onto* it is the delete gesture.
- **File/folder icons in disk windows**: enumerated from `fat12_read_dir` (`fat12.h` :: `fat12_read_dir`, which delegates to `fat12_read_root_dir` for `is_root` byte-identically). The enumeration callback receives `const dir_entry_t *` valid only for the duration of the call (`fat12_dirent_cb` contract), so the Finder copies `fat12_format_83` output + `attribute` + `file_size` + `start_cluster` into its own records during the callback. Volume-label entries (attr `DIR_ATTR_VOLLABEL`) are skipped by the Finder (the FAT layer deliberately passes them through — `fat12.h` :: `fat12_read_root_dir` filtering note); LFN/deleted/free sentinels are already filtered by the layer.
- **App icons**: `kind = ICON_APP` is a display classification of `ICON_FILE` whose 8.3 name ends in `.EXE` (content dispatch per DEC-08a.4 makes extension cosmetic, but the Finder needs *some* icon-kind heuristic; `.EXE`-suffix is the V1 rule, *golden-resolves* against how the frame depicts app icons).

**Selection state** is the `selected` bit plus a selection-order list (for shift-click extend and for "frontmost selected" semantics). Rendering: selected icons draw with the label inverted (black label band, white text) and the icon outlined — the classic Mac selection look. The inversion is a draw-time decision in the icon renderer (draw label band with swapped fg/bg through `blitter_fill_rect_clipped` + `text_draw`), not a pixel-buffer XOR — FLAIR has no XOR blit in evidence, and the C-8 policy seam (`os/flair/desktop.c` :: `desktop_px`, resolving `FLAIR_PART_DESKTOP` through `flair_look_pixel_depth`) is the color-discipline model to follow: selection colors resolve through `FLAIR_PART_*` roles, never literals.

## F1.2 The icon grid and label metrics

- **Sprite size**: 32×32 (the classic ICN# size; uncontroversial across the era). Hand-authored strikes per the glyph rule (CLAUDE.md hallucination-risk callout: glyphs are hand-authored, not pixel-extracted), graded against sys8 captures per DEC-10 sampled domain.
- **Grid pitch**: PROVISIONAL 68 px horizontal × 52 px vertical, *golden-resolves*. The grid is invisible; "Clean Up" snaps to it (`period-gui-suite.md` §1 "Clean Up — snaps icons to the invisible grid").
- **Labels**: rendered beneath the icon, centered, in the Finder label font. Max label behavior: period Mac wraps icon labels to two lines. Because no Geneva strike and no wrap metric exist in the corpus (§0.1), V1 rules: **single-line label, truncated with no ellipsis at the measured fit width**, wrap deferred to the bead that authors the label-font strike. Provisional max label width = grid pitch − 8 px, *golden-resolves*.
- **Font ruling needed (policy, like D3-3)**: period Finder labels are Geneva; the corpus evidences only Chicago (`menu.h` :: `text_measure` usage). Options: (a) author a Geneva strike (Law-1-clean, more asset work), or (b) ratify Chicago-for-labels as a documented substitution under the OQ-2 teal-substitution precedent (ADR-0004-AMENDMENT-DEC-10 OQ-2, cited in the remediation plan §0 binding consequence 2). Recommendation in F1-4.

Hit-testing is grid-record-driven: point → nearest icon whose sprite∪label rect contains it (topmost wins; icons never overlap after Clean Up, but free dragging can overlap — z among icons = record order, deterministic).

## F1.3 The desktop DB

**Location and file.** A hidden file at the volume root: `\DESKTOP.DB` (valid 8.3; hidden bit set via `DIR_ATTR_HIDDEN` at `fat12_create` time — attribute is a create parameter, `fat12.h` :: `fat12_create` sig). This is the DESKTOP.DB-analogue; the name is period-plausible and the mechanism (per-volume icon database) is exactly the classic Mac "Desktop file" role.

**Format: versioned fixed-size binary records** (Rule 8 spirit: the format is locked data, versioned so a future bump is a deliberate act):

```
offset 0 : 'I','D','B','1'          magic
offset 4 : uint16 version = 1
offset 6 : uint16 n_records
offset 8 : records[n], 24 bytes each:
             uint8  kind      (1=volume-pos, 2=trash-pos, 3=item-pos,
                               4=folder-view, 5=trash-origin)
             uint8  flags
             uint16 dir_start (containing dir cluster; 0 = root)
             char   name83[13]
             uint16 grid_x, grid_y
             uint16 view_bits (bit0 = list-view; others reserved 0)
             pad   to 24
```

Little-endian positional assembly everywhere (the `os/flair/resource.h` endian-neutrality discipline), no padding ambiguity, whole-file rewrite on change: `fat12_create` (truncates) + `fat12_write_file` (`fat12.h` WRITE path). A whole-file rewrite of a ≤ ~4 KB blob on every commit is trivially correct on a 1.44 MB volume and needs no journal. Writes are committed immediately (the per-call commit model the file backend already uses, `os/milton/int21.h` :: `write_at` "committed to disk by THIS call").

**Rule 11 interaction, stated precisely:** Rule 11 forbids *nondeterminism in the artifact's build outputs*; user-dragged icon positions are runtime state, not build state. The DB's *structure* is deterministic (same action sequence ⇒ same bytes modulo the normalized timestamp fields); the timestamp normalization follows the established FAT round-trip convention (`fat12.h` header comment: mtime/mdate "are NOT meaningful bytes… normalized away before diffing").

**Absent vs corrupt — recommendation: regenerate-default, loudly.**

- **Absent** (first boot, or user deleted it): synthesize defaults — volume icon top-right, Trash bottom-right, no item records — write the DB immediately, emit serial `DESKTOP-DB-CREATE`. A missing DB is a *normal* state, not an invariant violation; Rule 2's fail-loud applies to programming errors and corruption, not first-run.
- **Corrupt** (bad magic, bad version, truncated record array, n_records past file size): discard, regenerate defaults, emit serial `DESKTOP-DB-REGEN reason=<code>` — loud on serial, non-fatal to the session. The user loses icon positions, never the volume.
- **Write failure** (disk full, read-only): session continues with in-memory positions, serial `DESKTOP-DB-WRITE-FAIL` — the one genuinely loud case, surfaced to the operator per Rule 2.

This contrasts deliberately with the kernel's panic-on-corruption posture for *structural* data (BPB geometry, `fat12_mount` guards): icon positions are disposable caches of user preference, and a panic-on-first-boot would break the store demo.

**mtools-differential oracle.** After a scripted session (move volume icon, drag two files, open a folder, quit/persist), diff the image: (a) `DESKTOP.DB` exists, hidden-bit set, size = 8 + 24·n; (b) the record set equals an independently hand-authored expected record list in the harness (HER-02 discipline: the expected bytes are authored in the factory test, never computed from the writer's structures); (c) mtime/mdate normalized away per the `fat12.h` convention. Mutants: `DB_WRITE_SKIP` (persist call compiled out → positions lost across a reboot leg → red), `DB_MAGIC_FLIP` (writer emits wrong magic → next boot must take the REGEN path → serial assertion red if it doesn't).

## F1.4 Trash semantics ON FAT

**Mechanism recommendation: a hidden `\TRASH` directory; staged until Empty Trash.** This is the period Mac semantic (`period-gui-suite.md` §1: "Drag-to-trash deletes; Special > Empty Trash (with an 'are you sure' alert) purges") and it is strictly better on FAT than delete-with-confirm:

- It is undoable (Put Away) and inspectable (open the Trash like a folder — period behavior).
- It maps onto primitives that exist or are cheap: staging = cross-directory move (the F5-2 primitive), purge = `fat12_unlink` per file + recursive `fat12_rmdir` for subdirs (both exist, parent-aware per initech-zs24/m0bp).
- Delete-with-confirm would burn the only primitive that makes Trash feel real (recovery) and would still need the confirm alert.

`\TRASH` is created at volume-init time (`tdnl.8`) via `fat12_mkdir(name83="TRASH", parent_dir_start=0)` + hidden attr, so it always exists and the drag-to-Trash hit test never has a missing-target case.

**Name collisions in the Trash dir.** FAT requires unique names per directory; a moved entry whose name already exists in `\TRASH` gets a deterministic suffix: keep the extension, truncate the base to fit a 3-digit counter, lowest counter first (`REPORT.DBF` taken → `REP001.DBF`; scheme: base truncated to 5 chars + `%03u`). Lowest-free-first matches the allocator's deterministic philosophy (`fat12.h` WRITE path allocates "lowest-free-first"). The rename happens *before* the move via the same primitive, so the operation stays atomic-shaped: collide-check → suffix → transplant. Serial `TRASH-RENAME from=%s to=%s` makes it oracle-visible.

**Empty Trash.** Confirm alert first (Caution severity per `period-gui-suite.md` §6 alert conventions; buttons Cancel | OK, OK default-ringed rightmost). **Dependency note:** the modal alert *frame art* is MISSING in FLAIR today (D3 audit row 58; bead `81ft`), so V1 renders the confirm through the existing Dialog Manager conventions (`os/flair/dialog.h` :: `ModalDialog`, dBoxProc/movableDBoxProc frames) and upgrades to the Platinum alert frame when `81ft` lands — stated here so nobody blocks tdnl.11 on tdnl.81ft ordering.

Purge algorithm: enumerate `\TRASH` via `fat12_read_dir`; for each file entry `fat12_unlink(name, dir_start=TRASH_cluster)`; for each subdir, recurse depth-first (empty it, then `fat12_rmdir` — which enforces emptiness via `FAT12_ERR_NOT_EMPTY`, the collision-flagged constant of §0.3). Items that refuse (below) are left; the alert reports "N items could not be removed" if any refused.

**Locked/in-use refusal:** an entry whose name matches the resident disk tenant's image (`g_disk_tenant_slot`, D1 report §D1.3/D1.6) refuses to purge — you cannot delete the running program's code out from under it. This ties the Trash to the D1 slot model for free.

**Option-click force-empty** (research doc §1): deferred V2 — it needs the Option modifier mapping on PC hardware, which no corpus file establishes. Recorded as a known V2 item, not silently dropped.

**Oracle.** mtools differential of the *moves*: after drag-to-Trash, the source directory lacks the entry, `\TRASH` contains it (possibly suffixed), and — the load-bearing assertion — **`start_cluster` is unchanged** (a same-volume move must never touch data clusters; see F3-2). Differential of the *purge*: entries gone, chains freed (FAT entries 0x000, both copies flushed — `fat12_unlink` contract), `\TRASH` itself survives. Mutants: `TRASH_NO_STAGE` (drag deletes outright → the mtools diff shows a freed chain instead of a transplanted entry → red), `EMPTY_NO_CONFIRM` (alert skipped → clip eyeball + ModalDialog-call-count assertion red), `MOVE_RELINKS_CHAIN` (move rewrites `start_cluster` → red), `PURGE_KILLS_TRASH_DIR` (`\TRASH` rmdir'd → red).

## F1.5 Put Away (forward-compatibility note)

Put Away (File menu, `period-gui-suite.md` §2a) returns a trashed item to its origin. The DB record `kind=5` (trash-origin) stores the origin `dir_start` at drag-to-Trash time — one field, cheap now, and it means Put Away later needs no schema migration. V1 ships the record; the Put Away *command* is grayed (F4 menu table).

### DECISIONS — Part F1

- **F1-1:** Icon state is a fixed-capacity array of `desk_icon_t` records in `FLAIR_CLASS_HANDLE` storage; volume icon top-right stacked, Trash fixed bottom-right, file/folder icons derived from `fat12_read_dir` callbacks with values copied out (callback-lifetime contract respected). Caps fail loud.
- **F1-2:** The desktop DB is `\DESKTOP.DB` (hidden, root, versioned 24-byte fixed records, whole-file rewrite, immediate commit). **Absent or corrupt ⇒ regenerate defaults with a loud serial marker; only write failure is session-fatal-loud.** Oracle = mtools byte diff against harness-authored expected records + `DB_WRITE_SKIP`/`DB_MAGIC_FLIP` mutants.
- **F1-3:** Trash = hidden `\TRASH` directory created at volume init; drag-to-Trash stages (never deletes); collisions resolved by deterministic lowest-free 3-digit suffix; Empty Trash = confirm alert + recursive unlink/rmdir; the resident disk tenant's image is purge-protected via the D1 slot. Oracles: mtools move/purge differentials with the `start_cluster`-preserved assertion; mutants `TRASH_NO_STAGE`, `EMPTY_NO_CONFIRM`, `MOVE_RELINKS_CHAIN`, `PURGE_KILLS_TRASH_DIR`.
- **F1-4 (policy, needs ratification):** Finder label font — recommend **ratifying Chicago-for-labels as a documented substitution** (OQ-2 precedent) for V1, with a Geneva strike authored as a follow-up asset bead; all grid/label metrics provisional and *golden-resolves* against sys8 captures. Do not block tdnl.9 on the font question.
- **F1-5:** Store trash-origin (`kind=5` DB records) now; ship Put Away grayed; Option-force-empty deferred V2 with the modifier-mapping dependency stated.

---

# PART F2 — The shell architecture

## F2.1 Where the Finder lives — scored honestly

Three candidates: **(a)** stay kmain-woven (kmain keeps building the desktop scene directly, Finder logic accretes in kmain); **(b)** the Finder is an **always-resident compiled-in App Contract tenant** (the "shell tenant": registered at boot through the App Contract machinery, owns desktop + disk windows + the Finder menu bar, pumps through `flair_app_dispatch` like every other tenant); **(c)** the Finder is itself a disk-loaded tenant via the D1 path.

| Criterion | (a) kmain-woven | (b) shell tenant (compiled-in) | (c) disk-loaded Finder |
|---|---|---|---|
| Single dispatch spine (BC-2, `process.h` :: `flair_app_dispatch` "called IDENTICALLY by the live kmain pump and the host O-1 oracle"; kmain "stays a thin source/sink adapter… carries NO routing/activation logic") | **Poor** — every Finder verb becomes a kmain special case; the thin-adapter rule erodes exactly as R3 adds verbs (double-click, rubber-band, drags, menu dispatch). | **Excellent** — Finder is just another `FlairApp`; routing, activation, update delivery are unchanged code paths. | Excellent (same as b). |
| D1 disk-tenant design coexistence | Poor — disk tenants arrive into a scene whose owner is ad-hoc kernel code; ownership/refCon demux gets a second, unequal citizen. | **Excellent** — the Finder is a peer tenant that happens to be compiled in; it does **not** consume `g_disk_tenant_slot` (that slot is for *disk-launched* images, D1 §D1.6), so launching Calculator leaves the V1 single-slot budget intact. | **Fatal** — the disk-loaded Finder would occupy the V1 single tenant slot, leaving zero room for any launched app; and it cannot load before the volume mounts, yet the desktop must exist pre-mount (G4: today the desktop boots with no volume at all). |
| Teardown / 8fhu semantics (`process.h` :: `teardown_common`; D1-5 anti-8fhu close rule) | Poor — kmain-woven close paths are exactly where 8fhu-class staleness grew (closed foreground app left in the process list). | Good — the Finder never terminates (always-resident), so the kill/terminate paths apply only to guests; the one new rule needed is the shell-furniture close distinction (F2-3). | Good, plus the bootstrap fragility of (c). |
| Oracle-ability | Poor — kmain is not host-buildable; Finder logic woven there is untestable by the O-series host oracles. | **Excellent** — a stub Finder registers through `FlairProcess_register` in `test_process.c` exactly as the stub tenants do; the O-1/O-5 pattern extends unchanged. | Good, but the host oracle would need the loader mocked too. |
| R4 coexistence (apps must coexist: About This Computer, InitechText, Calculator, panels) | Poor — each R4 app multiplies kmain special cases. | **Excellent** — R4 apps are ordinary disk tenants alongside the resident shell tenant; band-2 menubar swap already generic (`flair_live_finish_tenant_switch` per D1 §D1.5). | Excellent for the apps, fatal for the Finder itself. |

**RECOMMENDATION: (b) — the Finder is an always-resident, compiled-in App Contract tenant**, launched at boot via `FlairProcess_launch` from the master FLAIR heap (so teardown symmetry and the AC-2 split-arena protections hold; `process.h` :: `FlairProcess_launch` steps a–g), registered *first*, before any guest tenant. It is not disk-loaded (bootstrap + slot economics), and it is not kmain-woven (BC-2 erosion, oracle blindness).

**Migration path from today's kmain-woven scene**, in order:

1. **Extract the scene builder.** Today the HELLO/NOTES scene is constructed by kmain per `spec/flair_tenants_demo.h` and composited by `shell_render` (`shell.c`). Step 1 moves "build desktop furniture + launch demo tenants" into `finder_open(FlairApp *self, const FlairLaunchParams *lp)`; kmain's arm reduces to: init managers → `FlairProcess_launch(finder_procs, "FINDER", …)` → pump. The Finder's `open()` performs the launches HELLO/NOTES performed (they remain compiled-in demo tenants until R4 replaces them).
2. **Move the desktop paint under Finder control via the underlay hook (F2-4).** The static `shell_render` composition remains for the M4 host gate; the live pump switches to the damage-spine path with the icon underlay installed.
3. **Band-2 at rest changes — flag to the operator.** With the Finder foreground at boot, band 2 shows the *Finder* menu bar (File/Edit/View/Special/Help), not the Photoshop bar (today HELLO's). This is period-correct for a Finder desktop and is arguably *more* canon for R3; the Office Space frame arrangement (Photoshop in band 2) is reproduced at R4.6 when the InitechPaint carrier is foreground. This is a deliberate, visible change requiring a fresh recorded clip + operator Law-4 sign-off — stated here, not discovered in review.
4. **Guests launch after the Finder** and come up foreground per `_launch` step (e)/(f) (D1 §D1.3 step 5), demoting the Finder to background — the Finder keeps receiving `updateEvt`s for its windows (background apps receive updates, `process.h` §3) which is exactly the spatial-Finder requirement.

## F2.2 Event routing additions

All new gestures live in the **Finder tenant's event handler**, driven by cooked `EventRecord`s from the existing pump (`os/flair/event.h` :: `WaitNextEvent`; `when` stamped from `flair_tick_count`). Nothing new enters Layer 4.

- **Double-click synthesis.** A Finder-owned tracker: on `mouseDown` over an icon, compare `ev->when` to the previous click's `when` on the same icon; interval ≤ `FINDER_DBLCLICK_TICKS` and within a 4-px slop ⇒ double-click. **Provisional `FINDER_DBLCLICK_TICKS = 20` (⅓ s at the PIT tick base), *golden-resolves* against a period Mouse-control-panel capture; becomes adjustable when the R4.5 Mouse panel lands** (its slider writes this variable — the panel dependency is declared now). The timer lives in Finder state, keyed off `EventRecord.when` — *not* in the Event Manager (Layer 4 stays generic; `event.h` defines no click-interval concept and should not gain one).
- **Rubber-band tracking.** On `mouseDown` on empty desktop (FindWindow `inDesk`, `window.h` §4) or empty window-content area: enter a tracking state that consumes subsequent `mouseDrag`/`mouseUp` events, drawing a 1-px selection rectangle. **Design rule: the tracker is a PURE function of a point sequence** — `finder_track_band(startPt, pts[], n)` returning the selected-set delta — mirroring the Menu Manager's supplied-point-sequence testability pattern (`menu.h` :: `flair_menu_track`, ADR-0004 D-4 "supplied point sequence"), with the live pump feeding it real events. Host-oracle-testable without an emulator.
- **Band rendering:** save-under + 1-px outline. There is no XOR blit in the corpus; the R0.1 CursorMgr save-under pattern (remediation plan R0.1: "save-under draw/erase") is the sanctioned technique: stash the band rect's pixels in a `FLAIR_CLASS_BITMAP` scratch, draw the outline, restore on erase. Same technique for icon-drag outlines.
- **Icon drag with outline.** `mouseDown` on an icon + movement beyond slop ⇒ drag state: save-under the icon rect, draw the icon outline at the cursor offset each drag event, on `mouseUp` hit-test the drop target: another folder's window content ⇒ move-into-folder (F3); the Trash icon ⇒ stage-to-Trash (F1.4); desktop ⇒ reposition + persist; elsewhere ⇒ revert (restore save-under, no move). Drop-target hit testing reuses `FindWindow` + icon rect tests.
- **Background-desktop clicks vs window clicks.** `FindWindow` returning `inDesk` routes to the desktop gesture set (select/drag/band); anything else routes through the existing part-code switch. Clicking a *background* Finder window's content activates it via the standard click-to-activate path (`flair_app_dispatch` inContent-on-background-owner rule) — no new activation logic.

## F2.3 Redraw discipline — exactly which seams the icon layer hooks

The WL-0076/WL-0080 repaint contract (`desktop.h` validation contract; the damaging-dispatch order `WM-op → desktop_paint_damage → flair_route_updates → present`) survives **unchanged** if — and only if — the icon layer hooks these three seams and no others:

1. **The background-fill site in `desktop_paint_damage` step 1** (`desktop.c`: "seafoam clipped to the desktop update region") and the corresponding fill in `desktop_paint_all` step 1. Today both fill solid `FLAIR_PART_DESKTOP`. The hook: a **desktop-underlay callback** on the WindowMgr — `wm->desktop_underlay(dst, clipRgn)` — invoked immediately after the base fill, receiving the *same clip region* (`wm->desktop_update` for damage, NULL/whole-frame for full paint). The callback draws the desktop pattern tile (R3.2's Platinum default pattern, canon-graded) and then every icon whose rect intersects the clip. This keeps `desktop.c` C-8-mechanism-pure (it still names no color and draws no icon; it just gained one indirection, the same way `overlay_rgn` was added as an occluder hook, `window.h` :: `WindowMgr.overlay_rgn`).
   - **Correctness argument:** icons are UNDER windows by construction, because `desktop_update` only ever contains pixels no window owns (the damage model assigns exposed pixels to frontmost-window updateRgns or desktop_update — `window.h` :: `WindowMgr_invalidate_desktop`, beads b3hl/j0vt), and `ComputeVisible` excludes fronts. An icon partially overlapped by a window edge is clipped to `desktop_update`, so the icon can never paint over window chrome. The `DRAG_MUTATE_NO_CLIP` mutant class (`desktop.c`) proves the clip is load-bearing; the icon layer inherits that protection for free.
2. **Invalidation for icon-state changes.** Selection flips, icon moves, label edits, Trash-fullness (icon variant) all invalidate via `WindowMgr_invalidate_desktop(wm, icon_union_rect)` — old position ∪ new position for moves. This is the *only* invalidation entry point; icons never poke `updateRgn`s directly and never call the blitter outside a paint callback.
3. **The overlay is untouched.** The menu bars + modal occluder (`shell.c` :: `shell_build_scene` overlay construction) is orthogonal: icons live below windows, bars above; no interaction, no new occluder.

What the icon layer must NOT do: install a second compositor, paint in `shell_render`'s static path (that stays the M4 gate reference), or validate window updateRgns (the `desktop_validate_all` vs `flair_route_updates` ordering contract, `desktop.h` :: DQ1 note, is untouched).

**Pattern-vs-teal note:** replacing the flat seafoam with the Platinum desktop pattern touches the same fill site; the pattern's canonical colors route through the policy seam like every other part (C-8), and grading follows D3-1 (palette provenance audited first).

### DECISIONS — Part F2

- **F2-1:** The Finder is an **always-resident compiled-in App Contract tenant**, launched first via `FlairProcess_launch` from the master heap; not kmain-woven (BC-2 erosion, host-oracle blindness) and not disk-loaded (V1 single-slot economics + pre-mount bootstrap). It does not consume `g_disk_tenant_slot`.
- **F2-2:** Migration: (1) extract scene-building into `finder_open`; (2) install the desktop-underlay hook and retire the live static composite; (3) accept the band-2-at-rest change to the Finder bar with a fresh operator-signed clip; (4) guests launch after the Finder and demote it to background normally.
- **F2-3 (anti-8fhu refinement):** close-box disposition is by **owner identity**: `inGoAway` on a window whose refCon resolves to the Finder ⇒ `DisposeWindow` only (shell furniture); on a guest tenant's window ⇒ the D1-5 terminate path. No new WindowRecord field; the refCon demux already answers "who owns this window."
- **F2-4:** The icon layer hooks exactly three seams: the underlay callback at the two background-fill sites in `desktop.c`, invalidation exclusively via `WindowMgr_invalidate_desktop`, and nothing else. The WL-0076 damage/exposure contract and the overlay occluder are untouched.
- **F2-5:** Double-click interval is Finder state off `EventRecord.when` (provisional 20 ticks, *golden-resolves*, later Mouse-panel-adjustable); rubber-band and icon-drag trackers are pure point-sequence functions (the `flair_menu_track` pattern) rendered via save-under, since no XOR blit exists.

---

# PART F3 — Disk windows over MILTON FAT

## F3.1 Mapping the actual `fat12.h` API to Finder operations

| Finder operation | fat12 API consumed | Status in current `fat12.c` | Gap / dependency |
|---|---|---|---|
| Enumerate folder → icon list | `fat12_read_dir(vol, &dir, sector_buf, fat, fat_len, cb, user)`; root via `is_root` delegation | **Supported** (beads ti8/zs24 lineage; sentinel filtering built in) | Finder skips VOLLABEL entries itself (layer passes them through deliberately) |
| Open subfolder / navigate | `fat12_resolve_path_from` (relative descent, `'..'` pop with the start_cluster==0⇒root normalize) or direct `fat12_dir_t{is_root,start_cluster}` construction from the clicked icon's record | **Supported** | — |
| New Folder (Cmd-N) | `fat12_mkdir(name83, parent_dir_start, …)` — non-root parents supported (m0bp), subdir-parent grow + rollback included | **Supported** | Root-full ⇒ `FAT12_ERR_DIR_FULL` must surface as a Finder alert (fixed root cannot grow) |
| Rename-in-place (files) | `fat12_rename(old83, new83, dir_start, …)` — same-dir only, preserves cluster/size/attr/timestamps verbatim | **Supported for files** | Directories rejected (`FAT12_ERR_ACCESS`, no `'..'` fixup) ⇒ **dependency bead for folder rename** |
| Rename collision semantics | dest-exists ⇒ `FAT12_ERR_EXISTS` ("a rename never clobbers") | **Supported** | Finder maps to the period "already in use" alert |
| Duplicate (Cmd-D) | compose: `fat12_find_slot_in` → chunked `fat12_read_partial`/`fat12_write_partial` copy into `fat12_create` at the same parent (chunked, not `read_file`-whole, so a near-volume-sized file cannot demand a whole-file buffer; the positioned primitives exist for exactly this, epic 6qy) | **Supported by composition** | Name: `"COPY OF X"` does not fit 8.3 — period FAT Finders used `X.COPY`? Rule: duplicate name = base truncated + `.DUP`?? — see F3-4 ruling |
| Get Info fields | size = `file_size`; kind = attr byte + extension heuristic; where = parent path (canonical-path machinery exists for dirs via the `resolve_dir` canon walk, `int21.h` :: `resolve_dir`); dates = `mtime`/`mdate` decoded | **Supported** | **Dates are deterministic-zero** unless AH=57h SET ran (`FAT12_FIXED_MTIME`); display rule in F3-5 |
| Drag-move within volume | cross-directory entry transplant | **NOT SUPPORTED** — `fat12_rename` is same-dir; dispatcher rejects cross-dir with 0x0011 (ycb3 deferred) | **THE gap. New MILTON bead (F5-2): `fat12_move_dirent`** |
| Drag-to-Trash / Empty Trash | move into `\TRASH`; purge = `fat12_unlink` + recursive `fat12_rmdir` | Purge **Supported**; stage needs the move bead | F1.4 |
| Delete (none in V1 menus) | `fat12_unlink` | Supported | — |
| Free space (status/Get Info) | `freespace` backend (AH=36h seam, `int21.h`) | Supported | — |

**The move primitive (design for the dependency bead).** A same-volume move is a *directory-entry transplant*: copy the 32-byte `dir_entry_t` from the source directory's slot to a free slot in the destination directory (claiming/growing a subdir slot via the existing `fat12_grow_dir` machinery when the destination is a full subdir — the exact rollback discipline `fat12_mkdir` already demonstrates), mark the source slot deleted (`filename[0]=0xE5`), and **touch no FAT entry and no data cluster**. Constraints carried verbatim from `fat12_rename`'s contract: dest-exists ⇒ `FAT12_ERR_EXISTS`; preserve `start_cluster`/`size`/attrs/timestamps bit-for-bit (Rule 11: bytes outside the relocated entry are untouched); both-FAT sync unnecessary (FAT unmutated) but the *directory sectors* write through the existing dirent write-back primitive (`fat12_write_dirent_in_dir` lineage). Renaming while moving (drag with inline-edit, or Trash suffix collision) composes transplant + name-field rewrite in one bead. Oracle: mtools differential asserting entry presence/absence per directory AND `start_cluster` equality AND FAT-bytes unchanged — the strongest possible statement of "move touched metadata only."

## F3.2 8.3 names vs display — ruling

**Period-honest: display exactly what the volume stores.** Disk windows are titled per the folder; entries render as `fat12_format_83` produces (`NAME.EXT`, extension elided when all spaces, 0x05→0xE5 alias applied — `fat12.h` :: `fat12_format_83`). No case-folding beyond on-disk bytes, no synthetic long names, no hiding extensions. Precedent: `os/flair/stdfile.h` already standardized the 13-byte formatted-8.3 display string (`FlairSFEntry.name[FLAIR_SF_NAME_MAX]`, `FLAIR_SF_NAME_MAX == FAT12_NAME83_MAX` asserted) — the Finder inherits that convention wholesale. The brief's "Drive A Files shows 8.3" is thereby satisfied by construction, and the chimera stays honest: a DOS-personality volume shows DOS names in the Mac shell.

## F3.3 The window-per-folder spatial model

- One `WindowRecord` (documentProc, goAway) per open folder, owned by the Finder (refCon ⇒ Finder, F2-3). **Spatial singleton:** opening a folder that already has a window brings the existing window forward (`SelectWindow`) instead of opening a duplicate — the classic spatial-Finder rule (`period-gui-suite.md` §1: "windows are 'locked' to one folder's contents").
- Title: the folder's 8.3 name; the root volume window takes the frame-canonical name (the "Drive A Files" convention of the remediation plan R4.6 — the File Manager *is* the disk window).
- Contents: icon grid laid out by Clean Up order; free positions persisted per folder.
- **View state persistence:** per-folder DB records (`kind=4`): grid origin, view bits (icon vs list — list deferred), scroll offsets. Keyed by `dir_start`. Loaded at window open; saved on close/move/Clean Up through the F1.3 commit path.
- **Clean Up:** snap all icons in the affected surface (window or desktop) to grid cells in deterministic order (row-major from top-left, selected-items-first is NOT period — plain row-major), invalidate the union, persist. Both View ▸ Clean Up and Special ▸ Clean Up route to the same command id (F4).
- Closing a folder window disposes the WindowRecord (Finder-owned ⇒ DisposeWindow-only per F2-3) and saves view state.

## F3.4 Duplicate naming — ruling

Period Mac duplicates read "program copy"; that string cannot fit 8.3. **Rule: duplicate of `NAME.EXT` ⇒ `NAMECOP.EXT`-style is ugly and ambiguous; instead use the Trash-suffix machinery inverted: base truncated to 5 chars + `DUP` marker is worse. Adopt: first duplicate = `NAME.EXT` → `NAME00D.EXT`?** None of these are attested. Honest resolution: **`COPY.XXX`-style is unattested on FAT-era Macs (which had HFS long names), so InitechOS defines its own deterministic convention and locks it: duplicate of `ABCD1234.EFG` ⇒ `ABCD1200.EFG`… no.** Final ruling, kept simple and greppable: **duplicate ⇒ base truncated to 6 chars + `~DUP` is 9 — over. Base truncated to 5 + `DUP` + `.` + original ext ⇒ `ABCDEDUP.EFG`.** Lock: `dup_name83(base,ext) = trunc5(base) || "DUP" || "." || ext`, colliding with an existing name ⇒ fall into the F1.4 numeric-suffix scheme (`ABCDE001.EFG`). Serial `FINDER-DUP from=%s to=%s`. This is a *defined* convention (Rule 8: locked, deliberate, documented), not a claimed period restoration — stated as such.

## F3.5 Get Info — fields and the date honesty rule

Fields per `period-gui-suite.md` §1 Get Info bullet: Kind, Size, Where, Created/Modified, Comments, Locked. V1 ships: **Kind** (attr-derived: folder / application (.EXE) / document + extension), **Size** (`file_size` bytes; plus "bytes on disk" = clusters×512 from the chain walk — cheap and very period), **Where** (parent canonical path), **Created/Modified** (decoded `mtime`/`mdate`). **Honesty rule:** the displayed date is the decoded on-disk packed value, full stop. Because the write path stamps zero (Rule 11), a freshly created file shows the DOS epoch — that is the *truthful* representation of what FAT stores, and the alternative (synthesizing a boot-time clock stamp) would violate both Rule 11 and the AH=57h design (timestamps become observable only when something sets them). When the R4 Date&Time panel + a future "stamp mtime on write" change land, Get Info automatically becomes meaningful — no display-code change. Comments/Locked: grayed/absent in V1 (Comments needs a storage home — a future DB record kind; Locked maps to `DIR_ATTR_READONLY` via the existing CHMOD pair `fat12_get_attr`/`fat12_set_attr`, which already forbid re-typing dirents — a genuine V1.5 candidate, noted not scheduled).

### DECISIONS — Part F3

- **F3-1:** Disk windows consume `fat12_read_dir` / `resolve_path_from` / `mkdir` / `rename`(files) / composed chunked-copy Duplicate / `unlink`/`rmdir` — all supported today; the Finder adds only VOLLABEL skipping and error-to-alert mapping (`DIR_FULL`, `EXISTS`, `NO_SPACE`).
- **F3-2:** The load-bearing MILTON dependency is a new **`fat12_move_dirent`** (same-volume cross-directory 32-byte entry transplant; dest-grow with mkdir-grade rollback; FAT and data clusters provably untouched). It unblocks drag-move, drag-to-Trash, and future Put Away. Folder rename needs a follow-up `'..'`-fixup bead; both are filed as dependencies of tdnl.11 (F5-2).
- **F3-3:** Display convention is raw formatted 8.3 (`fat12_format_83`), inheriting the `stdfile.h` 13-byte convention; no synthetic long names.
- **F3-4:** Spatial singleton per folder; per-folder view state in DB `kind=4` records; Clean Up = deterministic row-major snap via the shared command. Duplicate naming is a **defined, locked convention** (`trunc5(base)||"DUP"||"."||ext`, numeric-suffix fallback), explicitly not a period claim.
- **F3-5:** Get Info displays decoded on-disk truth including deterministic-zero dates; no synthesized timestamps. Locked checkbox (READONLY attr via CHMOD pair) is a noted V1.5 candidate.

---

# PART F4 — The Finder menu bar + command dispatch as DATA

## F4.1 Where the Finder bar renders

Band 2 (rows [20,40)), via the existing tenant-menubar swap: the Finder's `MenuBar` is `app->menubar`; foreground status installs it through the same `flair_live_finish_tenant_switch` path every tenant uses (D1 §D1.5). Band 1 is never touched by Finder code (the initech-4w15 static-bar rule, §0.8). Menu ID range: **512–517**, disjoint from the shell bar's 128–131, Photoshop's 256–263, and NOTES' 384–386 (`spec/flair_tenants_demo.h` documents the occupied ranges).

## F4.2 The menu resource layout (expressed in the existing `menu.h` structures)

Storage is caller-supplied static `MenuInfo`/`MenuItem` arrays (no malloc; `menu.h` §3). `has_apple = 1`. Item fields per `MenuItem`: `text`, `cmdChar`, `enabled`, `is_divider`. Dynamic enablement is recomputed into `enabled` before each draw/track (predicates in F4.4). "Grayed in V1" = `enabled = 0` at rest; the Platinum disabled-ink rendering is D3 row 45's defect, owned by `sjvq` — the *data* is correct regardless of when the ink lands.

**Apple (has_apple slot):**
| Item | Cmd | V1 |
|---|---|---|
| About This Computer | — | grayed until R4.1 (serial marker on attempt) |
| — divider — | | |
| Calculator | — | launchable (target on volume) |
| Note Pad | — | launchable |
| Scrapbook / Key Caps / Chooser / Control Panels / Find File / Jigsaw Puzzle / Stickies / SimpleSound | — | **omitted, not grayed** — the Apple menu reflects what is installed (F4-3 ruling) |

**File (id 512):** New Folder `N` · Open `O` · Print (grayed) · Close Window `W` · div · Get Info `I` · Duplicate `D` · Make Alias (grayed) · Put Away `Y` (grayed; DB origin records already stored, F1-5) · div · Find `F` (grayed) · Page Setup (grayed) · Sharing (grayed) · Label (omitted — no submenu support, D3 row 46) · Eject (grayed V1).

**Edit (id 513):** Undo `Z` (grayed) · Cut `X` / Copy `C` / Paste `V` (grayed until the R4.2 scrap wiring, `ww9c`) · Clear (grayed) · Select All `A` (**functional**) · div · Show Clipboard (grayed until R4) · div · Preferences (grayed, bottom per the Mac OS 8 HIG standardization, `period-gui-suite.md` §2a).

**View (id 514):** by Icons ✓(mark char) (**functional**) · by Small Icon (grayed) · by Name/Size/Kind/Label/Date (grayed — list view deferred per plan R3.3) · as Buttons (grayed) · as Pop-up Window (grayed) · div · Clean Up (**functional**) · Arrange (by Name) (**functional** — sorts icons by 8.3 name then snaps).

**Special (id 515):** Clean Up (**functional**, same cmd id as View's) · Empty Trash (**functional**, enabled iff Trash non-empty) · div · Erase Disk (grayed) · div · Restart (**functional** — existing reboot path) · Shut Down (**functional** — halt) · Sleep (grayed).

**Help (id 516, rightmost per `period-gui-suite.md` §2a):** About Help (grayed V1) · Show Balloons (grayed) — `Cmd-?` routed to Help.

## F4.3 The Apple menu ↔ D1 launch path

Each launchable Apple-menu entry carries a target 8.3 filename; its handler invokes **the same launch helper the desktop double-click uses** (D1 §D1.3 step 2: locate via `fat12_find_slot_in`, `loader_load_tenant`, register, foreground). Slot-busy surfaces the `TENANT-SLOT-BUSY` condition as a Finder alert (mirroring D1-3's enforcement) rather than a silent no-op. Serial: `FINDER-LAUNCH src=apple|desktop name=%s`.

**Omit-not-gray ruling for DA entries:** a period Apple menu lists installed items; shipping nine grayed rows for unbuilt DAs reads as broken, while omitting them reads as a smaller honest machine. Context-dependent *commands* (Get Info with no selection) are the opposite case — they stay visible-and-grayed because their absence would look like a bug. One rule, two applications, stated once (F4-3).

## F4.4 Command table — single, data, serial-traceable

```c
typedef enum {
    FCMD_NONE = 0,
    FCMD_ABOUT_THIS Computer /* ... */ ,
    /* lifecycle */  FCMD_NEW_FOLDER, FCMD_OPEN, FCMD_CLOSE_WINDOW,
    /* item ops */   FCMD_GET_INFO, FCMD_DUPLICATE, FCMD_PUT_AWAY,
                     FCMD_SELECT_ALL, FCMD_CLEANUP, FCMD_ARRANGE_BY_NAME,
    /* trash */      FCMD_EMPTY_TRASH,
    /* system */     FCMD_RESTART, FCMD_SHUTDOWN,
    /* view */       FCMD_VIEW_ICONS,
    /* apple */      FCMD_LAUNCH_APPLE,   /* + per-entry target index   */
    FCMD_COUNT
} finder_cmd_id;

typedef struct finder_cmd {
    finder_cmd_id id;
    int16_t  menu_id;         /* 512..516; 0 = Apple-slot entry            */
    uint16_t item_1based;     /* IM 1-based item number (menu.h §4)        */
    char     cmd_char;        /* 0 == none                                 */
    uint8_t  (*enabled)(const FinderCtx *fx);   /* predicate, may be NULL=always */
    void     (*handler)(FinderCtx *fx, const finder_cmd *c);
    const char *serial_name;
} finder_cmd_t;

/* THE table. Mouse and keyboard both land here. */
extern const finder_cmd_t FINDER_COMMANDS[];
```

Dispatch is one function:

```c
void finder_dispatch(uint32_t menu_result, const char *src);
/* menu_result = MenuSelect/MenuKey packing (menuID<<16|item, menu.h §4);
   src = "mouse" | "key". Emits:
   FINDER-CMD id=<decimal> name=<serial_name> src=<src> sel=<n>
   then calls handler. Unknown (menu,item) => serial FINDER-CMD-UNKNOWN
   + no-op (fail loud on serial, never silent). */
```

- **Mouse path:** `flair_menu_track`/`MenuSelect` result → `finder_dispatch(result, "mouse")`.
- **Keyboard path (R1.6 rides this):** `MenuKey(ch)` over the Finder bar (`menu.h` :: `MenuKey` scans for the first enabled non-divider item with the matching `cmdChar`) → `finder_dispatch(result, "key")`. PC Ctrl = Cmd per the remediation plan R1.6. Menu-flash-on-select is the `HiliteMenu` geometry helper (`menu.h` §9) plus the panel-draw hilite — noting D3 rows 34/44: the hilited-title accent fill and cmd-glyph column are `sjvq` deliverables; the dispatch logic is independent of their landing.
- **Predicates** (`enabled`): `HAS_SELECTION` (Get Info/Duplicate/Open), `TRASH_NONEMPTY` (Empty Trash), `WINDOW_FRONT_IS_DISKWIN` (Close Window), `ALWAYS`. Recomputed into the static `MenuItem.enabled` bytes before every `DrawMenuBar`/track — the menu *resource* stays static data; only the enable bits mutate.
- **Traceability:** every dispatch emits exactly one `FINDER-CMD` line; a scripted session's trace is a locked golden. Mutants: `CMD_TABLE_BYPASS` (keyboard path calls handlers directly, skipping the table → trace loses `src=key` lines → red), `CMD_SILENT_UNKNOWN` (unknown result no-ops without the serial line → red), `PRED_STUCK_ENABLED` (Empty Trash enabled on empty Trash → handler must no-op-with-alert; assertion red if it purges nothing-but-succeeds silently).

## F4.5 The menu-bar clock (R3.6)

- **Where:** right end of **band 2**, appended after the active app's menus by the *shell-side* band-2 composer — not by the Finder, and never in band 1 (static-bar rule, §0.8). Rationale: the clock is system furniture that must tick regardless of which tenant is foreground; band 2 is the only legally-repaintable bar. The Finder's tdnl.13 delivers the renderer + tick source; the integration point is the band-2 draw path that already exists for tenant swaps.
- **Format:** `HH:MM` (Chicago, right-aligned with the bar's right pad). Seconds omitted (period SuperClock default posture; *golden-resolves*).
- **Redraw cadence:** the Finder's idle/event slice compares `floor(flair_tick_count() / TICKS_PER_SEC)` (18.2 Hz PIT base; `event.h` :: `flair_tick_count`) against the last-drawn second; on minute-rollover it triggers a **full band-2 bar redraw** through the existing `DrawMenuBar` path (the bar is 20 px tall; partial-clock invalidation machinery is unjustified complexity against a full-bar repaint that costs microseconds). No new partial-redraw seam; the overlay/damage contracts untouched.
- **Time source:** the INT-21 clock seam (`int21.h` :: `int21_set_clock`, the AH=2Ch GET path backed by `rtc_now` on metal) — the R3.6 "RTC via the INT-21 clock seam" requirement verbatim.
- **Injected-RTC oracle:** the host/emulator harness binds a mock clock advancing deterministically (e.g., +1 simulated minute per N injected ticks); the `clock_tick` clip shows the minute rollover; serial `CLOCK-MIN m=<n>` on each render gives a lockable golden. Mutant: `CLOCK_STATIC` (rollover detection compiled out → no second `CLOCK-MIN` line → red).

### DECISIONS — Part F4

- **F4-1:** The Finder menu bar renders in band 2 via the existing tenant-swap path; menu IDs 512–516; band 1 untouched.
- **F4-2:** The full Apple/File/Edit/View/Special/Help layout above is expressed as static `MenuBar`/`MenuInfo`/`MenuItem` data with predicate-recomputed enable bits; Label submenu omitted (no submenu support, D3-46); dynamic graying is data maintenance, not new rendering.
- **F4-3:** Apple-menu launchables are **omit-if-not-installed**; context-dependent commands are **visible-and-grayed**. Apple entries launch through the identical D1 helper as desktop double-click; slot-busy alerts.
- **F4-4:** One command table (`finder_cmd_t[]`), one dispatch function, both input paths converge, every dispatch emits `FINDER-CMD`; mutants `CMD_TABLE_BYPASS`, `CMD_SILENT_UNKNOWN`, `PRED_STUCK_ENABLED`.
- **F4-5:** The clock is shell-side band-2 furniture (ticks regardless of foreground tenant), minute-rollover detected off `flair_tick_count`, full band-2 redraw on change, time from the INT-21 clock seam, mock-clock oracle with `CLOCK-MIN` goldens.

---

# PART F5 — Slicing + oracle map

## F5.1 Reslice rulings (the dependency order demands two changes — saying so)

1. **Insert `tdnl.11a` (NEW child bead): MILTON `fat12_move_dirent` + Trash helpers.** tdnl.11 (file ops) is *impossible* as specified without a cross-directory move (`fat12_rename` is same-dir; §0.4). tdnl.11a lands the transplant primitive + the `\TRASH` collision/suffix helper, with its own mtools differential, BEFORE any tdnl.11 UI. Estimated M.
2. **Pull the command table forward as `tdnl.9a` (small child of tdnl.9).** The table + `finder_dispatch` spine is pure data/logic, host-testable, and R1.6 rides it; landing it with tdnl.9 makes tdnl.12 pure menu-resource wiring + graying predicates. Estimated S.
3. **Execution order diverges from numeric order:** recommended close order is **8 → 9 (+9a) → 10 → 12 → 11a → 11 → 13 → 14**. Rationale: menu commands (tdnl.12) should exist before file ops (tdnl.11) so Duplicate/Get Info/New Folder land as *commands*, not ad-hoc call sites; tdnl.13 (clock) is independent and slots anywhere after 9a; tdnl.14 depends on the D1 machinery build-out and is last. Beads may close out of numeric order; the epic records the order rationale.

## F5.2 The slices

| Slice | Scope | Files | Oracle (incl. mutants) | Size |
|---|---|---|---|---|
| **tdnl.8** — image + mount + DB/TRASH skeleton | Flagship image gains the FAT12 data volume; mount OK at boot; `\DESKTOP.DB` + `\TRASH` created at init; DB read/write/regen primitives; **fix the `FAT12_ERR_NOT_EMPTY`/`_ACCESS` -15 collision (§0.3)** | image build, `os/milton/fileio_fat.c` bindings, new `os/flair/finder_db.c` | Boot serial `MOUNT-OK` + `DESKTOP-DB-CREATE`; mtools diff of shipped image; DB round-trip vs harness-authored records; mutants `DB_WRITE_SKIP`, `DB_MAGIC_FLIP`, corrupt-BPB fail-loud (existing mount-guard class) | M |
| **tdnl.9** — DesktopMgr core | Icon records, grid + hit-test, selection (click/shift/marquee), icon drag w/ save-under outline, double-click synthesis, desktop pattern underlay hook, volume + Trash icons, DB load/save | new `os/flair/finder_desktop.c`, `desktop.c` underlay hook, `window.h` no change | Host unit suite (grid/hit/track pure functions, the `flair_menu_track` pattern); clips `icon_select`, `rubber_band`, `icon_dragdrop`; mutants `NO_UNDERLAY` (icons vanish on damage repaint → red), `DBLTICK_OFF` (double-click never fires → red) | L |
| **tdnl.9a** — command table spine | `finder_cmd_t[]`, `finder_dispatch`, predicates, serial trace | new `os/flair/finder_cmd.c` | Host trace golden; mutants `CMD_TABLE_BYPASS`, `CMD_SILENT_UNKNOWN` | S |
| **tdnl.10** — disk windows | Enumeration→icons, folder navigation (spatial singleton), New Folder, Clean Up, per-folder view persistence, window titles | `finder_windows.c`, reuses List-free icon grid | mtools diff after New Folder; clip `folder_nav` + `cleanup_snap`; mutant `SPATIAL_DUP` (second open creates a second window → red) | L |
| **tdnl.12** — Finder menu bar | Full F4.2 resource, graying predicates, Apple-menu launch entries, band-2 integration | `finder_menu.c`, static resource arrays | `FINDER-CMD` trace golden over a scripted session; clip `menu_dispatch` + `cmdkey_dispatch`; mutant `PRED_STUCK_ENABLED` | M |
| **tdnl.11a** — MILTON move primitive (NEW) | `fat12_move_dirent` (transplant, dest-grow rollback, FAT untouched), Trash suffix helper | `os/milton/fat12.c/.h` | mtools differential: entry moved, `start_cluster` equal, FAT bytes equal; mutants `MOVE_RELINKS_CHAIN`, `MOVE_LOSES_ENTRY` | M |
| **tdnl.11** — file ops UI | Drag-move, drag-to-Trash, Empty Trash confirm + recursive purge, Duplicate, Get Info window, rename-in-place | `finder_ops.c`, `finder_getinfo.c` | mtools diffs of moves/deletes/duplicates; clips `trash_drag`, `trash_empty`, `duplicate_cmd_d`, `get_info_window`, `rename_inline`; mutants `TRASH_NO_STAGE`, `EMPTY_NO_CONFIRM`, `PURGE_KILLS_TRASH_DIR` | L |
| **tdnl.13** — menu-bar clock | Shell-side band-2 clock renderer + rollover detector + INT-21 clock binding | band-2 composer, `finder_clock.c` | Mock-clock golden `CLOCK-MIN` trace; clip `clock_tick`; mutant `CLOCK_STATIC` | S |
| **tdnl.14** — app launch (D1 build-out) | Implement D1: INT 0x81 gate, `loader_load_tenant`, park/resume WNE, register/exit/kill, slot enforcement; desktop double-click `.EXE` → tenant | `os/milton/loader.c`, gate dispatcher, Finder launch helper | The full D1.7 oracle set verbatim: launch-trace golden, `TENANTFIX.EXE`, mutants `NO_REGISTER`, `DOUBLE_LAUNCH`, `EXIT_LEAK`, `CLOSE_STALE_FOCUS`; clip `app_launch` | L |

Tri-emulator discipline at phase close per Rule 5 (QEMU dev loop; Bochs legs in `make test`; 86Box pending `x0i`).

## F5.3 Record-flair scripts R3 adds

| Script | Clip must show |
|---|---|
| `icon_select` | Click selects (inverted label); shift-click extends; marquee drag selects the touched set; click-away clears |
| `rubber_band` | The 1-px band tracks the pointer with no trail (save-under correct) and no over-repaint of windows it crosses |
| `icon_dragdrop` | Outline follows pointer; drop on desktop repositions + persists across a reboot leg; illegal drop reverts |
| `folder_nav` | Double-click volume opens root window; double-click folder opens spatial window (singleton); close saves view state |
| `new_folder` | Cmd-N creates "untitled"-analog 8.3 dir with name field ready for inline rename |
| `rename_inline` | Inline edit commits via rename; colliding name rejected with alert |
| `duplicate_cmd_d` | Cmd-D produces the locked `DUP`-convention copy; mtools confirms two entries, distinct chains |
| `get_info_window` | Live size/kind/where/date fields for a selected file; dates show decoded on-disk truth |
| `trash_drag` | Drag onto Trash stages the item (entry appears in `\TRASH`, source emptied, Trash icon fullness variant flips) |
| `trash_empty` | Confirm alert (Cancel escapes, OK purges); purge frees chains; protected in-use item refuses with report |
| `cleanup_snap` | Scattered icons snap to grid deterministically; persists |
| `menu_dispatch` | Mouse pull-down: hilite, blink-on-select, item executes, `FINDER-CMD src=mouse` in trace |
| `cmdkey_dispatch` | Ctrl-equivalent triggers the same command; trace shows `src=key` converging on the same id |
| `clock_tick` | Minute rollover repaints the band-2 clock; mock RTC advances |
| `app_launch` | Double-click `TENANTFIX.EXE`/Calculator → D1 launch trace → tenant window → clean exit → Finder refocused |
| `db_regen` | Corrupted `DESKTOP.DB` at boot → `DESKTOP-DB-REGEN` serial → defaults rendered, session proceeds |

### DECISIONS — Part F5

- **F5-1:** Reslice: add `tdnl.11a` (MILTON move primitive — hard dependency, impossible-to-avoid) and `tdnl.9a` (command-table spine pulled forward so tdnl.12 is wiring and R1.6 rides the table). Execution order 8 → 9(+9a) → 10 → 12 → 11a → 11 → 13 → 14, recorded in the epic.
- **F5-2:** Every slice ships its oracle set with named mutants (Rule 6 mutation-proven goldens); motion slices ship their `record-flair` script with byte-identical repro gates (mandate consequence 3); tri-emulator legs at phase close.
- **F5-3:** Sixteen named clips above constitute the R3 motion-evidence set for operator Law-4 eyeball, extending the R6 script list.

---

*End of design. All rulings are drafted for lift into the R3 epic notes / an ADR amendment subject to the §0 corrections — especially the absent `text.h`/`kmain.c` corpora, the `-15` error-code collision fix in tdnl.8, and the F1-4 font policy ratification.*
