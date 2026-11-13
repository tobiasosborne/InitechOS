# Application SDK

The SDK is the C application interface to the Toolbox gate, INT 81h.
The interface and record layouts preserve legacy-final. Applications include
only sdk/include and their own headers. No private graphics or kernel header
is required. sdk/rt contains the freestanding runtime and linker script;
sdk/host contains the factory's shared recording Toolbox library. Product
code depends only on standard integer/offset types, with no libc.

The authority for trap behavior is the claimed toolbox_gate.h contract and
legacy-final flair/tbxgate.c (formerly the same gate in the legacy layout).
Event codes/Point are from Inside Macintosh, cited in event_model.h and
geometry.h. cp_dialog.h and cpanels_record.h preserve the existing Control
Panels transport and disk record. The save helper preserves the DOS-style
create-new/write/close/delete/rename transaction; it is not crash-atomic
between deletion and rename. See NOTES-FOR-FLAIR.md for private adapter work.

## Life cycle

The loader relocates the module and zeroes e_minalloc BSS. Module byte zero
is tbx_entry, which calls tbx_app_main. The application supplies its name,
main and tbx_app_event. Main calls tbx_register (which submits the runtime's
registration record), installs a menu bar, creates its windows, then returns.
Registration makes the application resident. The shell pushes events through
the runtime's event callback and flat event buffer. Returning from main or
an event callback yields cooperatively; applications do not own an event loop.
An updateEvt requests painting. Mouse coordinates are global; origin/local
bindings translate them. Menu messages pack menu ID and 1-based item number.
Alerts, Standard File, tracking and FILE COPY complete through later events.
tbx_exit records a request; the shell tears the application down after return.
A callback that never returns hangs the desktop. The OS initializes x87 to
0x037F; SDK code does not replace the kernel's floating-point setup.

## Building an application

A ten-line Makefile (source files are relative to the application directory):

```make
TOP := $(abspath ../..)
COMPONENT := apps/example
include $(TOP)/mk/config.mk
TENANT := example
TENANT_SRCS := main.c document.c
TENANT_EXE := EXAMPLE.EXE
TENANT_CPPFLAGS := -Iinclude
TENANT_RESOURCES := $(OUT)/EXAMPLE.DAT
include $(TOP)/mk/tenant.mk
$(OUT)/EXAMPLE.DAT: data.dat ; cp $< $@
```

