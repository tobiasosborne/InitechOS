# Header ownership handoff

No file under flair/ or kernel/ was edited by this import.

- Claimed from attic/spec with git mv: toolbox_gate.h, event_model.h,
  cp_dialog.h, cpanels_record.h, initechmz_extended.h. Their canonical paths are sdk/include/.
  OS-side include paths must add sdk/include and stop using attic/spec for
  these names. kernel/loader.c and kernel/mz.c are read-only ground truth.
- attic/spec/grafport.h: remove its typedef struct flair_point declaration
  (v,h int16_t) and include geometry.h instead. event_model.h now includes
  geometry.h, never GrafPort/imaging/surface. Preserve the remaining GrafPort
  implementation contracts inside FLAIR. The Point size/offsets are pinned.
- flair/region/include/region_algebra.h: keep rgn_rect_t as the region API
  type. The SDK exposes the byte-identical tbx_rect_t (top,left,bottom,right)
  without importing region implementation headers. No alias/redefinition is
  needed; do not duplicate struct tbx_rect inside FLAIR.
- flair/menu.h: keep the manager's MenuItem/MenuInfo/MenuBar types. The
  app-side layouts remain tbx_menuitem_t/tbx_menuinfo_t/tbx_menubar_t in
  tbx.h. flair/tbxmenu.h and flair/tbxgate.c should include SDK declarations
  and pin manager sizes/offsets against the SDK records in TARGET builds;
  pointer-bearing native host records must not be used as i386 wire records.
- cp_dialog.h now contains only the flat cp_item_t transport, trap numbers
  and arities. Move its former conditional cp_dialog_items adapter into
  flair/cp_service.h (or a FLAIR-private cp_dialog_items.h included before
  v_dialog in flair/tbxgate.c). Recover its unchanged body from legacy-final
  spec/cp_dialog.h after the '#if defined(INITECH_OS_FLAIR_DIALOG_H)' line.
  It belongs with DialogRecord/DialogItem/ControlRecord, not app headers.
- flair/tbxgate.c's registration/event/menu offsets should use tbx_rec_t,
  tbx_event_t and the SDK menu records where practical, rather than repeat
  public declarations. Keep its validation and manager-side adapters private.

sdk/tests/audit.c reads the current switch(ax) and argn[] table without
compiling or modifying FLAIR. It checks dispatch membership and arities,
including implicit zero-argument BEEP/TRACKMOUSE. It fails closed if the
source idiom changes. A future dispatcher refactor should publish a single
read-only (trap, arity) manifest consumed by both the OS and this audit; do
not weaken the check into grepping arbitrary TBX_ mentions.
