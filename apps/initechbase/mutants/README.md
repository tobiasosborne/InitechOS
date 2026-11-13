# InitechBase fault patches

The original inventory listed 303 macro locations / 255 macro names. A source
scan also found five uses of three leading MUTATE_ names omitted by that scan:
REPLACE_NO_INDEX, REPLACE_NO_SCOPE, PACK_NO_REINDEX. All 308 macro locations /
258 names are converted; no mutation activation directives remain in product.

Each original macro has one patch. Repeated uses of one macro stay together.
Nine uppercase function aliases (EXP, LOG, SQRT, TIME, ISCOLOR, FKLABEL, OS,
VERSION, DISKSPACE) select the same concrete fault as their lowercase sibling;
*-uppercase-alias.patch explicitly preserves those legacy registrations.
The additional drop-replace-write.patch is the booted disk-readback red control.

Patches are component-relative, zero-fuzz, and select a passing host suite in
# test:. The strict factory runner requires compilation then the same failure
twice. The component groups identical selections so their baseline is built
once; four local workers bound resource use. MUTANT_SERIAL=1 uses the shared
mk/mutants.mk target directly. Optional ccache changes compilation cost only;
no oracle or timeout is disabled. Pending files have individual reasons and
are excluded from the passing host sweep.
