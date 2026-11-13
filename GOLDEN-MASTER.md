# The golden master

This is the closed list of what ships in the first master. If it is not
here it is not being built. Only the operator moves an item between this
list and the Release 2 list below. Ratified by the operator 2026-11-13.

The master is one reproducible disk image set plus its manuals. Each
item below is done when its acceptance check is green on the release
candidate. Item 3 also needs the operator's eye, and says so.

Where an item says "that exist today" or "present depth", it means the
feature inventory frozen in that component's `README.md` when it was
imported from `legacy-final`. The inventory is the scope; reading the
legacy tree to find more is not.

## 1. It boots

- Boots from the image to the InitechDOS prompt, then to the desktop, on
  QEMU and on Bochs. A 486 with 8 MiB.
- The same source builds the same image bytes twice in a row.
- *Accept:* `make cert` boot legs green on both emulators; image hash
  equal across two clean builds.

## 2. InitechDOS

- FAT12/16, the INT 21h services the bundled software uses, the program
  loader, `COMMAND.COM` with batch files, `CONFIG.SYS` and
  `AUTOEXEC.BAT`, and the external utilities already written (CHKDSK,
  FORMAT, SYS, FDISK, EDLIN, XCOPY, TREE, ATTRIB, LABEL, FIND, SORT,
  MORE, DISKCOPY, DISKCOMP).
- Nothing accepted silently: a directive or switch either works or is
  refused with a period error message.
- *Accept:* the FAT differential against mtools; the kernel host suite;
  a scripted DOS session on the booted image.

## 3. The frame, alive

- The "Saving tables to disk..." frame as a live arrangement: the
  InitechCalc ledger and pie chart, the Finder's Drive A window, the
  FILE COPY dialog with the hourglass. One menu bar, not the frame's
  two (operator, 2026-11-13); InitechPaint's menus are the Photoshop
  ones and show when it is in front.
- The ledger shows real recovered or agreed content. No `TODO_FRAME`.
- Windows drag, menus pull down, the copy really copies, and a failed
  or cancelled copy never destroys the destination.
- The canon bugs are present: 116%, the hourglass, `570-`.
- *Accept:* the frame scene smoke; the operator looks at the live scene
  and the clip and says yes.

## 4. The desktop and the small apps

- Finder: volumes, folders, icons, launching, the file operations that
  exist today, Trash, the menu bar.
- Calculator, Note Pad, Scrapbook, Control Panels, InitechText,
  InitechPaint (the menu-bar carrier the frame needs; a blank canvas).
- Every visible command either works or is absent. No dead menu items,
  no path that loses a user's data.
- *Accept:* each app's host suite and smoke; a dead-command sweep.

## 5. InitechBase

- `.dbf`, `.dbt` and `.ndx` read and written so that real dBASE III+
  opens them; the dot prompt; programs (`DO`), with the commands,
  functions and `SET` options that exist today plus `@...SAY/GET` and
  `READ`.
- A frozen omission list: what real dBASE III+ has that InitechBase
  refuses, each refused with the period error message.
- *Accept:* the differential corpus against real dBASE III+; round-trip
  in both directions; the Y2K accounting program runs with its canon bug.

## 6. Initech 123 and InitechWord

- At their present depth: the worksheet, formulas, recalculation and
  `.WK1` files; the document, editing, Reveal Codes and `.WP5` files.
- Every menu leaf either works or is removed. Saves are safe.
- *Accept:* the `.WK1` and `.WP5` differentials against real files;
  each app's host suite and smoke; a dead-command sweep.

## 7. Turbo Initech (the finale)

- The Pascal compiler runs inside InitechOS and compiles real programs,
  with no host assembler or linker in the loop.
- A minimal blue IDE: edit, save, compile, see the error, run.
- It compiles itself inside the OS and reproduces itself bit for bit:
  `K2 == K3`. A second, independent bootstrap path agrees (DDC): the
  same compiler source built by Free Pascal instead of the seed.
- *Accept:* the corpus differential against Free Pascal; `make selfhost`
  and `make ddc`; the finale run from boot to a self-reproduced compiler.

## 8. Delivery

- A working way to install or start the system that matches what the
  README tells the user to do.
- The manuals for what ships, regenerated once from the final image so
  that every screen and message in them is real.
- *Accept:* a clean install-and-boot run; the manuals' screen and
  transcript check against the release candidate.

---

## Release 2 (not being built now)

- InitechBase full-screen suite: BROWSE, EDIT, the designers, ASSIST,
  and the rest of dBASE III+ parity.
- Full Lotus 1-2-3 R2.2 and WordPerfect 5.1 parity; the second "face" of
  each.
- A real paint program.
- SideKick.
- 86Box and real hardware.
- The physical edition: diskettes, box, printed manuals.
- DOS 5-era utilities and memory managers.
- The wider app-platform bar: Print Manager, CopyBits modes, GWorlds,
  several disk apps resident at once.
- Games and further easter eggs.
