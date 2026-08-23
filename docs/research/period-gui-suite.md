<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
<!-- Minted 2026-08-23 (web research, sonnet research agent, orchestrator-reviewed). -->
<!-- Ground-truth reference for docs/plans/GUI-remediation-plan.md (Law 1). -->
<!-- The BINDING fidelity source for pixels remains ../system7-decomp/specs/sys8/; -->
<!-- this document is the FUNCTIONALITY bar: what a sold-in-stores 1995-97 GUI OS shipped. -->

# Mac OS 8 / System 7.5.x Out-of-the-Box GUI Ground Truth
### (secondary comparison: Windows 3.1 / 95) — compiled for InitechOS remediation

---

## 1. The Desktop Shell (Finder)

- [ ] **Desktop icons for mounted volumes** — every mounted disk (startup HD, floppy, CD, server) gets an icon in the top-right of the desktop, stacked downward as more mount; double-click opens its window. (Wikipedia, *Finder (software)*: https://en.wikipedia.org/wiki/Finder_(software))
- [ ] **Trash icon**, fixed bottom-right corner of the desktop. Drag-to-trash deletes; **Special > Empty Trash** (with an "are you sure" alert) purges. Locked/in-use items refuse to empty; holding **Option** while choosing Empty Trash forces removal of locked items. (whitefiles.org, "Desktop Items": https://whitefiles.org/mac/pgs/a04.htm)
- [ ] **Rubber-band (marquee) selection** — click-drag on empty desktop/window space draws a selection rectangle; any icon it touches is selected/highlighted. Standard since System 1 onward. (whitefiles.org "Working on the Desktop": https://whitefiles.org/mac/pgs/a01.htm)
- [ ] **Click-select / Shift-click extend / drag to move** icons; **double-click to open** (launches app or opens folder window).
- [ ] **Disk/folder windows** open one per folder (spatial Finder — "opening a new folder opens it in a new window"; windows are "locked" to one folder's contents). (Wikipedia, Finder)
- [ ] **Icon View** and **List View** (columns: Name, Size, Kind, Label, Date Modified — sortable by clicking column head) as the two core window views, selectable from the View menu (`View > as Icons`, `View > as List`). Mac OS 8 adds **View > as Buttons** (single-click-to-open button icons, Launcher-like). (eshop.macsales.com Finder views; whitefiles.org)
- [ ] **New Folder** (`Cmd-N` in Finder context — creates "untitled folder" with name field ready for immediate rename).
- [ ] **Get Info** (`Cmd-I`) — per-item window showing Kind, Size, Where (path), Created/Modified dates, a user Comments field, a Locked checkbox, and (for apps) Memory Requirements; supports pasting a **custom icon** onto the icon well. (kb.iu.edu: https://kb.iu.edu/d/ahzi; whitefiles.org)
- [ ] **Find File** (Mac OS 7.5-8.1) — `Cmd-F`, searches by name/criteria across mounted volumes, results list double-clickable to reveal in Finder. (whitefiles.org: https://whitefiles.org/mac/pgs/s03.htm)
- [ ] **Drag to move** (same volume) / **Option-drag = force-copy** (same volume) / **plain drag across volumes = copy** (standard Mac drag semantics).
- [ ] **Clean Up** (View menu) — snaps icons to the invisible grid. (whitefiles.org)
- [ ] **Desktop pattern / picture** — a **Desktop Patterns** control panel (pre-8.5): choose a tiled pattern. (Apple Wiki, Desktop Patterns control panel: https://apple.fandom.com/wiki/Desktop_Patterns_control_panel)
- [ ] **Spring-loaded folders** (new in Mac OS 8 Finder) — drag onto a closed folder and hold; it pops open, drill without releasing; releasing closes intermediates. (Wikipedia, Finder; O'Reilly Mac OS 9 QuickStart)
- [ ] **Rebuild Desktop** — `Cmd-Option` at startup forces icon-database rebuild. (whitefiles.org)

---

## 2. Menus

### 2a. The canonical Mac OS 8 Finder menu bar

Order (left to right, after the Apple menu): **File, Edit, View, Special, Help**. Under Platinum, the menu bar gained "a three-dimensional Apple logo, beveled edges, and anti-aliased corners"; menu dividers got an "etched" look. (Inside Macintosh, *Menu Bar Changes*: https://dev.os9.ca/techpubs/mac/HIGOS8Guide/thig-54.html)

**Apple menu contents (as shipped, System 7.5/Mac OS 8 era)**: About This Computer; Apple System Profiler; AppleCD Audio Player; Automated Tasks; Calculator; Chooser; Control Panels (alias); Find File; Jigsaw Puzzle; Key Caps; Note Pad; Recent Applications/Documents/Servers (auto submenus, last 10); Scrapbook; SimpleSound; Stickies. (whitefiles.org, "Apple Menu Items": https://whitefiles.org/mac/pgs/s03.htm)

**File menu (Finder)**:
- New Folder — `Cmd-N` · Open — `Cmd-O` · Print · Close Window — `Cmd-W`
- Get Info — `Cmd-I` · Duplicate — `Cmd-D` · Make Alias — `Cmd-M` · Put Away — `Cmd-Y`
- Find — `Cmd-F` · Page Setup · Sharing · Label (submenu)
- Eject — moved into **File** as of Mac OS 8 (was in Special). (Apple Wiki, Eject Disk: https://apple.fandom.com/wiki/Eject_Disk)

**Edit menu (Finder and standard app convention)**:
- Undo — `Cmd-Z` · Cut — `Cmd-X` · Copy — `Cmd-C` · Paste — `Cmd-V` · Clear · Select All — `Cmd-A`
- Show Clipboard (Finder-only) · Preferences — Mac OS 8 HIG standardized to the **bottom** of Edit below a divider. (Inside Macintosh, Menu Bar Changes)

**View menu (Finder)**: by Icon / by Small Icon; by Name/Size/Kind/Label/Date; as Buttons; as Pop-up Window; Clean Up; Arrange.

**Special menu (Finder)**: Clean Up, Empty Trash, Eject Disk (pre-8), Erase Disk, Restart, Shut Down, Sleep. (Apple Wiki, Special menu: https://apple.fandom.com/wiki/Special_menu)

**Help menu** — as of Mac OS 8 the Guide menu became the plain word **Help**, rightmost. Contents: About Help, Show Balloons, app-specific entries. `Cmd-?` invokes Help. (Inside Macintosh, Menu Bar Changes; Apple Wiki, Guide menu)

### 2b. Typical application menu bar

Convention: **File, Edit**, then 0-N app menus, ending in **Help**. App File menu: New (`Cmd-N`), Open (`Cmd-O`), Close (`Cmd-W`), Save (`Cmd-S`), Save As (`Cmd-Shift-S`), Page Setup, Print (`Cmd-P`), Quit (`Cmd-Q`). Edit matches the standard Edit menu (HIG uniformity). Mac OS 8 HIG adds extended modifiers (Ctrl/Shift/Option combine with Cmd). (Inside Macintosh, Menu Bar Changes)

---

## 3. Window Behaviors (Mac OS 8 Platinum)

- [ ] **Close box** — top-left; click closes (`Cmd-W` equivalent).
- [ ] **Zoom box** — toggles between "user size" and best-fit size.
- [ ] **Collapse box (WindowShade)** — dedicated Platinum title-bar widget: collapses to title bar; click restores. `Option`-click collapses/expands ALL windows. Sound effect, disableable. (Wikipedia, *WindowShade*: https://en.wikipedia.org/wiki/WindowShade; Inside Macintosh, "Collapsing a Window": https://dev.os9.ca/techpubs/mac/HIGOS8Guide/thig-61.html)
- [ ] **Grow box** — bottom-right resize handle, drag to resize.
- [ ] **Scroll bars** — end arrows (line step), track-click paging, draggable thumb; re-skinned to Platinum bevels in Mac OS 8.
- [ ] **Window drag = outline only** (not live-content redraw); content redraws once at drop. (classic-Mac convention)
- [ ] **Active vs. inactive** — active title bar shows full Platinum treatment, solid-black title text, widgets enabled; inactive shows dimmed bar, widgets hidden/grayed, dimmed scrollbars.
- [ ] **Pop-up (tabbed) windows** — Mac OS 8 Finder: drag a window's title to screen bottom to dock as a tab. (Wikipedia, Finder; O'Reilly)
- [ ] **Spring-loaded folders** — see §1.

---

## 4. Bundled Small Apps & Desk Accessories (as shipped)

| App | Core function | UI complexity |
|---|---|---|
| **SimpleText** | Default text editor (replaced TeachText in 7.5); styled text (bold/italic/underline, fonts/sizes). (Wikipedia: https://en.wikipedia.org/wiki/SimpleText) | 1 document window, scroll bar, Font/Size/Style menus |
| **Calculator** | Four-function calculator DA, present since System 1. (https://en.wikipedia.org/wiki/Calculator_(Apple)) | 1 small fixed window, keypad buttons, readout |
| **Note Pad** | Tiny multi-page scratchpad DA (8 pages, page-flip dog-ear animation). (https://classic-notepad.macupdate.com/) | 1 small window, page-turn corner, Edit menu only |
| **Scrapbook** | Persistent clipping store — pasted text/graphics/sounds; paged browser. (https://en.wikipedia.org/wiki/Scrapbook_(Mac_OS)) | 1 window, page controls, Edit menu |
| **Key Caps** | Shows any font's character set on an on-screen keyboard. (whitefiles.org s03) | 1 window, keyboard grid, font menu |
| **Chooser** | Printer/network selection; icon list + instance list + AppleTalk radios. (https://en.wikipedia.org/wiki/Chooser_(Mac_OS)) | 1 window, split lists, 2 radios |
| **Jigsaw Puzzle** | Puzzle game (replaced Puzzle DA in 7.5). (https://apple.fandom.com/wiki/Jigsaw_Puzzle_(classic_Mac_OS)) | 1 board window + options dialog |
| **Stickies** | Post-it notes on the desktop; color menu, `Cmd-N` new note. (https://en.wikipedia.org/wiki/Stickies_(Apple)) | N floating chrome-less note windows |
| **AppleCD Audio Player** | Audio CD transport + playlists. (https://apple.fandom.com/wiki/AppleCD_Audio_Player) | 1 compact transport window |
| **Find File** | Search utility; criteria fields + results list. (whitefiles.org s03) | 1 window |
| **SimpleSound** | Record/play system sounds. | 1 small transport window |

---

## 5. Control Panels (Mac OS 8)

Full shipped roster includes: Appearance, Apple Menu Options, AppleTalk, ColorSync, Control Strip, Date & Time, Desktop Patterns/Pictures, Easy Access, Energy Saver, Extensions Manager, File Exchange, File Sharing, General Controls, Internet, Keyboard, Launcher, Location Manager, Map, Memory, Modem, Monitors, Mouse, Numbers, QuickTime Settings, Sound, Speech, Startup Disk, TCP/IP, Text, Users & Groups. (whitefiles.org, "Control Panels": https://whitefiles.org/mac/pgs/s04.htm)

**Minimum credible set (5+1) for a first-boot demo:**
1. **Date & Time** — date/time/zone + the Menu Bar Clock tab.
2. **Appearance / Desktop Patterns** — pattern choice (the customizability demo reflex).
3. **Mouse** — tracking speed + double-click speed slider.
4. **Sound** — alert sound selection.
5. **Monitors** — resolution/depth.
6. (**Memory** — cache/VM display — 6th if budget allows.)

---

## 6. System-Wide UX Furniture

- [ ] **About This Computer** — system version, total RAM, **per-application memory bar graph** (used-vs-allocated per app) + **Largest Unused Block**. (https://en.wikipedia.org/wiki/Classic_Mac_OS_memory_management; whitefiles.org m02)
- [ ] **Alert dialogs** — three severities w/ prescribed icons: **Note** (talking-face, informational), **Caution** (warning triangle, Cancel escape), **Stop** (stop sign). Icon + text + buttons ONLY; bold summary line + plain explanation; up to 4 buttons (OK, Cancel, Help, one extra e.g. "Don't Save"). (Inside Macintosh, "Alert Boxes": https://dev.os9.ca/techpubs/mac/HIGOS8Guide/thig-42.html)
- [ ] **Button order / default ring** — primary action furthest right; Cancel to its left; default button gets the highlighted ring, Return-triggered. (Apple HIG convention)
- [ ] **Copy-progress dialogs** — modal; animated flying-pages icon, "X of Y items"/"About N seconds remaining", determinate bar, Stop button.
- [ ] **Menu-item blink-on-selection** — chosen item visibly blinks before executing (signature feel cue).
- [ ] **Standard File dialogs (Open/Save)** — scrollable file list, folder-path pop-up at top (click-hold to ascend), Eject + Desktop buttons, New Folder (Save), affirmative default-ringed button bottom-right. (https://developer.apple.com/library/archive/documentation/mac/Files/Files-307.html)
- [ ] **Show Clipboard** — Finder Edit command; window displaying current Clipboard contents.
- [ ] **Balloon Help** — hover balloons; Help > Show Balloons toggle. (https://en.wikipedia.org/wiki/Balloon_help)
- [ ] **Menu-bar clock** — right-aligned; native via Date & Time in Mac OS 8 (SuperClock descendant).

---

## 7. Windows 3.1 Secondary Comparison (chimera-accent purposes)

- **Program Manager** — Group icons (Main, Accessories, Games, StartUp) opening child windows of app icons. Main: File Manager, Control Panel, Print Manager, Clipboard, MS-DOS Prompt, Windows Setup, PIF Editor, Read Me. Accessories: Write, Paintbrush, Terminal, Notepad, Recorder, Cardfile, Calculator, Clock, Object Packager, Character Map, Media Player, Sound Recorder. Games: Solitaire, Minesweeper, Hearts. (https://jeffpar.github.io/kbarchive/kb/084/Q84773/; https://en.wikipedia.org/wiki/Program_Manager; https://www.pcworld.com/article/469692/windows-3-1-twenty-years-later.html)
- **File Manager** — dual-pane: directory tree left, file list right; drive icon strip; View menu sort/pane control. (https://www.cs.oberlin.edu/~rms/mmcc/a/mod6/mod6410.html)
- **Control Panel** — one applet-picker window.
- *(Windows 95 context: Start menu, Taskbar, My Computer, Recycle Bin, min/max/close buttons — the richer 1996 comparison bar on the PC side.)*

---

## 8. Prioritized "Sellability Floor" — the 15 features whose absence most screams "not a real OS" in a 5-minute 1996 store demo

1. **Working icon double-click to open (apps launch, folders open into windows)** — the motor action every demo begins with; without it nothing else is reachable.
2. **Trash can with drag-to-delete and Empty Trash** — the most universally recognized Mac affordance; its absence reads instantly as "not a Mac."
3. **A real Finder menu bar (File/Edit/View/Special/Help) with working commands, not placeholders** — a menu bar that opens but does nothing is worse than none; it's the first thing a shopper clicks.
4. **New Folder + rename-in-place** — the basic act of organizing your own stuff; absence makes the desktop a fixed diorama.
5. **Rubber-band / click-drag selection of icons** — without it, multi-select and batch operations are impossible; the drag rectangle is period reflex.
6. **Working Get Info** — the demo-floor "prove it's real": live size/kind/date metadata shows an actual filesystem behind the shell.
7. **Title-bar close/zoom boxes that actually close/resize** — painted-pixel chrome is caught within seconds of a curious click.
8. **Grow box + working scroll bars** — a static non-scrolling window looks broken, not stylized.
9. **A functioning About This Computer memory-bar display** — period buyers and salespeople opened this routinely; a real bar graph signals real memory management.
10. **Standard Open/Save dialogs that browse the real filesystem** — a stub dialog collapses the whole illusion the moment any app opens or saves.
11. **At least one or two real desk accessories (Calculator, Note Pad, or SimpleText) that do their one job** — an OS with zero working apps is a screensaver, not an OS.
12. **Desktop pattern control (even a single alternate)** — "can I change how it looks" is the reflexive first question; Mac OS was sold partly on this.
13. **Alert dialogs with correct icon/button conventions (OK/Cancel, right-aligned default)** — wrong or missing alerts on destructive actions break trust immediately.
14. **Menu-bar clock** — the small ever-present "is this alive" signal; missing/static reads as a frozen mockup.
15. **WindowShade/collapse box (or at minimum a correctly zooming window)** — Mac OS 8's headline affordance vs 7.5; reviewers of the era called it out by name.
