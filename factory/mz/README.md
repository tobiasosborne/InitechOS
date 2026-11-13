# InitechMZ factory tools

Host C tools, never linked into the product. mzlink was claimed with git mv
from attic/tools/mzlink.c; its relocation I/O tests and close-fault injector
were claimed from attic/harness/diff/dbf_diff. initechmz_extended.h now lives
in kernel/include, shared executable-format ownership.
Dependency: the kernel public initechmz_extended.h constants (not app policy). The unrelated application-specific shorthand option was
removed; the generic extended/reserve/minimum-heap options remain available.
No application memory-policy header is needed by this factory component.
InitechBase uses the same mzlink with --extended, --reserve, --minimum-heap
and --minalloc generated from its own include/ibase/memory_contract.h; the
factory tool neither includes that header nor offers an app-specific option.
Both import branches carried identical relocation I/O tests; the single copy
here is also run by InitechBase's mzlink-io suite.

make builds mzlink and mzcheck into build/factory/mz. make test preserves the
legacy valid/short/read-error/close-error tests and adds an independently
encoded tiny flat module and five malformed-container checks. The checker
shares no format header, writer helper or calculated expected bytes with
mzlink. Its authority is legacy-final kernel/mz.c's flat-MZ decoder and
kernel/loader.c's tenant model: tagged MZ header, flat entry zero, uint32
relocation sites, pointer targets inside image+BSS, BSS in e_minalloc. It
checks header/trailer table extents and overlap/duplicate sites, and can
compare the payload with the original flat binary and a supplied BSS length.

Usage: mzcheck EXE [raw-module bss-bytes]. The optional operands assert exact
payload preservation and paragraph-rounded BSS. Generic extended containers
are still emitted by mzlink but are not tenant ABI and are not accepted by
mzcheck. mk/tenant.mk uses header relocations to avoid a u16 trailer-offset
limit for large modules; it prints size without enforcing OS loader policy.

No emulator is involved. Product launch tests belong to the booted desktop.
