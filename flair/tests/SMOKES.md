# Deferred booted-image gates

250 actual legacy FLAIR emulator gate rules, grouped below; mutant and
Bochs variants count separately. This includes application/desktop integration
that exercises the Toolbox, not a claim of sole Toolbox ownership. SMOKES.tsv
records every gate, actual recipe origin, trace variable/file and checker.
Rules were checked against legacy-final Makefile and harness/*.mk, not accepted
from the draft ownership manifest alone. No trace/checker is moved or ported.

| Group | Gates | What they prove |
|---|---:|---|
| Desktop policy and scene integration | 77 | Finder verbs, compositor scenes, app switching, disk operations, film and power policy |
| Menu tracking and appearance | 10 | held pull-downs, cross-menu dragging, item selection, independent menu crops |
| Surface, depth, cursor and system integration | 54 | pixel presentation, input, cursor overlays and mode-dependent output |
| Tenant/application integration | 94 | INT 81h calls, menus, launch/return, saves, close/activation and per-app content |
| Window, control, dialog, event and routing | 15 | exposure, clipping, drags, close termination, blocked modal clicks, scrolling/grow widgets |

Primary trace inputs remain in attic/spec/flair_*_traces.mk and
flair_toolbox_hardening_trace.mk. Retired make syntax is evidence only; the
injected coordinate/key sequences must be translated into the new factory's
external driver once the product image boots. Checkers remain in attic/tools/
(ppm_flair_*_check.c and related per-app checkers). The index uses legacy paths
so the recipe can be found verbatim in the frozen tree; prefix with attic/ for
files now parked in the new tree. Generic serial greps, PPM equality/diff and
saved-file readbacks are also explicitly part of the recipes.

Important manager gates: test-flair-menu and its mutants; menu-crossdrag;
scroll-look and grow-box; solid/repaint and window-ops; close-terminate and its
mutant; modal-block; toolbox-hardening, its mutant and Bochs leg. Their serial
markers and screenshots grade the real pump order that host tests cannot prove.
Host property suites supplement those gates; they do not replace them.

The draft manifest also attributed kernel GC, kernel size r3/r4, executable
overlay lazy/context/behavior/panic gates to the Toolbox. Those are retired
SIZE/LAYOUT or kernel integration, not Toolbox smoke claims. Their recipes
remain in attic/harness/proptest/{kernel_size*,overlay}.mk. No overlay-layout
constraint is being imported. test-flair-toolbox-captures is a HOST diagnostic,
not an emulator gate: it intentionally fails because disabled/pressed CDEF
captures never existed. That fidelity debt remains open; it is not in make test.

make smoke fails loudly until the kernel/desktop importer supplies one product
image and the new external emulator driver. No test scenario is compiled into
the Toolbox archive. No emulator certification was run for this import.
