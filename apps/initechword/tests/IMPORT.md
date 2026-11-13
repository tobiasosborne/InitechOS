# Independent host coverage

All positive legacy-final host gates remain represented. The shared SDK fake
replaces the private recording Toolbox; the app drivers, captured expectations
and independent saved-file post-checks remain component-local. No host check
was removed. mk/import-check.mk audits target reachability, attic dependencies,
escaping product includes and product mutation hooks on every make test.

| Entry (test_iword_ prefix) | Counted checks |
|---|---:|
| core | 27 |
| codes | 6 |
| wp5_roundtrip | 584 |
| reveal_codes | 69 |
| face / face_small | 188 / 4 |
| fkeys | 154 |
| search, including 32-document corpus | 17 + 13944 |
| listfiles / listfiles_ops | 1926 / 138 |
| replace, including 32-document corpus | 11 + 3534 |
| undo, including 32-document corpus | 11 + 1769 |
| setup | 12 |
| **Total** | **22394** |

Seven strong oracle sources and three mixed unit/face sources form thirteen
runner entries plus two support programs. Additional shell oracle checks are
not counted as C assertions. Core/codes aggregate seeded piece-table, typing,
reflow, geometry and block programs; no full corpus leg is replaced by a sample.

wp5_roundtrip rewrites 31 real files and compares real-key replay with the
independent corpus wp5_ref.py. Reveal Codes checks 69 captured panes. Search,
replace and undo each visit 32 real documents. Face MEMO has MINT02's exact
327-byte prefix and literal wrapped body; MEMO/COPY/GUARD/BIG/CODES/TABS retain
the independent reader checks. TABS matches MINT09's body. The 4096-byte small
build is a host compaction test, never a product configuration. List Files
retains all real LISTF grids, MD5 checks and eight lf005.img files. Ops retains
the captured prompt grids, protected/full/collision/rollback checks and source
SHA256SUMS. Setup retains its independent mtools FAT roundtrip. fkeys checks
the committed header against its C generator; literal manual expectations
remain in the test. All consumed external inputs are hash-pinned by corpus.mk.

The legacy kernel FAT-dump differential remains with the kernel/factory owner:
these app tests compare mtools-extracted original bytes with the real WP input,
not kernel implementation linked into the application host executable. The
kernel/factory importer owns the remaining attic FAT/pixel harnesses; the
publication importer owns the legacy publication inventories and manuals.
Deferred emulator coverage is named in SMOKES.md, distinct from the booted
Finder/save/format/exit journey now included in the ordinary desktop smoke.
