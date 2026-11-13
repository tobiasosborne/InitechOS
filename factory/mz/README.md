# InitechMZ container emitter

mzlink.c is claimed from attic/tools/mzlink.c. It writes the flat 32-bit tagged
MZ container, with deterministic relocation/header handling. Its loader memory
contract belongs to the kernel and is temporarily borrowed from attic/spec;
InitechBase supplies its own memory profile header. The component builds this
host tool and runs the existing short/read/close-error relocation I/O suite.
This directory supplies no product implementation or emulator wiring.
