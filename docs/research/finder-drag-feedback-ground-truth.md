<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
<!-- Minted 2026-10-03 for bead initech-34dh (R3.4a drag-move + drag-to-Trash).
     Law 1: the local copy of the passages the Finder drag feedback cites. -->

# Finder drag feedback -- ground truth (what a period user SEES while dragging)

The question bead initech-34dh had to answer before code: when a System 7 /
Mac OS 8 user drags an icon, what follows the pointer, what tells them a drop
target is valid, and what happens on a drop that cannot happen? Three local or
acquired sources, quoted verbatim, and the ruling each one backs.

## 1. A gray OUTLINE follows the pointer (not the icon itself)

**Inside Macintosh: Macintosh Toolbox Essentials (1992), p. 4-96..4-97,
DragGrayRgn** (local: `../system7-decomp/refs/MacintoshToolboxEssentials.pdf`):

> "The DragGrayRgn function moves a gray outline of a region on the screen,
> following the movements of the cursor, until the mouse button is released."
>
> "actionProc  A pointer to a procedure that defines an action to be performed
> repeatedly as long as the user holds down the mouse button."

The Finder's icon drag is this primitive over the icon's region; the actionProc
is where destination feedback runs.

**Apple patent US5754178A** (assignee Apple Computer Inc, priority 1993-03-03;
describes the then-shipping System 7 Finder as prior art; fetched from
patents.google.com/patent/US5754178A on 2026-10-03):

> "the user may start to move cursor 110 while the icon is selected causing an
> outline image representation of the icon and its file name, which is
> illustrated as 130, to be moved on the computer system display"

**Ruling:** the dragged icon stays where it is; a gray outline of its cell
(sprite + name) follows the pointer. InitechOS draws it with the existing
R0.1/R1.2 save-under outline (FLAIR has no XOR blit; design F2-5).

## 2. The drop target is drawn in its HIGHLIGHTED state

**US5754178A** (as above):

> "When the 'folder' icon 140 is pointed to by pointer 110, it becomes shown in
> its highlighted state, as is illustrated in FIG. 1c"

**Inside Macintosh Volume VI (1991), p. 2-19..2-20** (local:
`../system7-decomp/refs/InsideMacintosh_Vol6_1991.pdf`):

> "The selection mechanism for color icons lowers the brightness of colors to
> indicate selection. This means that the colors appear darker when selected.
> On a color monitor, a black-and-white icon turns gray when selected. On a
> monochrome monitor, a black-and-white icon uses reverse video to show
> selection."

**OpenDoc Programmer's Guide, "Using Drag and Drop"** (Inside Macintosh-series,
dev.os9.ca/techpubs/mac/ODProgGuide/ODProgGuide-95.html, fetched 2026-10-03):

> "If your part is an eligible drop target, it should display destination
> feedback to that effect whenever the user drags content into its frame."
> "When the pointer moves outside of your eligible target, remove the
> destination feedback."

**Ruling:** a folder icon, the volume icon or the Trash under the pointer is
drawn highlighted -- body and detail darkened (white -> #777777, #C0C0C0 ->
#3F3F3F, both EXISTING canon ramp rows; black ink unchanged) with its name
inverted -- and returns to normal the moment the pointer leaves it. Window
bodies are valid targets but carry no highlight (7.x had no window-body
destination feedback).

## 3. A drop that cannot happen ZOOMS BACK

**US5754178A** (as above):

> "the abort may be indicated by series of 'zooming rectangles' 2030 generated
> by the subroutine ZoomRects() discussed below"
>
> "zooming rectangles head back towards the original folder 2010 until the
> zooming rectangles completely disappear from the screen"

**Ruling:** a refused drop (samedir / cycle / exists / err) or a drop on no
target animates a short series of gray outlines from the drop position back to
the icon's own cell (8 steps, one PIT tick each), and nothing on the volume
changes. The animation lives between injected events, so a record-flair clip
(one frame per event) shows the before and after, not the in-between frames.
