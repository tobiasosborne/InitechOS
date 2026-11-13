# Converted mutation inventory

All inactive product guards in kernel and DOS were removed. There are no
remaining deliberately wrong branches in product sources. Patches preserve
the wrong-code variants from legacy-final. Counts are patches, not guard sites.

268 converted; 229 killed by retained tier-0 gates; 39 pending.

The default `make mutants` sweep includes the proved host patches. Nested
`mutants/pending/` patches are converted but NOT certified: they survived the
retained host gates or have no host invocation. Do not count conversion as a
kill. They require the mapped program/session/diagnostic checks in SMOKES.md
or additional independent host coverage. The explicit list below is the
remaining kernel/DOS test owner work; no source hook remains to convert.

INITDOS.SYS before/after conversion:
`19f2607679594684bbc2af47dcb3e890fc2d532fa66dcbb1e9de726ecb2800cc`.
The live-loader exit-code negative control is temporary and was restored.

| Pending patch | Existing host selection (survived/unavailable) |
|---|---|
| kernel/mutants/pending/absdisk-mutate-drive-a-only.patch | test-int21 |
| kernel/mutants/pending/config-mutate-overflow-offbyone.patch | test-config-sys test-config-fuzz |
| kernel/mutants/pending/dos-volumes-mut-fat16.patch | test-dos-volumes-host |
| kernel/mutants/pending/du-mut-label.patch | test-int21 |
| kernel/mutants/pending/du-mutate-label-cache.patch | test-zs24 |
| kernel/mutants/pending/fat12-mutate-create-chars.patch | test-dos-safety-create test-dos-safety-create-ro test-dos-safety-unlink-ro test-dos-safety-write-ro test-dos-safety-identity |
| kernel/mutants/pending/fat12-mutate-grow-no-wrap.patch | test-zs24 |
| kernel/mutants/pending/fat12-mutate-hide-dir-no-write.patch | test-fat |
| kernel/mutants/pending/fat12-mutate-rename-reject-dir.patch | test-fat-move test-gnrc |
| kernel/mutants/pending/fileio-mutate-cwd-noroot.patch | test-b53d |
| kernel/mutants/pending/fileio-mutate-dir-boot-label.patch | test-zs24 |
| kernel/mutants/pending/fileio-mutate-dir-free.patch | test-zs24 |
| kernel/mutants/pending/initechbase-mutate-full-error.patch | test-zs24 |
| kernel/mutants/pending/int21-mut-indos-exec.patch | test-int21 |
| kernel/mutants/pending/int21-mut-setvect-trap.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-break-cleanup.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-break-recovery.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-caret-c.patch | test-conin-unit |
| kernel/mutants/pending/int21-mutate-caret-z.patch | test-conin-unit |
| kernel/mutants/pending/int21-mutate-cd-root.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-con-explicit-raw.patch | test-conin-unit |
| kernel/mutants/pending/int21-mutate-disk-break.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-exec-rootreject.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-fcb-ext-noattr.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-fcb-partial-nozero.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-fcb-seq-no-advance.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-mkdir-device.patch | test-int21 |
| kernel/mutants/pending/int21-mutate-not-ready.patch | test-int21 |
| kernel/mutants/pending/k36g-mutate-no-redir.patch | test-ansi-wire |
| kernel/mutants/pending/loader-mut-cwd-reset.patch | test-loader |
| kernel/mutants/pending/loader-mut-high-low.patch | test-loader |
| kernel/mutants/pending/loader-mut-high-owner.patch | test-loader |
| kernel/mutants/pending/loader-mut-high-reclaim.patch | test-loader |
| kernel/mutants/pending/loader-mut-tsr-exec-allow-flat.patch | test-loader test-keep test-tsr-unit |
| kernel/mutants/pending/loader-mut-tsr-exec-fixed-reloc.patch | test-loader test-keep test-tsr-unit |
| kernel/mutants/pending/loader-mut-tsr-exec-overlap.patch | test-loader test-keep test-tsr-unit |
| kernel/mutants/pending/loader-mutate-arena-overlap.patch | test-loader |
| kernel/mutants/pending/loader-mutate-exec-rootonly.patch | test-loader |
| kernel/mutants/pending/loader-mutate-inplace-double-handle.patch | test-loader |
