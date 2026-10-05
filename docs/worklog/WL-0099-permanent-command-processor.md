# Permanent COMMAND.COM - audit L035

Context: lane-dosprot, initech-pbru. The primary processor previously accepted
EXIT through the common dispatcher, returned to kmain, and halted permanently.
The CONFIG.SYS /P baseline was printed but gave the shell no lifetime state.

What changed: command_repl takes explicit permanence from its caller, resets
its exit state at entry, and the common EXIT dispatcher ignores EXIT for a
permanent processor without printing or setting the exit flag. kmain supplies
permanence for the primary independently of typed/batch/configuration text;
a secondary caller supplies zero and regains control on EXIT. No primary EXIT
path can bypass the guard through AUTOEXEC, IF, FOR, CALL, pipes or redirection.

Why: local MS-DOS_3.3_Users_Guide_198707.pdf, User's Reference printed pp. 46-47
(COMMAND /P) and 66 (EXIT returns to a previous level, if one exists); local
6138519_DOS_3.10_Reference_Feb85.pdf pp. 7-53/7-54 gives permanent /P and
returning secondary processors. These documents were read locally; no live
real-DOS transcript was minted and no diagnostic/catalogue text was added.

Acceptance: the new primary gate first failed "primary processor exited".
After repair AUTOEXEC/IF/FOR and redirected EXIT all continue, EXIT.LOG is empty
under independent mtools extraction, interactive EXIT is silent, two following
VER commands report InitechDOS Version 3.30, and fsck.fat is clean. The guard-
removal mutant fails "primary processor exited". The existing test-shell still
passes its prompt, DIR, TYPE, child EXEC, error and returning EXIT assertions.

Fixture change: EXIT-ending factory traces now explicitly boot the named
tracer_secondary.img and use BOOT_SECONDARY_SHELL in their returning caller.
The public tracer_boot.img remains the permanent primary. Both link the same
dispatcher/file/batch implementation. Existing assertions were retained; the
new primary gate is separately registered. This does not implement the broader
unbuilt interactive COMMAND or COMMAND /C nesting feature (triage B46).

Frictions: the first version of the new oracle expected the boot banner string
from VER; its transcript showed the existing version format, so the assertion
was corrected to the exact VER line. The primary EXIT regression had already
been observed RED, and its runtime mutant independently proves it still bites.

Pointers: harness/diff/fat_diff/shell_permanent.sh; command.c/command.h;
build/dosprot-evidence/{shell-red,shell-final,unit}.log. Final lane-wide
acceptance and size measurements are recorded in WL-0100.
