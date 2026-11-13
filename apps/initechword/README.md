# InitechWord

WordPerfect 5.1 document library, .WP5 codec and the FLAIR editing face.
The core implements a piece table, editing and wrap/layout, attribute codes,
Reveal Codes, blocks, search and the List Files model. This import preserves
existing behavior. It adds no word-processing features.

Public API: include/iword.h; owned format/keyboard/screen/settings contracts:
include/iword/*.h, with the keyboard source table fkeys.json alongside them.
Consumers use -Iapps/initechword/include. Core and fs have no OS dependency:
all seven translation units compile with -Iinclude alone. The face depends on
sdk/tbx.h and sdk/tbx_save.h and reaches the OS only through the Toolbox gate.

make compiles eight freestanding i386 objects under build/apps/initechword
using plain TARGET_CC/TARGET_CFLAGS. WORD.EXE linking awaits SDK tenant rules.
The shared host-test fragment also builds host test binaries for all, matching
the Initech 123 import. make test builds/runs 13 host entries, with one status
line each, a summary and individual logs/scratch under build/apps/initechword/host.
All legacy host document cases run by default; no THOROUGH subset is needed.
A clean run takes about 25 seconds. make smoke and make mutants fail with a
pointer to the deferred inventories, rather than claiming unperformed checks.

Set WORDPERFECT51_DECOMP to the real WordPerfect 5.1 corpus (default from
mk/config.mk). Missing/incomplete corpus is an error. Host tools required:
C compiler, POSIX shell, Python 3 for the corpus's unchanged independent reader,
mtools, fsck.fat, md5sum and sha256sum. Factory additions are C, make and shell.
The tests compare real files, captured screen transcriptions and independent
property models. See tests/IMPORT.md for classification, exact legacy parity,
retained independent post-checks and the separated InitechDOS FAT differential.

Local build extensions: host-core objects avoid recompiling the seven library
sources for each suite; the shared host_test_run macro is wrapped locally for
isolated fresh scratch and multi-command checks. Header dependencies cover the
included face, driver, fake and shared SDK contracts. _DEFAULT_SOURCE enables
MAP_ANONYMOUS in the existing fork-based driver. IWORD_SMALL_BUFFERS is retained
only for the four-check compaction test. No target behavior selector or extra
ABI/size flags are required. The function-key C generator checks the committed
header against fkeys.json without writing the source tree.

## Temporary SDK bridge

Only app/ and face tests add -I$(TOP)/sdk, -I$(OUT)/borrowed and the explicitly
temporary -I$(TOP)/attic/spec. Shared toolbox_gate.h, cpanels_record.h,
cp_dialog.h, event_model.h and grafport.h remain owned by their importers.
tests/borrow-events.sh is the Initech 123 bridge verbatim: extract the original
flair_point_t from grafport.h and remove only event_model.h's grafport include.
The generated header keeps the event definitions and assertions, avoiding the
unused private imaging/surface dependency. Remove this bridge when SDK exports
self-contained events. No attic implementation or legacy make fragment is built.

List Files previously pulled toolbox_gate.h into the pure model for six packed
record constants. Its input contract now lives in include/iword/listfiles.h,
citing DOS 3.3 AH=4Eh's DTA tail and Toolbox Sec 10a. Every face test statically
asserts all six constants against the shared ABI. No data layout changed.

## Fake Toolbox and follow-ups

The extracted fake is tests/fake_tbx/recording.h. A shared SDK host fake must
record 76x24 cell text, colors, style bits and reverse cursor cells; validate
cell alignment/style tokens; support window registration/title, fills, menus
and exits; supply a controllable clock, directory find records and disk free;
and implement DOS file create/create-new/open/read/write/seek/close/delete/rename
with short transfers, collision errors and path semantics. The List Files ops
suite overlays fault-injectable file/find verbs for read-only/full/collision
cases; preserve that injection seam. Component drivers still need static face
state, pristine forked launches and shared check counters. Saved documents and
settings get independent byte/reader checks outside the fake.

73 distinct mutation macros at 75 conditional sites remain for patch conversion
in tests/MUTANTS-TODO.md. tests/SMOKES.md records the unported emulator gates.
The legacy source's unimplemented/gray commands and authored 80x25 to 76x24
adaptations remain; full WordPerfect parity is outside this import. Fixture
minting is manual and excluded from all/test; its C SDL driver and provenance
are retained with the captures. Pending SDK linking, fake consolidation, corpus
pinning and mutation/smoke conversion are explicit follow-up work here.
