# Converted mutation inventory

All inactive product guards in kernel and DOS were removed. There are no
remaining deliberately wrong branches in product sources. Patches preserve
the wrong-code variants from legacy-final. Counts are patches, not guard sites.

160 converted; 114 killed by retained tier-0 gates; 46 pending.

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
| dos/command/mutants/pending/cmd-mutate-batch-body.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-cd-drive.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-command-c.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-command-env.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-copy-date.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-copy-full-badcmd.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-copy-time.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-del-silent-error.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-dir-args.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-dir-break.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-dir-no-volume.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-for-prompt.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-for-wildcard.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-no-autoexec.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-no-cwd-prompt.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-no-read-cf.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-permanent-exit.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-pipe-clobber.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-pipe-ignore-full.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-pipe-no-stdin.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-prompt-date.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-prompt-dollar.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-rd-noop.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-redir-bypass.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-redir-create-badcmd.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-redir-ignore-full.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-redir-no-restore.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-shift-zero.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-startup.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-type-break.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-volume-message.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/command/mutants/pending/cmd-mutate-volume-prompt.patch | test-command test-shell-files-host test-batch-break-host test-dos-batch2-host test-dos-stamp-host test-del-directory-host test-redir-parse |
| dos/common/mutants/pending/du-mutate-drive.patch | test-attrib test-label-vol |
| dos/common/mutants/pending/du-mutate-label-drive.patch | test-attrib test-label-vol |
| dos/fdisk/mutants/pending/fdisk-mut-letter.patch | test-fdisk |
| dos/fdisk/mutants/pending/fdisk-mut-loaded.patch | test-fdisk |
| dos/find/mutants/pending/find-mutate-invert.patch | (no host selection) |
| dos/more/mutants/pending/more-mutate-drop.patch | (no host selection) |
| dos/sort/mutants/pending/sort-mutate-reverse.patch | (no host selection) |
| dos/tpsrep/mutants/pending/tps-mutate-count6.patch | (no host selection) |
| dos/tpsrep/mutants/pending/tps-mutate-linesok.patch | (no host selection) |
| dos/tpsrep/mutants/pending/tps-mutate-nowrchk.patch | (no host selection) |
| dos/tpsrep/mutants/pending/tps-mutate-pagesok.patch | (no host selection) |
| dos/tpsrep/mutants/pending/tps-mutate-say7.patch | (no host selection) |
| dos/tpsrep/mutants/pending/tps-mutate-unchecked-backup.patch | (no host selection) |
| dos/xcopy/mutants/pending/du-mut-drive.patch | test-xcopy-tree test-xcopy-cwd-host test-dos-stamp-copy-host |