All product C, including runtime, uses plain TARGET_CFLAGS. TENANT_CPPFLAGS
is for semantic defines/local includes, not size tuning. TARGET_LD and
TARGET_LDFLAGS link twice, at zero and an aligned probe base with a nonzero
low byte. TARGET_OBJCOPY and TARGET_NM default to objcopy/nm. Two binaries
are compared by factory/mz/mzlink to derive absolute uint32 relocation sites;
any difference other than the probe delta is refused. BSS length from the
linker is rounded into e_minalloc paragraphs. Header relocations allow a
module larger than 64 KiB without overflowing the 16-bit table offset.
Entry must be byte zero. The independent mzcheck reader verifies container,
entry, sites, pointer targets, unchanged module bytes and recorded BSS.
The final file size is printed; it is not capped by the build. tenant-check
reruns validation. Application rules provide resources beside the executable.
There are no changes to existing mk/*.mk; tenant.mk is a new fragment.

## Limits imposed by the OS side today

- kernel/loader.c:loader_load_tenant rejects the complete directory file size
  above TBX_TENANT_IMAGE_MAX (64 KiB), before allocation or file reading. It
  is an explicit loader policy, not an unavoidable staging-area limitation.
  A heap block holds PSP + rounded file + BSS. This policy must be lifted in
  the OS before the plain-flags 123.EXE built here can launch.
- The same function rejects e_minalloc BSS above TBX_TENANT_BSS_MAX (512 KiB).
  e_minalloc itself is a 16-bit paragraph count (format maximum 1,048,560 B).
  The runtime records the module length separately from BSS and relocation
  container overhead. A live app uses the existing tenant slot model.
- flair/tbxgate.c:in_image/image_str validate registration, callback, event
  buffer, drawing/title/alert strings, dialog items and the menu graph against
  [load_base, load_base + module_len). BSS and automatic/shared kernel-stack
  objects fail these checks. TBX_IN_IMAGE forces writable drawing/menu data
  into .data. Keep large private state in BSS.
- File paths/buffers, scrap, Standard File replies, find records, clock,
  settings and memory transports instead use tbxf_span/tbxf_path over
  img_lo()/img_len(): the allocated block after the PSP, including BSS.
  Automatic stack buffers still fail. These two ranges must not be conflated.
- Current gate limits include 2 windows, 8 menus, 16 items per menu, 32-byte
  menu strings, 80 cells per draw, 128-byte paths and 512 find records. Some
  old V1 budget constants (including MAX_WINDOWS=1) remain in toolbox_gate.h;
  the live window validator uses TBX_TENANT_WINDOWS=2. This import preserves
  both constants rather than silently changing the ABI contract.

## Host recording and injection

make host builds build/sdk/host/libtbx_host.a. Host tests use -Isdk/include
-Isdk/host and link that archive once. tbx_host_reset releases files and
resets state; call it before app main. tbx_host is public test state:
operation names, scalar arguments/text snapshots, window frames/titles,
configurable content origin/size, text/cell/background/face/color recording,
menu pointer/draw count, alerts/exit, scroll/beep counts, injectable clock,
settings/memory and dialog-result injection. Cell recording preserves the
InitechWord cursor overlay's underlying style/color. Fills record rectangles;
they do not rasterize glyphs or erase the cell text model.

The file system is in memory: case-insensitive DOS names, drive/root prefix
normalization, read/write access, create-new, seek/close/delete/rename, exact
or wildcard find with metadata, and injectable free space. Seed with
 tbx_host_put_file and inspect with tbx_host_file. File metadata is editable
through tbx_host.files (including directory/read-only attributes). file_error
selects a DOS error for each file operation. fail_create/fail_write, short
read/write limits, finite write_budget and one-shot handle-specific close
failure cover existing save/copy fault cases. File capacity is 64 entries and
32 handles; these are factory bounds, not ABI promises.

Scrap is TEXT-only, 8192 bytes, with absent-type and insufficient-buffer
errors. Standard File retains the reply pointer, tracking records a request,
and FILE COPY records its caption/paths. Tests write replies and enqueue
completion events explicitly with tbx_host_push_event; tbx_host_dispatch
pushes callbacks until the queue empties or exit is requested. The 256-entry
queue and 16384-operation log report overflow instead of overwriting data.
Raw tbx_trap supports the legacy zero-argument beep; use typed bindings for
pointer-bearing calls on a 64-bit host. Dialog injection records requests;
it does not duplicate the real FLAIR Dialog Manager's hit testing.

Sources for this union are the original Initech 123 recording fake and attic
harness/proptest/test_{calc_tenant,itext_route,notepad_route,paint_tenant,
calcda_route,cpanels,iword_face}.c plus test_iword_listfiles_ops.c. They were
read, not built from attic. The model follows the actual gate when old fakes
differ (for example, scrap never permits a partial paste).

Explicit tbx_host_import/export fixture adapters recursively transfer host
files at test boundaries, including deletions/renames. They are separate from
all file verbs. Initech 123 keeps only app driving/fork counters locally;
its 18 suites and independent saved-file checks are retained unchanged.
InitechWord can replace its fake with this archive, add the two SDK include
paths and archive dependency, alias its recording names to tbx_host fields,
and import/export scratch files around each fork. Its app-static driver and
oracle assertions stay local. No InitechWord file was edited here.

## Checks and known debts

make test runs target compile-only layout assertions with TARGET_CC (-m32),
a tbx.h-only target application compile, the real runtime compile, stable
trap/source audit, every external binding's host link, shared fake behavior
and factory/mz tests. It also probes hosted -m32 compilation/execution and,
when available, executes layout and shared fake checks on i386. The mandatory
target proof never depends on hosted multilib availability. traps.tsv is the
independently pinned legacy-final table (including five reserved/dissolved
numbers); it is not regenerated from the implementation by the test. The
source audit validates exactly one runtime binding per active trap, declared
arity and argument-array length, and OS switch/argument-table membership.
The compatibility tbx_get_clock alias calls the single CLOCK binding.
make red-once proves number/layout/fake-binding failures in isolated copies.

Known debts: no ABI version negotiation; menus and dialog items contain native
i386 pointers, not portable serialized records; the raw host trap cannot
carry 64-bit pointers; the private OS dialog adapter needs its header handoff;
and emulator launch/behavior remains tier 1 pending a booted desktop and the
OS file-cap change. The source audit intentionally fails if the dispatcher is
refactored; FLAIR should then publish a read-only dispatch/arity manifest.

## Trap table

Usage below is a grep of product C/headers under apps/, excluding tests;
raw Control Panels calls count as usage. A dash means no current caller.
Arity is gate dwords, including implicit runtime registration arguments.

| Number | Trap / C binding | Arity | Meaning | Apps |
|---|---|---|---|---|
| 0x0001 | TBX_REGISTER / tbx_register | 1 | Register resident application | calculator, controlpanels, ctenant, fonts, initech123, initechcalc, initechpaint, initechtext, initechword, notepad |
| 0x0002 | TBX_KEEPRESIDENT | - | Reserved/dissolved; no binding | - |
| 0x0010 | TBX_EVENTCHANNEL | - | Reserved/dissolved; no binding | - |
| 0x0020 | TBX_NEWWINDOW / tbx_new_window | 5 | Create window | calculator, controlpanels, ctenant, fonts, initech123, initechcalc, initechpaint, initechtext, initechword, notepad |
| 0x0021 | TBX_DISPOSEWINDOW | - | Reserved/dissolved; no binding | - |
| 0x0022 | TBX_SETWTITLE / tbx_set_wtitle | 2 | Set window title | calculator, controlpanels, ctenant, fonts, initech123, initechcalc, initechpaint, initechtext, initechword, notepad |
| 0x0023 | TBX_CONTENTORIGIN / tbx_content_origin | 1 | Read content origin | calculator, initech123, notepad |
| 0x0024 | TBX_CONTENT_SIZE / tbx_content_size | 1 | Read content dimensions | initech123 |
| 0x0030 | TBX_TEXTDRAW / tbx_draw_text | 6 | Draw proportional text | ctenant, fonts, initechcalc, initechtext, notepad |
| 0x0031 | TBX_FILLRECT / tbx_fill_rect | 6 | Fill rectangle | calculator, controlpanels, ctenant, fonts, initech123, initechcalc, initechpaint, initechtext, initechword, notepad |
| 0x0032 | TBX_FRAMERECT | - | Reserved/dissolved; no binding | - |
| 0x0033 | TBX_DRAWCELLS / tbx_draw_cells | 6 | Draw fixed-grid cells | ctenant, initech123, initechword |
| 0x0050 | TBX_SETMBAR / tbx_set_mbar | 1 | Install menu graph | calculator, controlpanels, ctenant, fonts, initech123, initechcalc, initechpaint, initechtext, initechword, notepad |
| 0x0051 | TBX_MENUSELECT | - | Reserved/dissolved; no binding | - |
| 0x0052 | TBX_DRAWMENUBAR / tbx_draw_menubar | 0 | Redraw menu band | - |
| 0x0070 | TBX_EXIT / tbx_exit | 1 | Request exit | calculator, controlpanels, ctenant, fonts, initech123, initechcalc, initechpaint, initechtext, initechword, notepad |
| 0x0080 | TBX_FILE_CREATE / tbx_file_create | 1 | Create/truncate file | ctenant, initechcalc, initechword |
| 0x0081 | TBX_FILE_OPEN / tbx_file_open | 2 | Open file | ctenant, initech123, initechcalc, initechtext, initechword, notepad |
| 0x0082 | TBX_FILE_READ / tbx_file_read | 3 | Read bytes | ctenant, initech123, initechtext, initechword, notepad |
| 0x0083 | TBX_FILE_WRITE / tbx_file_write | 3 | Write bytes | ctenant, initechcalc |
| 0x0084 | TBX_FILE_SEEK / tbx_file_seek | 3 | Seek file | ctenant, initech123, initechword |
| 0x0085 | TBX_FILE_CLOSE / tbx_file_close | 1 | Close handle | ctenant, initech123, initechcalc, initechtext, initechword, notepad |
| 0x0086 | TBX_FILE_DELETE / tbx_file_delete | 1 | Delete file | ctenant, initechcalc, initechword |
| 0x0087 | TBX_FILE_FIND / tbx_file_find | 4 | Find directory records | initechword |
| 0x0088 | TBX_DISK_FREE / tbx_disk_free | 0 | Read free disk bytes | initechword |
| 0x0089 | TBX_FILE_RENAME / tbx_file_rename | 2 | Rename file | initechword |
| 0x008A | TBX_FILE_CREATE_NEW / tbx_file_create_new | 1 | Create without truncation | - |
| 0x0090 | TBX_ALERT / tbx_alert | 4 | Ask alert | initech123, initechcalc, initechtext, notepad |
| 0x0091 | TBX_FILECOPY / tbx_file_copy | 3 | Ask FILE COPY | initechcalc |
| 0x0092 | TBX_DIALOG / tbx_dialog | 6 | Draw/route dialog | controlpanels |
| 0x0093 | TBX_SETTINGS / tbx_settings | 2 | Read/apply settings | controlpanels |
| 0x0094 | TBX_CLOCK / tbx_clock | 2 | Read/set guest clock | controlpanels, initech123, initechword |
| 0x0095 | TBX_MEMORY / tbx_memory | 1 | Read memory statistics | controlpanels |
| 0x0096 | TBX_BEEP / tbx_beep | 0 | Sound speaker | controlpanels, initech123 |
| 0x00A0 | TBX_SCRAP_PUT / tbx_scrap_put | 2 | Replace TEXT scrap | initechtext, notepad |
| 0x00A1 | TBX_SCRAP_GET / tbx_scrap_get | 2 | Read TEXT scrap | initechtext, notepad |
| 0x00B0 | TBX_SFGETFILE / tbx_sf_getfile | 1 | Ask Open dialog | initechtext |
| 0x00B1 | TBX_SFPUTFILE / tbx_sf_putfile | 3 | Ask Save dialog | initechtext |
| 0x00C0 | TBX_TRACKMOUSE / tbx_track_mouse | 0 | Track pointer | initechtext, notepad |
| 0x00C1 | TBX_SETSCROLL / tbx_set_scroll | 3 | Set vertical scroll | initechtext, notepad |
| 0x00C2 | TBX_GETSCROLL / tbx_get_scroll | 1 | Read vertical scroll | initechtext, notepad |
| 0x00C3 | TBX_GLOBALTOLOCAL / tbx_global_to_local | 3 | Translate point | initechtext |

Calls through tbx_file_save/tbx_file_copy_save are indirect: controlpanels,
initech123, initechtext, initechword and notepad also use CREATE_NEW, WRITE,
CLOSE, DELETE and RENAME through sdk/include/tbx_save.h. The table shows direct
callers so the source grep remains reproducible.

Shared build conventions: command stamps rebuild on tool/flag changes; C
translation units use -MMD dependencies. TEST_BUDGET_S is 30 seconds for the
complete tier-0 selection; measured execution fits the default, excluding builds.
The hosted i386 probe/layout/fake are cached by dependencies and commands;
the mandatory target ABI compiles and all six existing check groups remain.
Import audit stays disabled: tbx_save.h retains three TBX_MUT_ hooks.
